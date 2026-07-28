import ZEN from "./root.js";
import __mesh from "./mesh.js";

var Zen = ZEN;
Zen.Mesh = Zen.Mesh || __mesh;

// zen.attach(port, opts) — wire this graph to another ZEN realm over a
// MessagePort-like channel: browser MessagePort, Worker, or Node
// worker_threads MessagePort/Worker. Both realms call attach() on their
// own end of the channel; the DAM "?" handshake then runs in both
// directions exactly as it does for an accepted socket (lib/wire.js).
//
// Isomorphic contract (the only capabilities used):
//   port.postMessage(raw)                      — outbound
//   port.on("message", fn)                     — inbound, Node EventEmitter style
//   port.addEventListener("message", fn)       — inbound, browser style
//   port.start()                               — browser MessagePort only
//
// Payloads are the mesh's own JSON strings — structured-clone safe.
//
// opts: { id } — optional stable peer id; omit to let the handshake assign one.
// Returns a detach() function that unhooks the listener and says bye,
// or null when the chain has no root or the port is unusable.
function attach(root, port, opts) {
  opts = opts || {};
  if (!root || !port || typeof port.postMessage !== "function") {
    return null;
  }
  var opt = root.opt;
  var mesh = (opt.mesh = opt.mesh || Zen.Mesh(root));

  var peer = {
    id: opts.id,
    wire: port,
    say: function (raw) {
      try {
        port.postMessage(raw);
      } catch (e) {
        (peer.queue = peer.queue || []).push(raw);
      }
    },
  };

  function hear(msg) {
    var raw = msg && msg.data !== undefined ? msg.data : msg;
    mesh.hear(raw, peer);
  }

  function bye() {
    mesh.bye(peer);
  }

  var node = typeof port.on === "function";
  if (node) {
    port.on("message", hear);
    port.on("close", bye);
  } else {
    port.addEventListener("message", hear);
    if (typeof port.start === "function") {
      port.start();
    }
  }

  mesh.hi(peer);

  return function detach() {
    if (node) {
      if (typeof port.off === "function") {
        port.off("message", hear);
        port.off("close", bye);
      }
    } else {
      port.removeEventListener("message", hear);
    }
    bye();
  };
}

if (!ZEN.chain.attach) {
  ZEN.chain.attach = function (port, opts) {
    var at = this._;
    var root = at && at.root;
    return attach(root, port, opts);
  };
}

export default attach;
