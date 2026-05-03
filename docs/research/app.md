# ZEN App Primitives & ZACP — Design Spec

> Cập nhật lần cuối: 2026-05-03

---

## 1. Primitive Overview

| Soul type | Pattern | Ý nghĩa |
|-----------|---------|---------|
| User namespace | `~<pub>` | Owner-only write, signed data |
| Content address | `#<hash>` | Immutable, self-authenticating |
| PEN policy | `!<bytecode>` | Write conditions enforced by WASM VM on every peer |
| Open node | `<any>` | Không kiểm soát |

Crypto API (v1.0.9):
- `ZEN.pair(cb, {curve?, seed?, pub?})` → `{curve, pub, priv, address}`
- `ZEN.sign/verify` — ECDSA compact wire format
- `ZEN.encrypt(data, pub)` / `ZEN.decrypt(enc, pair)` — AES-GCM + ECDH
- `ZEN.secret(peer_pub, pair)` → base62 ECDH shared secret
- `ZEN.certify(recipient_pub, policy, issuer_pair, cb, {expiry?})` → compact signed string
- `ZEN.hash(data, cb, cb, {name?, encode?, pow?})`
- `ZEN.pen(spec)` → `"!" + pack(bytecode)` — compile policy spec to soul prefix
- `ZEN.candle({seg, sep, size, back, fwd})` → key expression (dùng trong `spec.key`)

---

## 2. Architecture Layers

```
┌─────────────────────────────────────────────────────┐
│  lib/mcp.js — thin MCP tool wrapper (stdio JSON-RPC) │
│  Chỉ: input validation, key resolution, tool dispatch │
├─────────────────────────────────────────────────────┤
│  lib/protocol.js — ZACP business logic (NEW)         │
│  project/channel/inbox/dm helpers                    │
├─────────────────────────────────────────────────────┤
│  lib/identity.js — agent identity (enhanced)         │
│  hardware fingerprint + seed-based portable mode     │
├─────────────────────────────────────────────────────┤
│  zen.js (src/) — ZEN graph + crypto primitives       │
│  Không thay đổi core                                 │
└─────────────────────────────────────────────────────┘
```

**Nguyên tắc**: Logic ở đúng layer. `lib/mcp.js` không chứa business logic — chỉ delegate sang `lib/protocol.js`. `src/` không thay đổi.

---

## 3. Inbox — P2P Messaging không cần Certify

### Vấn đề key overflow

Soul đơn giản `@<pub>` với SGN: peer độc hại flood bằng keys có timestamp cũ — graph chết khi key space phát nổ. Convention-only không đủ vì peer khác propagate mọi write hợp lệ về mặt CRDT.

**Giải pháp**: PEN soul với candle constraint, enforced bởi WASM VM trên mọi peer.

### Soul Spec

```js
// Standard inbox candle (protocol constant)
// ZEN.candle() default sep là "_" — phải truyền rõ sep: ":"
const INBOX_CANDLE = { seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 }

// aliceInboxSoul = "!<bytecode>" — deterministic từ alice.pub
// Write soul thực tế: aliceInboxSoul + "/" + alice.pub
// penStage: pathpart = soul.slice(firstSlash + 1) = alice.pub → R[6]
// Bytecode: AND( EQ(R[6], alice.pub), candle_expr(R[0]) ) + SGN tail
const aliceInboxSoul = ZEN.pen({
  path: alice.pub,
  key: ZEN.candle(INBOX_CANDLE),
  sign: true,
})
```

### Key format

```
key = <candle_segment>:<sha256_of_content_prefix>

candle_segment = Math.floor(Date.now() / 3600000)
```

PEN validate: `tonum(seg(key, ":", 0))` ∈ `[current - back, current + fwd]`

### Usage

```js
// Bob gửi cho Alice
const soul    = ZEN.pen({ path: alice.pub, key: ZEN.candle(INBOX_CANDLE), sign: true })
const enc     = await ZEN.encrypt(message, alice.pub)
const seg     = Math.floor(Date.now() / INBOX_CANDLE.size)
const keyHash = (await ZEN.hash(enc, null, null, { name: "SHA-256", encode: "base62" })).slice(0, 16)
const key     = seg + ":" + keyHash
zen.get(soul + "/" + alice.pub).get(key).put(enc, null, { authenticator: bob_pair })

// Alice đọc
zen.get(soul + "/" + alice.pub).map().on(async (val, key) => {
  const plaintext = await ZEN.decrypt(val, alice_pair)
  const senderPub = await ZEN.recover(val)   // nếu val là signed blob
})
```

