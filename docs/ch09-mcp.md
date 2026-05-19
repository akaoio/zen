# Chapter 9 — MCP (AI Integration)

> **Goal:** Run ZEN as a real Model Context Protocol server, understand the exact tool surface in `lib/mcp.js`, and use it from an AI client or editor without guessing what the implementation does.

---

## 9.1 What MCP means in ZEN

ZEN ships a stdio JSON-RPC 2.0 server at `lib/mcp.js`.

This server is not a thin wrapper around a remote API. It starts a real ZEN node inside the MCP process:

- imports `../index.js`, so it gets the full Node runtime stack
- creates a local ZEN instance with persistent storage under XDG data directories
- joins the peer mesh using `ZEN_PEERS` when provided
- exposes a small, explicit MCP tool surface on top of graph, crypto, and protocol helpers

At runtime, the stack looks like this:

```text
AI Client / IDE
  └── MCP (stdio, JSON-RPC 2.0)
        └── lib/mcp.js
              ├── ZEN graph instance
              ├── built-in server identity alias: self
              ├── crypto primitives
              └── ZACP protocol helpers
```

The MCP process is therefore both:

- a local automation endpoint for an agent
- a real peer participating in the ZEN network

---

## 9.2 How to run it

### With `npx`

```bash
npx -y -p @akaoio/zen mcp
```

The published package maps the `mcp` binary to `lib/mcp.min.js`.

### From a cloned repository

```bash
node lib/mcp.js
```

### With custom peers

```bash
ZEN_PEERS="wss://zen0.akao.io,wss://zen1.akao.io" node lib/mcp.js
```

The server speaks JSON-RPC 2.0 over `stdin` and `stdout`.
Diagnostic logs are suppressed from stdout because stdout must remain valid JSON-RPC transport.

---

## 9.3 Quick editor setup

### Cursor

```json
{
  "mcpServers": {
    "zen": {
      "command": "npx",
      "args": ["-y", "-p", "@akaoio/zen", "mcp"]
    }
  }
}
```

### VS Code / GitHub Copilot

```json
{
  "mcpServers": {
    "zen": {
      "command": "npx",
      "args": ["-y", "-p", "@akaoio/zen", "mcp"]
    }
  }
}
```

### Custom peers from editor config

```json
{
  "mcpServers": {
    "zen": {
      "command": "npx",
      "args": ["-y", "-p", "@akaoio/zen", "mcp"],
      "env": {
        "ZEN_PEERS": "ws://localhost:8420/zen,wss://zen0.akao.io"
      }
    }
  }
}
```

---

## 9.4 Storage and environment

The MCP server calls:

```js
const zenOpt = { file: xdg.data() + "/mcp" };
```

So the graph is stored at:

- default: `~/.local/share/zen/mcp`
- override: `$XDG_DATA_HOME/zen/mcp`

Relevant environment variables:

| Variable | Purpose |
|----------|---------|
| `ZEN_PEERS` | Comma-separated bootstrap peer URLs |
| `XDG_DATA_HOME` | Root for graph storage |
| `ZEN_SILENCE_TEST_WARNINGS` | Suppresses test-time warnings; useful in automated sessions |

---

## 9.5 Identity model

The current MCP implementation exposes exactly one built-in key alias:

```text
pairId = "self"
```

At startup, `lib/mcp.js` loads a persistent local identity via `getOrCreateIdentity()` from `lib/identity.js` and stores it in an in-memory `Map`.

Important properties of this design:

- the public key is exposed via the `identity` tool
- the private key stays inside the MCP process
- callers do not pass raw private keys for normal authenticated operations
- `pairId: "self"` is the expected way to sign, decrypt, certify, or write as the server identity

### Current behavior versus older drafts

The current code does **not** expose:

- a `hw` alias
- a `getHardwareIdentity` tool
- a `storePair` tool

If documentation elsewhere mentions `hw`, `getHardwareIdentity`, or `storePair`, that documentation is describing an older design, not the current server.

---

## 9.6 MCP transport protocol

The server supports the standard JSON-RPC methods needed for tool use:

- `initialize`
- `tools/list`
- `tools/call`

Everything else returns `-32601 Method not found`, except notifications, which are ignored.

### `initialize`

Request:

```json
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}
```

