#!/usr/bin/env node
// ZEN MCP — full peer node + Model Context Protocol stdio server
// Each running instance contributes to the ZEN P2P network
import ZEN from "../index.js";
import * as xdg from "./xdg.js";
import { getOrCreateIdentity } from "./identity.js";

// Full ZEN peer — rfs + axe + multicast activated via index.js
xdg.ensure(xdg.data());
ZEN.log.off = true; // stdout is JSON-RPC; suppress diagnostic logs
const zenOpt = { file: xdg.data() + "/mcp" };
if (process.env.ZEN_PEERS) {
  zenOpt.peers = process.env.ZEN_PEERS.split(",").filter(Boolean);
}

// Stable identity derived from hardware — shared with script/server.js
// The full keypair (including private keys) is available for authenticated operations
let hwIdentity = null;
try {
  hwIdentity = await getOrCreateIdentity();
  if (hwIdentity) {
    zenOpt.pid = hwIdentity.pair.pub;
    // Store the hardware identity globally for MCP tools to access
    // This allows tools to use the server's persistent identity for signing/encryption
  }
} catch (e) {
  // Non-fatal: will fall back to random peer ID
}

const zen = new ZEN(zenOpt);

// In-memory key store — private keys never leave this process after being stored
const pairStore = new Map();

// Store hardware identity with special "hw" ID if available
if (hwIdentity && hwIdentity.pair) {
  pairStore.set("hw", hwIdentity.pair);
}

function storeKeys(fields) {
  const id = "pair_" + Date.now().toString(36) + Math.random().toString(36).slice(2, 7);
  pairStore.set(id, { priv: fields.priv, pub: fields.pub, epriv: fields.epriv, epub: fields.epub });
  return id;
}
function resolveKeys(args) {
  if (args.pairId) {
    const stored = pairStore.get(args.pairId);
    if (!stored) throw new Error("Unknown pairId: " + args.pairId);
    return stored;
  }
  return { priv: args.priv, pub: args.pub, epriv: args.epriv, epub: args.epub };
}

// JSON-RPC 2.0 over stdio — no external dependencies
let buf = "";
process.stdin.setEncoding("utf8");
process.stdin.on("data", (d) => {
  buf += d;
  const lines = buf.split("\n");
  buf = lines.pop();
  for (const line of lines) {
    if (!line.trim()) continue;
    try { handle(JSON.parse(line)); } catch (e) {}
  }
});

function send(obj) { process.stdout.write(JSON.stringify(obj) + "\n"); }

