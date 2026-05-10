// lib/mcp/server.js — ZEN MCP server logic
//
// Three exports:
//   boot()              — create a ZEN instance (relay or thin peer) for this machine
//   attach(zen, opts)   — mount MCP stdio + relay-RPC onto any ZEN instance
//   start()             — boot() + attach() (CLI entry point, backward-compat)
//
// The attach/boot split lets relay.service embed MCP directly (Gun/SEA pattern):
//   import { attach } from './lib/mcp/server.js'
//   attach(zen, { hwIdentity })   // no second ZEN instance, no pid collision

import cluster from "node:cluster";
import { createRequire } from "node:module";
import http from "node:http";
import net from "node:net";
import { existsSync, unlinkSync } from "node:fs";
import { join } from "node:path";
import ZEN from "../../index.js";
import * as xdg from "../xdg.js";
import { getOrCreateIdentity } from "../identity.js";
import { disc } from "../discover.js";
import { buildStatus } from "../status.js";
import {
  inboxSoul, chanSoul, dmSoul,
  createChannel, inviteMember, rotateChanKey,
  sendToChannel, sendDM, sendInbox, readInbox, readChannel, readDMs,
  unwrapChanKey, setProjectMeta, getProjectMeta, setProjectRole, getProjectRoles,
} from "../protocol.js";

const require = createRequire(import.meta.url);
const { version: packageVersion } = require("../../package.json");

// Detect whether the local relay uses TLS by checking for a cert file.
// This must match script/server.js which enables HTTPS when cert.pem exists.
function localRelayUseTLS() {
  return existsSync(join(xdg.config(), "cert.pem"));
}

// Read the relay port from the config file, defaulting to net-appropriate port.
function localRelayPort() {
  try {
    return parseInt(require("fs").readFileSync(join(xdg.config(), "port"), "utf8").trim(), 10) || xdg.DEFAULT_PORT;
  } catch (_) {
    return xdg.DEFAULT_PORT;
  }
}

function buildLocalRelayURL() {
  const port = localRelayPort();
  const scheme = localRelayUseTLS() ? "https" : "http";
  return `${scheme}://localhost:${port}/zen`;
}

// Try to bind the relay port. Returns the listening http.Server if successful,
// or null if the port is already in use (relay already running).
function tryBindRelay() {
  const port = localRelayPort();
  return new Promise((resolve) => {
    const server = http.createServer();
    server.once("error", (err) => {
      resolve(err.code === "EADDRINUSE" ? null : null);
    });
    server.listen(port, () => resolve(server));
  });
}

export function ipcSocketPath() {
  return xdg.data() + "/mcp.sock";
}

export async function boot() {
  xdg.ensure(xdg.data());
  ZEN.log.off = true; // stdout is JSON-RPC; suppress diagnostic logs

  let zenOpt = {};
  if (process.env.ZEN_PEERS) {
    zenOpt.peers = process.env.ZEN_PEERS.split(",").filter(Boolean);
  } else if (process.env.NO_BOOTSTRAP === "1") {
    zenOpt.peers = [];
    zenOpt.axe = false;
    zenOpt.multicast = false;
    zenOpt.file = xdg.data() + "/mcp"; // isolated storage for tests
  } else {
    const boundServer = await tryBindRelay();
    if (boundServer) {
      // MCP binds port 8420 = MCP IS the relay. Participate fully in the mesh
      // (DHT, BOOT peers, AXE topology). The unique pid = hash(hwPub + "/mcp")
      // below ensures no AXE conflict with the relay process.
      zenOpt.web = boundServer;
    } else {
      const localRelayURL = buildLocalRelayURL();
      if (localRelayUseTLS()) {
        process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";
      }
      zenOpt.peers = [localRelayURL];
      zenOpt.rfs = false;
    }
  }

  let hwIdentity = null;
  try {
    hwIdentity = await getOrCreateIdentity();
    if (hwIdentity) {
      const hwPub = hwIdentity.pair.pub;
      // pub: routing identity used by relay to route relay messages back to us
      zenOpt.pub = hwPub;
      // pid derivation strategy:
      // - Case 1 (boundServer): MCP IS the relay. Use stable /mcp pid so DHT peers
      //   can identify this machine's MCP relay consistently across restarts.
      // - Case 2 (connect to local relay): Multiple Copilot sessions can run simultaneously,
      //   all with the same hwPub. If they share a pid, AXE at the relay deduplicates them
      //   and drops all but one — breaking the other sessions. Use process.pid to make each
      //   MCP session unique so they coexist without AXE conflict.
      const pidSuffix = zenOpt.web ? "/mcp" : `/mcp/${process.pid}`;
      zenOpt.pid = await ZEN.hash({ data: hwPub + pidSuffix });
    }
  } catch (e) {
    // Non-fatal: ZEN will fall back to String.random(9) for pid
  }

  // Capture initial peers before new ZEN() mutates zenOpt.peers into an object
  const initialPeers = Array.isArray(zenOpt.peers) ? [...zenOpt.peers] : [];

  const zen = new ZEN(zenOpt);
  return { zen, hwIdentity, initialPeers };
}

