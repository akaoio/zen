/**
 * test/mesh/dam.js — unit tests for DAM ping/pong RTT and XOR distance
 *
 * Tests are split into two groups:
 *   1. Pure math: mesh.xor() and mesh.closer() — no network, no peers
 *   2. Protocol:  ping/pong round-trip via mock peer wire
 */

import assert from "assert";
import { ZEN } from "../../zen.js";

// ── helpers ────────────────────────────────────────────────────────────────

/** Build a minimal mesh instance from a fresh ZEN graph root. */
function makeMesh() {
  const graph = ZEN.graph.create({ localStorage: false, peers: {}, WebSocket: false });
  const root = graph._;
  const mesh = root.opt.mesh || ZEN.Mesh(root);
  root.opt.mesh = mesh;
  return { mesh, root };
}

/**
 * Create a mock peer pair (A ↔ B) where messages sent by A appear as
 * received messages on B's mesh and vice-versa, all in-process.
 */
function makePeerPair(meshA, meshB) {
  const peerA = {
    id: "peer-a",
    url: "mock://a",
    wire: {
      send: function (raw) {
        // A sends → B receives
        meshB.hear(raw, peerB);
      },
    },
  };
  const peerB = {
    id: "peer-b",
    url: "mock://b",
    wire: {
      send: function (raw) {
        // B sends → A receives
        meshA.hear(raw, peerA);
      },
    },
  };
  return { peerA, peerB };
}

// ── XOR distance ───────────────────────────────────────────────────────────

describe("mesh.xor(a, b)", function () {
  let mesh;

  before(function () {
    ({ mesh } = makeMesh());
  });

  it("returns a BigInt for two valid base62 pub keys", async function () {
    const pairA = await ZEN.pair();
    const pairB = await ZEN.pair();
    const dist = mesh.xor(pairA.pub, pairB.pub);
    assert.strictEqual(typeof dist, "bigint");
    assert.ok(dist >= 0n);
  });

  it("returns 0n when both keys are identical", async function () {
    const pair = await ZEN.pair();
    const dist = mesh.xor(pair.pub, pair.pub);
    assert.strictEqual(dist, 0n);
  });

  it("is symmetric: xor(a,b) === xor(b,a)", async function () {
    const pairA = await ZEN.pair();
    const pairB = await ZEN.pair();
    assert.strictEqual(mesh.xor(pairA.pub, pairB.pub), mesh.xor(pairB.pub, pairA.pub));
  });

  it("returns null for missing or invalid inputs", function () {
    assert.strictEqual(mesh.xor(null, "abc"), null);
    assert.strictEqual(mesh.xor("abc", null), null);
    assert.strictEqual(mesh.xor(null, null), null);
    assert.strictEqual(mesh.xor("", "abc"), null);
  });

  it("returns null for strings containing non-base62 chars", function () {
    // '@' is not in base62 alphabet
    assert.strictEqual(mesh.xor("@invalid!", "alsoBAD@"), null);
  });
});

// ── mesh.closer ────────────────────────────────────────────────────────────

describe("mesh.closer(target, a, b)", function () {
  let mesh;

  before(function () {
    ({ mesh } = makeMesh());
  });

  it("returns a or b (not null) for valid pub keys", async function () {
    const [t, a, b] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    const result = mesh.closer(t.pub, a.pub, b.pub);
    assert.ok(result === a.pub || result === b.pub);
  });

  it("returns a when a === target (distance 0)", async function () {
    const [target, b] = await Promise.all([ZEN.pair(), ZEN.pair()]);
    const result = mesh.closer(target.pub, target.pub, b.pub);
    assert.strictEqual(result, target.pub);
  });

  it("returns b when b === target (distance 0)", async function () {
    const [a, target] = await Promise.all([ZEN.pair(), ZEN.pair()]);
    const result = mesh.closer(target.pub, a.pub, target.pub);
    assert.strictEqual(result, target.pub);
  });

  it("is consistent with xor distances", async function () {
    const [t, a, b] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    const distA = mesh.xor(t.pub, a.pub);
    const distB = mesh.xor(t.pub, b.pub);
    const expected = distA <= distB ? a.pub : b.pub;
    assert.strictEqual(mesh.closer(t.pub, a.pub, b.pub), expected);
  });

  it("returns null for missing arguments", function () {
    assert.strictEqual(mesh.closer(null, "a", "b"), null);
    assert.strictEqual(mesh.closer("a", null, "b"), null);
    assert.strictEqual(mesh.closer("a", "b", null), null);
  });
});

// ── ping / pong RTT ────────────────────────────────────────────────────────