Response:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "2024-11-05",
    "capabilities": { "tools": {} },
    "serverInfo": { "name": "zen", "version": "1.0.0" }
  }
}
```

### `tools/list`

Returns the exact tool metadata declared in `lib/mcp.js`.

### `tools/call`

All tool invocations use this shape:

```json
{
  "jsonrpc": "2.0",
  "id": 10,
  "method": "tools/call",
  "params": {
    "name": "identity",
    "arguments": {}
  }
}
```

Successful calls return `result.content[0].text` as JSON text.
Failures return `error.code = -32000` with the thrown message.

---

## 9.7 Tool surface

The current MCP server exposes **exactly seven tools**:

| Tool | Purpose |
|------|---------|
| `graph` | Graph chain operations — get / put / set / subscribe / unsubscribe |
| `crypto` | All crypto primitives — pair / sign / verify / encrypt / decrypt / hash / certify / … |
| `identity` | Return this server's public key |
| `protocol` | ZACP collaboration — channels, DMs, inboxes, project metadata |
| `push` | Ephemeral peer-to-peer relay message via `zen.push()` |
| `storage` | Local rfs store diagnostics — quota / degraded state / recover |
| `status` | Relay operational status (mirrors HTTP GET `/status`) |

There are no standalone `get`, `put`, `on`, `pair`, `sign`, or `storePair` tools. Those behaviors live under `graph` and `crypto`.

---

## 9.8 `identity` tool

### Purpose

Return the public identity of the MCP server.

### Request

```json
{
  "name": "identity",
  "arguments": {}
}
```

### Response

```json
{
  "pub": "0TSEiscYykziuDXSIv0zcIc2Av4yJoZXViu5G6wkWqKI1"
}
```

### Use cases

- tell another peer where to send a DM
- verify signatures from this MCP server
- invite this identity into a channel
- use as `owner_pub` in protocol flows

---

## 9.9 `graph` tool

### Purpose

Execute raw graph operations against a local ZEN instance.

### Supported operations

- `get`
- `put`
- `set`
- `subscribe`
- `unsubscribe`

### Input schema

```json
{
  "soul": "string",
  "path": ["optional", "key", "segments"],
  "op": "get | put | set | subscribe | unsubscribe",
  "value": "any JSON value",
  "opt": {
    "pairId": "self",
    "cert": "optional certificate string",
    "pow": { "unit": "0", "difficulty": 1 },
    "sub_id": "sub_1"
  }
}
```

### Behavior

The server resolves the chain like this:

```js
let node = zen.get(args.soul)
for (const k of path) node = node.get(k)
```

Then it executes:

- `node.once(...)` for `get`
- `node.put(...)` for `put`
- `node.set(...)` for `set`
- `node.map().on(...)` for `subscribe`
- removes the listener for `unsubscribe`

### Security behavior

- raw private keys are rejected
- `pairId: "self"` attaches `opt.authenticator = kp`
- `cert` is passed through when provided
- `pow` is parsed and passed through when provided

### Examples

Read:

```json
{
  "name": "graph",
  "arguments": {
    "soul": "demo/manual",
    "path": ["status"],
    "op": "get"
  }
}
```

Authenticated write:

```json
{
  "name": "graph",
  "arguments": {
    "soul": "demo/manual",
    "path": ["status"],
    "op": "put",
    "value": "ok-from-mcp",
    "opt": { "pairId": "self" }
  }
}
```

### Live subscriptions

`subscribe` attaches a `map().on()` listener to the soul and returns a `sub_id`. For every change the server emits a JSON-RPC notification to the client:

```json
{
  "jsonrpc": "2.0",
  "method": "notifications/message",
  "params": {
    "level": "info",
    "logger": "zen/subscribe",
    "data": "{\"sub_id\":\"sub_1\",\"soul\":\"demo/node\",\"key\":\"status\",\"val\":\"updated\"}"
  }
}
```

Subscribe:

```json
{
  "name": "graph",
  "arguments": {
    "soul": "demo/node",
    "op": "subscribe"
  }
}
```

Response: `{ "sub_id": "sub_1" }`

Unsubscribe:

```json
{
  "name": "graph",
  "arguments": {
    "soul": "demo/node",
    "op": "unsubscribe",
    "opt": { "sub_id": "sub_1" }
  }
}
```

Response: `{ "ok": true }`

Subscriptions are process-scoped and do not persist across server restarts. This feature works in stdio mode. HTTP/SSE push mode is planned but not yet implemented.

---

## 9.10 `crypto` tool

### Purpose

Expose the main static crypto and policy helpers from the ZEN runtime.

### Supported methods

- `pair`
- `sign`
- `verify`
- `encrypt`
- `decrypt`
- `secret`
- `hash`
- `certify`
- `recover`
- `pen`
- `candle`

### `pair`

Generate a public keypair description.
The MCP layer strips private fields before returning the result.

Example:

```json
{
  "name": "crypto",
  "arguments": {
    "method": "pair",
    "curve": "p256",
    "format": "evm"
  }
}
```

### `sign`

```json
{
  "name": "crypto",
  "arguments": {
    "method": "sign",
    "data": "hello",
    "pairId": "self"
  }
}
```

Returns a compact signed string:

- secp256k1: `<86 base62 sig><v>:<message>`
- p256: `<86 base62 sig><v>/p256:<message>`

### `verify`

```json
{
  "name": "crypto",
  "arguments": {
    "method": "verify",
    "signed": "<signed string>",
    "pub": "<pub>"
  }
}
```

Returns the original data or `null`/`undefined` when verification fails.

### `encrypt` / `decrypt`

`encrypt` can use an explicit target pub or the resolved keypair.
`decrypt` uses `pairId: "self"`.

Encrypted output uses the compact base62 wire format:

```text
<ct_base62>.<iv_base62_21>.<salt_base62_13>
```

### `secret`

Derive an ECDH shared secret from a remote pub and the local keypair.

### `hash`

Supports the same options as the runtime, including PoW:

```json
{
  "method": "hash",
  "data": "payload",
  "name": "SHA-256",
  "encode": "hex",
  "pow": { "unit": "0", "difficulty": 2 }
}
```

### `certify`

Create a compact certificate string signed by `pairId: "self"`.

```json
{
  "method": "certify",
  "pairId": "self",
  "pub": "<recipient pub>",
  "policy": { "write": { "#": "proj/demo/chan/general" } }
}
```

### `recover`

Recover the signer pub from a compact signed string.

### `pen`

Compile a PEN soul from a policy spec.

### `candle`

Return a candle expression object suitable for `ZEN.pen(...)`.

---

## 9.11 `protocol` tool

### Purpose

Expose higher-level ZACP collaboration helpers from `lib/protocol.js`.

### Supported operations

- `inbox_soul`
- `chan_soul`
- `dm_soul`
- `get_project_meta`
- `get_project_roles`
- `set_project_meta`
- `set_project_role`
- `create_channel`
- `invite`
- `kick`
- `send_channel`
- `send_dm`
- `send_inbox`
- `read_dms`
- `read_inbox`
- `read_channel`

Soul-only operations (`inbox_soul`, `chan_soul`, `dm_soul`) and read-only operations (`get_project_meta`, `get_project_roles`) do not require `pairId`.
All write/send/create operations require `pairId: "self"`.

### `inbox_soul`

Compile the PEN soul for a public inbox.

```json
{ "op": "inbox_soul", "pub": "<pub>" }
```

### `chan_soul`

Compile the PEN soul for a channel.

```json
{ "op": "chan_soul", "proj_id": "demo", "chan_id": "general", "owner_pub": "<pub>" }
```

### `dm_soul`

Compile the PEN soul for a direct-message inbox.

```json
{ "op": "dm_soul", "recipient_pub": "<pub>" }
```

### `get_project_meta`

Read project metadata stored by the project owner.

```json
{ "op": "get_project_meta", "proj_id": "demo", "owner_pub": "<owner pub>" }
```

Response:

```json
{ "name": "Demo Project", "description": "...", "version": 1 }
```

### `get_project_roles`

Read the role map for a project. Keys are member public keys, values are role strings.

```json
{ "op": "get_project_roles", "proj_id": "demo", "owner_pub": "<owner pub>" }
```

Response (example):

```json
{
  "0TSEisc...": "owner",
  "0GFuf6J...": "member"
}
```

### `set_project_meta`

Write project metadata. Requires `pairId: "self"` — the caller becomes the project owner (stored under `zacp/<owner_pub>/<proj_id>/meta`).

```json
{
  "op": "set_project_meta",
  "proj_id": "demo",
  "meta": { "name": "Demo Project", "description": "An example" },
  "pairId": "self"
}
```

Returns `{ "ok": true }`.

### `set_project_role`

Assign a role to a member inside a project.

```json
{
  "op": "set_project_role",
  "proj_id": "demo",
  "member_pub": "<member pub>",
  "role": "member",
  "pairId": "self"
}
```

Returns `{ "ok": true }`.

### `create_channel`

Creates a new channel namespace and stores:

- wrapped channel private key for each listed member
- `meta/pub`
- `meta/version`
- `meta/since`

The storage namespace is:

```text
zacp/<owner_pub>/<proj_id>/<chan_id>
```

Example:

```json
{
  "op": "create_channel",
  "proj_id": "demo_proj_real",
  "chan_id": "general",
  "pairId": "self"
}
```

Response:

```json
{
  "chan_pub": "0GFuf6JaKTfoxwLEE2NNxYS426UDdOCOVFe7qFS08d7V0",
  "version": 1,
  "soul": "!CCt1..."
}
```

### `invite`

Wraps the current channel key for a new member and issues a compact write certificate.

Example:

```json
{
  "op": "invite",
  "proj_id": "demo_proj_real",
  "chan_id": "general",
  "member_pub": "<recipient pub>",
  "pairId": "self"
}
```

Response:

```json
{
  "cert": "<compact signed certificate>",
  "chan_pub": "<channel pub>"
}
```

### `kick`

Rotates the channel key by incrementing `version` and re-wrapping only for remaining members.

### `send_channel`

Workflow:

1. load `meta.pub` for the channel
2. unwrap the caller's encrypted channel key
3. encrypt the plaintext with the channel secret
4. write `JSON.stringify({ a, m, "+": cert })` into the channel PEN soul
5. authenticate the write with `pairId: "self"`

Example:

```json
{
  "op": "send_channel",
  "proj_id": "demo_proj_real",
  "chan_id": "general",
  "owner_pub": "<owner pub>",
  "message": "hello from real mcp session",
  "cert": "<compact certificate>",
  "pairId": "self"
}
```

Returns:

```json
{ "ok": true }
```

### `send_dm`

Workflow:

1. derive an ECDH shared secret from recipient pub and sender pair
2. encrypt plaintext with that shared secret
3. choose a candle-based key using the current hour and a content hash
4. write `JSON.stringify({ a, m })` to the DM PEN soul
5. authenticate the write and auto-mine PoW difficulty 1

Example:

```json
{
  "op": "send_dm",
  "recipient_pub": "<recipient pub>",
  "message": "hello dm from real mcp session",
  "pairId": "self"
}
```

Returns:

```json
{ "ok": true }
```

### `read_dms`

Reads from the caller's DM soul, decodes each signed or already-unwrapped message payload, derives the shared secret per sender, and decrypts the message body.

Example:

```json
{
  "op": "read_dms",
  "pairId": "self",
  "limit": 10
}
```

Example response:

```json
[
  {
    "key": "493827:0csoYMXjUpczthM5",
    "plaintext": "hello dm from real mcp session",
    "sender_pub": "0TSEiscYykziuDXSIv0zcIc2Av4yJoZXViu5G6wkWqKI1"
  }
]
```

### `send_inbox`

Send a message to a recipient's public inbox. Uses ECDH shared secret with the recipient for encryption.

```json
{
  "op": "send_inbox",
  "recipient_pub": "<pub>",
  "message": "hello via inbox",
  "pairId": "self"
}
```

Returns `{ "ok": true }`.

The inbox soul enforces `sign: true` (no cert required). The hourly candle key window provides time-based expiry. Add `pow` to the `opt` argument for anti-spam:

```json
{ "op": "send_inbox", "recipient_pub": "<pub>", "message": "hello", "pairId": "self", "pow": { "unit": "0", "difficulty": 1 } }
```

### `read_inbox`

Read messages from the caller's own inbox soul. Decrypts each using ECDH shared secret with sender.

```json
{
  "op": "read_inbox",
  "pairId": "self",
  "limit": 20
}
```

Response format matches `read_dms`:

```json
[
  { "key": "493827:0csoYMXjUpczthM5", "plaintext": "hello via inbox", "sender_pub": "<pub>" }
]
```

### `read_channel`

Read messages from a channel soul. Requires the caller to have their wrapped channel key in the graph (i.e. they were invited via `invite`).

```json
{
  "op": "read_channel",
  "proj_id": "demo",
  "chan_id": "general",
  "owner_pub": "<owner pub>",
  "pairId": "self",
  "limit": 50
}
```

Response format:

```json
[
  { "key": "493827:0csoYMXjUpczthM5", "plaintext": "hello from channel", "sender_pub": "<pub>" }
]
```

---

## 9.12 `push` tool

### Purpose

Send an ephemeral, best-effort message to any ZEN peer identified by public key.
Uses `zen.push(pub, data)` under the hood — backed by the DAM relay mesh.
There is no persistence and no delivery acknowledgement.
Use `graph.put` when durability matters.

### Request

```json
{
  "method": "tools/call",
  "params": {
    "name": "push",
    "arguments": {
      "to": "<recipient-pub>",
      "data": { "type": "ping", "ts": 1715000000 }
    }
  }
}
```

### Response

```json
{ "ok": true }
```

`ok: true` means the message was handed to the relay — not that it was delivered.

### Receiving pushed messages

Subscribe to a soul that the sender writes into:

```json
{ "name": "graph", "arguments": { "soul": "inbox/<your-pub>", "op": "subscribe" } }
```

Or use `protocol.read_inbox` / `protocol.read_dms` for authenticated inboxes.

---

## 9.13 `storage` tool

### Purpose

Inspect and manage the local rfs (relay file store) that backs graph persistence.
Only available when the MCP process **is** the relay (binds port 8420 directly).
When running as a thin peer connected to an external relay, all ops return `{ "error": "no store" }`.

### Supported operations

| op | Arguments | Returns |
|----|-----------|---------|
| `quota` | — | `{ used, free, total }` in bytes |
| `degraded` | — | `{ degraded: bool }` |
| `recover` | — | `{ ok: true }` |

`degraded: true` means the store hit an ENOSPC or OOM error and rejected all new writes.
Call `recover` after freeing disk space to resume normal writes.

### `quota`

```json
{
  "method": "tools/call",
  "params": {
    "name": "storage",
    "arguments": { "op": "quota" }
  }
}
```

```json
{ "used": 1073741824, "free": 5368709120, "total": 6442450944 }
```

All values in bytes. Backed by Node.js `fs.statfs()` (requires Node.js 19+).

### `degraded`

```json
{ "name": "storage", "arguments": { "op": "degraded" } }
```

```json
{ "degraded": false }
```

### `recover`

```json
{ "name": "storage", "arguments": { "op": "recover" } }
```

```json
{ "ok": true }
```

Clears the degraded flag so writes are accepted again. Call this only after you have freed sufficient disk space — otherwise the store will immediately re-enter degraded mode on the next write attempt.

---

## 9.14 `status` tool

### Purpose

Return live relay metadata — the same payload as HTTP `GET /status`.
Useful for an agent that needs to confirm the relay's pub key, version, or mesh connectivity before performing authenticated graph writes.

### Supported operations

| op | Returns |
|----|---------|
| `status` | `{ pub, version, domain, port, peers_near, mcp: true }` |
| `peers` | `{ count: N }` |

`peers_near` is `mesh.near` — the DAM layer's live estimate of directly reachable peers.

### `status`

```json
{
  "method": "tools/call",
  "params": {
    "name": "status",
    "arguments": { "op": "status" }
  }
}
```

```json
{
  "pub": "0wTwUprS...",
  "version": "1.0.25",
  "domain": "zen.akao.io",
  "port": 443,
  "peers_near": 3,
  "mcp": true
}
```

### `peers`

```json
{ "name": "status", "arguments": { "op": "peers" } }
```

```json
{ "count": 3 }
```

---

## 9.15 A real end-to-end session

The current server has been exercised directly over stdio with this flow:

1. `initialize`
2. `tools/list`
3. `identity`
4. `protocol.create_channel`
5. `protocol.invite`
6. `protocol.send_channel`
7. `protocol.send_dm`
8. `protocol.read_dms`
9. `crypto.sign`
10. `crypto.verify`
11. `graph.put`
12. `graph.get`

This matters because it confirms the server works as a real MCP endpoint, not only through the repository's Mocha tests.

---

## 9.16 Security model

Important constraints in the current implementation:

- private keys remain inside the MCP process
- agents are expected to use `pairId: "self"`
- raw `priv` values are rejected on write/signing paths exposed to agents
- `identity` reveals only the public key
- `protocol` composes authenticated graph writes with PEN-enforced souls

The MCP server is therefore suitable for:

- AI-assisted graph automation
- agent-to-agent collaboration over channels and DMs
- local secure data workflows where the editor acts as a ZEN peer

It is not designed to be a general wallet server that accepts arbitrary private key material from untrusted clients.

---

## 9.17 Relationship to the rest of the book

- Use Chapter 3 for the exact crypto method semantics.
- Use Chapter 4 for how authenticated writes and certificates are enforced.
- Use Chapter 7 for how PEN souls used by `inbox_soul`, `chan_soul`, and `dm_soul` are compiled and checked.
- Use Chapter 8 when modifying `lib/mcp.js`, `lib/protocol.js`, or the build/test workflow around them.

---

## 9.18 Relay RPC mode — cross-machine MCP without HTTP

When the MCP server has an identity, it automatically starts a **relay RPC listener** on the same WebSocket mesh that ZEN uses for graph sync.

```
Agent process (any machine, any network)
  └── ZenMcpClient (lib/mcp/client.js)
        └── mesh.relay(serverPub, encryptedRequest)
              ↓  DAM relay routing (multi-hop, XOR-distance)
              ↓