const TOOLS = [
  {
    name: "graph",
    description:
`Execute a ZEN graph chain operation.

Build a chain: zen.get(soul)[.get(path[0])...].op(value, opt)

ops:
  get — read once, returns value (or null)
  put — write value, returns {ok:true}
  set — append value to a set node, returns {ok:true}

opt (all optional):
  pairId          — key alias from storePair (preferred over raw keys)
  priv/pub/epriv/epub — raw key fields (avoid — visible to LLM)
  cert            — certificate string from crypto({method:"certify",...})
  pow             — {unit, difficulty} — auto-mines before write

Examples:
  { soul:"users", path:["alice","age"], op:"get" }
  { soul:"users", path:["alice","age"], op:"put", value:30, opt:{pairId:"pair_x"} }
  { soul:"tags",  path:["popular"],     op:"set", value:"zen", opt:{pairId:"pair_x"} }`,
    inputSchema: {
      type: "object",
      required: ["soul", "op"],
      properties: {
        soul:  { type: "string", description: "Root node soul (ID)" },
        path:  { type: "array",  items: { type: "string" }, description: "Key chain, e.g. [\"alice\",\"age\"]" },
        op:    { type: "string", enum: ["get", "put", "set"], description: "Operation" },
        value: { description: "Value to write (put/set). Any JSON type." },
        opt:   { type: "object", description: "Write options: { pairId?, priv?, pub?, epriv?, epub?, cert?, pow? }" },
      },
    },
  },
  {
    name: "crypto",
    description:
`Call any ZEN static crypto method by name.

method — required args:
  pair    — { curve?, seed?, priv?, epriv?, pub?, epub?, store? }
            → key pair; store:true returns {pairId,pub,epub} without exposing private keys
  sign    — { data } + pairId or {priv,pub?}               → signed string
  verify  — { signed, pub }                                  → original data or null
  encrypt — { data, epub }                                   → encrypted blob
  decrypt — { enc } + pairId or {epriv}                     → plaintext
  secret  — { epub } + pairId or {epriv}                    → shared secret string
  hash    — { data, name?, encode?, salt?, iterations?, pow? }
            → hash string; pow:{unit,difficulty} returns {hash,nonce,proof}
  certify — { pub (recipient), policy } + pairId or {priv}  → certificate string
  recover — { signed }                                       → public key
  pen     — { key?,val?,soul?,state?,path?,sign?,cert?,open?,pow? } → soul string
  candle  — { seg?,sep?,size?,back?,fwd? }                   → key expression

Use pairId (from storePair) instead of raw private key fields whenever possible.`,
    inputSchema: {
      type: "object",
      required: ["method"],
      properties: {
        method: { type: "string", description: "ZEN static method name (see description)" },
        pairId: { type: "string", description: "Key alias from storePair — avoids passing raw private keys through the LLM" },
      },
      additionalProperties: true,
    },
  },
  {
    name: "storePair",
    description:
`Store a key pair in MCP server memory and return an alias ID.
Use the returned pairId in graph/crypto calls instead of passing raw private keys.
Private keys stay inside the server process and never pass through the LLM again after this one call.`,
    inputSchema: {
      type: "object",
      properties: {
        priv:  { type: "string" },
        pub:   { type: "string" },
        epriv: { type: "string" },
        epub:  { type: "string" },
      },
    },
  },
  {
    name: "getHardwareIdentity",
    description:
`Get information about the hardware-derived identity used by this MCP server.
This is the same stable identity shared with ZEN server instances on the same hardware.

Returns:
  - pairId: "hw" — can be used in graph/crypto operations with pairId:"hw"
  - pub: public signing key
  - epub: public encryption key
  - hwid: truncated hardware fingerprint (for verification)

The hardware identity is derived from machine-id + MAC address + hostname.
Private keys never leave the server process.

Use pairId:"hw" in graph/crypto operations to sign/encrypt with the server's persistent identity:
  { soul:"users", path:["alice"], op:"put", value:{...}, opt:{pairId:"hw"} }
  { method:"sign", data:"hello", pairId:"hw" }`,
    inputSchema: {
      type: "object",
      properties: {},
    },
  },
];

async function handle(msg) {
  const { id, method, params } = msg;

  if (method === "initialize") {
    return send({ jsonrpc: "2.0", id, result: {
      protocolVersion: "2024-11-05",
      capabilities: { tools: {} },
      serverInfo: { name: "zen", version: "1.0.0" },
    }});
  }

  if (!method || method.startsWith("notifications/")) return;

  if (method === "tools/list") {
    return send({ jsonrpc: "2.0", id, result: { tools: TOOLS } });
  }

  if (method === "tools/call") {
    const { name, arguments: args } = params;
    try {
      const result = await call(name, args || {});
      send({ jsonrpc: "2.0", id, result: {
        content: [{ type: "text", text: JSON.stringify(result, null, 2) }],
      }});
    } catch (e) {
      send({ jsonrpc: "2.0", id, error: { code: -32000, message: e.message } });
    }
    return;
  }

  if (id !== undefined) {
    send({ jsonrpc: "2.0", id, error: { code: -32601, message: "Method not found" } });
  }
}