### Anti-spam levels

| Level | Spec thêm | Chi phí spammer |
|-------|-----------|----------------|
| 1 (default) | `sign: true` | Cần keypair, lộ danh tính |
| 2 | + `pow: { unit: "0", difficulty: 1 }` | ~50ms/msg |
| 3 | + `pow: { unit: "0", difficulty: 2 }` | ~500ms/msg |

Alice publish preferred policy tại `~<alice.pub>/@` để peer biết level nào.

**Lưu ý sender privacy**: SGN lưu signature trong graph → `ZEN.recover(val)` lấy được `bob.pub`. Metadata "bob wrote to alice" là public. Nếu cần ẩn danh: thay SGN bằng PoW.

---

## 4. ZACP — ZEN Agent Collaboration Protocol

### 4.1 Entity Model

```
Agent      = MCP instance. Identity = keypair (pub public, priv ẩn trong MCP)
Project    = Namespace chung. owner + members + roles
Channel    = Message stream trong project
Role       = owner | admin | member | observer
```

### 4.2 Soul Naming

```
# Project data — owner-only (user namespace, security.js enforce)
~<owner_pub>/proj/<proj_id>/meta
~<owner_pub>/proj/<proj_id>/roles                           → { <member_pub>: role }
~<owner_pub>/proj/<proj_id>/chan/<chan_id>/meta
~<owner_pub>/proj/<proj_id>/chan/<chan_id>/keys/<member_pub> → wrapped channel key

# Channel messages — PEN soul, unique per channel (path hardcoded in bytecode)
!<pen_bytecode>/proj/<proj_id>/chan/<chan_id>
  key = <candle_seg>:<hash>
  val = { a: author_pub, m: <encrypted_blob> }

# DM 1:1 — PEN soul, unique per recipient
!<pen_bytecode>/dm/<recipient_pub>
  key = <candle_seg>:<hash>
  val = { a: sender_pub, m: <encrypted_blob> }
```

### 4.3 PEN Soul per Channel/DM

```js
// Channel soul — cert-gated, unique per (proj_id, chan_id)
// path hardcode trong bytecode → bytecode khác nhau → soul khác nhau
const chanSoul = ZEN.pen({
  path: "proj/" + proj_id + "/chan/" + chan_id,
  key:  ZEN.candle({ seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 }),
  sign: true,
  cert: owner_pub,
  // pow: { unit: "0", difficulty: 1 }  // bật cho public channel
})
// Write: zen.get(chanSoul + "/proj/" + proj_id + "/chan/" + chan_id).get(key).put(...)
// R[6] = "proj/<proj_id>/chan/<chan_id>" ← matches EQ in bytecode ✓

// DM soul — SGN + PoW, unique per recipient
const dmSoul = ZEN.pen({
  path: "dm/" + recipient_pub,
  key:  ZEN.candle({ seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 }),
  sign: true,
  pow: { unit: "0", difficulty: 1 },  // khuyến nghị — không có cert → cần PoW chặn flood
})
// Write: zen.get(dmSoul + "/dm/" + recipient_pub).get(key).put(...)
// R[6] = "dm/<recipient_pub>" ← matches EQ in bytecode ✓
```

### 4.4 Group Encryption

**Yêu cầu**: Nội dung channel private chỉ member decrypt được, dù mọi peer lưu graph.

**`owner_priv` không bao giờ lộ ra ngoài MCP process.** Agent chỉ gọi tool `createChannel(proj_id, chan_id)` — không biết và không cần biết `owner_priv`. Code dưới đây chạy bên trong `lib/protocol.js`, truy cập `pairStore` nội bộ của MCP:

```js
// Trong lib/protocol.js — chạy bên trong MCP, không phải agent code
// owner_pair được lấy từ pairStore[pairId] — không lộ ra ngoài
async function createChannel(proj_id, chan_id, version, owner_pair, members, zen) {
  // ❌ SAI: seed public → chan_pair.priv tính được bởi bất kỳ ai
  // const chan_pair = await ZEN.pair(null, { seed: proj_id + "/" + chan_id + "/v" + version })

  // ✓ ĐÚNG: seed chỉ owner tính được vì owner_pair.priv là bí mật nằm trong pairStore
  const chan_seed = await ZEN.hash(
    owner_pair.priv + proj_id + chan_id + "v" + version, null, null,
    { name: "SHA-256", encode: "base62" }
  )
  const chan_pair = await ZEN.pair(null, { seed: chan_seed })
  // ...
}
```

