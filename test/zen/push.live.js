/**
 * test/zen/push.live.js — live end-to-end test for zen.push() / zen.mesh.on()
 *
 * Runs against real production relay nodes (zen.akao.io, zen0.akao.io).
 * AXE is enabled (no axe:false). super:false so client nodes open outbound
 * WebSocket connections (lib/server.js sets super:true by default).
 *
 * Usage (run on a VPS or any machine with outbound access to port 8420):
 *   node ~/zen/test/zen/push.live.js
 *
 * Pass/fail reported via process.exit(0/1) — no mocha required.
 */
import ZEN from "../../index.js";

const RELAY_A = "wss://zen.akao.io:8420/zen";
const RELAY_B = "wss://zen0.akao.io:8420/zen";
const TIMEOUT = 20000;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let pass = 0;
let fail = 0;

function ok(name) {
  console.log("  \u2713 " + name);
  pass++;
}
function err(name, e) {
  console.error("  \u2717 " + name + ": " + (e && e.message || e));
  fail++;
}

async function waitConnected(zen, label, timeout) {
  const start = Date.now();
  while (Date.now() - start < timeout) {
    const m = zen.mesh;
    if (m && m.near > 0) return;
    await sleep(100);
  }
  throw new Error("timeout waiting for " + label + " to connect (mesh.near=0 after " + timeout + "ms)");
}

function closeClient(zen) {
  try {
    const opt = zen._graph && zen._graph._ && zen._graph._.root && zen._graph._.root.opt;
    const peers = opt && opt.peers;
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

// ── helpers ────────────────────────────────────────────────────────────────

function makeClient(relayUrl, pair) {
  return new ZEN({
    peers: [relayUrl],
    pub: pair.pub,
    localStorage: false,
    super: false,      // must be false — lib/server.js sets true by default
    multicast: false,  // no LAN multicast in tests
  });
  // axe: intentionally NOT disabled — AXE must be on per project rules
}

// ── main ────────────────────────────────────────────────────────────────────

async function run() {
  console.log("\nzen.push + zen.mesh.on — live relay test");
  console.log("  relay A: " + RELAY_A);
  console.log("  relay B: " + RELAY_B);
  console.log("");

  let pairA, pairB, clientA, clientB;

  try {
    [pairA, pairB] = await Promise.all([ZEN.pair(), ZEN.pair()]);
  } catch (e) {
    err("key generation", e);
    process.exit(1);
  }

  try {
    clientA = makeClient(RELAY_A, pairA);
    clientB = makeClient(RELAY_B, pairB);
    await Promise.all([
      waitConnected(clientA, "clientA→" + RELAY_A, TIMEOUT),
      waitConnected(clientB, "clientB→" + RELAY_B, TIMEOUT),
    ]);
    ok("both clients connected to relays (mesh.near ≥ 1)");
  } catch (e) {
    err("connect to relays", e);
    closeClient(clientA);
    closeClient(clientB);
    process.exit(1);
  }

  // Allow relays to exchange peers via AXE PEX before routing test
  await sleep(2000);

  // ── test 1: A pushes a string to B ───────────────────────────────────────
  try {
    await new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error("no delivery after 8s")), 8000);
      const off = clientB.mesh.on(function ({ from, data }) {
        if (data !== "live-hello") return;
        clearTimeout(timer);
        off();
        if (from !== pairA.pub) {
          return reject(new Error("wrong sender: " + from));
        }
        resolve();
      });
      clientA.push(pairB.pub, "live-hello");
    });
    ok("A.push(B.pub, string) delivered via relay");
  } catch (e) {
    err("string delivery", e);
  }

  // ── test 2: object payload ────────────────────────────────────────────────
  try {
    const payload = { x: 1, msg: "live-obj" };
    await new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error("no delivery after 8s")), 8000);
      const off = clientB.mesh.on(function ({ data }) {
        if (!data || data.msg !== "live-obj") return;
        clearTimeout(timer);
        off();
        try {
          if (data.x !== 1) throw new Error("payload mismatch");
          resolve();
        } catch (e2) { reject(e2); }
      });
      clientA.push(pairB.pub, payload);
    });
    ok("A.push(B.pub, object) delivered and matches");
  } catch (e) {
    err("object delivery", e);
  }

  // ── test 3: off() stops delivery ─────────────────────────────────────────
  try {
    let count = 0;
    const off = clientB.mesh.on(function ({ data }) {
      if (data === "live-off") count++;
    });
    clientA.push(pairB.pub, "live-off");
    await sleep(2000);
    if (count !== 1) throw new Error("expected 1 before off, got " + count);
    off();
    clientA.push(pairB.pub, "live-off");
    await sleep(2000);
    if (count !== 1) throw new Error("handler fired after off() — count=" + count);
    ok("off() stops further delivery");
  } catch (e) {
    err("off() stops delivery", e);
  }

  // ── test 4: message to unknown pub not delivered to B ────────────────────
  try {
    let received = false;
    const off = clientB.mesh.on(function ({ data }) {
      if (data === "live-wrong") received = true;
    });
    clientA.push("unknownpub000000000000000000000000000000000000000000000000000", "live-wrong");
    await sleep(2000);
    off();
    if (received) throw new Error("B received message aimed at unknown pub");
    ok("push to unknown pub not delivered to B");
  } catch (e) {
    err("unknown pub not delivered", e);
  }

  // ── cleanup ──────────────────────────────────────────────────────────────
  closeClient(clientA);
  closeClient(clientB);

  console.log("\n  " + pass + " passed, " + fail + " failed\n");
  process.exit(fail > 0 ? 1 : 0);
}

run().catch((e) => {
  console.error("Fatal:", e);
  process.exit(1);
});
