/**
 * Browser test: zen.push() and zen.mesh.on()
 *
 * Two Chromium pages connect to an in-process ZEN relay.
 * Covers: basic delivery (from/data), and off() unsubscribe.
 */

import { test, expect } from "@playwright/test";
import http from "http";

let relayServer;
let relayUrl;
const openSockets = new Set();

test.beforeAll(async () => {
  const { default: ZEN } = await import("../../index.js");
  relayServer = http.createServer();
  relayServer.on("connection", (sock) => {
    openSockets.add(sock);
    sock.on("close", () => openSockets.delete(sock));
  });
  new ZEN({ web: relayServer, localStorage: false, axe: false, multicast: false });
  await new Promise((r) => relayServer.listen(0, "127.0.0.1", r));
  relayUrl = `ws://127.0.0.1:${relayServer.address().port}/zen`;
  console.log("[push] relay at", relayUrl);
});

test.afterAll(async () => {
  if (relayServer) {
    for (const s of openSockets) s.destroy();
    await new Promise((r) => relayServer.close(r));
  }
});

// ── helpers ───────────────────────────────────────────────────────────────────

async function openPage(browser) {
  const ctx  = await browser.newContext();
  const page = await ctx.newPage();
  page.on("console", (msg) => {
    if (msg.type() === "error") console.error("[browser]", msg.text());
  });
  await page.goto("/push.html?relay=" + encodeURIComponent(relayUrl));
  await expect(page.locator("#ready")).toHaveText("ready", { timeout: 10000 });
  return page;
}

async function waitConnected(page) {
  await expect
    .poll(() => page.evaluate(() => window.pushTests.meshNear()), { timeout: 8000 })
    .toBeGreaterThan(0);
}

// ── tests ─────────────────────────────────────────────────────────────────────

test("zen.push delivers payload; from matches sender pub", async ({ browser }) => {
  const [pReceiver, pSender] = await Promise.all([
    openPage(browser),
    openPage(browser),
  ]);

  await Promise.all([waitConnected(pReceiver), waitConnected(pSender)]);

  const receiverPub = await pReceiver.evaluate(() => window.pushTests.myPub);

  // Arm receiver wait BEFORE sending
  const recvPromise = pReceiver.evaluate(
    (t) => window.pushTests.waitForMessage(t),
    8000,
  );
  await pReceiver.waitForTimeout(200);

  const payload   = { hello: "world", n: 42 };
  const senderPub = await pSender.evaluate(
    ([to, data]) => {
      window.pushTests.sendPush(to, data);
      return window.pushTests.myPub;
    },
    [receiverPub, payload],
  );

  const received = await recvPromise;

  console.log("[push] received:", received);
  expect(received.from).toBe(senderPub);
  expect(received.data).toEqual(payload);
});

test("zen.mesh.on off() unsubscribes handler; subsequent message is not delivered to it", async ({ browser }) => {
  const [pReceiver, pSender] = await Promise.all([
    openPage(browser),
    openPage(browser),
  ]);

  await Promise.all([waitConnected(pReceiver), waitConnected(pSender)]);

  const receiverPub = await pReceiver.evaluate(() => window.pushTests.myPub);

  // onceExtra registers a handler, waits for one message, then calls off().
  // We call it for msg-1 to confirm it fires normally.
  const extraPromise = pReceiver.evaluate(
    (t) => window.pushTests.onceExtra(t),
    8000,
  );
  const builtinPromise = pReceiver.evaluate(
    (t) => window.pushTests.waitForMessage(t),
    8000,
  );
  await pReceiver.waitForTimeout(200);

  await pSender.evaluate(
    ([to]) => window.pushTests.sendPush(to, "msg-1"),
    [receiverPub],
  );

  const extra   = await extraPromise;
  const builtin = await builtinPromise;

  expect(extra.called).toBe(true);
  expect(extra.data).toBe("msg-1");
  expect(builtin.data).toBe("msg-1");

  // Now send msg-2 — onceExtra's handler already called off(), so it must NOT fire again.
  // Only the built-in waitForMessage handler should receive it.
  const builtin2 = pReceiver.evaluate(
    (t) => window.pushTests.waitForMessage(t),
    3000,
  );
  await pReceiver.waitForTimeout(200);

  await pSender.evaluate(
    ([to]) => window.pushTests.sendPush(to, "msg-2"),
    [receiverPub],
  );

  const r2 = await builtin2;
  expect(r2.data).toBe("msg-2");

  // onceExtra is already off — calling it again would register a NEW handler that
  // times out (returns { called: false }) since msg-2 was already consumed.
  const extra2 = await pReceiver.evaluate(
    (t) => window.pushTests.onceExtra(t),
    500, // short timeout — should not fire
  );
  expect(extra2.called).toBe(false);
});
