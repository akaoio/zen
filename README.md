# ZEN — Zen Entropy Network

**A realtime, decentralized, offline-first graph database with a built-in cryptographic runtime and policy VM.**

```js
import ZEN from "@akaoio/zen";

const zen = new ZEN({ file: "data" });
zen.get("user").put({ name: "Alice" });
zen.get("user").once(console.log);  // { name: "Alice" }
```

---

## The Book

This repository is documented as a structured book. Read it in order or jump to the chapter you need.

| Chapter | Topic |
|---------|-------|
| [Ch 1 — Getting Started](docs/ch01-getting-started.md) | Install, Hello ZEN!, first graph, first crypto call |
| [Ch 2 — Graph Model](docs/ch02-graph-model.md) | Nodes, souls, HAM/CRDT, state timestamps, `get`, `put`, `on`, `map` |
| [Ch 3 — Cryptography](docs/ch03-crypto.md) | `pair`, `sign`, `verify`, `encrypt`, `decrypt`, `secret`, `hash`, `certify` |
| [Ch 4 — Authenticated Data](docs/ch04-authenticated-data.md) | Owned namespaces, signing writes, certificates, security pipeline |
| [Ch 5 — Storage Adapters](docs/ch05-storage.md) | Radisk, filesystem, IndexedDB, OPFS, S3, writing your own |
| [Ch 6 — Networking](docs/ch06-networking.md) | Mesh, peers, WebSocket, WebRTC, message protocol |
| [Ch 7 — PEN Policy VM](docs/ch07-pen.md) | WASM bytecode engine, opcodes, `ZEN.pen()`, `ZEN.run()` |
| [Ch 8 — Contributing](docs/ch08-contributing.md) | Build system, test suite, adding chain methods, adding adapters |
| [Ch 9 — MCP (AI Integration)](docs/ch09-mcp.md) | IDE peer, stdio server, tools, Cursor/VSCode config |

---

## What ZEN does differently

- **Offline-first CRDT** — every write is a HAM (Hypothetical Amnesia Machine) state vector; peers converge without coordination
- **No central server** — peers are symmetric; any node can relay
- **Self-discovery** — peers find their own domain/IP via config → STUN → `ip route`; scan sibling peers by numeric index pattern (`peer1.akao.io` → probes `peer0`, `peer2`…)
- **PEX** — peers share known URLs on connect; no public graph required
- **Graph, not table** — arbitrary node relationships, circular references native
- **Crypto built in** — secp256k1 keys, AES-GCM encryption, ECDH shared secrets, deterministic certifications
- **Policy VM** — PEN is a Zig-compiled WASM bytecode engine for write-access policies
- **Multi-curve** — secp256k1 (default), P-256/secp256r1, EVM (0x), Bitcoin P2PKH
- **Zero dependencies in browser** — bundled `zen.js` has no npm runtime dependencies

---

## Install

**As a library** (Node.js / browser bundle):

```bash
npm install @akaoio/zen
```

Node.js ≥ 18 is required. The bundled `zen.js` runs in any modern browser with no build step.

---

## Deploy a Relay Peer

One command installs Node.js 24, clones ZEN, creates a systemd service, and adds the `zen` CLI:

```bash
curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash
```

After install:

```bash
zen status      # service state, version, XDG paths, SSL expiry
zen update      # pull latest & restart
zen uninstall   # remove everything
```

Options (pass after `bash -s --`):

```bash
curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash -s -- \
  --port 443 \
  --domain peer1.akao.io \
  --peers "https://peer0.akao.io/zen"
```

| Option | Default | Description |
|--------|---------|-------------|
| `--port` | `8420` | Listening port |
| `--domain` | auto-detected | Your relay domain (saved to `~/.config/zen/domain`) |
| `--peers` | none | Comma-separated seed peer URLs |

The domain is used for self-identification and peer scanning. If omitted, ZEN detects it at runtime via STUN or the incoming request `Host` header.

### Automatic peer discovery

Once your relay is running it finds other peers automatically — no seed peers required:

1. **Smart scan** — detects numeric index in your domain (`peer1.akao.io`) and probes `peer0`, `peer2`… up to `peer100`; stops after 10 found
2. **PEX** — each new connection immediately shares its peer list; new discoveries are forwarded to all existing connections
3. **Backoff** — empty scan cycles back off from 10 m → 20 m → 40 m → capped at 2 h; resets immediately on any discovery
4. **Upstream cap** — at most 10 outbound connections from scan; inbound excess redirected by AXE MOB

