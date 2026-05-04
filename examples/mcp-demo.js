#!/usr/bin/env node
/**
 * examples/mcp-demo.js — Thử MCP server thực tế qua stdio transport
 *
 * Chạy: node examples/mcp-demo.js
 *
 * Demo flow:
 *   initialize → identity → sign → verify → encrypt → decrypt → hash → protocol:inbox_soul
 */

import { spawn } from "child_process";
import { createInterface } from "readline";
import { fileURLToPath } from "url";
import { join, dirname } from "path";

const __dir = dirname(fileURLToPath(import.meta.url));
const MCP_BIN = join(__dir, "../lib/mcp.js");

const proc = spawn("node", [MCP_BIN], { stdio: ["pipe", "pipe", "pipe"] });
const rl = createInterface({ input: proc.stdout });
let seq = 0;
const pending = new Map();

proc.stderr.on("data", d => {
  const s = String(d).trim();
  if (s) process.stderr.write("  [server] " + s + "\n");
});

proc.on("exit", code => {
  if (code != null) process.exit(0);
});

function rpc(method, params) {
  const id = ++seq;
  return new Promise((resolve, reject) => {
    const t = setTimeout(() => {
      pending.delete(id);
      reject(new Error("timeout: " + method));
    }, 12000);
    pending.set(id, { resolve, reject, t });
    proc.stdin.write(JSON.stringify({ jsonrpc: "2.0", id, method, params }) + "\n");
  });
}

rl.on("line", line => {
  let msg;
  try { msg = JSON.parse(line); } catch { return; }
  const entry = pending.get(msg.id);
  if (!entry) return;
  clearTimeout(entry.t);
  pending.delete(msg.id);
  if (msg.error) entry.reject(new Error(msg.error.message));
  else entry.resolve(msg.result);
});

function content(result) {
  return JSON.parse(result.content[0].text);
}

function ok(label, value) {
  const preview = typeof value === "string"
    ? (value.length > 50 ? value.slice(0, 50) + "…" : value)
    : JSON.stringify(value).slice(0, 80);
  console.log("✓", label + ":", preview);
}

async function run() {
  // 1. Initialize
  const init = await rpc("initialize", {});
  console.log("\n=== ZEN MCP demo ===");
  console.log("server:", init.serverInfo.name, "v" + init.serverInfo.version);
  console.log();

  // 2. List tools
  const { tools } = await rpc("tools/list", {});
  console.log("tools:", tools.map(t => t.name).join(", "));
  console.log();

  // 3. Identity
  const identity = content(await rpc("tools/call", { name: "identity", arguments: {} }));
  ok("pub", identity.pub);

  // 4. Sign
  const signed = content(await rpc("tools/call", {
    name: "crypto",
    arguments: { method: "sign", pairId: "self", data: "hello zen mcp" },
  }));
  ok("signed", signed);

  // 5. Verify
  const verified = content(await rpc("tools/call", {
    name: "crypto",
    arguments: { method: "verify", signed, pub: identity.pub },
  }));
  ok("verified", verified);

  // 6. Encrypt + Decrypt (self)
  const enc = content(await rpc("tools/call", {
    name: "crypto",
    arguments: { method: "encrypt", pairId: "self", data: "secret message" },
  }));
  ok("encrypted", enc);

  const dec = content(await rpc("tools/call", {
    name: "crypto",
    arguments: { method: "decrypt", pairId: "self", enc },
  }));
  ok("decrypted", dec);

  // 7. Hash
  const hash = content(await rpc("tools/call", {
    name: "crypto",
    arguments: { method: "hash", data: "hello zen", encode: "base62" },
  }));
  ok("hash", hash);

  // 8. Protocol: inbox_soul
  const { soul } = content(await rpc("tools/call", {
    name: "protocol",
    arguments: { op: "inbox_soul", pub: identity.pub },
  }));
  ok("inbox_soul", soul);

  // 9. Protocol: send_inbox to self
  const sent = content(await rpc("tools/call", {
    name: "protocol",
    arguments: { op: "send_inbox", recipient_pub: identity.pub, message: "hello from demo", pairId: "self" },
  }));
  ok("send_inbox", sent);

  // 10. Protocol: read_inbox (2s window)
  process.stdout.write("  reading inbox (wait 2s)… ");
  const msgs = content(await rpc("tools/call", {
    name: "protocol",
    arguments: { op: "read_inbox", pairId: "self", limit: 10 },
  }, 15000));
  const found = msgs.find(m => m.plaintext === "hello from demo");
  console.log(found ? "✓ found: \"" + found.plaintext + "\"" : "✗ not found");

  console.log("\ndone.\n");
  proc.kill();
}

run().catch(e => { console.error("error:", e.message); proc.kill(); process.exit(1); });
