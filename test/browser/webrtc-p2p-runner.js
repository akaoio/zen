/**
 * Standalone Playwright runner for WebRTC cross-machine tests.
 * Run on peer0 / peer1 via SSH from the zen orchestrator.
 *
 * Usage:
 *   node test/browser/webrtc-p2p-runner.js \
 *     --relay=wss://zen0.akao.io:8420/zen \
 *     [--port=8767] [--min=1] [--timeout=50000]
 *
 * Exits 0 and prints "DC_OPEN:N" when N DataChannels open.
 * Exits 1 and prints "DC_TIMEOUT" on failure.
 */

import { chromium } from "@playwright/test";
import http from "http";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "node:url";
import { dirname as dirnameOf } from "node:path";

const __filename = fileURLToPath(import.meta.url);
const __dirname  = dirnameOf(__filename);
const rootDir    = path.resolve(__dirname, "../..");

// Parse --key=value args
const args = {};
for (const a of process.argv.slice(2)) {
  if (a.startsWith("--")) {
    const [k, ...rest] = a.slice(2).split("=");
    args[k] = rest.join("=") || "true";
  }
}

const relay   = args.relay   || "wss://zen.akao.io:8420/zen";
const port    = parseInt(args.port    || "8767",  10);
const minDC   = parseInt(args.min     || "1",     10);
const timeout = parseInt(args.timeout || "50000", 10);

// Minimal static file server — serves test page and zen bundle
const SERVE = {
  "/webrtc-p2p.html": path.join(__dirname, "fixtures", "webrtc-p2p.html"),
  "/zen.js":          path.join(rootDir, "zen.js"),
  "/webrtc.js":       path.join(rootDir, "webrtc.js"),
  "/crypto.wasm":     path.join(rootDir, "crypto.wasm"),
  "/pen.wasm":        path.join(rootDir, "pen.wasm"),
};

function mime(p) {
  if (p.endsWith(".html")) return "text/html; charset=utf-8";
  if (p.endsWith(".js"))   return "application/javascript; charset=utf-8";
  return "application/octet-stream";
}

const server = http.createServer((req, res) => {
  const file = SERVE[req.url.split("?")[0]];
  if (!file) { res.writeHead(404); res.end("Not found"); return; }
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(500); res.end(err.message); return; }
    res.writeHead(200, { "Content-Type": mime(file), "Cache-Control": "no-store" });
    res.end(data);
  });
});

await new Promise(r => server.listen(port, "127.0.0.1", r));
console.log(`[runner] server=http://127.0.0.1:${port} relay=${relay}`);

const browser = await chromium.launch({
  headless: true,
  args: ["--no-sandbox", "--disable-dev-shm-usage"],
});
const ctx  = await browser.newContext({ ignoreHTTPSErrors: true });
const page = await ctx.newPage();
page.on("console", msg => {
  if (msg.type() === "error") process.stderr.write(`[browser] ${msg.text()}\n`);
});

const pageUrl = `http://127.0.0.1:${port}/webrtc-p2p.html?relay=${encodeURIComponent(relay)}`;
await page.goto(pageUrl);
await page.locator("#ready").waitFor({ timeout: 15000 });

const readyText = await page.locator("#ready").textContent();
if (readyText !== "ready") {
  console.log("DC_TIMEOUT: page not ready (" + readyText + ")");
  await browser.close();
  server.close();
  process.exit(1);
}

console.log(`[runner] page ready — waiting for ${minDC} WebRTC DC(s), timeout=${timeout}ms`);

try {
  const pids = await page.evaluate(
    ({ minDC, timeout }) => window.webrtcTest.waitForDC(minDC, timeout),
    { minDC, timeout },
  );
  console.log("DC_OPEN:" + pids.length + " peers=" + pids.join(","));
  await browser.close();
  server.close();
  process.exit(0);
} catch (e) {
  console.log("DC_TIMEOUT: " + e.message);
  await browser.close();
  server.close();
  process.exit(1);
}
