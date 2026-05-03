// test/mcp.js — integration tests for lib/mcp.js
// Spawns the real MCP server process and communicates via JSON-RPC 2.0 over stdio.
import assert from "assert";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";

const __dirname = dirname(fileURLToPath(import.meta.url));
const MCP_BIN   = join(__dirname, "../lib/mcp.js");
// Use a fresh temp dir for each test run so stale MCP data never triggers
// the null-auth re-propagation path in penStage.
const MCP_TEST_XDG = mkdtempSync(join(tmpdir(), "zen-mcp-test-"));

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
      env: { ...process.env, ZEN_SILENCE_TEST_WARNINGS: "1", XDG_DATA_HOME: MCP_TEST_XDG },
    });
    this._proc.stdout.setEncoding("utf8");
    this._proc.stderr && this._proc.stderr.on("data", d => process.stderr.write(d));
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

  it("tool names are graph, crypto, identity, protocol", function () {
    const names = tools.map(t => t.name);
    assert.deepStrictEqual(names, ["graph", "crypto", "identity", "protocol"]);
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

  it("identity tool exists with no required fields", function () {
    const t = tools.find(t => t.name === "identity");
    assert.ok(t, "identity tool should be present");
    assert.deepStrictEqual(t.inputSchema.required, undefined);
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

  it("pair returns pub only (no private keys, no epub)", async function () {
    const res = await call("crypto", { method: "pair" });
    const pair = content(res);
    assert.ok(pair.pub);
    assert.ok(pair.address, "address present");
    assert.match(pair.address, /^0x[0-9a-fA-F]{40}$/, "address is EVM checksum");
    assert.strictEqual(pair.epub, undefined);
    assert.strictEqual(pair.priv, undefined);
    assert.strictEqual(pair.epriv, undefined);
  });

  it("pair with seed is deterministic", async function () {
    const [r1, r2] = await Promise.all([
      call("crypto", { method: "pair", seed: "test-seed-123" }),
      call("crypto", { method: "pair", seed: "test-seed-123" }),
    ]);
    const p1 = content(r1), p2 = content(r2);
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.address, p2.address);
  });

  it("pair with format:evm returns 0x-prefixed pub and address", async function () {
    const res = await call("crypto", { method: "pair", format: "evm" });
    const pair = content(res);
    assert.match(pair.pub, /^0x04[0-9a-fA-F]{128}$/, "evm pub is uncompressed 04...");
    assert.match(pair.address, /^0x[0-9a-fA-F]{40}$/, "evm address is checksummed");
    assert.strictEqual(pair.priv, undefined);
    assert.strictEqual(pair.epriv, undefined);
  });

  it("pair with format:btc returns compressed pub and P2PKH address", async function () {
    const res = await call("crypto", { method: "pair", format: "btc" });
    const pair = content(res);
    assert.match(pair.pub, /^0x0[23][0-9a-fA-F]{64}$/, "btc pub is compressed 02/03...");
    assert.match(pair.address, /^1[1-9A-HJ-NP-Za-km-z]{25,34}$/, "btc address is P2PKH");
    assert.strictEqual(pair.priv, undefined);
    assert.strictEqual(pair.epriv, undefined);
  });

  it("sign → verify round-trip", async function () {
    await withSession(async s => {
      const identity = content(await s.tool("identity", {}));
      const signed = content(await s.tool("crypto", { method: "sign", data: "hello", pairId: "self" }));
      assert.ok(typeof signed === "string");
      const plain  = content(await s.tool("crypto", { method: "verify", signed, pub: identity.pub }));
      assert.strictEqual(plain, "hello");
    });
  });

  it("verify with wrong pub returns null/undefined (not throw)", async function () {
    await withSession(async s => {
      const p2 = content(await s.tool("crypto", { method: "pair" }));
      const signed = content(await s.tool("crypto", { method: "sign", data: "x", pairId: "self" }));
      const res = await s.tool("crypto", { method: "verify", signed, pub: p2.pub });
      const val = res.error ? null : content(res);
      assert.notStrictEqual(val, "x");
    });
  });

  it("encrypt → decrypt round-trip via pairId:self", async function () {
    await withSession(async s => {
      const enc = content(await s.tool("crypto", { method: "encrypt", data: "secret", pairId: "self" }));
      assert.ok(typeof enc === "string" && enc.length > 0);
      const dec = content(await s.tool("crypto", { method: "decrypt", enc, pairId: "self" }));
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
      const identity = content(await s.tool("identity", {}));
      const signed   = content(await s.tool("crypto", { method: "sign", data: "recov", pairId: "self" }));
      const pub      = content(await s.tool("crypto", { method: "recover", signed }));
      assert.strictEqual(pub, identity.pub);
    });
  });

  it("pen returns a soul string starting with !", async function () {
    const soul = content(await call("crypto", { method: "pen", key: "fixed" }));
    assert.ok(typeof soul === "string");
    assert.ok(soul.startsWith("!"));
  });
});

// ─── pairId:"self" flow ───────────────────────────────────────────────────────
// All pairId tests share a SINGLE session so the in-memory pairStore persists.

describe("MCP — pairId:\"self\" flow", function () {
  this.timeout(20000);

  let s;
  before(function () { s = new McpSession().start(); return s.init(); });
  after(function ()  { s.end(); });

  it("sign with pairId:self produces valid signature", async function () {
    const identity = content(await s.tool("identity", {}));
    const signed = content(await s.tool("crypto", { method: "sign", pairId: "self", data: "via-self" }));
    assert.ok(typeof signed === "string");
    const verified = content(await s.tool("crypto", { method: "verify", signed, pub: identity.pub }));
    assert.strictEqual(verified, "via-self");
  });

  it("encrypt/decrypt with pairId:self round-trips", async function () {
    const enc = content(await s.tool("crypto", { method: "encrypt", pairId: "self", data: "private" }));
    const dec = content(await s.tool("crypto", { method: "decrypt", pairId: "self", enc }));
    assert.strictEqual(dec, "private");
  });

  it("unknown pairId returns error", async function () {
    const res = await call("crypto", { method: "sign", pairId: "pair_stale123", data: "x" });
    assert.ok(res.error);
    assert.ok(res.error.message.includes("Unknown pairId"));
  });
});

// ─── authenticated graph write ────────────────────────────────────────────────

describe("MCP — authenticated graph write", function () {
  this.timeout(20000);

  it("put with pairId:self stores and reads back", async function () {
    const soul = "mcp_auth_" + Date.now();
    await withSession(async s => {
      const putRes = await s.tool("graph", {
        soul, path: ["msg"], op: "put", value: "auth-write",
        opt: { pairId: "self" },
      });
      const getRes = await s.tool("graph", { soul, path: ["msg"], op: "get" });
      assert.deepStrictEqual(content(putRes), { ok: true });
      assert.strictEqual(content(getRes), "auth-write");
    });
  });
});

// ─── protocol tool ───────────────────────────────────────────────────────────
// All protocol tests share ONE session so pairStore persists across calls.

describe("MCP — protocol tool", function () {
  this.timeout(30000);

  let s;
  let selfPub;
  before(async function () {
    s = new McpSession().start();
    await s.init();
    selfPub = content(await s.tool("identity", {})).pub;
  });
  after(function () { s.end(); });

  // ── soul computation (pure, no graph) ──────────────────────────────────────

  it("inbox_soul returns a ! soul for a pub", async function () {
    const res = content(await s.tool("protocol", { op: "inbox_soul", pub: selfPub }));
    assert.ok(typeof res.soul === "string");
    assert.ok(res.soul.startsWith("!"));
  });

  it("inbox_soul is deterministic", async function () {
    const r1 = content(await s.tool("protocol", { op: "inbox_soul", pub: selfPub }));
    const r2 = content(await s.tool("protocol", { op: "inbox_soul", pub: selfPub }));
    assert.strictEqual(r1.soul, r2.soul);
  });

  it("chan_soul returns a ! soul", async function () {
    const res = content(await s.tool("protocol", {
      op: "chan_soul", proj_id: "p1", chan_id: "c1", owner_pub: selfPub,
    }));
    assert.ok(res.soul.startsWith("!"));
  });

  it("dm_soul returns a ! soul", async function () {
    const res = content(await s.tool("protocol", { op: "dm_soul", recipient_pub: selfPub }));
    assert.ok(res.soul.startsWith("!"));
  });

  it("missing pairId on create_channel returns error", async function () {
    const res = await s.tool("protocol", { op: "create_channel", proj_id: "p1", chan_id: "c1" });
    assert.ok(res.error);
    assert.ok(res.error.message.includes("pairId"));
  });

  // ── channel lifecycle ──────────────────────────────────────────────────────
  // Uses unique proj/chan IDs to avoid CRDT conflicts across test runs.

  let chanPub;
  let memberCert;
  const PROJ = "test_" + Date.now();
  const CHAN  = "general";

  it("create_channel stores meta and wrapped key", async function () {
    const res = content(await s.tool("protocol", {
      op: "create_channel", proj_id: PROJ, chan_id: CHAN, pairId: "self",
    }));
    assert.ok(res.chan_pub, "should return chan_pub");
    assert.strictEqual(res.version, 1);
    assert.ok(res.soul.startsWith("!"));
    chanPub = res.chan_pub;
  });

  it("invite self as member returns cert and chan_pub", async function () {
    const res = content(await s.tool("protocol", {
      op: "invite", proj_id: PROJ, chan_id: CHAN, member_pub: selfPub, pairId: "self",
    }));
    assert.ok(typeof res.cert === "string" && res.cert.length > 0, "cert should be a string");
    assert.strictEqual(res.chan_pub, chanPub);
    memberCert = res.cert;
  });

  it("send_channel encrypts and writes message", async function () {
    const res = content(await s.tool("protocol", {
      op: "send_channel",
      proj_id: PROJ, chan_id: CHAN, owner_pub: selfPub,
      message: "hello channel",
      cert: memberCert,
      pairId: "self",
    }));
    assert.deepStrictEqual(res, { ok: true });
  });

  it("kick bumps version and returns new chan_pub", async function () {
    const res = content(await s.tool("protocol", {
      op: "kick", proj_id: PROJ, chan_id: CHAN, remaining: [], pairId: "self",
    }));
    assert.strictEqual(res.version, 2);
    assert.ok(res.chan_pub !== chanPub, "new chan_pub should differ after rotation");
  });

  // ── DM ─────────────────────────────────────────────────────────────────────

  it("send_dm to self returns ok", async function () {
    const res = content(await s.tool("protocol", {
      op: "send_dm", recipient_pub: selfPub, message: "hello dm", pairId: "self",
    }));
    assert.deepStrictEqual(res, { ok: true });
  });

  it("read_dms returns array (with self-DM decrypted)", async function () {
    const res = content(await s.tool("protocol", {
      op: "read_dms", pairId: "self", limit: 10,
    }));
    assert.ok(Array.isArray(res));
    const found = res.find(m => m.plaintext === "hello dm");
    assert.ok(found, "self-DM plaintext should appear in read_dms");
    assert.strictEqual(found.sender_pub, selfPub);
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
