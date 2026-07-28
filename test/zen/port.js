/**
 * test/zen/port.js — tests for zen.attach() (src/port.js MessagePort transport)
 *
 * zen.attach(port, opts) wires a graph to another ZEN realm over a
 * MessagePort-like channel. These tests exercise the REAL sync path — two
 * live ZEN instances exchanging DAM messages over Node MessageChannel /
 * worker_threads — no mocks, no stubbed mesh.
 */
import assert from "assert";
import { MessageChannel, Worker } from "node:worker_threads";
import { fileURLToPath } from "node:url";
import ZEN from "../../zen.js"; // core bundle — where src/port.js lives

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function makeEphemeral() {
  return new ZEN({
    peers: [],
    WebSocket: false,
    localStorage: false,
    radisk: false,
    rfs: false, // critical: Node rfs stores are memoized per file path and
    stats: false, // shared process-wide — a live store would sync realms
    axe: false, //   through the disk behind the transport's back
    multicast: false,
  });
}

/**
 * Wait for a chain to emit a defined value via .on(), or resolve undefined
 * after `ms`. Event-driven — no polling races under load.
 */
function read(chain, ms = 8000) {
  return new Promise((resolve) => {
    let done = false;
    const timer = setTimeout(() => {
      if (!done) { done = true; resolve(undefined); }
    }, ms);
    chain.on((value) => {
      if (done || value === undefined) return;
      done = true;
      clearTimeout(timer);
      resolve(value);
    });
  });
}

describe("zen.attach — MessagePort transport", function () {
  this.timeout(15000);

  it("returns null for an unusable port", () => {
    const zen = makeEphemeral();
    assert.equal(zen.attach(null), null);
    assert.equal(zen.attach({}), null);
  });

  it("syncs puts both ways across a MessageChannel", async () => {
    const { port1, port2 } = new MessageChannel();
    const a = makeEphemeral();
    const b = makeEphemeral();

    const detachA = a.attach(port1);
    const detachB = b.attach(port2);
    assert.equal(typeof detachA, "function");
    assert.equal(typeof detachB, "function");

    a.get("port-test").get("msg").put("hello");
    assert.equal(await read(b.get("port-test").get("msg")), "hello");

    b.get("port-test").get("reply").put("world");
    assert.equal(await read(a.get("port-test").get("reply")), "world");

    detachA();
    detachB();
    port1.close();
    port2.close();
  });

  it("counts the attached realm as a connected peer", async () => {
    const { port1, port2 } = new MessageChannel();
    const a = makeEphemeral();
    const b = makeEphemeral();
    const detachA = a.attach(port1);
    const detachB = b.attach(port2);

    // The DAM "?" handshake runs in both directions; wait for it to settle.
    let near = 0;
    for (let i = 0; i < 40 && near < 1; i++) {
      near = a.mesh ? a.mesh.near : 0;
      if (near < 1) await sleep(50);
    }
    assert.ok(near >= 1, `expected at least 1 connected peer, got ${near}`);

    detachA();
    detachB();
    port1.close();
    port2.close();
  });

  it("stops propagation after detach", async () => {
    const { port1, port2 } = new MessageChannel();
    const a = makeEphemeral();
    const b = makeEphemeral();
    const detachA = a.attach(port1);
    const detachB = b.attach(port2);

    a.get("detach-test").get("before").put("yes");
    assert.equal(await read(b.get("detach-test").get("before")), "yes");

    detachA();
    detachB();

    a.get("detach-test").get("after").put("no");
    const leaked = await read(b.get("detach-test").get("after"), 1500);
    assert.equal(leaked, undefined, "detached realms must not sync");

    port1.close();
    port2.close();
  });

  it("syncs with a real worker_threads realm", async () => {
    const fixture = fileURLToPath(new URL("./fixtures/port-worker.js", import.meta.url));
    const worker = new Worker(fixture);
    const { port1, port2 } = new MessageChannel();

    const main = makeEphemeral();
    const detach = main.attach(port2);

    const attached = new Promise((resolve) => worker.once("message", resolve));
    worker.postMessage(port1, [port1]);
    assert.equal(await attached, "attached");

    // Worker wrote before acking; value must arrive through the port.
    assert.equal(await read(main.get("worker-room").get("hello")), "from-worker");

    // Reverse direction: main writes, worker reports what it read.
    const echoed = new Promise((resolve) => worker.once("message", resolve));
    main.get("worker-room").get("question").put("ping");
    assert.equal(await echoed, "ping");

    detach();
    port2.close();
    await worker.terminate();
  });
});
