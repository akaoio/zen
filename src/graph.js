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