MCP server (lib/mcp.js, any machine)
  └── mesh.onRelay → decrypt → dispatchRelay → encrypt → mesh.relay back
```

**Properties:**
- No HTTP server, no open ports — both sides only need outbound WebSocket
- All messages encrypted with ECDH shared secret (`ZEN.secret(pub, pair)` + `ZEN.encrypt`)
- Server auto-publishes a discovery soul: `~<serverPub>/mcp/info`
- Relay mode starts automatically 500ms after ZEN connects to peers (if identity exists)
- Relay mode is unavailable without an identity (`self` key in `pairStore`)

**Discovery soul** — read before connecting to confirm the server is reachable:

```js
const info = await ZenMcpClient.discover(serverPub, zen, 2000);
// info = { name: "zen-mcp", version: "1.0.x", pub: serverPub, relay: true }
// info = null if server not reachable within 2 seconds
```

---

## 9.19 `ZenMcpClient` — JavaScript relay client

`lib/mcp/client.js` provides a `ZenMcpClient` class for making MCP calls to a relay-mode server from any JavaScript environment (Node, browser, or another ZEN agent).

### Import

```js
import { ZenMcpClient } from "./lib/mcp/client.js";
// or
import ZenMcpClient from "./lib/mcp/client.js";
```

### Constructor

```js
const client = new ZenMcpClient(serverPub, zen, myPair, { timeout: 10000, ttl: 5 });
```

| Param | Type | Description |
|-------|------|-------------|
| `serverPub` | string | Target MCP server's public key |
| `zen` | ZEN | A live ZEN instance with WebSocket peers |
| `myPair` | object | Caller's key pair `{ pub, priv }` |
| `opt.timeout` | number | Default request timeout in ms (default: 10000) |
| `opt.ttl` | number | DAM relay hop limit (default: 5) |

### Methods

#### `await client.ready()`

Wire up the relay listener. Must be called once before making requests.

#### `await client.initialize()`

Send the MCP `initialize` handshake. Returns `{ protocolVersion, serverInfo, capabilities }`.

#### `await client.listTools()`

Returns an array of tool descriptors matching `tools/list`.

#### `await client.call(name, args, timeout?)`

Call a tool by name. Returns the parsed tool result.

```js
const info = await client.call("identity", {});
// { pub: "0TSE..." }