**Key distribution (owner → member)**:
```js
// ECDH: shared chỉ tính được bởi người có member.priv hoặc chan_pair.priv (secret)
const shared  = await ZEN.secret(member.pub, chan_pair)
const wrapped = await ZEN.encrypt(chan_pair.priv, { priv: shared })
// Lưu: ~<owner_pub>/proj/<pid>/chan/<cid>/keys/<member.pub> = wrapped
```

**Member giải mã**:
```js
// ECDH ngược = cùng shared secret
const shared    = await ZEN.secret(chan_pair.pub, member_pair)
const chan_priv = await ZEN.decrypt(wrapped, { priv: shared })
const plaintext = await ZEN.decrypt(msg.m, { priv: chan_priv })
```

### 4.5 DM Encryption

```js
// Alice → Bob (tính trong MCP của alice, priv không lộ ra ngoài)
const shared = await ZEN.secret(bob.pub, alice_pair)
const enc    = await ZEN.encrypt(message, { priv: shared })

// Bob giải mã (tính trong MCP của bob)
const shared = await ZEN.secret(alice.pub, bob_pair)
const msg    = await ZEN.decrypt(enc, { priv: shared })
```

### 4.6 Cert & Key Rotation

**Cert expiry thay revocation list**: `$vfy` kiểm tra `cert.e` nhưng không check revocation list ở protocol level. Giải pháp sạch: cấp cert với expiry ngắn — khi kick không renew, cert tự hết hạn.

```js
// Cấp cert với expiry 30 ngày
const cert = await ZEN.certify(
  member_pub,
  "proj/" + proj_id + "/chan/" + chan_id,   // write policy khớp pathpart
  owner_pair, null,
  { expiry: Date.now() + 30 * 24 * 3600000 }
)
```

**Key rotation khi kick** (forward secrecy từ thời điểm kick):
```
1. Tạo chan_seed_v(N+1) = hash(owner_priv + proj_id + chan_id + "v" + (N+1))
2. Tạo chan_pair từ seed mới
3. Re-encrypt chan_pair.priv mới cho các member còn lại
4. Ghi chan meta: { pub: chan_pair.pub, version: N+1, since: Date.now() }
5. Không renew cert cho kicked member — cert cũ hết hạn tự nhiên
```

Kicked member đọc được message cũ (đã biết key cũ — intentional, không thể ngăn). Không đọc/ghi được message mới.

### 4.7 Agent Identity

| Mode | Source | Portable | Config |
|------|--------|----------|--------|
| Hardware (default) | `lib/identity.js` fingerprint | Không | Zero-config |
| Seed-based | Passphrase / BIP39 | Có | Seed từ env/file/stdin |

```bash
ZEN_IDENTITY_SEED="passphrase"   node lib/mcp.js
node lib/mcp.js --identity-file  ~/.zen/identity   # chmod 600, ngoài project dir
```

Logic seed loading thuộc `lib/identity.js` — không phải `lib/mcp.js`. MCP chỉ gọi `getOrCreateIdentity(opt)`.

### 4.8 Known Tradeoffs

| Issue | Decision | Ghi chú |
|-------|----------|---------|
| Sender metadata public (SGN) | Intentional | Dùng PoW thay SGN nếu cần ẩn danh |
| Member list visible | Acceptable | Keys được encrypt riêng từng member |
| Old messages readable after kick | Intentional | Forward secrecy không retroactive |
| DM flood (no cert) | PoW mitigates | `difficulty: 1` recommended |

---

## 5. Code Placement

| Code | Nơi | Lý do |
|------|-----|-------|
| `ZEN.pen/candle/certify/secret` | `src/` | Đã có, không thay đổi |
| `INBOX_CANDLE` constant | `lib/protocol.js` | Shared constant |
| Soul name computation | `lib/protocol.js` | Reusable, không phụ thuộc MCP |
| Channel key wrap/unwrap | `lib/protocol.js` | Crypto logic, không phải transport |
| Project/channel/dm helpers | `lib/protocol.js` | Business logic |
| Seed-based identity | `lib/identity.js` | Identity concern |
| MCP tool handlers | `lib/mcp.js` | Thin wrapper, delegate sang protocol.js |

**lib/protocol.js** exports cần có:
```js
export function inboxSoul(pub, opts?)          // → soul string
export function chanSoul(proj_id, chan_id, owner_pub, opts?)  // → soul string
export function dmSoul(recipient_pub, opts?)   // → soul string
export async function wrapChanKey(chan_pair, members)   // → { [member_pub]: wrapped }
export async function unwrapChanKey(chan_pair_pub, owner_pub, proj_id, chan_id, version, member_pair, zen)
export async function chanSeed(owner_priv, proj_id, chan_id, version)  // → base62 seed
```

