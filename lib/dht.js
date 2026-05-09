// lib/dht.js — Kademlia k-bucket DHT for ZEN relay nodes.
//
// Loaded by lib/server.js AFTER axe.js so opt.mesh is already initialised.
// Browser nodes are leaf nodes and are skipped automatically.
//
// Protocol: DAM message { dam:"dht", type:"fn"|"fn_r", target, id, nodes }
//   fn    — FIND_NODE request: return K closest known peers to `target`
//   fn_r  — FIND_NODE response: list of { pub, url } nodes
//
// mesh.route() is extended to prefer k-bucket candidates over greedy scan.
// Proactive refresh: self-lookup every REFRESH_IV ms to fill sparse buckets.

import ZEN from "../zen.js";

var Zen = ZEN;
var K             = 20;               // max entries per bucket
var ALPHA         = 3;                // lookup concurrency
var BUCKET_COUNT  = 512;              // one bucket per XOR bit-position
                                      // 512 > 264 bits (secp256k1 compressed)
var REFRESH_IV    = 5 * 60 * 1000;   // proactive bucket refresh interval

Zen.on("opt", function (at) {
  start(at);
  this.to.next(at);
});

function start(root) {
  if (root.dht) { return; }
  if (typeof process === "undefined") { return; } // Node.js only
  if (false === root.opt.dht) { return; }         // opt.dht = false to disable

  var opt  = root.opt;
  var mesh = opt.mesh;
  if (!mesh) { return; } // axe must be loaded first

  // Self node-ID: prefer signing pub key, fall back to stable process ID.
  // Both are base62 strings — mesh.xor handles any length.
  function selfId() { return opt.pub || opt.pid || ""; }

  // ── k-buckets ────────────────────────────────────────────────────────────
  var buckets = [];
  for (var _i = 0; _i < BUCKET_COUNT; _i++) { buckets[_i] = []; }

  // Bit-length of a BigInt (= log2 floor + 1, 0 for 0n).
  function bitlen(n) {
    if (n === 0n) { return 0; }
    return n.toString(2).length;
  }

  // Bucket index for a peer relative to self (0 … BUCKET_COUNT-1, or -1 on error).
  function bucketIdx(peerPub) {
    var sid = selfId();
    if (!sid || !peerPub || sid === peerPub) { return -1; }
    // Guard startup race: selfId() = opt.pid (short) but peer sent full opt.pub.
    if (opt.pub && peerPub === opt.pub) { return -1; }
    if (opt.pid && peerPub === opt.pid) { return -1; }
    var d = mesh.xor(sid, peerPub);
    if (d === null) { return -1; }
    var idx = bitlen(d) - 1;
    if (idx < 0) { idx = 0; }
    return Math.min(idx, BUCKET_COUNT - 1);
  }

  // Return live peer object for a pub key, or null.
  // Returns a truthy sentinel for self so refresh() never tries to connect to self.
  // Also checks axe.up for inbound-only connections (AXE kept inbound, tombstoned outbound).
  function peerByPub(pub) {
    if (!pub) { return null; }
    if (pub === opt.pub || pub === opt.pid) { return true; }
    var peers = opt.peers || {};
    for (var k in peers) {
      var p = peers[k];
      if (p && p.pub === pub && p.wire) { return p; }
    }
    var axeUp = root.axe && root.axe.up;
    if (axeUp) {
      for (var axePid in axeUp) {
        var ap = axeUp[axePid];
        if (ap && (ap.pub === pub || axePid === pub) && ap.wire) { return ap; }
      }
    }
    return null;
  }

  // Add / refresh a peer in the appropriate bucket (Kademlia LRU-tail rule).
  function bucketAdd(pub, url) {
    if (!pub || !url) { return; }
    var idx = bucketIdx(pub);
    if (idx < 0) { return; }
    var bucket = buckets[idx];

    // Already present → update URL and move to tail (most recently seen).
    for (var i = 0; i < bucket.length; i++) {
      if (bucket[i].pub === pub) {
        if (url) { bucket[i].url = url; }
        var entry = bucket.splice(i, 1)[0];
        bucket.push(entry);
        return;
      }
    }

    // Bucket not full → append.
    if (bucket.length < K) {
      bucket.push({ pub: pub, url: url });
      return;
    }

    // Bucket full → evict head only if it is no longer connected.
    var head = bucket[0];
    var headPeer = peerByPub(head.pub);
    if (!headPeer || !headPeer.wire) {
      bucket.shift();
      bucket.push({ pub: pub, url: url });
    }
    // else: all alive → drop the candidate (Kademlia: prefer stable nodes).
  }

  // ── K closest entries across all buckets to a target pub key ────────────
  function closest(target, count) {
    if (!target) { return []; }
    count = count || K;
    var all = [];
    for (var i = 0; i < BUCKET_COUNT; i++) {
      for (var j = 0; j < buckets[i].length; j++) {
        var e = buckets[i][j];
        var d = mesh.xor(target, e.pub);
        if (d !== null) { all.push({ pub: e.pub, url: e.url, dist: d }); }
      }
    }
    all.sort(function (a, b) {
      return a.dist < b.dist ? -1 : a.dist > b.dist ? 1 : 0;
    });
    return all.slice(0, count);
  }

  // ── FIND_NODE wire call: ask one connected peer ──────────────────────────
  var _pending = {}; // id → { cb, timer }

  function findNode(peer, target, cb) {
    if (!peer || !peer.wire) { cb(null); return; }
    var id    = String.random(9);
    var timer = setTimeout(function () {
      delete _pending[id];
      cb(null);
    }, 2000);
    _pending[id] = { cb: cb, timer: timer };
    mesh.say({ dam: "dht", type: "fn", target: target, id: id }, peer);
  }

  // ── Iterative lookup: collect K globally-closest peers to a target ───────
  // cb(nodes) where nodes = [{ pub, url }, …] sorted by XOR distance.
  function lookup(target, cb) {
    var seen  = {};  // pub → dist
    var asked = {};  // pub → true
    var found = [];  // [{pub,url,dist}] sorted ascending

    var initial = closest(target, ALPHA);
    if (!initial.length) { cb([]); return; }
    initial.forEach(function (e) { seen[e.pub] = e.dist; found.push(e); });

    function step() {
      // ALPHA unseen+connected peers closest to target.
      var toAsk = found
        .filter(function (e) { return !asked[e.pub] && peerByPub(e.pub); })
        .slice(0, ALPHA);

      if (!toAsk.length) { cb(found.slice(0, K)); return; }

      var pending = toAsk.length;
      toAsk.forEach(function (e) {
        asked[e.pub] = true;
        var peer = peerByPub(e.pub);
        if (!peer) { if (!--pending) { step(); } return; }
        findNode(peer, target, function (nodes) {
          if (nodes) {
            nodes.forEach(function (n) {
              if (n.pub && !seen[n.pub]) {
                var d = mesh.xor(target, n.pub);
                if (d !== null) {
                  seen[n.pub] = d;
                  found.push({ pub: n.pub, url: n.url, dist: d });
                  bucketAdd(n.pub, n.url); // learn new peers passively
                }
              }
            });
            found.sort(function (a, b) {
              return a.dist < b.dist ? -1 : a.dist > b.dist ? 1 : 0;
            });
          }
          if (!--pending) { step(); }
        });
      });
    }
    step();
  }

  // ── DAM handler: serve/receive FIND_NODE ─────────────────────────────────
  mesh.hear["dht"] = function (msg, peer) {
    if (!msg) { return; }

    if (msg.type === "fn" && msg.target && msg.id) {
      // Incoming FIND_NODE request → reply with K closest we know.
      var nodes = closest(msg.target, K).map(function (e) {
        return { pub: e.pub, url: e.url };
      });
      mesh.say({ dam: "dht", type: "fn_r", id: msg.id, nodes: nodes }, peer);
      return;
    }

    if (msg.type === "fn_r" && msg.id) {
      // Incoming FIND_NODE response → dispatch to waiting callback.
      var pending = _pending[msg.id];
      if (pending) {
        clearTimeout(pending.timer);
        delete _pending[msg.id];
        pending.cb(Array.isArray(msg.nodes) ? msg.nodes : []);
      }
      return;
    }
  };

  // ── Extend mesh.route() with k-bucket awareness ──────────────────────────
  var _origRoute = mesh.route;
  mesh.route = function (targetPub, skip) {
    // Prefer a connected peer that our buckets rank closest to the target.
    if (targetPub) {
      var cands = closest(targetPub, K);
      for (var i = 0; i < cands.length; i++) {
        var p = peerByPub(cands[i].pub);
        if (p && p.wire && p !== skip) { return p; }
      }
    }
    // Fall back to axe's greedy scan over connected peers.
    return _origRoute(targetPub, skip);
  };

  // ── Learn peers after the ? handshake completes (peer.pub is now known) ───
  // mesh.hi() fires root.on("hi") BEFORE the ? exchange, so peer.pub is
  // empty at that point.  axe.js uses the same pattern: hook hear["?"] after
  // calling the previous handler so peer.pub is guaranteed to be set.
  var _prevHearQ = mesh.hear["?"];
  mesh.hear["?"] = function (msg, peer) {
    if (_prevHearQ) { _prevHearQ(msg, peer); }
    if (peer && peer.pub && peer.url) { bucketAdd(peer.pub, peer.url); }
  };

  // Note: on disconnect we keep the entry — Kademlia evicts only when
  // a bucket is full AND the head node is offline.

  // ── Proactive bucket refresh: self-lookup fills sparse buckets ───────────
  function refresh() {
    var sid = selfId();
    if (!sid) { return; }
    lookup(sid, function (nodes) {
      // Connect to any newly discovered peer not yet reachable, up to K.
      nodes.slice(0, K).forEach(function (n) {
        var found = n.pub ? peerByPub(n.pub) : null;
        console.log('[DHT-REFRESH] node url=' + n.url + ' pub=' + (n.pub||'?').slice(0,8) + ' peerByPub=' + (found ? (found.pub||'?').slice(0,8) + ' wire=' + (found.wire ? 'alive' : 'null') + ' met=' + !!found.met : 'null'));
        if (n.url && !found) {
          console.log('[DHT-REFRESH] calling mesh.hi for', n.url);
          try { mesh.hi({ id: n.url, url: n.url, retry: 9 }); } catch (e) {}
        }
      });
    });
  }

  var refreshIv = setInterval(refresh, REFRESH_IV);
  if (refreshIv.unref) { refreshIv.unref(); }

  // Expose internals for tests and instrumentation.
  var dht = root.dht = {
    buckets:  buckets,
    closest:  closest,
    lookup:   lookup,
    add:      bucketAdd,
    refresh:  refresh,
  };

  Zen.log.once("DHT", "DHT k-buckets enabled (K=" + K + ", buckets=" + BUCKET_COUNT + ")");
}
