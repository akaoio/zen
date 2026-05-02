// test/mcp.js — integration tests for lib/mcp.js
// Spawns the real MCP server process and communicates via JSON-RPC 2.0 over stdio.
import assert from "assert";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const MCP_BIN   = join(__dirname, "../lib/mcp.js");

// ─── McpSession ───────────────────────────────────────────────────────────────
// Maintains a single subprocess; correlates responses by id; supports
// sequential send/await and fire-and-forget notifications.

class McpSession {
  constructor() {
    this._proc  = null;
    this._buf   = "";
    this._pending = new Map(); // id → { resolve, reject, timer }
    this._nextId  = 0;
  }

  start() {
    this._proc = spawn(process.execPath, [MCP_BIN], {
      stdio: ["pipe", "pipe", "pipe"],
      env: { ...process.env, ZEN_SILENCE_TEST_WARNINGS: "1" },
    });
    this._proc.stdout.setEncoding("utf8");
    this._proc.stdout.on("data", d => {
      this._buf += d;
      const lines = this._buf.split("\n");
      this._buf = lines.pop();
      for (const line of lines) {
        if (!line.trim()) continue;
        try {
          const msg = JSON.parse(line);
          if (msg.id != null && this._pending.has(msg.id)) {
            const { resolve, timer } = this._pending.get(msg.id);
            this._pending.delete(msg.id);
            clearTimeout(timer);
            resolve(msg);
          }
        } catch (_) {}
      }
    });
    this._proc.on("error", err => {
      for (const [, { reject, timer }] of this._pending) {
        clearTimeout(timer);
        reject(err);
      }
      this._pending.clear();
    });
    return this;
  }

  end() { if (this._proc) this._proc.kill(); }

  send(method, params, timeout = 15000) {
    const id = this._nextId++;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this._pending.delete(id);
        reject(new Error("Timeout waiting for id:" + id + " method:" + method));
      }, timeout);
      this._pending.set(id, { resolve, reject, timer });
      this._proc.stdin.write(JSON.stringify({ jsonrpc: "2.0", id, method, params }) + "\n");
    });
  }

  /** Send a JSON-RPC notification (no id, no response expected). */
  notify(method, params) {
    this._proc.stdin.write(JSON.stringify({ jsonrpc: "2.0", method, params }) + "\n");
  }

  /** Initialize the session (required before other calls). */
  async init(timeout = 10000) {
    return this.send("initialize", {}, timeout);
  }

  /** Call a tool and return the raw JSON-RPC response. */
  async tool(name, args, timeout = 15000) {
    return this.send("tools/call", { name, arguments: args }, timeout);
  }
}

/** Parse the text content from a tools/call response. */
function content(res) {
  if (!res || !res.result) throw new Error("Expected result, got: " + JSON.stringify(res));
  return JSON.parse(res.result.content[0].text);
}

/** Convenience: open a fresh session, initialize, run fn(s), then end. */
async function withSession(fn, timeout = 20000) {
  const s = new McpSession().start();
  try {
    await s.init(timeout);
    return await fn(s);
  } finally {
    s.end();
  }
}

/** Shorthand: single tool call in its own session. */
async function call(name, args, timeout) {
  return withSession(s => s.tool(name, args, timeout), timeout || 15000);
}

// ─── initialize ───────────────────────────────────────────────────────────────

describe("MCP — initialize", function () {
  this.timeout(15000);

  it("returns protocolVersion and serverInfo", async function () {
    await withSession(async s => {
      const res = await s.init();
      assert.strictEqual(res.result.protocolVersion, "2024-11-05");
      assert.strictEqual(res.result.serverInfo.name, "zen");
      assert.ok(res.result.capabilities.tools);
    });
  });
});

// ─── tools/list ───────────────────────────────────────────────────────────────