---

## 6. Roadmap

### MCP Priorities (2026-05-03)

**Must have**

- [x] `protocol.read_inbox` — expose inbox read path through MCP
- [x] `protocol.read_channel` — expose channel read path through MCP
- [x] `protocol.send_inbox` — inbox round-trip testability / usability through MCP
- [x] `initialize.serverInfo.version` must reflect package version, not hardcoded stale value
- [x] Project meta / roles ops (`proj/<id>/meta`, `proj/<id>/roles`) through MCP protocol surface
- [x] Stable read result schema across inbox / channel / DM (`{ key, plaintext, sender_pub }`) — `readInbox`, `readChannel`, `readDMs` all return this shape
- [ ] Graph enumeration surface designed for large datasets
- [ ] Graph subscribe / polling surface

**Later**

- [ ] Identity seed via stdin in `lib/identity.js`
- [ ] BIP39 mnemonic support in `lib/identity.js`
- [ ] Cert renewal / lifecycle helpers above raw `certify`
- [ ] SSE push subscribe mode after polling mode is stable
- [ ] IPFS adapter and longer-term transport/storage work

**graph.list note**

`graph.list` is intentionally deferred. A naive implementation that reads the whole soul every call is not acceptable. ZEN already has chunked / partial read behavior through graph traversal and LEX-oriented access patterns, so any list API should be designed around:

- cursor / continuation tokens
- bounded result windows (`limit`)
- prefix / range filters
- compatibility with partial / incremental loading rather than full materialization

`Book` from `src/book.js` is relevant here, but only as an internal building block. It already implements a paged in-memory key/value index with page splitting, exact lookup, and bounded page reads, so it can help as:

- an in-memory page buffer for one `graph.list` window
- a continuation/cursor backing structure after data has already been narrowed by range/prefix
- a local cache for incremental reads over large result sets

`Book` should not be the public `graph.list` contract by itself:

- it does not define network/storage traversal semantics
- current range/prefix matching belongs more naturally to LEX/Radix-style access patterns
- `Book` still has TODOs around non-exact / radix-like lookup behavior

Until that design is clear, MCP should not ship a misleading `graph.list` that behaves like an eager full scan.

```
v1.1.x:
  [x] lib/protocol.js — ZACP helpers (inboxSoul, chanSoul, dmSoul, wrapChanKey, unwrapChanKey, chanSeed)
  [x] lib/identity.js — seed-based mode (ZEN_IDENTITY_SEED / --identity-file / stdin)
  [x] lib/mcp.js — add "protocol" tool delegating to lib/protocol.js
  [x] Test suite cho inbox soul + ZACP group encryption (29 tests passing)
  [x] src/meta.js — Zen.chain.meta(cb) / await .meta() — signature metadata API

v1.2.x:
  [ ] Cert expiry support trong MCP certify tool
  [ ] Channel key rotation trong lib/protocol.js
  [ ] End-to-end messaging tests: sendInbox→readInbox, sendToChannel→readChannel, sendDM→readDMs round-trip
  [ ] MCP list tool (enumerate keys trong một soul, cursor/limit/prefix, xem graph.list note)
  [ ] MCP subscribe tool — polling mode trước (stdio-compatible)
  [ ] MCP SSE transport — flag `--sse` trong `lib/mcp.js`, cùng process với stdio mode (xem §7 Streaming)

Long-term:
  [ ] IPFS storage adapter (pattern như rs3.js)
  [ ] On-chain bridge (secp256k1 = ETH key, sign ZEN data = sign ETH tx)
  [ ] BIP39 mnemonic support trong lib/identity.js
  [ ] Identity seed via stdin trong lib/identity.js
```

---

## 7. Streaming Architecture

### MCP transport constraints

**Stdio (hiện tại)** — request/response thuần, không có server push:
- Polling: client gọi tool định kỳ, nhận snapshot
- Long-poll: tool block đến timeout hoặc limit đạt, trả batch

**HTTP + SSE transport** (MCP spec 2024-11-05) — server push:
- Server giữ ZEN `.on()` subscription sống
- Mỗi khi ZEN fire event → push SSE frame xuống client
- - `lib/mcp-sse.js` **không tồn tại** — SSE mode được tích hợp trong `lib/mcp.js` qua flag `--sse`:

```bash
node lib/mcp.js           # stdio mode (mặc định, dùng với Claude Desktop)
node lib/mcp.js --sse     # HTTP+SSE mode (port: ZEN_MCP_PORT hoặc 8421)
```