async function call(name, args) {
  // ── getHardwareIdentity ───────────────────────────────────────────────────
  if (name === "getHardwareIdentity") {
    if (!hwIdentity) {
      throw new Error("Hardware identity not available (no hardware entropy source)");
    }
    return {
      pairId: "hw",
      pub: hwIdentity.pair.pub,
      epub: hwIdentity.pair.epub,
      hwid: hwIdentity.hwid.slice(0, 30) + "...", // Truncate for privacy
    };
  }

  // ── storePair ─────────────────────────────────────────────────────────────
  if (name === "storePair") {
    const id = storeKeys(args);
    return { pairId: id, pub: args.pub, epub: args.epub };
  }

  // ── graph ─────────────────────────────────────────────────────────────────
  if (name === "graph") {
    const path = args.path || [];
    let node = zen.get(args.soul);
    for (const k of path) node = node.get(k);

    if (args.op === "get") {
      return new Promise((r) => node.once((v) => r(v ?? null)));
    }

    // put / set — resolve value
    let value = args.value;
    if (typeof value === "string") { try { value = JSON.parse(value); } catch (_) {} }

    // resolve write options
    const rawOpt = args.opt || {};
    const kp = resolveKeys(rawOpt);
    const opt = {};
    if (kp.priv)     opt.authenticator = kp;
    if (rawOpt.cert) opt.cert = rawOpt.cert;
    if (rawOpt.pow)  opt.pow  = typeof rawOpt.pow === "string" ? JSON.parse(rawOpt.pow) : rawOpt.pow;
    const writeOpt = Object.keys(opt).length ? opt : undefined;

    const chainOp = args.op === "set" ? "set" : "put";
    return new Promise((resolve, reject) => {
      node[chainOp](value, (ack) => {
        if (ack && ack.err) reject(new Error(ack.err));
        else resolve({ ok: true });
      }, writeOpt);
    });
  }

  // ── crypto ────────────────────────────────────────────────────────────────
  if (name === "crypto") {
    const { method, pairId, ...rest } = args;
    // kp resolves from pairId or raw fields in rest — used by sign/decrypt/secret/certify
    const kp = resolveKeys({ pairId, ...rest });

    switch (method) {
      case "pair": {
        const opt = {};
        ["curve","seed","priv","epriv","pub","epub"].forEach(k => { if (rest[k] != null) opt[k] = rest[k]; });
        const pair = await ZEN.pair(null, Object.keys(opt).length ? opt : undefined);
        if (rest.store) { const id = storeKeys(pair); return { pairId: id, pub: pair.pub, epub: pair.epub }; }
        return pair;
      }
      case "sign":    return ZEN.sign(rest.data, kp);
      case "verify":  return ZEN.verify(rest.signed, rest.pub);
      case "encrypt": return ZEN.encrypt(rest.data, rest.epriv ? rest.epriv : kp);
      case "decrypt": return ZEN.decrypt(rest.enc, kp);
      case "secret":  return ZEN.secret(rest.epub, kp);
      case "hash": {
        const opt = {};
        ["name","encode","salt","iterations","pow"].forEach(k => { if (rest[k] != null) opt[k] = rest[k]; });
        if (typeof opt.pow === "string") opt.pow = JSON.parse(opt.pow);
        return ZEN.hash(rest.data, null, null, Object.keys(opt).length ? opt : { name: "SHA-256", encode: "base62" });
      }
      case "certify": {
        const policy = typeof rest.policy === "string" ? JSON.parse(rest.policy) : rest.policy;
        const issuer = pairId ? kp : { priv: rest.priv, pub: rest.pub };
        const opt    = rest.expiry ? { expiry: rest.expiry } : undefined;
        return ZEN.certify(rest.pub, policy, issuer, null, opt);
      }
      case "recover": return ZEN.recover(rest.signed);
      case "pen":     return ZEN.pen(rest);
      case "candle":  return ZEN.candle(rest);
      default: throw new Error("Unknown crypto method: " + method);
    }
  }

  throw new Error("Unknown tool: " + name);
}