const signed = await client.call("crypto", {
  method: "sign",
  data: "hello",
  pairId: "self",
});

const data = await client.call("graph", {
  soul: "demo/node",
  path: ["status"],
  op: "get",
});
```

#### `await client.request(method, params, timeout?)`

Send a raw JSON-RPC request and return the full response object `{ jsonrpc, id, result?, error? }`.

#### `client.close()`

Stop the relay listener and reject all pending requests.

### Full example

```js
import ZEN from "./zen.js";
import { ZenMcpClient } from "./lib/mcp/client.js";

const myPair = await ZEN.pair();
const zen    = new ZEN({ peers: ["wss://zen0.akao.io"] });

// Discover the server
const serverPub = "0TSEiscYykziuDXSIv0zcIc2Av4yJoZXViu5G6wkWqKI1";
const info = await ZenMcpClient.discover(serverPub, zen);
if (!info) throw new Error("server not found");

// Connect and use
const client = new ZenMcpClient(serverPub, zen, myPair);
await client.ready();
await client.initialize();

const result = await client.call("protocol", {
  op: "send_dm",
  recipient_pub: serverPub,
  message: "hello from relay client",
  pairId: "self",
});

client.close();
```

---

## 9.20 SSE transport mode (planned)

> **Not yet implemented.** The `--sse` flag does not exist in the current codebase. This section describes the planned design.

The planned SSE transport would expose the MCP server over HTTP + Server-Sent Events so that browser-based agents and multi-client workflows can connect without spawning a child process.

```bash
node lib/mcp.js           # stdio mode (default, use with Claude Desktop and VS Code)
node lib/mcp.js --sse     # planned: HTTP + SSE mode (port: ZEN_MCP_PORT env var or 8421)
```

When implemented, both modes would run in the same process, sharing the same `pairStore`, `zen` instance, and tool dispatch logic.

**Intended use cases:**
- Browser-based agents that cannot spawn child processes
- Workflows where multiple clients share one ZEN peer

**Relationship to relay mode:** Relay mode (§9.15) is the current solution for cross-machine MCP access. It works across any network with only outbound WebSocket and requires no open HTTP port. SSE mode would complement relay mode for local network / intranet scenarios where HTTP reachability is guaranteed.

---

## 9.21 Relay-embedded MCP — `attach()` and the IPC transport

When the relay service (`script/server.js`) is running, MCP is embedded directly into the relay process via `attach()`. No second ZEN instance is created — the relay's own ZEN graph is exposed to AI clients.

### Architecture

```
relay process (cluster worker)
  ├── ZEN graph instance          ← shared between relay and MCP
  ├── WebSocket server (port 8420)
  ├── UDP fast path (port 8421)
  └── MCP IPC server (Unix socket: ~/.local/share/zen/mcp.sock)
        └── client connects via lib/mcp.js bridge
              ├── reads requests from agent's stdin
              └── forwards over socket to relay's MCP handler
