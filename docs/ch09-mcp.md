# Chapter 9 — MCP (AI Integration)

> **Goal:** Make any AI-powered IDE a full ZEN peer. Your editor stores data in the decentralized graph, syncs with other peers, and exposes every ZEN crypto and graph API to AI agents.

---

## 9.1 What is MCP?

MCP (Model Context Protocol) is the standard protocol for connecting AI assistants in IDEs to external tools and data sources. ZEN's MCP server (`lib/mcp.js`) turns your IDE into a full ZEN P2P peer — not a thin client.

When you start the MCP server:

- A ZEN node starts in your IDE process
- It connects to `wss://peer0.akao.io` and `wss://peer1.akao.io` (or `$ZEN_PEERS`)
- LAN peers are discovered via IPv4/IPv6 multicast on `233.255.255.255:8420` / `ff02::1:8420`
- Graph data is persisted at `~/.local/share/zen/mcp` (XDG standard)
- Every write propagates to all connected peers; every peer contributes to routing

---

## 9.2 Quick start

### Cursor

Add to `~/.cursor/mcp.json` (global) or `.cursor/mcp.json` (project):

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

### VSCode / GitHub Copilot

Add to `~/.copilot/mcp-config.json`:

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

### Claude Desktop

Add to `~/Library/Application Support/Claude/claude_desktop_config.json` (macOS) or `%APPDATA%\Claude\claude_desktop_config.json` (Windows):

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

No ZEN installation needed — `npx` downloads `@akaoio/zen` automatically on first use.

---

## 9.3 Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `ZEN_PEERS` | `wss://peer0.akao.io,wss://peer1.akao.io` | Comma-separated bootstrap peer URLs |
| `XDG_DATA_HOME` | `~/.local/share` | Data directory root (graph stored at `$XDG_DATA_HOME/zen/mcp`) |
| `XDG_STATE_HOME` | `~/.local/state` | State directory root (hardware identity stored at `$XDG_STATE_HOME/zen/identity.json`) |

---

## 9.4 Hardware identity (shared with server)

The MCP server automatically generates a **hardware-derived identity** from your machine's:
- `/etc/machine-id` — stable 128-bit UUID set at OS install
- MAC address — first non-loopback interface (sorted for determinism)
- Hostname — fallback entropy source

This identity is:
- **Deterministic** — same keypair on every restart
- **Shared** — ZEN server (`script/server.js`) uses the same identity on the same hardware
- **Persistent** — metadata saved to `~/.local/state/zen/identity.json`
- **Secure** — private keys are never written to disk, only derived from hardware entropy

### Using the hardware identity in MCP tools

The hardware identity is available via the special `pairId: "hw"`:

```javascript
// Get hardware identity info
getHardwareIdentity() → { pairId: "hw", pub: "0...", epub: "0...", hwid: "..." }

// Sign with hardware identity
crypto({ method: "sign", data: "hello", pairId: "hw" })

// Write to graph with hardware identity
graph({ soul: "users", path: ["alice"], op: "put", value: {...}, opt: { pairId: "hw" } })
```

**Benefits:**
- No need to generate or store keys manually
- Same identity across MCP server and ZEN relay on the same machine
- Private keys never leave the process (not stored on disk, only derived on demand)
- Ideal for server-side authenticated operations

**Privacy note:** The hardware identity is derived from machine-specific data. If you need per-application or per-user identities, use `crypto({ method: "pair", seed: "app-specific", store: true })` instead.

---

## 9.5 Tool reference

All tools map 1:1 to the ZEN JavaScript API. All parameters are strings.

### `getHardwareIdentity` — get server's hardware-derived identity

No parameters required.

Returns:
```json
{
  "pairId": "hw",
  "pub": "0...",
  "epub": "0...",
  "hwid": "..."
}
```

Use `pairId: "hw"` in `graph` and `crypto` operations to sign/encrypt with the persistent server identity.

---

### `graph` — execute graph operations

| Param | Required | Description |
|-------|----------|-------------|
| `soul`  | ✓ | Soul (node ID) |
| `path`  | — | Array of keys for nested access, e.g. `["alice","age"]` |
| `op`    | ✓ | Operation: `"get"`, `"put"`, or `"set"` |
| `value` | — | Value to write (for `put`/`set`) |
| `opt`   | — | Options object: `{ pairId?, cert?, pow? }` |

