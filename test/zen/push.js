/**
 * test/zen/push.js — tests for zen.push() ephemeral relay method
 *
 * zen.push(targetPub, data, opt) sends an ephemeral DAM relay message.
 * It does NOT persist to the graph. Uses mesh.relay() under the hood.
 */
import assert from "assert";
import ZEN from "../../zen.js";

function makeZEN(label) {
  return new ZEN({
    peers: [],
    localStorage: false,
    file: "tmp/push-" + label + "-" + String(Math.random()).slice(2),
  });
}

describe("zen.push()", function () {
  this.timeout(10 * 1000);

  var zen;

  before(function () {
    zen = makeZEN("a");
  });

  after(function () {
    if (zen && zen.off) zen.off();
  });

  it("is a function on the zen instance", function () {
    assert.strictEqual(typeof zen.push, "function");
  });

  it("returns a chainable object", function () {
    // The chain method returns `this` (the chain proxy), which supports further chaining.
    // Note: the returned proxy is not reference-equal to the ZEN wrapper instance.
    var result = zen.push("fakepub", "hello");
    assert.ok(result != null, "should return something");
    assert.strictEqual(typeof result.push, "function", "returned value should support chaining");
  });

  it("does not throw when mesh is unavailable", function () {
    assert.doesNotThrow(function () {
      zen.push(null, "data");
      zen.push("", "data");
      zen.push("somepub", null);
      zen.push("somepub", undefined);
    });
  });

  it("is chainable after other chain calls", function () {
    assert.doesNotThrow(function () {
      zen.push("fakepub", "msg1").push("fakepub", "msg2");
    });
  });
});

// Integration: zen.push() routes via mesh.relay() when mesh is present
describe("zen.push() — mesh routing", function () {
  this.timeout(10 * 1000);

  /**
   * Build a minimal mesh instance linked to a ZEN graph.
   * Returns { zen, root, mesh }.
   */
  function makeGraphWithMesh(label) {
    const graph = ZEN.graph.create({
      peers: {},
      localStorage: false,
      WebSocket: false,
      file: "tmp/push-mesh-" + label + "-" + String(Math.random()).slice(2),
    });
    // graph._ is the chain "at" state; the actual root (with opt) is at.root.
    // WebSocket:false skips mesh init, so create one manually (same pattern as dam.js).
    const at = graph._;
    const root = at.root;
    const mesh = root.opt.mesh || ZEN.Mesh(root);
    root.opt.mesh = mesh;
    return { zen: graph, root, mesh };
  }

  it("delegates to mesh.relay() with matching arguments", function (done) {
    var { zen, mesh } = makeGraphWithMesh("delegate");
    if (!mesh) { this.skip(); return; } // skip if mesh unavailable in this build

    var captured = null;
    var origRelay = mesh.relay;
    mesh.relay = function (to, data, opt) {
      captured = { to, data, opt };
      if (origRelay) origRelay.call(this, to, data, opt);
    };

    zen.push("targetpub", "payload", { ttl: 3 });

    // relay call is synchronous inside push()
    assert.ok(captured, "mesh.relay should have been called");
    assert.strictEqual(captured.to, "targetpub");
    assert.strictEqual(captured.data, "payload");
    assert.deepStrictEqual(captured.opt, { ttl: 3 });

    if (mesh.relay !== origRelay) mesh.relay = origRelay;
    done();
  });

  it("delivers to mesh.onRelay() subscriber when mesh routes back to self", function (done) {
    var { zen, root, mesh } = makeGraphWithMesh("onrelay");
    if (!mesh || !mesh.onRelay) { this.skip(); return; }

    // relay handler delivers locally only when msg.to === opt.pub
    var selfPub = "ZmockPubKeyForTest0000000000000000000000000001";
    root.opt.pub = selfPub;

    var received = null;
    var off = mesh.onRelay(function (payload) {
      received = payload;
    });

    // Simulate the relay arriving at this node addressed to our pub key
    if (mesh.hear && mesh.hear["relay"]) {
      mesh.hear["relay"](
        { "#": "test-push-1", dam: "relay", to: selfPub, from: "other", ttl: 3, data: "direct" },
        { id: "other" }
      );
    }

    setTimeout(function () {
      off();
      assert.ok(received !== null, "onRelay handler should have fired");
      assert.strictEqual(received.data, "direct");
      assert.strictEqual(received.from, "other");
      done();
    }, 100);
  });
});
