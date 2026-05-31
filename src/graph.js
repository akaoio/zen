import "./book.js";
import "./chain.js";
import "./back.js";
import "./put.js";
import "./get.js";
import "./on.js";
import "./map.js";
import "./set.js";
import "./meta.js";
import "./mesh.js";
import "./websocket.js";
import "./locstore.js";
import ZEN from "./root.js";

function consumeAsyncResult(zen) {
  var at = zen && zen._,
    async = at && at.asyncResult,
    p = async && async.promise;
  if (!p) {
    return;
  }
  delete at.asyncResult;
  return p;
}

if (!ZEN.chain.then) {
  ZEN.chain.then = function (cb, opt) {
    var zen = this,
      p = consumeAsyncResult(zen);
    if (p) {
      return cb ? p.then(cb) : p;
    }
    p = new Promise(function (res) {
      zen.once(res, opt);
    });
    return cb ? p.then(cb) : p;
  };
}

// zen.push(targetPub, data, opt) — ephemeral targeted relay message.
// Routes via DAM multi-hop XOR relay to the peer with matching pub key.
// Not persisted, not replicated, no CRDT. Target must be online within TTL hops.
// opt: { ttl: number } — max hops (default 5)
if (!ZEN.chain.push) {
  ZEN.chain.push = function (to, data, opt) {
    var at = this._;
    var mesh = at && at.root && at.root.opt && at.root.opt.mesh;
    if (mesh && mesh.relay) {
      mesh.relay(to, data, opt);
    }
    return this;
  };
}

// zen.mesh — public facade for the mesh subsystem.
// Exposes safe, stable APIs only. Internal transport/state is intentionally hidden.
//   zen.mesh.on(fn)          subscribe to incoming push messages; fn({from,data}); returns off()
//   zen.mesh.near            number of currently connected peers
//   zen.mesh.peers           snapshot array of connected peers { pub, url, id }
//   zen.mesh.route(pub)      next-hop peer toward pub by XOR distance { pub, url, id } or null
//   zen.mesh.xor(a, b)       XOR distance between two pub keys (BigInt), or null
//   zen.mesh.closer(t, a, b) return whichever of pub a or b is XOR-closer to target t
//   zen.mesh.ping(pub)       send a ping to the peer with matching pub key
//   zen.mesh.stats           { msgs, bytes } counters for received messages
if (!Object.getOwnPropertyDescriptor(ZEN.chain, 'mesh')) {
  Object.defineProperty(ZEN.chain, 'mesh', {
    get: function () {
      var root = this._ && this._.root;
      var m = root && root.opt && root.opt.mesh;
      if (!m) { return null; }
      var opt = root.opt;
      return {
        on: function (fn) { return m.on(fn); },
        get near() { return m.near || 0; },
        get peers() {
          var ps = opt.peers || {}, out = [];
          for (var k in ps) {
            var p = ps[k];
            if (p && p.wire) { out.push({ pub: p.pub || null, url: p.url || null, id: p.id || k }); }
          }
          return out;
        },
        route: function (pub) {
          var p = m.route(pub);
          if (!p) { return null; }
          return { pub: p.pub || null, url: p.url || null, id: p.id || null };
        },
        xor: function (a, b) { return m.xor(a, b); },
        closer: function (target, a, b) { return m.closer(target, a, b); },
        ping: function (pub) {
          var ps = opt.peers || {};
          for (var k in ps) {
            var p = ps[k];
            if (p && p.pub === pub && p.wire) { m.ping(p); return; }
          }
        },
        get stats() {
          var h = m.hear;
          return { msgs: (h && h.c) || 0, bytes: (h && h.d) || 0 };
        },
      };
    },
  });
}

const graph = {
  core: ZEN,
  chain: ZEN.chain,
  create(opt = {}) {
    return ZEN(opt);
  },
  is(value) {
    return ZEN.is(value);
  },
};

export { graph };
export default graph;
