import __root from "./root.js";
import __mesh from "./mesh.js";

var Zen = __root;
Zen.Mesh = __mesh;

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
      if (peer._noReconnect) { return; }
      if (opt._tombUrls && (opt._tombUrls.has(peer.url) ||
          opt._tombUrls.has(peer.url.replace(/^https?/, 'ws')))) { return; }
      var url = peer.url.replace(/^http/, "ws");
      peer._isOutbound = true;
      var wire = (peer.wire = new opt.WebSocket(url));
      wire.onclose = function () {
        // Exponential backoff for AXE-dropped outbound peers (closed before HI).
        if (peer._isOutbound && !peer.met) {
          peer._axeGuess = (peer._axeGuess || 0) + 1;
          if (peer._axeGuess >= 5) {
            peer._noReconnect = true;
            if (peer.url) {
              opt._tombUrls = opt._tombUrls || new Set();
              opt._tombUrls.add(peer.url);
              opt._tombUrls.add(peer.url.replace(/^wss?/, 'http'));
              opt._tombUrls.add(peer.url.replace(/^https?/, 'ws'));
            }
          }
        }
        // Backoff for peers that accept then quickly close (AXE PID-sort drop).
        if (peer._isOutbound && peer.met && peer._openAt &&
            (+new Date() - peer._openAt) < 8000) {
          peer._hiGuess = (peer._hiGuess || 0) + 1;
          if (peer._hiGuess >= 3) {
            peer._noReconnect = true;
            if (peer.url) {
              opt._tombUrls = opt._tombUrls || new Set();
              opt._tombUrls.add(peer.url);
              opt._tombUrls.add(peer.url.replace(/^wss?/, 'http'));
              opt._tombUrls.add(peer.url.replace(/^https?/, 'ws'));
            }
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
    if (opt._tombUrls && (opt._tombUrls.has(peer.url) ||
        opt._tombUrls.has((peer.url || '').replace(/^https?/, 'ws')))) {
      return;
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