### POSIX / XDG Base Directory layout

ZEN follows the [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/) — no files land in `~/` or the project directory.

| Purpose | Default path | Override |
|---------|-------------|---------|
| SSL key & cert, pass file | `~/.config/zen/` | `$XDG_CONFIG_HOME/zen/` |
| Graph data (radata) | `~/.local/share/zen/radata/` | `$XDG_DATA_HOME/zen/` |
| Stats / state | `~/.local/state/zen/` | `$XDG_STATE_HOME/zen/` |
| `zen` CLI binary | `/usr/local/bin/zen` or `~/.local/bin/zen` | — |

SSL setup (Let's Encrypt via acme.sh):

```bash
bash script/ssl.sh --domain relay.example.com --email you@example.com
```

---

## Quick examples

### Graph

```js
import ZEN from "@akaoio/zen";

const zen = new ZEN({ file: "data" });  // persists to disk

// Write
zen.get("profile").put({ name: "Alice", age: 30 });

// Read once
zen.get("profile").get("name").once(function(data) {
  console.log(data);  // "Alice"
});

// Subscribe to changes
zen.get("profile").get("age").on(function(data) {
  console.log("age changed:", data);
});
```

### Key pairs and crypto

```js
const pair = await ZEN.pair();                       // secp256k1

const signed  = await ZEN.sign("hello", pair);
// signed = "<86 base62 chars>0:\"hello\""
// signed[86] → recovery bit (0 or 1)
// signed[87] → ':'
const ok      = await ZEN.verify(signed, pair.pub);  // "hello"

const enc = await ZEN.encrypt("secret", pair.pub);
// enc = "<ct_b64url>:<iv_b64url>:<s_b64url>"
const dec = await ZEN.decrypt(enc, pair.priv);       // "secret"

const bob    = await ZEN.pair();
const shared = await ZEN.secret(bob.pub, pair);      // ECDH
```

A pair object has four fields:

| Field | Length | Description |
|-------|--------|-------------|
| `pub`     | 45 chars | Compressed EC public key — 44-char base62 x-coord + `"0"`/`"1"` parity |
| `priv`    | 44 chars | Private key scalar |
| `address` | 42 chars | Ethereum-style keccak160 address (hex, checksummed) |
| `curve`   | string   | `"secp256k1"` by default |

The 45-char compressed format (`x || parity`) saves space versus the legacy 88-char uncompressed form. Legacy keys are still accepted for backward compatibility.

### Multi-curve

```js
const secp = await ZEN.pair();                        // secp256k1 (default)
const p256 = await ZEN.pair(null, { curve: "p256" }); // P-256
const evm  = await ZEN.pair(null, { format: "evm" }); // 0x EVM address
const btc  = await ZEN.pair(null, { format: "btc" }); // P2PKH mainnet
```

### Collections

```js
const list = zen.get("todos");

// Add items
list.set({ text: "buy milk" });
list.set({ text: "learn ZEN" });

// Iterate over all items in realtime
list.map().on(function(item, id) {
  console.log(id, item);
});
```

### Seed-based deterministic keys

```js
const pair1 = await ZEN.pair(null, { seed: "my-deterministic-seed" });
const pair2 = await ZEN.pair(null, { seed: "my-deterministic-seed" });
// pair1.pub === pair2.pub — always
```

### Additive key derivation

```js
// Bob derives a child key pair from his private key + shared seed
// ZEN.pair() automatically performs additive derivation when given both priv and seed
const child = await ZEN.pair(null, { priv: pair.priv, seed: "shared-namespace" });

// Alice derives the same child public key from Bob's public key + same seed
// ZEN.pair() performs public-only derivation when given pub and seed
const childPub = await ZEN.pair(null, { pub: pair.pub, seed: "shared-namespace" });
// child.pub === childPub.pub — without either party revealing private keys
```

---

## API

### Graph (instance)

```js
const zen = new ZEN(opt);

zen.get(key)        // navigate to a node
zen.put(data)       // write data
zen.on(cb)          // subscribe to realtime updates
zen.once(cb)        // read once
zen.map()           // iterate a set
zen.set(data)       // add to a set (unordered collection)
zen.back(n)         // navigate up the chain
```

### Crypto (static + instance mirror)

```js
ZEN.pair(cb, opt)                // generate key pair
ZEN.sign(data, pair)             // sign data → { m, s, v } (v = recovery bit)
ZEN.verify(data, pub)            // verify signature
ZEN.recover(sig)                 // recover signer pub from signature (no pub needed)
ZEN.encrypt(data, pair)          // encrypt
ZEN.decrypt(data, pair)          // decrypt
ZEN.secret(pub, pair)            // ECDH shared secret
ZEN.hash(data, pair, cb, opt)    // hash (SHA-256, HKDF, keccak256, or PBKDF2)
ZEN.certify(certs, policy, pair) // create a certificate
```

All static methods are also available as instance methods: `zen.pair()`, `zen.sign()`, `zen.recover()`, etc.

### Recoverable signatures

Every `ZEN.sign()` output now includes `v` — a recovery bit (0 or 1). This allows `ZEN.recover()` to reconstruct the signer's public key from the signature alone, without needing the pub key as input:

```js
const pair = await ZEN.pair();
const sig  = await ZEN.sign("hello", pair);
// sig is a JSON string: { m, s, v }  (v is new)

const pub = await ZEN.recover(sig);
console.log(pub === pair.pub);  // true
```

Works for both secp256k1 (default) and P-256. Cross-curve recovery (e.g. P-256 sig forced into secp256k1) does not throw — it silently returns a different (wrong) public key. Security depends on the `c` field in the signature being intact.

### Hash mining (Proof-of-Work)

Pass `opt.pow` to mine: the function loops with base62 nonces until the hash starts with the required prefix.

```js
// string data — nonce appended as "data:nonce"
const { hash, nonce, proof } = await ZEN.hash("mykey", null, null, {
  name: "SHA-256",
  encode: "hex",
  pow: { unit: "0", difficulty: 2 },   // hash must start with "00"
});

// function data — full control over nonce placement
const result = await ZEN.hash(
  (nonce) => `prefix:${nonce}:suffix`,
  null, null,
  { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
);
// result.proof  — the winning value (what gets written to the graph)
// result.hash   — its SHA-256 hex hash (starts with the required prefix)
// result.nonce  — the base62 nonce that produced the win
```

**PEN compatibility:** pen reads the nonce from `msg.put["^"]` (register R[7]) and the key from R[0], reconstructs `proof = key + ":" + nonce`, then SHA-256-hashes it. For string data this is identical to `ZEN.hash(result.proof, null, null, { name: "SHA-256", encode: "hex" })`. The nonce is **not** embedded in the key string — it travels as a separate wire field (`^`) so keys stay clean.

### Hash modes

`opt.name` selects the algorithm. Each mode is optimised for a different use case:

| `opt.name` | Algorithm | Salt used? | When to use |
|------------|-----------|------------|-------------|
| `"SHA-256"` | WebCrypto SHA-256 | **No** | Content-addressing, PoW mining, fast fingerprints |
| `"HKDF"` | WebCrypto HKDF | **Yes** | Deriving keys from a high-entropy seed (WebAuthn, keypair) |
| `"keccak256"` | WASM Keccak-256 | No | EVM address derivation, Ethereum-compatible hashes |
| _(default)_ | PBKDF2 100k iter | Yes | Password hashing — slow by design to stretch low-entropy secrets |

```js
// Fast content hash (salt argument ignored by SHA-256 path)
const h = await ZEN.hash("data", null, null, { name: "SHA-256", encode: "hex" });

// Key derivation from a strong seed — HKDF is ~200× faster than PBKDF2
// and correctly uses the second argument as salt
const walletKey = await ZEN.hash(seed, "wallet", null, { name: "HKDF" });
const avatarKey = await ZEN.hash(seed, "avatar", null, { name: "HKDF" });

// Default — PBKDF2, correct for passwords
const stretched = await ZEN.hash(password, salt);
```

> **Important**: The SHA-256 path ignores the `pair`/salt argument. Two calls with different salts but the same data return the same hash. Use HKDF when salt separation is required.

---

## Storage

```js
import "@akaoio/zen/lib/store";    // RAD / Radisk (default)
import "@akaoio/zen/lib/rfs";      // filesystem (Node.js)
import "@akaoio/zen/lib/rindexed"; // IndexedDB (browser)
import "@akaoio/zen/lib/opfs";     // OPFS (browser)
import "@akaoio/zen/lib/rs3";      // AWS S3
```

---

## MCP — AI integration

ZEN ships an MCP (Model Context Protocol) server that turns any IDE into a full ZEN peer. Your editor joins the P2P mesh, contributes to routing, and persists data at `~/.local/share/zen/mcp`.

**Quick start (Cursor / VSCode + forks):**

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

- Cursor: `~/.cursor/mcp.json` (or project `.cursor/mcp.json`)
- VSCode Copilot: `~/.copilot/mcp-config.json`

No ZEN installation needed — `npx` fetches `@akaoio/zen` automatically.

**Available tools (1:1 ZEN API mapping):**

| Tool | Description |
|------|-------------|
| `get` | Read a value from the graph |
| `put` | Write a value to the graph |
| `on` | Read current state of a key |
| `pair` | Generate a key pair (secp256k1 or P-256, optional seed) |
| `sign` | Sign data with a private key |
| `verify` | Verify a signed value |
| `encrypt` | Encrypt data with a public key |
| `decrypt` | Decrypt data with a private key |
| `secret` | ECDH shared secret |
| `hash` | Hash data (SHA-256, KECCAK-256, PBKDF2, HKDF, or mining) |
| `certify` | Issue a write-access certificate |
| `recover` | Recover signer public key from a signature |

See [Ch 9 — MCP](docs/ch09-mcp.md) for full details.

---



PEN is a bytecode policy engine integrated into ZEN. It compiles Zig source to WASM and runs verifiable access policies over graph writes.

```js
const soul = ZEN.pen({ val: { type: "string" }, sign: true });
// soul is a bytecode-encoded access policy string
```

```bash
npm run buildPEN     # rebuild pen.wasm from src/pen.zig
npm run testPEN      # run PEN unit tests
```

---

## WASM crypto pipeline

ZEN ships a second WASM module — `crypto.wasm` — compiled from Zig for the algorithms where native WASM compute beats JavaScript.

The principle: **use whatever is fastest**. Measure at the micro level, then decide. Hardware wins for SHA/AES/HMAC. WASM wins for algorithms the platform does not expose natively.

| Algorithm | Runtime | Why |
|-----------|---------|-----|
| SHA-256 | WebCrypto (`subtle.digest`) | Hardware SHA-NI |
| AES-GCM | WebCrypto (`subtle.encrypt`) | Hardware AES-NI |
| HMAC-SHA-256 | WebCrypto (`subtle.sign`) | Hardware SHA-NI — WASM was 7× slower |
| secp256k1 point multiply | V8 BigInt (native C++) | V8 JIT > WASM32 emulated wide mul (8–12×) |
| P-256 point multiply | V8 BigInt (native C++) | same reason |
| **Keccak-256** | **WASM** (`crypto.wasm`) | 25-lane u64, no WebCrypto equivalent — **5× faster** |
| **RIPEMD-160** | **WASM** (`crypto.wasm`) | No WebCrypto equivalent — **1.6M ops/s** |
| **base62** encode/decode | **WASM** (`crypto.wasm`) | Faster than BigInt for encoding |

```bash
npm run buildCrypto  # rebuild crypto.wasm from src/crypto.zig
```

---

## Benchmarks

ZEN ships a micro-benchmark harness in `test/bench/` for data-driven optimization. Each benchmark suite runs with configurable warmup and iteration counts and outputs human-readable color output.

```bash
npm run bench          # run all suites
npm run bench:hash     # hash algorithms (SHA-256, keccak256, ripemd160, DJB2)
npm run bench:json     # JSON.parse / parseAsync / YSON chunk parser
npm run bench:dup      # dedup pipeline (dup.check + dup.track)
npm run bench:radix    # Radix tree vs native Map
npm run bench:ham      # HAM CRDT comparisons, State(), put() e2e
npm run bench:sign     # ZEN.pair / sign / verify / encrypt
npm run bench:base62   # base62 WASM vs base64 baseline
```

Selected baselines (Node.js, 5000 iters):

| Suite | Operation | Throughput |
|-------|-----------|-----------|
| hash | keccak256 WASM 4B | 275K ops/s |
| hash | ripemd160 WASM 4B | 1.6M ops/s |
| hash | String.hash DJB2 | 3.6M ops/s |
| dup | full pipeline (Map) | 1.13M ops/s |
| dup | check missing (Map) | 4.54M ops/s |

---

## Build and test

```bash
npm test             # build zen.js + run full suite (PEN + ZEN unit + core)
npm run testZEN      # build + ZEN unit tests only
npm run testPEN      # build + PEN unit tests only
npm run buildZEN     # buildPEN + buildCrypto + bundle + minify
npm run buildRelease # buildZEN + uglify all lib adapters
npm start            # start example relay (examples/zen-http.js)
```

Current baseline: **422 passing, 10 pending** (across PEN unit + ZEN unit + core suites).

---

## Architecture

```
src/                  — runtime source (source of truth)
  shim.js             — setTimeout.turn, setTimeout.each, String.hash DJB2
  dup.js              — message deduplication (called on every message)
  state.js            — CRDT vector clock (HAM)
  valid.js            — value validation
  onto.js             — doubly-linked list event emitter
  root.js             — ZEN constructor, universe() pipeline
  index.js            — main entry point
  core.js             — graph operations
  mesh.js             — P2P networking, JSON parse, batching
  websocket.js        — WebSocket transport layer
  chain.js            — method chaining logic
  get.js              — graph navigation
  put.js              — graph writes
  on.js               — event subscriptions
  map.js              — map/reduce operations
  set.js              — unordered collections
  back.js             — chain traversal
  
  # Crypto (formerly SEA, now integrated into ZEN)
  pair.js             — key pair generation
  sign.js             — signing
  verify.js           — signature verification
  encrypt.js          — encryption
  decrypt.js          — decryption
  secret.js           — ECDH shared secrets
  hash.js             — hashing (SHA-256, HKDF, keccak256, PBKDF2)
  certify.js          — certificates
  recover.js          — recover signer pub from signature (no pub input needed)
  aeskey.js           — AES key derivation
  
  # Curves & formats
  curves/             — ECC implementations
    secp256k1.js      — secp256k1 curve (Bitcoin/Ethereum)
    secp256k1.zig     — Zig implementation
    p256.js           — P-256/secp256r1 curve
    p256.zig          — Zig implementation
    utils.js          — curve utilities
    utils.zig         — Zig utilities
  curves.js           — curve registry
  format.js           — key format conversions (base62, EVM, BTC)
  keyid.js            — key identifiers
  
  # WASM-accelerated crypto
  keccak256.js        — Keccak-256 wrapper
  keccak256.zig       — Zig implementation
  ripemd160.js        — RIPEMD-160 wrapper
  ripemd160.zig       — Zig implementation
  base62.js           — base62 encode/decode wrapper
  base62.zig          — Zig implementation
  sha256.js           — SHA-256 wrapper
  sha256.zig          — Zig implementation
  hmac_sha256.zig     — HMAC-SHA-256 implementation
  crypto.js — lazy WASM loader + typed wrappers
  crypto.zig     — WASM crypto module source
  wasm.zig            — WASM utilities
  
  # Policy engine
  pen.js              — PEN policy VM
  pen.zig             — PEN implementation in Zig
  
  # Utilities
  json.js             — JSON parsing & YSON
  buffer.js           — buffer utilities
  array.js            — array utilities
  ask.js              — query handling
  book.js             — peer registry
  graph.js            — graph utilities
  locstore.js         — local storage adapter
  runtime.js          — runtime detection
  security.js         — security utilities
  settings.js         — configuration

zen.js               — bundled browser/Node.js artifact
zen.min.js           — minified
crypto.wasm          — 66KB WASM crypto module
pen.wasm             — ~27KB WASM policy engine

lib/                 — storage adapters, extensions, build scripts
  server.js          — ZEN relay / server identity
  builder/zen.js       — bundle script
  builder/pen.js       — PEN WASM build
  builder/crypto.js    — crypto WASM build
  
  # Storage adapters
  store.js           — storage abstraction
  radisk.js          — RAD/Radisk (default)
  radix.js           — Radix tree implementation
  rfs.js             — filesystem (Node.js)
  rindexed.js        — IndexedDB (browser)
  opfs.js            — OPFS (browser)
  rs3.js             — AWS S3
  memdisk.js         — in-memory storage
  
  # Extensions & utilities
  lib/axe.js         — automatic peering / DHT
  mcp.js             — MCP stdio server (IDE-as-ZEN-peer, AI integration)
  webrtc.js          — WebRTC transport
  promise.js         — Promise API
  then.js            — Promise chaining
  yson.js            — YSON parser
  verify.js          — verification utilities
  and 100+ other adapters, utilities, middleware...

test/
  bench/             — micro-benchmark harness + 7 suites
  zen/               — ZEN unit tests (instance, crypto, multicurve, certify)
  pen.js             — PEN unit tests
  zen.js             — core graph integration tests
  rad/               — RAD storage tests
```

---

## Lineage

```
amark/gun
  → mimiza/gun
    → akaoio/gun
      → ZEN (Zen Entropy Network)
```

ZEN keeps the graph and sync inheritance but has a separate runtime identity, build system, and architectural direction. It is not a thin rebrand — the source has been materially rewritten.