describe("DAM ping/pong RTT", function () {
  this.timeout(5000);

  it("pong handler sets peer.rtt after a round trip", function (done) {
    const { mesh: meshA } = makeMesh();
    const { mesh: meshB } = makeMesh();
    const { peerA, peerB } = makePeerPair(meshA, meshB);

    // Wire up opt.peers so mesh.say can resolve the peer
    meshA._root = meshA._root || {};
    // Manually register the peer in meshB so it can send back
    meshB.hi(peerB);

    // Override peerA's wire.send to also call hi so meshA knows peerA
    meshA.hi(peerA);

    // At this point meshA has peerA as a connected peer.
    // Send a ping from meshA to peerA — peerA's wire.send will deliver to meshB,
    // which will reply with a pong that arrives back at meshA via peerB.wire.send.

    // Wait for peerA.rtt to be set (pong comes back to meshA updating peerA.rtt)
    const deadline = setTimeout(function () {
      done(new Error("Timed out waiting for peer.rtt"));
    }, 4000);

    var origSend = peerA.wire.send;
    peerA.wire.send = function (raw) {
      origSend(raw);
      // After a small delay check if rtt was set
      setTimeout(function () {
        if (peerA.rtt !== undefined) {
          clearTimeout(deadline);
          assert.ok(typeof peerA.rtt === "number", "peer.rtt should be a number");
          assert.ok(peerA.rtt >= 0, "peer.rtt should be non-negative");
          done();
        }
      }, 50);
    };

    meshA.ping(peerA);
  });

  it("pong handler updates rtt as rolling average on repeated pings", function (done) {
    this.timeout(5000);
    const { mesh: meshA } = makeMesh();
    const { mesh: meshB } = makeMesh();
    const { peerA, peerB } = makePeerPair(meshA, meshB);

    meshB.hi(peerB);
    meshA.hi(peerA);

    var count = 0;
    var orig = peerA.wire.send;

    function sendPing() {
      peerA.wire.send = function (raw) {
        orig(raw);
        setTimeout(function () {
          if (peerA.rtt !== undefined) {
            count++;
            if (count < 3) {
              sendPing();
            } else {
              // After 3 pings the rtt should still be a number ≥ 0
              assert.ok(typeof peerA.rtt === "number");
              assert.ok(peerA.rtt >= 0);
              done();
            }
          }
        }, 50);
      };
      meshA.ping(peerA);
    }

    sendPing();
  });

  it("ping handler sends a pong back with matching timestamp", function (done) {
    this.timeout(2000);
    const { mesh: meshA } = makeMesh();
    const { mesh: meshB } = makeMesh();

    // Create a spy peer on meshA side
    var receivedPong = null;
    const spyPeer = {
      id: "spy",
      url: "mock://spy",
      wire: {
        send: function (raw) {
          // parse the JSON and check if it's a pong
          try {
            var parsed = typeof raw === "string" ? JSON.parse(raw) : raw;
            if (parsed.dam === "pong") {
              receivedPong = parsed;
            }
          } catch (e) {}
        },
      },
    };

    const t = +new Date();
    meshB.hear["ping"]({ "#": "test-ping-id", dam: "ping", t: t }, spyPeer);

    setTimeout(function () {
      assert.ok(receivedPong !== null, "should have received pong");
      assert.strictEqual(receivedPong.dam, "pong");
      assert.strictEqual(receivedPong.t, t);
      assert.strictEqual(receivedPong["@"], "test-ping-id");
      done();
    }, 100);
  });

  it("pong handler ignores messages without timestamp", function () {
    const { mesh } = makeMesh();
    const peer = { id: "norts", wire: { send: function () {} } };
    // Should not throw and should not set rtt
    mesh.hear["pong"]({ dam: "pong" }, peer);
    assert.strictEqual(peer.rtt, undefined);
  });
});

// ── ? handshake pub propagation ────────────────────────────────────────────

describe("DAM ? handshake pub propagation", function () {
  it("stores peer.pub from handshake message", function () {
    const { mesh } = makeMesh();
    const peer = { id: "hello", wire: { send: function () {} } };
    const fakePub = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefgh01"; // 45-char
    mesh.hear["?"]({ "#": "h1", dam: "?", pid: "remote-pid", pub: fakePub }, peer);
    assert.strictEqual(peer.pub, fakePub);
  });

  it("does not overwrite existing peer.pub", function () {
    const { mesh } = makeMesh();
    const existingPub = "existing_pub_key_abc";
    const peer = { id: "hello2", pub: existingPub, wire: { send: function () {} } };
    mesh.hear["?"]({ "#": "h2", dam: "?", pid: "remote-pid2", pub: "new_pub_key_xyz" }, peer);
    assert.strictEqual(peer.pub, existingPub);
  });
});