Cùng một process, cùng `pairStore`, cùng `zen` instance:

| Kiến trúc | Transport | Push | Storage | Phù hợp |
|-----------|-----------|------|---------|----------|
| Polling | stdio | ❌ | Bình thường | Inbox batch read |
| Long-poll | stdio | ❌ | Bình thường | Low-freq notification |
| SSE push | HTTP+SSE | ✅ | Bình thường | Chat, live feed |
| SSE ephemeral | HTTP+SSE | ✅ | **Không** | Presence, typing indicator |

### Ephemeral ZEN instance

ZEN chạy hoàn toàn in-memory — không radisk, không localStorage:

```js
// lib/mcp.js — cùng file, detect mode qua --sse flag
const SSE_MODE = process.argv.includes("--sse");
const zen          = new ZEN({ file: "radata", peers: [...] })   // persisted
const ephemeralZen = new ZEN({ localStorage: false })             // RAM only, mất khi restart
// subscriptionStore chỉ active trong SSE mode
const subscriptionStore = new Map(); // Map<stream_id, { off, res }>
```

Kết hợp PEN candle cực hẹp để tự expire write policy:
```js
// Presence: candle 1 phút, không accept message cũ
const presenceSoul = ZEN.pen({
  path: agent_pub,
  key: ZEN.candle({ seg: 0, sep: ":", size: 60000, back: 0, fwd: 0 }),
  sign: true,
})
// Sau 1 phút, PEN từ chối mọi write mới với key cũ
// Node still has data in graph nhưng không ai write được nữa
```

### SSE subscribe/unsubscribe pattern

```
┌─────────────────────────────────────────────────────────┐
│  MCP Client (LLM agent)                                  │
│  tool: subscribe({ soul, pairId? }) → { stream_id }     │
│  GET /sse?stream=<stream_id>  — HTTP SSE connection      │
│  tool: unsubscribe({ stream_id })                        │
└───────────────┬─────────────────────────────────────────┘
                │ SSE frames: data: { key, val, soul }\n\n
┌───────────────▼─────────────────────────────────────────┐
│  lib/mcp.js (--sse flag)                                 │
│  subscriptionStore: Map<stream_id, { off, res }>         │
│  on subscribe: zen.get(soul).map().on(cb) → push SSE     │
│  on disconnect: off() cleanup                            │
└───────────────┬─────────────────────────────────────────┘
                │ ZEN .on() — realtime graph events
┌───────────────▼─────────────────────────────────────────┐
│  ZEN graph (persisted hoặc ephemeral tuỳ soul)           │
└─────────────────────────────────────────────────────────┘
```

### Routing: persisted vs ephemeral

| Soul prefix | ZEN instance | Lý do |
|-------------|-------------|-------|
| `!<pen>/<pub>` inbox/channel | persisted | Message phải tồn tại qua restart |
| `~<pub>/...` | persisted | Owner-signed data |
| `presence/<pub>` | ephemeral | Không cần lưu, mất là được |
| `typing/<chan>` | ephemeral | Indicator tạm thời |
| `cursor/<room>` | ephemeral | Live cursor position |

Logic routing thuộc `lib/mcp.js` (SSE path) — kiểm tra prefix soul để chọn đúng instance.

### Tradeoffs

| Issue | Decision |
|-------|----------|
| stdio vs SSE | Cùng `lib/mcp.js`, detect qua `--sse` flag. Stdio mode không bị ảnh hưởng. |
| Ephemeral data mất khi restart | Intentional — đó là mục đích. Client phải re-subscribe. |
| SSE connection limit | Mỗi stream_id = 1 HTTP connection. Scale theo process, không phải thread. |
| Decryption trên SSE | SSE chỉ push ciphertext. Client tự decrypt — priv không đi qua network. |

---

### Đã hoàn thành (v1.0.9)

```
[x] Compact wire format — signed/encrypted strings không wrap JSON
[x] Pair schema: {curve, pub, priv, address} — không còn epub/epriv
[x] EVM address tích hợp vào pair (pair.address)
[x] Base62 encrypt format (thay base64url)
[x] Legacy GUN backward compat removed
[x] Certify trả về compact signed string trực tiếp
[x] PEN WASM VM + candle helper sẵn sàng
[x] Cross-format encrypt/decrypt (zen ↔ evm)
```

---

*Xem thêm: [ch03-crypto.md](../ch03-crypto.md) · [ch04-authenticated-data.md](../ch04-authenticated-data.md) · [ch07-pen.md](../ch07-pen.md) · [ch09-mcp.md](../ch09-mcp.md)*
