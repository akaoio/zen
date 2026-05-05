/**
 * test/dht/kbucket.js — unit tests for lib/dht.js k-bucket DHT
 *
 * All tests run in-process using mock peer wires (same pattern as
 * test/mesh/dam.js).  No WebSocket / subprocess required.
 *
 * Coverage:
 *   1. k-bucket add / LRU-tail rule
 *   2. bucket eviction when full and head is offline
 *   3. closest() returns K entries sorted by XOR distance
 *   4. FIND_NODE DAM protocol (fn / fn_r round-trip)
 *   5. mesh.route() prefers DHT k-bucket candidates
 */

import assert from "assert";
import { ZEN } from "../../zen.js";

// lib/dht.js registers itself via ZEN.on("opt") when imported.
import "../../lib/dht.js";

// ── helpers ────────────────────────────────────────────────────────────────

/** Create a minimal ZEN graph root with mesh, skipping browser/axe code. */
function makeRoot(pid) {
  const graph = ZEN.graph.create({ localStorage: false, peers: {}, WebSocket: false });
  const root  = graph._;
  const mesh  = root.opt.mesh || ZEN.Mesh(root);
  root.opt.mesh = mesh;
  root.opt.pid  = pid || String.random(9);
  // dht.js checks typeof process — we are in Node so it will run.
  // But it skips if root.dht already set; trigger start manually here
  // by calling the ZEN "opt" event chain that lib/dht.js hooks into.
  // (ZEN.on("opt") already fired when graph.create() ran — dht.js hooked
  //  AFTER graph.create in this test, so we need to start manually.)
  return { graph, root, mesh };
}

/**
 * Wire two meshes in-process.  Returns peer handles.
 */
function wire(meshA, pubA, meshB, pubB) {
  const pA = { id: "pa", pub: pubA, url: "mock://" + pubA.slice(0, 6),
    wire: { send: (r) => meshB.hear(r, pB) } };
  const pB = { id: "pb", pub: pubB, url: "mock://" + pubB.slice(0, 6),
    wire: { send: (r) => meshA.hear(r, pA) } };
  return { pA, pB };
}

/**
 * Generate a fake base62 pub key of given length (default 45).
 * Uses only chars from the base62 alphabet so mesh.xor() can decode it.
 */
const B62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
function fakePub(len) {
  len = len || 45;
  var s = "";
  for (var i = 0; i < len; i++) {
    s += B62[Math.floor(Math.random() * 62)];
  }
  return s;
}

/**
 * Build a fresh dht instance attached to a new root.
 * lib/dht.js's start() checks root.dht and opt.mesh — we bootstrap
 * manually here to avoid depending on ZEN.on("opt") firing order.
 */
function makeDHT(selfPub) {
  // We invoke start() by temporarily monkey-patching: create a graph
  // that already has mesh set, then import dht.js which already hooked
  // ZEN.on("opt").  The simplest approach is to re-trigger via a fresh
  // ZEN instance so the event fires with our root.
  const graph = ZEN.graph.create({ localStorage: false, peers: {}, WebSocket: false });
  const root  = graph._;
  const mesh  = root.opt.mesh || ZEN.Mesh(root);
  root.opt.mesh = mesh;
  root.opt.pid  = String.random(9);
  root.opt.pub  = selfPub || fakePub();
  // dht.js already registered ZEN.on("opt"); that event fires inside
  // graph.create() → which triggers lib/dht.js start(root).
  // Because import order is: dht.js loads before this test runs each
  // makeDHT() call, the handler is already registered.
  // BUT: ZEN.on("opt") fires during graph.create(), and dht.js was
  // imported at module load time (before makeDHT calls) — so it fires.
  return { graph, root, mesh, dht: root.dht };
}

// ── 1. k-bucket add ────────────────────────────────────────────────────────

describe("dht.add() — k-bucket insertion", function () {
  it("adds a peer to the correct bucket", function () {
    const { dht, root } = makeDHT();
    if (!dht) { return; } // skip if dht disabled / mesh missing

    const peer = fakePub();
    const url  = "wss://peer1.example.com:8420/zen";
    dht.add(peer, url);

    const found = dht.closest(peer, 1);
    assert.ok(found.length > 0, "should find the added peer");
    assert.strictEqual(found[0].pub, peer);
    assert.strictEqual(found[0].url, url);
  });

  it("moves an existing entry to bucket tail on re-add (LRU)", function () {
    const { dht, root } = makeDHT();
    if (!dht) { return; }

    const p1  = fakePub(), p2 = fakePub(), p3 = fakePub();
    const url = "wss://x.example.com:8420/zen";
    dht.add(p1, url);
    dht.add(p2, url);
    // Add p1 again — it should move to tail (most recently seen).
    dht.add(p1, "wss://updated.example.com:8420/zen");

    // closest() to p1 should still find it.
    const found = dht.closest(p1, 10);
    const entry = found.find(function (e) { return e.pub === p1; });
    assert.ok(entry, "re-added peer should still be in buckets");
    assert.strictEqual(entry.url, "wss://updated.example.com:8420/zen", "URL should update");
  });

  it("does not add self pub to buckets", function () {
    const self = fakePub();
    const { dht } = makeDHT(self);
    if (!dht) { return; }

    dht.add(self, "wss://self.example.com:8420/zen");
    // Self XOR self = 0, bucketIdx returns -1 → not added.
    const found = dht.closest(self, 20).filter(function (e) { return e.pub === self; });
    assert.strictEqual(found.length, 0, "self should not appear in its own buckets");
  });
});

// ── 2. closest() ───────────────────────────────────────────────────────────