// ── DAM multi-hop relay ────────────────────────────────────────────────────

// Valid 45-char base62 pub key stubs (all chars are in base62 alphabet).
const PUB_A = "0000000000000000000000000000000000000000000A0";
const PUB_B = "0000000000000000000000000000000000000000000B0";
const PUB_C = "0000000000000000000000000000000000000000000C0";

describe("mesh.route(targetPub, skip)", function () {
  it("returns null when no peers are registered", function () {
    const { mesh } = makeMesh();
    assert.strictEqual(mesh.route(PUB_A), null);
  });

  it("returns null when targetPub is missing", function () {
    const { mesh } = makeMesh();
    assert.strictEqual(mesh.route(null), null);
    assert.strictEqual(mesh.route(""), null);
  });

  it("returns null when peers have no pub keys", function () {
    const { mesh } = makeMesh();
    const peer = { id: "nopub", wire: { send: function () {} } };
    mesh.hi(peer);
    assert.strictEqual(mesh.route(PUB_A), null);
  });

  it("returns the only peer when there is just one", async function () {
    const { mesh } = makeMesh();
    const peerPub = (await ZEN.pair()).pub;
    const peer = { id: "one", pub: peerPub, wire: { send: function () {} } };
    mesh.hi(peer);
    const targetPub = (await ZEN.pair()).pub;
    const result = mesh.route(targetPub);
    assert.strictEqual(result, peer);
  });

  it("returns the closer of two peers by XOR distance", async function () {
    const { mesh } = makeMesh();
    const [target, pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    const peerA = { id: "pa", pub: pairA.pub, wire: { send: function () {} } };
    const peerB = { id: "pb", pub: pairB.pub, wire: { send: function () {} } };
    mesh.hi(peerA);
    mesh.hi(peerB);
    const expected = mesh.closer(target.pub, pairA.pub, pairB.pub);
    const result = mesh.route(target.pub);
    assert.ok(result === peerA || result === peerB);
    assert.strictEqual(result.pub, expected);
  });

  it("skips the skip peer when routing", async function () {
    const { mesh } = makeMesh();
    const [target, pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    const peerA = { id: "pa", pub: pairA.pub, wire: { send: function () {} } };
    const peerB = { id: "pb", pub: pairB.pub, wire: { send: function () {} } };
    mesh.hi(peerA);
    mesh.hi(peerB);
    // Determine which peer is naturally closest
    const closerPub = mesh.closer(target.pub, pairA.pub, pairB.pub);
    const skipPeer = closerPub === pairA.pub ? peerA : peerB;
    const otherPeer = closerPub === pairA.pub ? peerB : peerA;
    const result = mesh.route(target.pub, skipPeer);
    assert.strictEqual(result, otherPeer);
  });
});

describe("mesh.relay(to, data, opt)", function () {
  it("does nothing when 'to' is missing", function () {
    const { mesh } = makeMesh();
    assert.doesNotThrow(() => mesh.relay(null, "data"));
    assert.doesNotThrow(() => mesh.relay("", "data"));
  });

  it("sends directly to a connected peer whose pub matches 'to'", function (done) {
    const { mesh } = makeMesh();
    const peer = {
      id: "target", pub: PUB_B,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") {
          assert.strictEqual(m.to, PUB_B);
          assert.strictEqual(m.data, "hi B");
          done();
        }
      }},
    };
    mesh.hi(peer);
    mesh.relay(PUB_B, "hi B");
  });

  it("defaults to ttl: 5", function (done) {
    const { mesh } = makeMesh();
    const peer = {
      id: "t", pub: PUB_B,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") { assert.strictEqual(m.ttl, 5); done(); }
      }},
    };
    mesh.hi(peer);
    mesh.relay(PUB_B, "data");
  });

  it("respects custom ttl in opt", function (done) {
    const { mesh } = makeMesh();
    const peer = {
      id: "t", pub: PUB_B,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") { assert.strictEqual(m.ttl, 3); done(); }
      }},
    };
    mesh.hi(peer);
    mesh.relay(PUB_B, "data", { ttl: 3 });
  });

  it("routes via closest peer when target is not directly connected", async function () {
    this.timeout(3000);
    const { mesh } = makeMesh();
    const [pairB] = await Promise.all([ZEN.pair()]);
    let sent = false;
    const peerB = {
      id: "b", pub: pairB.pub,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") { sent = true; }
      }},
    };
    mesh.hi(peerB);
    // PUB_C is not a connected peer, so the relay goes through peerB
    mesh.relay(PUB_C, "route me");
    await new Promise((r) => setTimeout(r, 200));
    assert.ok(sent, "expected relay message to be forwarded via closest peer");
  });
});

