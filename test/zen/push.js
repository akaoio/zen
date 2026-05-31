/**
 * test/zen/push.js — tests for zen.push() and zen.mesh.on()
 *
 * zen.push(targetPub, data, opt) sends an ephemeral DAM relay message.
 * zen.mesh.on(fn) subscribes to incoming push messages; returns off().
 * Neither persists to the graph. Transport: mesh.relay() under the hood.
 *
 * Integration tests use a real in-process HTTP+WebSocket relay (lib/wire.js)
 * so the full DAM relay path is exercised: WebSocket → dedup → mesh.hear →
 * mesh.relay routing → peer forwarding → local delivery via mesh.on().
 */
import assert from "assert";
import http from "http";
import ZEN from "../../index.js"; // full Node.js ZEN — includes lib/wire.js

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// ── helpers ────────────────────────────────────────────────────────────────────

/** Spin up a real HTTP+WebSocket ZEN relay on a random port. */
async function startRelay() {
  const srv = http.createServer();
  new ZEN({ web: srv, localStorage: false, axe: false, multicast: false });
  await new Promise((r) => srv.listen(0, r));
  return { srv, url: `http://127.0.0.1:${srv.address().port}/zen` };
}

async function stopRelay({ srv }) {
  await new Promise((r) => srv.close(r));
}

/**
 * Connect a ZEN client to the relay, announcing itself with pair.pub so the
 * relay can route DAM relay messages toward this node.
 *
 * super: false — overrides lib/server.js which sets super:true on every
 * instance; without it the websocket.js bootstrap broadcast is skipped and
 * no outbound connections are ever opened.
 */
function makeClient(relayUrl, pair) {
  return new ZEN({
    peers: [relayUrl],
    pub: pair.pub,
    localStorage: false,
    super: false,
    axe: false,
    multicast: false,
  });
}

/** Poll until zen.mesh.near ≥ 1 (WebSocket handshake complete). */
async function waitConnected(zen, timeout = 5000) {
  const start = Date.now();
  while (Date.now() - start < timeout) {
    const m = zen.mesh;
    if (m && m.near > 0) return;
    await sleep(50);
  }
  throw new Error("timeout waiting for mesh connection");
}

function closeClient(zen) {
  try {
    const peers = zen._opt && zen._opt.peers;
    if (!peers) return;
    for (const k of Object.keys(peers)) {
      const p = peers[k];
      if (p) {
        p._noReconnect = true;
        if (p.wire && typeof p.wire.close === "function") {
          try { p.wire.close(); } catch (_) {}
        }
      }
    }
  } catch (_) {}
}

// ── zen.push() — basic contract (no WS) ────────────────────────────────────

describe("zen.push() — basic contract", function () {
  this.timeout(5000);

  it("is a function on a class instance", function () {
    const zen = new ZEN({ localStorage: false, axe: false, peers: [] });
    assert.strictEqual(typeof zen.push, "function");
    closeClient(zen);
  });

  it("returns the chain (chainable)", function () {
    const zen = new ZEN({ localStorage: false, axe: false, peers: [] });
    const ret = zen.push("fakepub", "hello");
    assert.ok(ret != null);
    assert.strictEqual(typeof ret.push, "function");
    closeClient(zen);
  });

  it("does not throw for missing or null arguments", function () {
    const zen = new ZEN({ localStorage: false, axe: false, peers: [] });
    assert.doesNotThrow(() => {
      zen.push(null, "data");
      zen.push("", "data");
      zen.push("somepub", null);
    });
    closeClient(zen);
  });

  it("zen.mesh is null when no WebSocket peers are configured", function () {
    // graph.create with WebSocket:false → mesh not initialised → facade returns null
    const g = ZEN.graph.create({ localStorage: false, peers: {}, WebSocket: false });
    // override mesh to ensure null path
    g._.root.opt.mesh = null;
    assert.strictEqual(g.mesh, null);
  });
});

// ── zen.mesh facade — real ZEN class instances ─────────────────────────────