```

The key call in `script/server.js`:

```js
attachMcp(zen, { hwIdentity: identity, ipc: true });
```

This mounts the MCP tool surface (graph, crypto, identity, protocol) onto the relay's `zen` instance and starts a Unix domain socket server at `mcp.sock`. Any MCP client (`node lib/mcp.js`) that detects the socket bridges its stdio over the socket instead of booting a new ZEN peer.

### Why no stdio in cluster workers

The relay uses Node.js `cluster` — the actual relay logic runs in a **worker** process (not the master). In cluster workers:

- `process.stdin` is not a TTY (`process.stdin.isTTY === false`)
- The worker's stdin is `/dev/null`
- A `readable.once("end", cb)` listener on `/dev/null` fires **immediately**

The MCP stdio transport has a guard:

```js
// lib/mcp/server.js
if (!process.stdin.isTTY && !cluster.isWorker) {
  process.stdin.once("end", () => process.exit(0));
}
```

Without `!cluster.isWorker`, the `process.exit(0)` call was fired milliseconds after the worker started, causing a crash loop (`Worker died with code 0 and signal null`). The IPC transport (Unix socket) remains active in workers and is not affected by this guard.

### Singleton guarantee

- Only one process can bind TCP port 8420 — the relay is a natural singleton.
- The IPC socket path (`mcp.sock`) is overwritten on relay start and cleaned up on exit.
- All AI client sessions share the single relay ZEN instance — zero data duplication, zero extra peers.

### Fallback when relay is not running

If the relay is not running, `lib/mcp.js` detects the absent socket and starts its own ZEN instance in standalone mode (`boot()`). In this case the unique `pid = hash(hwPub + "/mcp/" + process.pid)` ensures no AXE conflict with other concurrent standalone MCP processes.