// attach — mount MCP server onto an existing ZEN instance.
// Safe to call from relay.service (Gun/SEA pattern): no new ZEN instance created,
// no WebSocket peer connection, no pid/pub collision.
// stdio handler is only activated when process.stdin is a pipe (not a daemon's /dev/null).
export function attach(zen, { hwIdentity = null, initialPeers = [], ipc = false } = {}) {
  process.stderr.write(`[zen-mcp] net=${xdg.net()} port=${xdg.DEFAULT_PORT} data=${xdg.data()}\n`);

  // In-memory key store — private keys never leave this process after being stored
  const pairStore = new Map();
  if (hwIdentity && hwIdentity.pair) {
    pairStore.set("self", hwIdentity.pair);
  }

  // Active subscriptions — sub_id → { node, from, sendFn }
  // from = relay requester pub key (null for local transport)
  // sendFn = function to push notification back (null for relay mode)
  const subStore = new Map();
  let subSeq = 0;
  function sendNotification(sub_id, soul, key, val) {
    const sub = subStore.get(sub_id);
    if (!sub) return;
    const { from, sendFn } = sub;
    const payload = { jsonrpc: "2.0", method: "notifications/message", params: {
      level: "info", logger: "zen/subscribe",
      data: JSON.stringify({ sub_id, soul, key, val }),
    }};
    if (from) {
      // Relay-connected client — push notification back encrypted via DAM relay
      const self = pairStore.get("self");
      if (!self) return;
      (async () => {
        try {
          const shared = await ZEN.secret(from, self);
          const enc = await ZEN.encrypt(JSON.stringify(payload), shared);
          zen.push(from, enc);
        } catch (_) {}
      })();
      return;
    }
    if (sendFn) sendFn(payload);
  }

  function storeKeys(fields) {
    const id = "pair_" + Date.now().toString(36) + Math.random().toString(36).slice(2, 7);
    pairStore.set(id, { priv: fields.priv, pub: fields.pub });
    return id;
  }
  function resolveKeys(args) {
    if (args.pairId) {
      const stored = pairStore.get(args.pairId);
      if (!stored) throw new Error("Unknown pairId: " + args.pairId);
      return stored;
    }
    return { priv: args.priv, pub: args.pub };
  }

  // ── relay RPC mode ────────────────────────────────────────────────────────
  // When the MCP server has an identity, it also accepts JSON-RPC requests
  // delivered via DAM relay messages (encrypted with server's pub key).
  // Responses are relayed back to the originating agent's pub key.
  // This enables cross-machine MCP without any HTTP server or open ports.

  function startRelayMode() {
    const self = pairStore.get("self");
    if (!self) return; // no identity → relay mode unavailable

    const mesh = zen._graph && zen._graph._ && zen._graph._.opt && zen._graph._.opt.mesh;
    if (!mesh) return;

    // Publish discovery soul: ~<pub>/status (aligned with HTTP GET /status)
    const discoverySoul = "~" + self.pub + "/status";
    const knownPeers = initialPeers;

    function publishStatus(ip4, ip6) {
      const payload = buildStatus({
        pub: self.pub,
        domain: process.env.DOMAIN || null,
        ip4: ip4 || null,
        ip6: ip6 || null,
        port: process.env.PORT ? parseInt(process.env.PORT, 10) : null,
        peers: knownPeers,
        mcp: true,
      });
      // ZEN graph does not support arrays; serialize peers as comma-separated string.
      const { timestamp: _ts, peers: _peers, ...soulPayload } = payload;
      soulPayload.peers = Array.isArray(_peers) ? _peers.join(",") : (_peers || "");
      zen.get(discoverySoul).put(soulPayload, null, { authenticator: self });
    }

    // Initial publish — without IP (immediate)
    publishStatus(null, null);

    // Re-publish with discovered IP after network probe
    disc({ noSave: true }).then(function(di) {
      publishStatus(di.ip || null, di.ip6 || null);
    }).catch(function() {});

    mesh.onRelay(async function ({ from, data }) {
      if (!from || !data) return;
      let req;
      try {
        const shared = await ZEN.secret(from, self);
        req = await ZEN.decrypt(data, shared);
      } catch (_) { return; }
      if (!req || req.jsonrpc !== "2.0" || !req.method) return;

      const res = await dispatchRelay(req, from);
      if (res == null) return;

      let encrypted;
      try {
        const shared = await ZEN.secret(from, self);
        encrypted = await ZEN.encrypt(JSON.stringify(res), shared);
      } catch (_) { return; }
      zen.push(from, encrypted);
    });
  }

  // Returns the JSON-RPC response object, or null for notifications.
  async function dispatchRelay(msg, from) {
    const { id, method, params } = msg;
    if (!method || method.startsWith("notifications/")) return null;

    if (method === "initialize") {
      return { jsonrpc: "2.0", id, result: {
        protocolVersion: "2024-11-05",
        capabilities: { tools: {} },
        serverInfo: { name: "zen", version: packageVersion },
      }};
    }

    if (method === "tools/list") {
      return { jsonrpc: "2.0", id, result: { tools: TOOLS } };
    }

    if (method === "tools/call") {
      const { name, arguments: args } = params || {};
      try {
        const result = await call(name, args || {}, from, null);
        return { jsonrpc: "2.0", id, result: {
          content: [{ type: "text", text: JSON.stringify(result, null, 2) }],
        }};
      } catch (e) {
        return { jsonrpc: "2.0", id, error: { code: -32000, message: e.message } };
      }
    }

    if (id !== undefined) {
      return { jsonrpc: "2.0", id, error: { code: -32601, message: "Method not found" } };
    }
    return null;
  }

  // JSON-RPC 2.0 line-protocol transport factory.
  // Returns an "onData" handler bound to a specific sendFn (one per connection).
  function makeJsonRpcTransport(sendFn) {
    let buf = "";
    return (d) => {
      buf += d;
      const lines = buf.split("\n");
      buf = lines.pop();
      for (const line of lines) {
        if (!line.trim()) continue;
        try { handle(JSON.parse(line), sendFn); } catch (_) {}
      }
    };
  }

  // stdio transport — only active when stdin is a pipe (not a daemon's /dev/null).
  // Skip in cluster workers: their stdin is /dev/null, which would immediately fire
  // "end" and call process.exit(0), killing the relay worker.
  if (!process.stdin.isTTY && !cluster.isWorker) {
    const stdioSend = (obj) => process.stdout.write(JSON.stringify(obj) + "\n");
    process.stdin.setEncoding("utf8");
    process.stdin.on("data", makeJsonRpcTransport(stdioSend));
    // Exit when the AI client closes stdin (session ends).  Without this, the
    // standalone ZEN instance (with DHT timers, UDP sockets, etc.) keeps running
    // as an orphaned process indefinitely — the root cause of stale-MCP churn.
    process.stdin.once("end", () => process.exit(0));
  }

  // IPC transport — Unix socket for relay-embedded MCP (attach() called with ipc:true)
  if (ipc) {
    const ipcPath = ipcSocketPath();
    try { unlinkSync(ipcPath); } catch (_) {}
    const ipcServer = net.createServer((socket) => {
      socket.setEncoding("utf8");
      const ipcSend = (obj) => { try { socket.write(JSON.stringify(obj) + "\n"); } catch (_) {} };
      socket.on("data", makeJsonRpcTransport(ipcSend));
      socket.on("close", () => {
        // Clean up subscriptions owned by this connection
        for (const [sub_id, entry] of subStore) {
          if (entry.sendFn === ipcSend) {
            try { entry.node.off(); } catch (_) {}
            subStore.delete(sub_id);
          }
        }
      });
    });
    ipcServer.listen(ipcPath);
  }

  const TOOLS = [
    {
      name: "graph",
      description:
`Execute a ZEN graph chain operation.

Build a chain: zen.get(soul)[.get(path[0])...].op(value, opt)

ops:
  get         — read once, returns value (or null)
  put         — write value, returns {ok:true}
  set         — append value to a set node, returns {ok:true}
  subscribe   — stream live updates; returns {sub_id}; server pushes notifications/message per event
  unsubscribe — stop a live subscription; requires sub_id in opt; returns {ok:true}

opt (all optional):
  pairId          — use "self" to sign as this server's identity
  cert            — certificate string from crypto({method:"certify",...})
  pow             — {unit, difficulty} — auto-mines before write
  { soul:"users", path:["alice","age"], op:"get" }
  { soul:"users", path:["alice","age"], op:"put", value:30, opt:{pairId:"self"} }
  { soul:"tags",  path:["popular"],     op:"set", value:"zen", opt:{pairId:"self"} }`,
      inputSchema: {
        type: "object",
        required: ["soul", "op"],
        properties: {
          soul:  { type: "string", description: "Root node soul (ID)" },
          path:  { type: "array",  items: { type: "string" }, description: "Key chain, e.g. [\"alice\",\"age\"]" },
          op:    { type: "string", enum: ["get", "put", "set", "subscribe", "unsubscribe"], description: "Operation" },
          value: { description: "Value to write (put/set). Any JSON type." },
          opt:   { type: "object", description: "Write options: { pairId?, priv?, pub?, cert?, pow? }" },
        },
      },
    },
    {
      name: "crypto",
      description:
`Call any ZEN static crypto method by name.

method — required args:
  pair    — { curve?, seed?, pub?, format? } → key pair (pub/address; priv never returned)
                format: "zen" (default base62) | "evm" (0x hex) | "btc" (compressed/WIF/P2PKH)
  sign    — { data, pairId }                                 → signed string
  verify  — { signed, pub }                                  → original data or null
  encrypt — { data, pub }                                    → encrypted blob
  decrypt — { enc, pairId }                                  → plaintext
  secret  — { pub, pairId }                                  → shared secret string
  hash    — { data, name?, encode?, salt?, iterations?, pow? }
            → hash string; pow:{unit,difficulty} returns {hash,nonce,proof}
  certify — { pub (recipient), policy, pairId }              → certificate string
  recover — { signed }                                       → public key
  pen     — { key?,val?,soul?,state?,path?,sign?,cert?,open?,pow? } → soul string
  candle  — { seg?,sep?,size?,back?,fwd? }                   → key expression

pairId: use "self" to operate with this server's persistent identity.
Private keys are never accessible — only "self" alias is valid for signing operations.`,
      inputSchema: {
        type: "object",
        required: ["method"],
        properties: {
          method: { type: "string", description: "ZEN static method name (see description)" },
          pairId: { type: "string", description: 'Use "self" to sign/decrypt/certify as this server identity' },
        },
        additionalProperties: true,
      },
    },
    {
      name: "identity",
      description:
`Get the public identity of this MCP server.

Returns:
  - pub:  public signing key (share with others so they can verify your signatures)

To sign or authenticate as this identity, use pairId:"self" in graph/crypto calls.
Private keys are never exposed.`,
      inputSchema: {
        type: "object",
        properties: {},
      },
    },
    {
      name: "protocol",
      description:
`ZACP — ZEN Agent Collaboration Protocol operations.

ops:
  inbox_soul     — { pub }                                          → { soul }
  chan_soul       — { proj_id, chan_id, owner_pub }                  → { soul }
  dm_soul         — { recipient_pub }                               → { soul }
  get_project_meta — { proj_id, owner_pub }                         → object | null
  set_project_meta — { proj_id, meta, pairId }                      → { ok }
  get_project_roles — { proj_id, owner_pub }                        → object | null
  set_project_role — { proj_id, member_pub, role, pairId }          → { ok }
  create_channel  — { proj_id, chan_id, members?, pairId }          → { chan_pub, version, soul }
  invite          — { proj_id, chan_id, member_pub, pairId }        → { cert, chan_pub }
  kick            — { proj_id, chan_id, remaining[], pairId }       → { chan_pub, version }
  send_inbox      — { recipient_pub, message, pairId }              → { ok }
  read_inbox      — { pairId, limit? }                              → [{ key, plaintext, sender_pub }]
  send_channel    — { proj_id, chan_id, owner_pub, message, cert, pairId } → { ok }
  read_channel    — { proj_id, chan_id, owner_pub, pairId, limit? } → [{ key, plaintext, sender_pub }]
  send_dm         — { recipient_pub, message, pairId }              → { ok }
  read_dms        — { pairId, limit? }                              → [{ key, plaintext, sender_pub }]

All private key operations run inside this process. No priv is ever exposed.
pairId: "self" uses this server's persistent identity.`,
      inputSchema: {
        type: "object",
        required: ["op"],
        properties: {
          op:     { type: "string",
                    enum: ["inbox_soul","chan_soul","dm_soul",
                           "get_project_meta","set_project_meta",
                           "get_project_roles","set_project_role",
                           "create_channel","invite","kick",
                           "send_inbox","read_inbox",
                           "send_channel","read_channel",
                           "send_dm","read_dms"] },
          pairId: { type: "string", description: 'Use "self" for server identity' },
        },
        additionalProperties: true,
      },
    },
    {
      name: "push",
      description:
`Send an ephemeral relay message to another agent by public key.

Uses zen.push(pub, data) — best-effort delivery via DAM relay.
No persistence, no delivery acknowledgement. Use graph.put for durable writes.

Useful for real-time agent-to-agent coordination without touching the graph.`,
      inputSchema: {
        type: "object",
        required: ["to", "data"],
        properties: {
          to:   { type: "string", description: "Recipient's public key" },
          data: { description: "Payload to deliver (any JSON value)" },
        },
      },
    },
    {
      name: "storage",
      description:
`Query or manage ZEN local storage (rfs — relay file store).

ops:
  quota    — {}                → { used, free, total } in bytes
  degraded — {}                → { degraded: bool }  — true when disk is full/OOM
  recover  — {}                → { ok: true }         — clear degraded flag after freeing space

Only available when the MCP process IS the relay (binds port 8420 directly).
Returns { error: "no store" } when running as a side-car connected to an external relay.`,
      inputSchema: {
        type: "object",
        required: ["op"],
        properties: {
          op: { type: "string", enum: ["quota", "degraded", "recover"], description: "Operation" },
        },
      },
    },
    {
      name: "status",
      description:
`Query relay operational status and mesh connectivity.
Mirrors the HTTP GET /status endpoint but via MCP.

ops:
  status   — {}  → { pub, version, domain, port, peers_near, mcp: true }
  peers    — {}  → { count: N }  — number of directly connected mesh peers

peers_near comes from mesh.near (DAM propagation hop count, not a strict list).`,
      inputSchema: {
        type: "object",
        required: ["op"],
        properties: {
          op: { type: "string", enum: ["status", "peers"], description: "Operation" },
        },
      },
    },
  ];

  async function handle(msg, sendFn) {
    if (!sendFn) sendFn = (obj) => process.stdout.write(JSON.stringify(obj) + "\n");
    const { id, method, params } = msg;

    if (method === "initialize") {
      return sendFn({ jsonrpc: "2.0", id, result: {
        protocolVersion: "2024-11-05",
        capabilities: { tools: {} },
        serverInfo: { name: "zen", version: packageVersion },
      }});
    }

    if (!method || method.startsWith("notifications/")) return;

    if (method === "tools/list") {
      return sendFn({ jsonrpc: "2.0", id, result: { tools: TOOLS } });
    }

    if (method === "tools/call") {
      const { name, arguments: args } = params;
      try {
        const result = await call(name, args || {}, null, sendFn);
        sendFn({ jsonrpc: "2.0", id, result: {
          content: [{ type: "text", text: JSON.stringify(result, null, 2) }],
        }});
      } catch (e) {
        sendFn({ jsonrpc: "2.0", id, error: { code: -32000, message: e.message } });
      }
      return;
    }

    if (id !== undefined) {
      sendFn({ jsonrpc: "2.0", id, error: { code: -32601, message: "Method not found" } });
    }
  }

  async function call(name, args, from, sendFn) {
    // ── push ──────────────────────────────────────────────────────────────────
    if (name === "push") {
      zen.push(args.to, args.data);
      return { ok: true };
    }

    // ── identity ──────────────────────────────────────────────────────────────
    if (name === "identity") {
      const self = pairStore.get("self");
      if (!self) throw new Error("Server identity not available");
      return { pub: self.pub };
    }

    // ── graph ─────────────────────────────────────────────────────────────────
    if (name === "graph") {
      const path = args.path || [];
      let node = zen.get(args.soul);
      for (const k of path) node = node.get(k);

      if (args.op === "get") {
        return new Promise((r) => node.once((v) => r(v ?? null)));
      }

      let value = args.value;
      if (typeof value === "string") { try { value = JSON.parse(value); } catch (_) {} }

      const rawOpt = args.opt || {};
      if (rawOpt.priv) throw new Error("Raw private keys not accepted. Use pairId:\"self\" instead.");
      const kp = resolveKeys(rawOpt);
      const opt = {};
      if (kp.priv)     opt.authenticator = kp;
      if (rawOpt.cert) opt.cert = rawOpt.cert;
      if (rawOpt.pow)  opt.pow  = typeof rawOpt.pow === "string" ? JSON.parse(rawOpt.pow) : rawOpt.pow;
      const writeOpt = Object.keys(opt).length ? opt : undefined;

      if (args.op === "subscribe") {
        const sub_id = "sub_" + (++subSeq).toString(36);
        const soul = args.soul;
        const node = zen.get(soul).map();
        node.on((val, key) => {
          sendNotification(sub_id, soul, key, val);
        });
        subStore.set(sub_id, { node, from: from || null, sendFn: from ? null : (sendFn || null) });
        return { sub_id };
      }

      if (args.op === "unsubscribe") {
        const sub_id = (args.opt || {}).sub_id;
        if (!sub_id) throw new Error("unsubscribe requires opt.sub_id");
        const entry = subStore.get(sub_id);
        if (!entry) throw new Error("Unknown sub_id: " + sub_id);
        entry.node.off();
        subStore.delete(sub_id);
        return { ok: true };
      }

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
      if (rest.priv) throw new Error("Raw private keys not accepted. Use pairId:\"self\" instead.");
      const kp = resolveKeys({ pairId, ...rest });

      switch (method) {
        case "pair": {
          const PAIR_OPT = ["curve", "seed", "pub", "format"];
          const opt = {};
          PAIR_OPT.forEach(k => { if (rest[k] != null) opt[k] = rest[k]; });
          const pair = await ZEN.pair(null, Object.keys(opt).length ? opt : undefined);
          const { priv: _p, ...pub } = pair;
          return pub;
        }
        case "sign":    return ZEN.sign(rest.data, kp);
        case "verify":  return ZEN.verify(rest.signed, rest.pub);
        case "encrypt": return ZEN.encrypt(rest.data, rest.pub ? rest.pub : kp);
        case "decrypt": return ZEN.decrypt(rest.enc, kp);
        case "secret":  return ZEN.secret(rest.pub, kp);
        case "hash": {
          const opt = {};
          ["name","encode","salt","iterations","pow"].forEach(k => { if (rest[k] != null) opt[k] = rest[k]; });
          if (typeof opt.pow === "string") opt.pow = JSON.parse(opt.pow);
          return ZEN.hash(rest.data, null, null, Object.keys(opt).length ? opt : { name: "SHA-256", encode: "base62" });
        }
        case "certify": {
          if (!pairId) throw new Error("certify requires pairId");
          const rawPol = rest.policy;
          const policy = typeof rawPol === "string" && (rawPol[0] === "{" || rawPol[0] === "[")
            ? JSON.parse(rawPol) : rawPol;
          const opt    = rest.expiry ? { expiry: rest.expiry } : undefined;
          return ZEN.certify(rest.pub, policy, kp, null, opt);
        }
        case "recover": return ZEN.recover(rest.signed);
        case "pen":     return ZEN.pen(rest);
        case "candle":  return ZEN.candle(rest);
        default: throw new Error("Unknown crypto method: " + method);
      }
    }

    // ── protocol ──────────────────────────────────────────────────────────────
    if (name === "protocol") {
      const { op, pairId, ...rest } = args;

      if (op === "inbox_soul") return { soul: inboxSoul(rest.pub, rest) };
      if (op === "chan_soul")  return { soul: chanSoul(rest.proj_id, rest.chan_id, rest.owner_pub, rest) };
      if (op === "dm_soul")   return { soul: dmSoul(rest.recipient_pub, rest) };
      if (op === "get_project_meta") return getProjectMeta(rest.proj_id, rest.owner_pub, zen);
      if (op === "get_project_roles") return getProjectRoles(rest.proj_id, rest.owner_pub, zen);

      if (!pairId) throw new Error(op + " requires pairId");
      const kp = resolveKeys({ pairId });

      if (op === "set_project_meta") {
        const meta = typeof rest.meta === "string" ? JSON.parse(rest.meta) : (rest.meta || {});
        return setProjectMeta(rest.proj_id, kp, meta, zen);
      }

      if (op === "set_project_role") {
        return setProjectRole(rest.proj_id, kp, rest.member_pub, rest.role, zen);
      }

      if (op === "create_channel") {
        return createChannel(rest.proj_id, rest.chan_id, rest.version || 1, kp, rest.members || [], zen);
      }

      if (op === "invite") {
        const meta = await new Promise(r =>
          zen.get("zacp/" + kp.pub + "/" + rest.proj_id + "/" + rest.chan_id + "/meta")
             .once(v => r(v ?? null))
        );
        const version = (meta && meta.version) || 1;
        return inviteMember(rest.proj_id, rest.chan_id, version, kp, rest.member_pub, zen);
      }

      if (op === "kick") {
        const meta = await new Promise(r =>
          zen.get("zacp/" + kp.pub + "/" + rest.proj_id + "/" + rest.chan_id + "/meta")
             .once(v => r(v ?? null))
        );
        const version = (meta && meta.version) || 1;
        return rotateChanKey(rest.proj_id, rest.chan_id, version, kp, rest.remaining || [], zen);
      }

      if (op === "send_inbox") {
        return sendInbox(rest.recipient_pub, rest.message, kp, zen, rest);
      }

      if (op === "read_inbox") {
        return readInbox(kp, zen, rest.limit || 50);
      }

      if (op === "send_channel") {
        const meta = await new Promise(r =>
          zen.get("zacp/" + rest.owner_pub + "/" + rest.proj_id + "/" + rest.chan_id + "/meta")
             .once(v => r(v ?? null))
        );
        if (!meta || !meta.pub) throw new Error("Channel not found");
        const chan_priv = await unwrapChanKey(meta.pub, rest.owner_pub, rest.proj_id, rest.chan_id, kp, zen);
        if (!chan_priv) throw new Error("No channel key for this identity");
        return sendToChannel(rest.proj_id, rest.chan_id, rest.owner_pub, chan_priv, rest.message, kp, rest.cert, zen);
      }

      if (op === "read_channel") {
        const meta = await new Promise(r =>
          zen.get("zacp/" + rest.owner_pub + "/" + rest.proj_id + "/" + rest.chan_id + "/meta")
             .once(v => r(v ?? null))
        );
        if (!meta || !meta.pub) throw new Error("Channel not found");
        const chan_priv = await unwrapChanKey(meta.pub, rest.owner_pub, rest.proj_id, rest.chan_id, kp, zen);
        if (!chan_priv) throw new Error("No channel key for this identity");
        return readChannel(rest.proj_id, rest.chan_id, rest.owner_pub, chan_priv, zen, rest.limit || 50);
      }

      if (op === "send_dm") {
        return sendDM(rest.recipient_pub, rest.message, kp, zen);
      }

      if (op === "read_dms") {
        return readDMs(kp, zen, rest.limit || 50);
      }

      throw new Error("Unknown protocol op: " + op);
    }

    // ── storage ───────────────────────────────────────────────────────────────
    if (name === "storage") {
      const store = zen._graph && zen._graph._ && zen._graph._.opt && zen._graph._.opt.store;
      if (!store) return { error: "no store" };

      if (args.op === "quota") {
        return new Promise((resolve, reject) => {
          store.quota((err, info) => {
            if (err) reject(new Error(err.message || String(err)));
            else resolve(info);
          });
        });
      }

      if (args.op === "degraded") {
        return { degraded: store.degraded || false };
      }

      if (args.op === "recover") {
        if (typeof store.recover === "function") store.recover();
        else store.degraded = false;
        return { ok: true };
      }

      throw new Error("Unknown storage op: " + args.op);
    }

    // ── relay ─────────────────────────────────────────────────────────────────
    if (name === "status") {
      const mesh = zen._graph && zen._graph._ && zen._graph._.opt && zen._graph._.opt.mesh;
      const self = pairStore.get("self");

      if (args.op === "status") {
        return {
          pub:         self ? self.pub : null,
          version:     packageVersion,
          domain:      process.env.DOMAIN || null,
          port:        process.env.PORT ? parseInt(process.env.PORT, 10) : null,
          peers_near:  mesh ? (mesh.near || 0) : 0,
          mcp:         true,
        };
      }

      if (args.op === "peers") {
        return { count: mesh ? (mesh.near || 0) : 0 };
      }

      throw new Error("Unknown relay op: " + args.op);
    }

    throw new Error("Unknown tool: " + name);
  }

  // Boot relay mode after the ZEN peer has had a moment to connect to the network.
  setTimeout(startRelayMode, 500);
}

// start — boot() + attach() — backward-compatible CLI entry point.
export async function start() {
  const { zen, hwIdentity, initialPeers } = await boot();
  attach(zen, { hwIdentity, initialPeers });
}
