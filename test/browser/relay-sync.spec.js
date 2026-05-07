/**
 * Cross-relay browser sync tests
 *
 * Each test opens 3 browser pages simultaneously, each connected to a
 * different relay (zen / peer0 / peer1), then verifies data PUT on one
 * relay propagates and is received live on the other two.
 *
 * Topology tested:
 *   zen.akao.io:8420   = relay A
 *   peer0.akao.io:8420 = relay B
 *   peer1.akao.io:8420 = relay C
 */

import { test, expect } from "@playwright/test";

const RELAYS = {
  zen:   "wss://zen.akao.io:8420/zen",
  peer0: "wss://peer0.akao.io:8420/zen",
  peer1: "wss://peer1.akao.io:8420/zen",
};

function pageUrl(relay) {
  return (
    "http://127.0.0.1:8766/relay-sync.html?relay=" +
    encodeURIComponent(RELAYS[relay])
  );
}

async function openRelayPage(browser, relay) {
  const ctx  = await browser.newContext({ ignoreHTTPSErrors: true });
  const page = await ctx.newPage();
  page.on("console", (msg) => {
    if (msg.type() === "error") console.error("[browser:" + relay + "]", msg.text());
  });
  await page.goto(pageUrl(relay));
  await expect(page.locator("#ready")).toHaveText("ready", { timeout: 10000 });
  await page.waitForTimeout(1500); // wait for WebSocket to relay to be established
  return page;
}

// ─── helpers ──────────────────────────────────────────────────────────────────

function rnd() { return "v-" + Math.random().toString(36).slice(2, 8); }
function soul() { return "bchain-" + Date.now() + "-" + Math.random().toString(36).slice(2, 5); }

async function put(page, s, val) {
  await page.evaluate(
    function(args) { return window.relayTests.put(args.soul, "x", args.val); },
    { soul: s, val },
  );
}

async function waitFor(page, s, timeout) {
  return page.evaluate(
    function(args) { return window.relayTests.waitFor(args.soul, "x", args.timeout); },
    { soul: s, timeout: timeout || 15000 },
  );
}

// ─── tests ────────────────────────────────────────────────────────────────────

test.describe("Cross-relay browser propagation (3 browsers × 3 relays)", () => {

  test("PUT on zen → live on peer0 and peer1", async ({ browser }) => {
    const s = soul(), v = rnd();
    const [pZen, pPeer0, pPeer1] = await Promise.all([
      openRelayPage(browser, "zen"),
      openRelayPage(browser, "peer0"),
      openRelayPage(browser, "peer1"),
    ]);

    // Subscribe readers BEFORE PUT so we catch live push
    const w0 = waitFor(pPeer0, s);
    const w1 = waitFor(pPeer1, s);
    await new Promise((r) => setTimeout(r, 500)); // let subscriptions reach relays

    await put(pZen, s, v);
    console.log("PUT zen  → " + s + " = " + v);

    const [d0, d1] = await Promise.all([w0, w1]);
    console.log("peer0 received:", d0);
    console.log("peer1 received:", d1);
    expect(d0).toBe(v);
    expect(d1).toBe(v);
  });

  test("PUT on peer0 → live on zen and peer1", async ({ browser }) => {
    const s = soul(), v = rnd();
    const [pZen, pPeer0, pPeer1] = await Promise.all([
      openRelayPage(browser, "zen"),
      openRelayPage(browser, "peer0"),
      openRelayPage(browser, "peer1"),
    ]);

    const wZen = waitFor(pZen,   s);
    const w1   = waitFor(pPeer1, s);
    await new Promise((r) => setTimeout(r, 500)); // let subscriptions reach relays

    await put(pPeer0, s, v);
    console.log("PUT peer0 → " + s + " = " + v);

    const [dz, d1] = await Promise.all([wZen, w1]);
    console.log("zen   received:", dz);
    console.log("peer1 received:", d1);
    expect(dz).toBe(v);
    expect(d1).toBe(v);
  });

  test("PUT on peer1 → live on zen and peer0", async ({ browser }) => {
    const s = soul(), v = rnd();
    const [pZen, pPeer0, pPeer1] = await Promise.all([
      openRelayPage(browser, "zen"),
      openRelayPage(browser, "peer0"),
      openRelayPage(browser, "peer1"),
    ]);

    const wZen = waitFor(pZen,   s);
    const w0   = waitFor(pPeer0, s);
    await new Promise((r) => setTimeout(r, 500)); // let subscriptions reach relays

    await put(pPeer1, s, v);
    console.log("PUT peer1 → " + s + " = " + v);

    const [dz, d0] = await Promise.all([wZen, w0]);
    console.log("zen   received:", dz);
    console.log("peer0 received:", d0);
    expect(dz).toBe(v);
    expect(d0).toBe(v);
  });

  test("chain A→peer0→peer1 without zen involved", async ({ browser }) => {
    // Client A is connected ONLY to peer0.
    // Client C is connected ONLY to peer1.
    // Data must travel: client-A → peer0 → peer1 → client-C.
    const s = soul(), v = rnd();
    const [pPeer0, pPeer1] = await Promise.all([
      openRelayPage(browser, "peer0"),
      openRelayPage(browser, "peer1"),
    ]);

    const w1 = waitFor(pPeer1, s);
    await put(pPeer0, s, v);
    console.log("PUT peer0 → " + s + " = " + v);

    const d1 = await w1;
    console.log("peer1 received:", d1);
    expect(d1).toBe(v);
  });

  test("realtime live subscription (subscribe first, PUT after)", async ({ browser }) => {
    const s = soul(), v = rnd();
    const [pWriter, pReader] = await Promise.all([
      openRelayPage(browser, "zen"),
      openRelayPage(browser, "peer1"),  // furthest hop
    ]);

    // Subscribe first — ensure .on() is wired before PUT
    const wLive = waitFor(pReader, s, 15000);
    await new Promise((r) => setTimeout(r, 500));  // let subscription register

    await put(pWriter, s, v);
    console.log("PUT zen → " + s + " = " + v);

    const received = await wLive;
    console.log("peer1 live received:", received);
    expect(received).toBe(v);
  });

});