describe("zen.mesh — facade on ZEN class instance", function () {
  this.timeout(10000);
  let relay, clientA, clientB, pairA, pairB;

  before(async function () {
    [pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair()]);
    relay = await startRelay();
    // NOTE: must NOT use Promise.all([makeClient(), makeClient()]) here.
    // ZEN.chain.then is mirrored to ZEN.prototype, making every instance
    // thenable. Promise.all would call .then() on each client and hang
    // forever waiting for graph data that never arrives.
    clientA = makeClient(relay.url, pairA);
    clientB = makeClient(relay.url, pairB);
    await Promise.all([waitConnected(clientA), waitConnected(clientB)]);
  });

  after(async function () {
    closeClient(clientA);
    closeClient(clientB);
    await stopRelay(relay);
  });

  it("zen.mesh is not null when connected to a relay", function () {
    assert.ok(clientA.mesh !== null);
  });

  it("zen.mesh.near ≥ 1 (connected to relay)", function () {
    assert.ok(clientA.mesh.near >= 1, `expected near≥1, got ${clientA.mesh.near}`);
  });

  it("zen.mesh.peers returns connected peer list", function () {
    const peers = clientA.mesh.peers;
    assert.ok(Array.isArray(peers));
    assert.ok(peers.length >= 1);
    assert.ok(typeof peers[0].id === "string");
  });

  it("zen.mesh.stats has { msgs, bytes } as numbers", function () {
    const s = clientA.mesh.stats;
    assert.strictEqual(typeof s.msgs, "number");
    assert.strictEqual(typeof s.bytes, "number");
  });

  it("zen.mesh.xor(a, b) returns BigInt XOR distance", function () {
    const d = clientA.mesh.xor(pairA.pub, pairB.pub);
    assert.strictEqual(typeof d, "bigint");
    assert.ok(d > 0n);
  });

  it("zen.mesh.on() returns an off() function", function () {
    const off = clientA.mesh.on(function () {});
    assert.strictEqual(typeof off, "function");
    off();
  });
});

// ── zen.push() → zen.mesh.on() — real WebSocket relay ─────────────────────

describe("zen.push() → zen.mesh.on() — real WebSocket relay", function () {
  this.timeout(10000);
  let relay, clientA, clientB, pairA, pairB;

  before(async function () {
    [pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair()]);
    relay = await startRelay();
    clientA = makeClient(relay.url, pairA);
    clientB = makeClient(relay.url, pairB);
    await Promise.all([waitConnected(clientA), waitConnected(clientB)]);
  });

  after(async function () {
    closeClient(clientA);
    closeClient(clientB);
    await stopRelay(relay);
  });

  it("A.push(B.pub, data) is received by B via zen.mesh.on()", function (done) {
    const off = clientB.mesh.on(function ({ from, data }) {
      if (data !== "hello-ws") return; // filter unrelated relay messages
      off();
      try {
        assert.strictEqual(from, pairA.pub);
        assert.strictEqual(data, "hello-ws");
        done();
      } catch (e) { done(e); }
    });
    clientA.push(pairB.pub, "hello-ws");
  });

  it("off() stops further delivery across the real relay", function (done) {
    let count = 0;
    const off = clientB.mesh.on(function ({ data }) {
      if (data === "off-test-1" || data === "off-test-2") count++;
    });
    clientA.push(pairB.pub, "off-test-1");
    setTimeout(function () {
      assert.strictEqual(count, 1);
      off();
      clientA.push(pairB.pub, "off-test-2");
      setTimeout(function () {
        assert.strictEqual(count, 1, "handler must not fire after off()");
        done();
      }, 300);
    }, 500);
  });

  it("push carries an object payload across the WebSocket relay", function (done) {
    const payload = { score: 42, ok: true, label: "ws-obj" };
    const off = clientB.mesh.on(function ({ data }) {
      if (!data || data.label !== "ws-obj") return;
      off();
      try {
        assert.deepStrictEqual(data, payload);
        done();
      } catch (e) { done(e); }
    });
    clientA.push(pairB.pub, payload);
  });

  it("multiple zen.mesh.on() subscribers all fire for one push", function (done) {
    let hits = 0;
    const check = ({ data }) => {
      if (data !== "broadcast-ws") return;
      if (++hits === 2) { off1(); off2(); done(); }
    };
    const off1 = clientB.mesh.on(check);
    const off2 = clientB.mesh.on(check);
    clientA.push(pairB.pub, "broadcast-ws");
  });

  it("push to unknown pub does not deliver to the wrong node", function (done) {
    let received = false;
    const off = clientB.mesh.on(function ({ data }) {
      if (data === "wrong-target") received = true;
    });
    clientA.push("completely-unknown-pub-xxxxxxxxxxxxxxxx", "wrong-target");
    setTimeout(function () {
      off();
      assert.strictEqual(received, false, "B must not receive a message aimed at an unknown pub");
      done();
    }, 400);
  });
});

