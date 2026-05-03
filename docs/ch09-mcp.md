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
ZEN_PEERS="wss://peer0.akao.io,wss://peer1.akao.io" node lib/mcp.js
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
        "ZEN_PEERS": "ws://localhost:8420/zen,wss://peer0.akao.io"
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

The current MCP server exposes **exactly four tools**:

- `graph`
- `crypto`
- `identity`
- `protocol`

There are no standalone `get`, `put`, `on`, `pair`, `sign`, or `storePair` tools in the current implementation. Those behaviors now live under `graph` and `crypto`.

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

### Input schema

```json
{
  "soul": "string",
  "path": ["optional", "key", "segments"],
  "op": "get | put | set",
  "value": "any JSON value",
  "opt": {
    "pairId": "self",
    "cert": "optional certificate string",
    "pow": { "unit": "0", "difficulty": 1 }
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
- `create_channel`
- `invite`
- `kick`
- `send_channel`
- `send_dm`
- `read_dms`

The soul-only operations do not require `pairId`.
All other operations require `pairId: "self"`.

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

---

## 9.12 A real end-to-end session

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

## 9.13 Security model

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

## 9.14 Relationship to the rest of the book

- Use Chapter 3 for the exact crypto method semantics.
- Use Chapter 4 for how authenticated writes and certificates are enforced.
- Use Chapter 7 for how PEN souls used by `inbox_soul`, `chan_soul`, and `dm_soul` are compiled and checked.
- Use Chapter 8 when modifying `lib/mcp.js`, `lib/protocol.js`, or the build/test workflow around them.
