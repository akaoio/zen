import __root from "./root.js";
import __mesh from "./mesh.js";

var Zen = __root;
Zen.Mesh = __mesh;

// Cap tombUrls at 500 entries (≈167 dead peers × 3 URL variants each).
// Set is insertion-ordered so oldest entries are dropped first.
function tombAdd(opt, url) {
  var t = (opt.tomb = opt.tomb || new Set());
  if (t.size >= 500) { t.delete(t.values().next().value); }
  t.add(url);
  t.add(url.replace(/^wss?:/, function(p){ return p[2]==='s'?'https:':'http:'; }));
  t.add(url.replace(/^https?:/, function(p){ return p[4]==='s'?'wss:':'ws:'; }));
}

// TODO: resync upon reconnect online/offline
//window.ononline = window.onoffline = function(){ console.log('online?', navigator.onLine) }

Zen.on("opt", function (root) {
  this.to.next(root);
  if (root.once) {
    return;
  }
  var opt = root.opt;
  if (false === opt.WebSocket) {
    return;
  }

  var mesh = (opt.mesh = opt.mesh || Zen.Mesh(root));

  var websocket =
    opt.WebSocket || globalThis.WebSocket;
  if (!websocket) {
    return;
  }
  opt.WebSocket = websocket;

  var wired = mesh.wire || opt.wire;
  mesh.wire = opt.wire = open;
  function open(peer) {
    try {
      if (!peer || !peer.url) {
        return wired && wired(peer);
      }
      // Do not open connections to tombstoned peers.
      // Normalise to https:// for lookup since tombstones are stored under https/http keys.
      if (peer._noReconnect) { return; }
      if (opt.tomb) {
        var _tu = peer.url;
        var _tn = _tu.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
        if (opt.tomb.has(_tu) || opt.tomb.has(_tn) ||
            opt.tomb.has(_tu.replace(/^https?/, 'ws'))) { return; }
      }
      var url = peer.url.replace(/^http/, "ws");
      peer._isOutbound = true;
      console.log('[WS-OPEN] new WS to:', peer.url, 'met:', !!peer.met, '_axeGuess:', peer._axeGuess||0, '_hiGuess:', peer._hiGuess||0, '_noReconnect:', !!peer._noReconnect);
      var wire = (peer.wire = new opt.WebSocket(url));
      wire.onclose = function () {
        console.log('[WS-CLOSE] WS closed:', peer.url, 'met:', !!peer.met, '_axeGuess:', peer._axeGuess||0, '_hiGuess:', peer._hiGuess||0, 'duration:', peer._openAt ? (Date.now()-peer._openAt)+'ms' : 'unknown');
        // Stop keepalive ping for this wire.
        clearInterval(peer._keepalive);
        peer._keepalive = null;
        // Exponential backoff for AXE-dropped outbound peers (closed before HI).
        if (peer._isOutbound && !peer.met) {
          peer._axeGuess = (peer._axeGuess || 0) + 1;
          if (peer._axeGuess >= 5) {
            peer._noReconnect = true;
            if (peer.url) { tombAdd(opt, peer.url); }
          }
        }
        // Backoff for peers that accept then quickly close (AXE PID-sort drop).
        if (peer._isOutbound && peer.met && peer._openAt &&
            (+new Date() - peer._openAt) < 8000) {
          peer._hiGuess = (peer._hiGuess || 0) + 1;
          if (peer._hiGuess >= 3) {
            peer._noReconnect = true;
            if (peer.url) { tombAdd(opt, peer.url); }
          }
        }
        reconnect(peer);
        opt.mesh.bye(peer);
      };
      wire.onerror = function (err) {
        reconnect(peer);
      };
      wire.onopen = function () {
        peer._openAt = +new Date();
        peer._axeGuess = 0; // reset on successful open — prevent spurious tombstoning
        peer._hiGuess = 0;
        // Keepalive: ping every 30s so idle relay connections don't time out at
        // network/proxy boundaries (typical idle timeout is ~60s).
        peer._keepalive = setInterval(function () {
          if (peer.wire === wire && wire.readyState === 1) {
            opt.mesh.ping(peer);
          } else {
            clearInterval(peer._keepalive);
            peer._keepalive = null;
          }
        }, 30 * 1000);
        opt.mesh.hi(peer);
      };
      wire.onmessage = function (msg) {
        if (!msg) {
          return;
        }
        opt.mesh.hear(msg.data || msg, peer);
      };
      return wire;
    } catch (e) {
      opt.mesh.bye(peer);
    }
  }

  setTimeout(function () {
    !opt.super && root.on("out", { dam: "hi" });
  }, 1); // it can take a while to open a socket, so maybe no longer lazy load for perf reasons?

  var wait = 2 * 999;
  function reconnect(peer) {
    clearTimeout(peer.defer);
    if (!opt.peers[peer.url] || peer._noReconnect) {
      return;
    }
    if (opt.tomb) {
      var _ru = peer.url || '';
      var _rn = _ru.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
      if (opt.tomb.has(_ru) || opt.tomb.has(_rn) ||
          opt.tomb.has(_ru.replace(/^https?/, 'ws'))) { return; }
    }
    if (doc && peer.retry <= 0) {
      return;
    }
    peer.retry =
      (peer.retry || opt.retry + 1 || 60) -
      (-peer.tried + (peer.tried = +new Date()) < wait * 4 ? 1 : 0);
    peer.defer = setTimeout(function to() {
      if (doc && doc.hidden) {
        return setTimeout(to, wait);
      }
      open(peer);
    }, wait);
  }
  var doc = "" + u !== typeof document && document;
});
var noop = function () {},
  u;