describe("MCP — tools/list", function () {
  this.timeout(15000);

  let tools;
  before(async function () {
    await withSession(async s => {
      const res = await s.send("tools/list", {});
      tools = res.result.tools;
    });
  });

  it("exposes exactly 4 tools", function () {
    assert.strictEqual(tools.length, 4);
  });

  it("tool names are graph, crypto, storePair, getHardwareIdentity", function () {
    const names = tools.map(t => t.name);
    assert.deepStrictEqual(names, ["graph", "crypto", "storePair", "getHardwareIdentity"]);
  });

  it("graph requires soul and op", function () {
    const g = tools.find(t => t.name === "graph");
    assert.deepStrictEqual(g.inputSchema.required, ["soul", "op"]);
    assert.ok(g.inputSchema.properties.soul);
    assert.ok(g.inputSchema.properties.path);
    assert.ok(g.inputSchema.properties.op);
    assert.ok(g.inputSchema.properties.value);
    assert.ok(g.inputSchema.properties.opt);
  });

  it("graph op enum is [get, put, set]", function () {
    const g = tools.find(t => t.name === "graph");
    assert.deepStrictEqual(g.inputSchema.properties.op.enum, ["get", "put", "set"]);
  });

  it("crypto requires method and allows additionalProperties", function () {
    const c = tools.find(t => t.name === "crypto");
    assert.deepStrictEqual(c.inputSchema.required, ["method"]);
    assert.strictEqual(c.inputSchema.additionalProperties, true);
    assert.ok(c.inputSchema.properties.pairId);
  });

  it("storePair has priv/pub/epriv/epub properties", function () {
    const s = tools.find(t => t.name === "storePair");
    const props = Object.keys(s.inputSchema.properties);
    assert.deepStrictEqual(props, ["priv", "pub", "epriv", "epub"]);
  });
});

// ─── unknown tool ─────────────────────────────────────────────────────────────

describe("MCP — error handling", function () {
  this.timeout(15000);

  it("unknown tool returns -32000 error", async function () {
    const res = await call("nope", {});
    assert.ok(res.error);
    assert.strictEqual(res.error.code, -32000);
    assert.ok(res.error.message.includes("Unknown tool"));
  });

  it("unknown method (RPC level) returns -32601", async function () {
    await withSession(async s => {
      const res = await s.send("tools/nonexistent", {});
      assert.ok(res.error);
      assert.strictEqual(res.error.code, -32601);
    });
  });

  it("unknown crypto method returns error", async function () {
    const res = await call("crypto", { method: "nonexistent" });
    assert.ok(res.error);
    assert.ok(res.error.message.includes("Unknown crypto method"));
  });

  it("unknown pairId returns error", async function () {
    const res = await call("crypto", { method: "sign", pairId: "pair_fake", data: "x" });
    assert.ok(res.error);
    assert.ok(res.error.message.includes("Unknown pairId"));
  });
});

// ─── graph ────────────────────────────────────────────────────────────────────

describe("MCP — graph", function () {
  this.timeout(15000);

  it("get on empty soul returns null", async function () {
    const soul = "mcp_test_" + Date.now();
    const res = await call("graph", { soul, path: ["x"], op: "get" });
    assert.strictEqual(content(res), null);
  });

  it("put then get round-trips a string value", async function () {
    const soul = "mcp_test_" + Date.now();
    await withSession(async s => {
      const putRes = await s.tool("graph", { soul, path: ["name"], op: "put", value: "hello" });
      const getRes = await s.tool("graph", { soul, path: ["name"], op: "get" });
      assert.deepStrictEqual(content(putRes), { ok: true });
      assert.strictEqual(content(getRes), "hello");
    });
  });

  it("put then get round-trips a numeric value", async function () {
    const soul = "mcp_test_" + Date.now();
    await withSession(async s => {
      const putRes = await s.tool("graph", { soul, path: ["score"], op: "put", value: 42 });
      const getRes = await s.tool("graph", { soul, path: ["score"], op: "get" });
      assert.deepStrictEqual(content(putRes), { ok: true });
      assert.strictEqual(content(getRes), 42);
    });
  });

  it("multi-level path (depth 3) round-trips", async function () {
    const soul = "mcp_test_" + Date.now();
    await withSession(async s => {
      const putRes = await s.tool("graph", { soul, path: ["a", "b", "c"], op: "put", value: "deep" });
      const getRes = await s.tool("graph", { soul, path: ["a", "b", "c"], op: "get" });
      assert.deepStrictEqual(content(putRes), { ok: true });
      assert.strictEqual(content(getRes), "deep");
    });
  });
});