describe("hear['relay'] handler", function () {
  it("drops message when ttl is 0", function () {
    const { mesh } = makeMesh();
    let forwarded = false;
    const peer = { id: "x", pub: PUB_A, wire: { send: function () { forwarded = true; } } };
    mesh.hi(peer);
    mesh.hear["relay"]({ "#": "d1", dam: "relay", to: PUB_A, from: "s", ttl: 0, data: "x" }, { id: "s" });
    assert.strictEqual(forwarded, false);
  });

  it("drops message when ttl is negative", function () {
    const { mesh } = makeMesh();
    let forwarded = false;
    const peer = { id: "x", pub: PUB_A, wire: { send: function () { forwarded = true; } } };
    mesh.hi(peer);
    mesh.hear["relay"]({ "#": "d2", dam: "relay", to: PUB_A, from: "s", ttl: -1, data: "x" }, { id: "s" });
    assert.strictEqual(forwarded, false);
  });

  it("drops message without 'to' or 'from' field", function () {
    const { mesh } = makeMesh();
    assert.doesNotThrow(() => {
      mesh.hear["relay"]({ "#": "d3", dam: "relay", ttl: 5, data: "x" }, { id: "s" });
      mesh.hear["relay"]({ "#": "d4", dam: "relay", to: PUB_A, ttl: 5, data: "x" }, { id: "s" });
    });
  });

  it("delivers locally when msg.to matches opt.pub", function (done) {
    const { mesh, root } = makeMesh();
    root.opt.pub = PUB_A;
    mesh.onRelay(function (payload) {
      assert.strictEqual(payload.from, "senderpub");
      assert.strictEqual(payload.data, "secret");
      done();
    });
    mesh.hear["relay"]({ "#": "dl1", dam: "relay", to: PUB_A, from: "senderpub", ttl: 3, data: "secret" }, { id: "s" });
  });

  it("forwards directly to connected peer with matching pub", function (done) {
    const { mesh } = makeMesh();
    const targetPeer = {
      id: "tgt", pub: PUB_B,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") {
          assert.strictEqual(m.to, PUB_B);
          assert.strictEqual(m.ttl, 3); // 4 - 1
          done();
        }
      }},
    };
    mesh.hi(targetPeer);
    mesh.hear["relay"]({ "#": "fwd1", dam: "relay", to: PUB_B, from: "s", ttl: 4, data: "payload" }, { id: "s" });
  });

  it("decrements ttl by 1 on every forward", function (done) {
    const { mesh } = makeMesh();
    const peer = {
      id: "p", pub: PUB_B,
      wire: { send: function (raw) {
        let m; try { m = JSON.parse(raw); } catch (e) {}
        if (m && m.dam === "relay") { assert.strictEqual(m.ttl, 1); done(); }
      }},
    };
    mesh.hi(peer);
    mesh.hear["relay"]({ "#": "fwd2", dam: "relay", to: PUB_B, from: "s", ttl: 2, data: "x" }, { id: "s" });
  });

  it("skips the sender peer when routing forward", async function () {
    const { mesh } = makeMesh();
    const [target, pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    let sentToA = 0, sentToB = 0;
    const peerA = { id: "pa", pub: pairA.pub, wire: { send: function (r) {
      let m; try { m = JSON.parse(r); } catch (e) {}
      if (m && m.dam === "relay") sentToA++;
    }}};
    const peerB = { id: "pb", pub: pairB.pub, wire: { send: function (r) {
      let m; try { m = JSON.parse(r); } catch (e) {}
      if (m && m.dam === "relay") sentToB++;
    }}};
    mesh.hi(peerA);
    mesh.hi(peerB);
    // Use the XOR-closer peer as the sender — it must be skipped
    const closerPub = mesh.closer(target.pub, pairA.pub, pairB.pub);
    const senderPeer = closerPub === pairA.pub ? peerA : peerB;
    mesh.hear["relay"]({ "#": "sk1", dam: "relay", to: target.pub, from: "s", ttl: 3, data: "x" }, senderPeer);
    await new Promise((r) => setTimeout(r, 200));
    if (senderPeer === peerA) {
      assert.strictEqual(sentToA, 0, "should not send back to sender");
      assert.strictEqual(sentToB, 1, "should forward to other peer");
    } else {
      assert.strictEqual(sentToB, 0, "should not send back to sender");
      assert.strictEqual(sentToA, 1, "should forward to other peer");
    }
  });
});