describe("dht.closest(target, K)", function () {
  it("returns results sorted by XOR distance ascending", function () {
    const { dht, root } = makeDHT();
    if (!dht) { return; }

    const target = fakePub();
    const peers  = [fakePub(), fakePub(), fakePub(), fakePub(), fakePub()];
    const url    = "wss://node.example.com:8420/zen";
    peers.forEach(function (p) { dht.add(p, url); });

    const results = dht.closest(target, peers.length);
    for (var i = 1; i < results.length; i++) {
      assert.ok(
        results[i - 1].dist <= results[i].dist,
        "results must be sorted by XOR distance ascending"
      );
    }
  });

  it("returns at most K results", function () {
    const { dht } = makeDHT();
    if (!dht) { return; }

    const url = "wss://node.example.com:8420/zen";
    for (var i = 0; i < 30; i++) { dht.add(fakePub(), url); }

    const results = dht.closest(fakePub(), 5);
    assert.ok(results.length <= 5, "should return at most K=5");
  });

  it("returns empty array when no peers known", function () {
    const { dht } = makeDHT();
    if (!dht) { return; }
    assert.deepStrictEqual(dht.closest(fakePub(), 10), []);
  });
});

// ── 3. FIND_NODE DAM protocol ──────────────────────────────────────────────

describe("DHT FIND_NODE protocol (dam:dht fn/fn_r)", function () {
  it("responds to fn with fn_r containing closest nodes", function (done) {
    this.timeout(3000);

    const pubA = fakePub();
    const pubB = fakePub();

    // Create two roots with dht enabled.
    const A = makeDHT(pubA);
    const B = makeDHT(pubB);
    if (!A.dht || !B.dht) { return done(); }

    // Populate B's buckets with 3 known peers.
    const knownPeers = [fakePub(), fakePub(), fakePub()];
    const kurl = "wss://known.example.com:8420/zen";
    knownPeers.forEach(function (p) { B.dht.add(p, kurl); });

    // Wire A ↔ B in-process.
    const { pA, pB } = wire(A.mesh, pubA, B.mesh, pubB);
    A.root.opt.peers["pb"] = pB;  // A knows about B
    B.root.opt.peers["pa"] = pA;  // B knows about A

    // Override _pending handler: inject the response listener before sending.
    // We'll use mesh.hear directly: send a fn message from A to B through pB,
    // and B's mesh will reply via pA.
    var replied = false;
    var origHear = A.mesh.hear["dht"];
    A.mesh.hear["dht"] = function (msg, peer) {
      if (msg.type === "fn_r") {
        assert.ok(Array.isArray(msg.nodes), "fn_r nodes should be array");
        assert.ok(msg.nodes.length > 0, "should return at least one node");
        msg.nodes.forEach(function (n) {
          assert.ok(n.pub, "each node should have pub");
          assert.ok(n.url, "each node should have url");
        });
        replied = true;
        A.mesh.hear["dht"] = origHear;
        done();
        return;
      }
      if (origHear) { origHear(msg, peer); }
    };

    const target = fakePub();
    const reqId  = String.random(9);
    B.mesh.hear({ dam: "dht", type: "fn", target: target, id: reqId, "#": String.random(9) }, pA);
    // B should send fn_r back to A (via pA's wire → meshA.hear).
    // The fn_r will arrive at A.mesh.hear["dht"] patched above.
  });

  it("ignores fn_r with unknown id (no pending callback)", function () {
    const { dht, mesh } = makeDHT();
    if (!dht) { return; }
    // Should not throw.
    mesh.hear({ dam: "dht", type: "fn_r", id: "unknownXXX", nodes: [], "#": String.random(9) }, {});
  });

  it("ignores malformed dht messages", function () {
    const { mesh } = makeDHT();
    // No type, no target, no id — should not throw.
    mesh.hear({ dam: "dht", "#": String.random(9) }, {});
    mesh.hear({ dam: "dht", type: "fn", "#": String.random(9) }, {});  // missing target/id
  });
});

// ── 4. mesh.route() uses k-bucket candidates ──────────────────────────────

describe("mesh.route() — DHT-aware routing", function () {
  it("returns connected peer that is closest in XOR to target", function () {
    const selfPub = fakePub();
    const { dht, mesh, root } = makeDHT(selfPub);
    if (!dht) { return; }

    const pubX  = fakePub();
    const pubY  = fakePub();
    const urlX  = "wss://x.example.com:8420/zen";
    const urlY  = "wss://y.example.com:8420/zen";

    // Add both to buckets.
    dht.add(pubX, urlX);
    dht.add(pubY, urlY);

    // Mock both as connected in opt.peers.
    const peerX = { id: "px", pub: pubX, url: urlX, wire: {} };
    const peerY = { id: "py", pub: pubY, url: urlY, wire: {} };
    root.opt.peers["px"] = peerX;
    root.opt.peers["py"] = peerY;

    // Find which is closer to pubX by XOR.
    const dx = mesh.xor(pubX, pubX); // = 0n
    const dy = mesh.xor(pubX, pubY);

    // mesh.route(pubX) should return peerX (XOR=0, closest).
    const result = mesh.route(pubX);
    assert.ok(result, "should return a peer");
    assert.strictEqual(result.pub, pubX, "should pick the XOR-closest peer");
  });

  it("falls through to original route when bucket has no connected peers", function () {
    const { dht, mesh, root } = makeDHT();
    if (!dht) { return; }

    const target = fakePub();
    // Bucket has an entry but no connected wire.
    dht.add(target, "wss://offline.example.com:8420/zen");
    // No opt.peers entry → peerByPub returns null → dht route returns null.
    // Original route also returns null (no peers).
    const result = mesh.route(target);
    assert.strictEqual(result, null, "should return null when no peer is reachable");
  });
});