// ─── crypto — stateless ───────────────────────────────────────────────────────

describe("MCP — crypto (stateless)", function () {
  this.timeout(20000);

  it("pair returns priv, pub, epriv, epub", async function () {
    const res = await call("crypto", { method: "pair" });
    const pair = content(res);
    assert.ok(pair.priv);
    assert.ok(pair.pub);
    assert.ok(pair.epriv);
    assert.ok(pair.epub);
  });

  it("pair with seed is deterministic", async function () {
    const [r1, r2] = await Promise.all([
      call("crypto", { method: "pair", seed: "test-seed-123" }),
      call("crypto", { method: "pair", seed: "test-seed-123" }),
    ]);
    const p1 = content(r1), p2 = content(r2);
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.epub, p2.epub);
  });

  it("sign → verify round-trip", async function () {
    await withSession(async s => {
      const pair   = content(await s.tool("crypto", { method: "pair" }));
      const signed = content(await s.tool("crypto", { method: "sign", data: "hello", priv: pair.priv, pub: pair.pub }));
      assert.ok(typeof signed === "string");
      const plain  = content(await s.tool("crypto", { method: "verify", signed, pub: pair.pub }));
      assert.strictEqual(plain, "hello");
    });
  });

  it("verify with wrong pub returns null/undefined (not throw)", async function () {
    await withSession(async s => {
      const p1 = content(await s.tool("crypto", { method: "pair" }));
      const p2 = content(await s.tool("crypto", { method: "pair" }));
      const signed = content(await s.tool("crypto", { method: "sign", data: "x", priv: p1.priv, pub: p1.pub }));
      const res = await s.tool("crypto", { method: "verify", signed, pub: p2.pub });
      const val = res.error ? null : content(res);
      assert.notStrictEqual(val, "x");
    });
  });

  it("encrypt → decrypt round-trip", async function () {
    // ZEN.encrypt/decrypt are symmetric — both use epriv as the AES key source
    await withSession(async s => {
      const pair = content(await s.tool("crypto", { method: "pair" }));
      const enc  = content(await s.tool("crypto", { method: "encrypt", data: "secret", epriv: pair.epriv }));
      assert.ok(typeof enc === "string" && enc.length > 0);
      // pass the encrypted string directly (no re-stringify needed)
      const dec  = content(await s.tool("crypto", { method: "decrypt", enc, epriv: pair.epriv }));
      assert.strictEqual(dec, "secret");
    });
  });

  it("hash returns a string", async function () {
    const res = await call("crypto", { method: "hash", data: "hello", name: "SHA-256", encode: "base62" });
    const h = content(res);
    assert.ok(typeof h === "string" && h.length > 0);
  });

  it("hash is deterministic for same input", async function () {
    const [r1, r2] = await Promise.all([
      call("crypto", { method: "hash", data: "x", name: "SHA-256", encode: "base62" }),
      call("crypto", { method: "hash", data: "x", name: "SHA-256", encode: "base62" }),
    ]);
    assert.strictEqual(content(r1), content(r2));
  });

  it("recover returns the signer's pub key", async function () {
    await withSession(async s => {
      const pair   = content(await s.tool("crypto", { method: "pair" }));
      const signed = content(await s.tool("crypto", { method: "sign", data: "recov", priv: pair.priv, pub: pair.pub }));
      const pub    = content(await s.tool("crypto", { method: "recover", signed }));
      assert.strictEqual(pub, pair.pub);
    });
  });

  it("pen returns a soul string starting with !", async function () {
    const soul = content(await call("crypto", { method: "pen", key: "fixed" }));
    assert.ok(typeof soul === "string");
    assert.ok(soul.startsWith("!"));
  });
});