describe("mesh.onRelay(fn)", function () {
  it("returns an off() function that stops delivery", function (done) {
    const { mesh, root } = makeMesh();
    root.opt.pub = PUB_A;
    let count = 0;
    const off = mesh.onRelay(function () { count++; });
    mesh.hear["relay"]({ "#": "or1", dam: "relay", to: PUB_A, from: "s", ttl: 2, data: "a" }, { id: "s" });
    setTimeout(function () {
      assert.strictEqual(count, 1);
      off();
      mesh.hear["relay"]({ "#": "or2", dam: "relay", to: PUB_A, from: "s", ttl: 2, data: "b" }, { id: "s" });
      setTimeout(function () {
        assert.strictEqual(count, 1, "handler should not fire after off()");
        done();
      }, 50);
    }, 50);
  });

  it("multiple handlers all receive the message", function (done) {
    const { mesh, root } = makeMesh();
    root.opt.pub = PUB_A;
    let count = 0;
    mesh.onRelay(function () { count++; });
    mesh.onRelay(function () { count++; });
    mesh.hear["relay"]({ "#": "or3", dam: "relay", to: PUB_A, from: "s", ttl: 2, data: "x" }, { id: "s" });
    setTimeout(function () {
      assert.strictEqual(count, 2);
      done();
    }, 50);
  });
});

describe("DAM relay end-to-end: A → B → C", function () {
  this.timeout(6000);

  it("message from A reaches C via B acting as relay", async function () {
    const [pairA, pairB, pairC] = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);

    const { mesh: meshA, root: rootA } = makeMesh();
    const { mesh: meshB, root: rootB } = makeMesh();
    const { mesh: meshC, root: rootC } = makeMesh();

    rootA.opt.pub = pairA.pub;
    rootB.opt.pub = pairB.pub;
    rootC.opt.pub = pairC.pub;

    // Cross-wire A ↔ B
    const peerBatA = { id: "b-at-a", pub: pairB.pub, wire: { send: function (r) { meshB.hear(r, peerAatB); } } };
    const peerAatB = { id: "a-at-b", pub: pairA.pub, wire: { send: function (r) { meshA.hear(r, peerBatA); } } };
    meshA.hi(peerBatA);
    meshB.hi(peerAatB);

    // Cross-wire B ↔ C
    const peerCatB = { id: "c-at-b", pub: pairC.pub, wire: { send: function (r) { meshC.hear(r, peerBatC); } } };
    const peerBatC = { id: "b-at-c", pub: pairB.pub, wire: { send: function (r) { meshB.hear(r, peerCatB); } } };
    meshB.hi(peerCatB);
    meshC.hi(peerBatC);

    // A has no direct connection to C — must route through B
    return new Promise(function (resolve, reject) {
      const t = setTimeout(function () { reject(new Error("Timed out: message did not reach C")); }, 5000);
      meshC.onRelay(function (payload) {
        clearTimeout(t);
        try {
          assert.strictEqual(payload.from, pairA.pub);
          assert.strictEqual(payload.data, "hello from A to C");
          resolve();
        } catch (e) { reject(e); }
      });
      meshA.relay(pairC.pub, "hello from A to C");
    });
  });

  it("TTL prevents infinite loops between two bidirectional peers", async function () {
    const [pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair()]);
    const { mesh: meshA, root: rootA } = makeMesh();
    const { mesh: meshB, root: rootB } = makeMesh();
    rootA.opt.pub = pairA.pub;
    rootB.opt.pub = pairB.pub;

    const peerBatA = { id: "b-at-a", pub: pairB.pub, wire: { send: function (r) { meshB.hear(r, peerAatB); } } };
    const peerAatB = { id: "a-at-b", pub: pairA.pub, wire: { send: function (r) { meshA.hear(r, peerBatA); } } };
    meshA.hi(peerBatA);
    meshB.hi(peerAatB);

    let deliveries = 0;
    meshB.onRelay(function () { deliveries++; });

    // Send to a non-existent pub — will be routed back and forth until TTL=0
    // The DAM dedup (#) ensures the same message is never processed twice per node
    const unknownPub = (await ZEN.pair()).pub;
    meshA.relay(unknownPub, "loop test", { ttl: 4 });

    await new Promise((r) => setTimeout(r, 500));
    assert.strictEqual(deliveries, 0, "should not deliver to wrong target");
  });
});