Examples:
```javascript
// Read
graph({ soul: "users", path: ["alice","age"], op: "get" }) → 30

// Write with hardware identity
graph({ soul: "users", path: ["alice","age"], op: "put", value: 31, opt: { pairId: "hw" } })

// Set operation (append to set node)
graph({ soul: "tags", path: ["popular"], op: "set", value: "zen", opt: { pairId: "hw" } })
```

---

### `crypto` — call ZEN crypto methods

| Param | Required | Description |
|-------|----------|-------------|
| `method` | ✓ | Crypto method name (see below) |
| `pairId` | — | Key alias from `storePair` or `"hw"` for hardware identity |
| ...      | — | Method-specific parameters |

**Methods:**

- `pair` — `{ curve?, seed?, priv?, epriv?, pub?, epub?, store? }` → key pair
- `sign` — `{ data, pairId }` or `{ data, priv, pub }` → signed string
- `verify` — `{ signed, pub }` → original data or null
- `encrypt` — `{ data, epub }` → encrypted blob
- `decrypt` — `{ enc, pairId }` or `{ enc, epriv }` → plaintext
- `secret` — `{ epub, pairId }` or `{ epub, epriv }` → shared secret
- `hash` — `{ data, name?, encode?, salt?, iterations?, pow? }` → hash string
- `certify` — `{ pub, policy, pairId }` or `{ pub, policy, priv }` → certificate
- `recover` — `{ signed }` → public key
- `pen` — `{ key?, val?, soul?, state?, path?, sign?, cert?, open?, pow? }` → soul string
- `candle` — `{ seg?, sep?, size?, back?, fwd? }` → key expression

**Using hardware identity:**
```javascript
// Sign with hardware identity
crypto({ method: "sign", data: "hello", pairId: "hw" })

// Generate new pair and store
crypto({ method: "pair", seed: "my-app", store: true }) → { pairId: "pair_xyz", pub: "...", epub: "..." }
```

---

### `storePair` — store a key pair in server memory

| Param | Required | Description |
|-------|----------|-------------|
| `priv`  | — | Signing private key |
| `pub`   | — | Signing public key |
| `epriv` | — | Encryption private key |
| `epub`  | — | Encryption public key |

Returns `{ pairId, pub, epub }`. Use the returned `pairId` in subsequent operations instead of passing raw private keys.

**Security benefit:** Private keys are stored only in the MCP server process and never exposed to the LLM after the initial `storePair` call.

---

### Legacy `get` / `put` / `on` tools

Deprecated in favor of the unified `graph` tool. Still supported for backward compatibility:

| Tool | Equiv | Description |
|------|-------|-------------|
| `get(soul, key)` | `graph({ soul, path: [key], op: "get" })` | Read once |
| `put(soul, key, value)` | `graph({ soul, path: [key], op: "put", value })` | Write |
| `on(soul, key)` | `graph({ soul, path: [key], op: "get" })` | Read (alias) |

---

## 9.6 Architecture

```
┌─────────────────────────────────────────┐
│  IDE (Cursor / VSCode / Claude Desktop) │
│                                         │
│  AI Agent ─── MCP protocol ───► zen     │
│                  (stdio JSON-RPC 2.0)   │
└────────────────────┬────────────────────┘
                     │
              lib/mcp.js
         (ZEN peer + MCP server)
                     │
         ┌───────────┼───────────┐
         │           │           │
    ZEN graph   XDG storage   P2P mesh
    get/put/on  ~/.local/share  WebSocket
                zen/mcp/       + multicast
```

The MCP server is a real ZEN peer:
- Imports `index.js` — the full Node.js stack (rfs, axe, multicast, websocket)
- Every `put` propagates to connected peers; every `get` reads from the merged graph
- Your IDE's data is part of the same decentralized graph as relay nodes

---

## 9.7 Coexistence with a relay

If you run a ZEN relay on port 8420 on the same machine, the MCP server connects to it as a WebSocket peer. No port conflict — the MCP server is a client, not a server on that port.

Set `ZEN_PEERS` to point at your local relay:

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

## 9.8 Running directly

```bash
# Without install (npx)
npx -p @akaoio/zen mcp

# From a cloned repo
node lib/mcp.js

# With custom peers
ZEN_PEERS=wss://my-relay.example.com node lib/mcp.js
```

The server speaks JSON-RPC 2.0 over `stdin`/`stdout`. Logs go to `stderr`.