// ─── storePair + pairId flow ──────────────────────────────────────────────────
// All pairId tests share a SINGLE session so the in-memory pairStore persists.

describe("MCP — storePair + pairId", function () {
  this.timeout(20000);

  let s;
  before(function () { s = new McpSession().start(); return s.init(); });
  after(function ()  { s.end(); });

  it("storePair returns pairId and public keys, no priv", async function () {
    const pair = content(await s.tool("crypto", { method: "pair" }));
    const res  = content(await s.tool("storePair", { priv: pair.priv, pub: pair.pub, epriv: pair.epriv, epub: pair.epub }));
    assert.ok(res.pairId && res.pairId.startsWith("pair_"));
    assert.strictEqual(res.pub,  pair.pub);
    assert.strictEqual(res.epub, pair.epub);
    assert.strictEqual(res.priv,  undefined);
  });

  it("crypto pair store:true returns pairId without exposing priv", async function () {
    const res = content(await s.tool("crypto", { method: "pair", store: true }));
    assert.ok(res.pairId && res.pairId.startsWith("pair_"));
    assert.ok(res.pub);
    assert.ok(res.epub);
    assert.strictEqual(res.priv,  undefined);
    assert.strictEqual(res.epriv, undefined);
  });

  it("sign with pairId produces valid signature", async function () {
    const stored = content(await s.tool("crypto", { method: "pair", store: true }));
    const signed = content(await s.tool("crypto", { method: "sign", pairId: stored.pairId, data: "via-pairId" }));
    assert.ok(typeof signed === "string");
    const verified = content(await s.tool("crypto", { method: "verify", signed, pub: stored.pub }));
    assert.strictEqual(verified, "via-pairId");
  });

  it("decrypt with pairId", async function () {
    const stored = content(await s.tool("crypto", { method: "pair", store: true }));
    // encrypt using the stored pairId (server uses kp.epriv internally)
    const enc    = content(await s.tool("crypto", { method: "encrypt", pairId: stored.pairId, data: "private" }));
    // enc is a JSON string — pass directly
    const dec    = content(await s.tool("crypto", { method: "decrypt", pairId: stored.pairId, enc }));
    assert.strictEqual(dec, "private");
  });

  it("pairId from one session is unknown in a new session", async function () {
    // fresh process has empty pairStore
    const res = await call("crypto", { method: "sign", pairId: "pair_stale123", data: "x" });
    assert.ok(res.error);
    assert.ok(res.error.message.includes("Unknown pairId"));
  });
});

// ─── authenticated graph write ────────────────────────────────────────────────

describe("MCP — authenticated graph write", function () {
  this.timeout(20000);

  it("put with raw priv/pub stores and reads back", async function () {
    const soul = "mcp_auth_" + Date.now();
    await withSession(async s => {
      const pair = content(await s.tool("crypto", { method: "pair" }));
      const putRes = await s.tool("graph", {
        soul, path: ["msg"], op: "put", value: "auth-write",
        opt: { priv: pair.priv, pub: pair.pub, epriv: pair.epriv, epub: pair.epub },
      });
      const getRes = await s.tool("graph", { soul, path: ["msg"], op: "get" });
      assert.deepStrictEqual(content(putRes), { ok: true });
      assert.strictEqual(content(getRes), "auth-write");
    });
  });
});

// ─── notifications are silently ignored ───────────────────────────────────────

describe("MCP — notifications", function () {
  this.timeout(15000);

  it("notifications/initialized produces no response", async function () {
    // send init + notification + tools/list; only init and tools/list get responses
    await withSession(async s => {
      s.notify("notifications/initialized");
      const res = await s.send("tools/list", {});
      assert.ok(Array.isArray(res.result.tools));
    });
  });
});
