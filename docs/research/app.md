# ZEN App Primitives & ZACP — Design Spec

> Cập nhật lần cuối: 2026-05-04

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
  [x] Cert expiry support trong MCP certify tool — certify nhận plain string / JSON string / object policy; expiry embeds cert.e; 6 tests
  [x] Channel key rotation trong lib/protocol.js — rotateChannel / kick op
  [x] End-to-end messaging tests: sendInbox→readInbox, sendToChannel→readChannel, sendDM→readDMs round-trip
  [x] MCP subscribe tool — graph op:"subscribe" / op:"unsubscribe"; server push via notifications/message; 6 tests
  [x] DAM multi-hop relay — dam:"relay"; mesh.relay(), mesh.route(), mesh.onRelay(); XOR routing; TTL loop prevention; 22 tests (257 total)
  [x] ZEN-native relay RPC transport — ZenMcpClient + startRelayMode() trong mcp.js; ECDH encrypted relay; discover via ~pub/mcp/info; 15 tests
  [ ] MCP list tool (enumerate keys trong một soul, cursor/limit/prefix, xem graph.list note)

Long-term:
  [ ] IPFS storage adapter (pattern như rs3.js)
  [ ] On-chain bridge (secp256k1 = ETH key, sign ZEN data = sign ETH tx)
  [ ] BIP39 mnemonic support trong lib/identity.js
  [ ] Identity seed via stdin trong lib/identity.js
```

---

## 7. ZEN-native MCP Transport

### MCP server là một ZEN relay

`lib/mcp.js` **là một ZEN relay** — nó chạy một ZEN instance đầy đủ (`file: "radata"`, `peers: [...]`), sync data với network, lưu trữ local. Điểm khác biệt duy nhất so với `lib/server.js`: nó còn expose MCP interface cho LLM agents.

```
lib/server.js  =  ZEN relay thuần
lib/mcp.js     =  ZEN relay  +  MCP interface
```

Đặc điểm của relay này: IP động, sau NAT, không có domain, có thể offline lúc bất kỳ. Đây là điều kiện bình thường của mọi ZEN peer — không phải ngoại lệ.

Agent (nếu dùng ZEN SDK) cũng là một peer như vậy. Hai peer liên lạc qua graph — không cần biết IP nhau, không cần HTTP.

### Tại sao HTTP transport không phù hợp

Mọi HTTP transport (Streamable HTTP, HTTP+SSE) đều giả định **server có IP/domain ổn định và port mở**. Thực tế của ZEN peers:

| Thực tế | HTTP transport | ZEN-native |
|---------|---------------|-----------|
| MCP server sau NAT | ❌ không reach được | ✅ connect outbound |
| IP động | ❌ không có địa chỉ | ✅ dùng pub key làm địa chỉ |
| Không có domain | ❌ cần domain/IP | ✅ relay xử lý routing |
| Agent cũng sau NAT | ❌ cả hai cần port mở | ✅ cả hai chỉ cần outbound |
| Phân tán | ❌ tạo hub trung tâm | ✅ không có điểm trung tâm |

**Kết luận**: `--sse` / Streamable HTTP giải quyết sai vấn đề. ZEN đã có transport rồi — đó chính là ZEN graph.

### ZEN-native RPC: graph là transport

Cả agent lẫn MCP server đều là ZEN peers — connect **outbound** tới relay network. Không cần inbound connection, không cần IP ổn định, không cần domain.

```
LLM Agent peer (IP động, sau NAT)   MCP server peer (IP động, sau NAT)
  ZEN instance + agent logic          ZEN instance (relay) + MCP interface
    │  WebSocket outbound                │  WebSocket outbound
    ▼                                    ▼
ZEN relay A ◄──────────────────► ZEN relay B ◄──► ZEN relay C
    │                   sync (P2P)                      │
    └───────────────────────────────────────────────────┘
                         ZEN P2P graph (shared state)
```

**Địa chỉ là pub key**, không phải IP:
- MCP server được identify bằng `server_pub` (pair của ZEN instance đó)
- Agent tìm server bằng pub key — không cần biết IP, không cần DNS

### Giao thức RPC qua ZEN graph

#### Request soul

Agent write request vào soul thuộc về chính nó, tagged bằng server pub:

```
Soul: ~<agent_pub>/mcp/<server_pub>/<nonce>
Data: { method, params }   ← ký bằng agent pair
```

Chỉ agent có thể write vào `~<agent_pub>/...` (ZEN security). Server sub `.on()` vào soul này.

#### Response soul

Server write response vào soul thuộc về chính nó, tagged bằng agent pub:

```
Soul: ~<server_pub>/mcp/<agent_pub>/<nonce>
Data: { result } hoặc { error }   ← ký bằng server pair
```

Chỉ server có thể write vào `~<server_pub>/...`. Agent sub `.on()` để nhận kết quả.

#### Flow

```
Agent                           ZEN graph                    MCP server (lib/mcp.js)
  │                                  │                              │
  │  zen.get(reqSoul).put(req)        │                              │
  │  ──────────────────────────────► │                              │
  │                                  │  .on(reqSoul) fires          │
  │                                  │ ──────────────────────────► │
  │                                  │                              │  xử lý tool call
  │                                  │  zen.get(resSoul).put(res)   │
  │                                  │ ◄────────────────────────── │
  │  .on(resSoul) fires              │                              │
  │ ◄────────────────────────────── │                              │
  │  nhận kết quả                    │                              │
```

#### Timeout tự động qua PEN candle

```js
// Request tự expire sau 30 giây — không cần cleanup thủ công
const reqSoul = ZEN.pen({
  soul: `~${agent_pub}/mcp/${server_pub}`,
  key:  ZEN.candle({ size: 30000, back: 0, fwd: 0 }),
  sign: true,
})
```

PEN từ chối write vào key cũ sau 30s — request cũ tự nhiên không thể replay.

### Streaming response

#### Giới hạn cơ bản: ZEN graph không có true delete

ZEN là CRDT — "xoá" chỉ là tombstone (null + HAM state) tồn tại mãi mãi. Các cơ chế hiện có đều chỉ là local filters:

| Cơ chế | Hoạt động | Giới hạn |
|--------|-----------|---------|
| `forget.js` | Local instance không lưu soul đã đánh dấu | Relay trung gian vẫn lưu |
| `erase.js` | Khi nhận null → xoá khỏi in-memory local | Tombstone vẫn sync đi khắp network |
| PEN candle | Block write mới sau time window | Không xoá data đã tồn tại |
| `dam` messages | Non-persistent peer signaling | Chỉ direct peer, không multi-hop relay |

Viết nhiều chunks vào graph = tích luỹ permanent storage trên relay. Không có network-wide ephemeral messaging trong ZEN hiện tại.

#### stdio: streaming qua MCP notifications (không dùng graph)

MCP spec cho phép server push `notifications/` không đồng bộ trước khi trả response cuối. Server sub ZEN `.on()` → mỗi event đẩy thẳng ra stdout — **không có graph write nào**:

```
Agent (Claude Desktop)            lib/mcp.js (stdout)
  │  tools/call "subscribe"            │
  │ ──────────────────────────────►   │
  │                                    │  zen.get(soul).map().on(cb)
  │ ◄──────────────────────────────   │  notifications/message {key, val}
  │ ◄──────────────────────────────   │  notifications/message {key, val}
  │  tools/call "unsubscribe"          │
  │ ──────────────────────────────►   │  off() → stop
  │ ◄──────────────────────────────   │  {ok:true}
```

Ephemeral hoàn toàn. Không tốn storage. **Giới hạn**: phụ thuộc vào MCP host có xử lý unsolicited notifications không — Claude Desktop làm được, một số client khác chưa.

#### ZEN-native RPC (cross-machine): không có streaming hiệu quả

Mọi data write vào graph đều persist. Lựa chọn thực tế:

**Single write** — serialize toàn bộ kết quả, một lần duy nhất:
```js
const result = await processToolCall(method, params);
zen.get(resSoul).put(JSON.stringify(result), null, { authenticator: serverPair });
```
`opt.max` ≈ 90MB — đủ cho hầu hết tool responses.

**Pagination** — cursor-based, mỗi page là một RPC round-trip:
```
Agent              MCP server
  req { page:0 } ──►
               ◄── res { data:[...], next:1 }
  req { page:1 } ──►
               ◄── res { data:[...], next:null }
```

**Giảm thiểu relay pollution**: nonce souls (`~pub/mcp/counterpart/nonce`) không bao giờ reuse → relay tự evict theo `opt.max`. Dùng `forget.js` trên cả hai đầu để không tích luỹ local.

#### Tóm tắt theo transport

| Transport | Streaming | Cơ chế | Storage cost |
|-----------|-----------|--------|-------------|
| stdio | ✅ có | MCP `notifications/` → stdout | Không có |
| ZEN-native RPC | ❌ không | Single write hoặc pagination | Persist trên relay |

#### DAM multi-hop relay (đã triển khai — v1.2.x)

`src/mesh.js` nay có `dam: "relay"` — ephemeral message routing không lưu vào graph:

```
Message: { dam: "relay", to: target_pub, from: sender_pub, ttl: 5, data: <payload> }
```

**Cơ chế routing**:
1. Relay nhận `dam: "relay"` → giảm TTL, drop nếu ≤ 0
2. Dedup bằng `msg["#"]` (cùng ID không xử lý hai lần → loop prevention tự nhiên)
3. Nếu `to === opt.pub` → deliver local qua `mesh.onRelay(fn)` callback
4. Nếu `to` là connected peer trực tiếp → forward thẳng
5. Fallback: `mesh.route(to, skip)` — tìm peer có XOR distance nhỏ nhất đến `to`, skip sender

**API**:
```js
// Gửi ephemeral message
mesh.relay(targetPub, data, { ttl: 5 })

// Nhận (trên node là target)
const off = mesh.onRelay(function({ from, data }) { ... })
off() // hủy đăng ký

// Routing utils
mesh.route(targetPub, skipPeer)  // → closest peer | null
mesh.xor(pubA, pubB)             // → BigInt XOR distance
mesh.closer(target, pubA, pubB)  // → pubA | pubB
```

**Đặc điểm**:
- Hoàn toàn ephemeral — không ghi vào ZEN graph, không persist trên relay
- Loop prevention tự nhiên qua DAM dup table (`msg["#"]` dedup)
- TTL mặc định 5 hops
- Routing greedy XOR (Kademlia-style) — không cần routing table đầy đủ
- Foundation cho ZEN-native RPC streaming (gửi chunks mà không tốn storage)

**Giới hạn hiện tại**:
- Không đảm bảo delivery (best-effort, no ACK)
- Không encrypt tự động (caller chịu trách nhiệm encrypt `data` nếu cần)
- Routing chỉ tốt khi pub keys phân bố đều — sparse network có thể không tìm được route

22 tests mới trong `test/mesh/dam.js` (từ 235 → 257).

### Discovery: tìm MCP server

Không có domain, không có IP — agent tìm server bằng pub key qua ZEN graph:

```js
// Server publish capability tại well-known soul
const discoverySoul = `~${server_pub}/mcp/info`;
zen.get(discoverySoul).put({
  name: "my-mcp-server",
  version: "1.0",
  tools: ["zen_put", "zen_get", "zen_sign", ...],
  pub: server_pub,
}, null, { authenticator: serverPair });

// Agent discover: biết pub → get info
zen.get(discoverySoul).once(info => {
  // info.tools, info.pub verified bởi ZEN security (signed bởi server_pub)
});
```

Agent có thể nhận `server_pub` qua QR code, link, hay channel message — không cần URL.

### stdio vẫn là mặc định

```bash
node lib/mcp.js           # stdio — Claude Desktop, CLI agents (cùng máy)
```

stdio hoạt động hoàn hảo cho use case **agent chạy trên cùng máy với MCP relay** (Claude Desktop + local ZEN node). Đây là trường hợp phổ biến nhất — agent không phải ZEN peer độc lập, nó chỉ là process con giao tiếp qua stdio.

ZEN-native RPC dành cho **agent là ZEN peer độc lập** chạy trên máy khác, hoặc LLM service bên ngoài (API) có thể embed ZEN SDK.

| Mode | Khi nào dùng | Transport |
|------|-------------|-----------|
| stdio | Agent cùng máy với MCP relay | OS pipes |
| ZEN-native RPC | Agent là ZEN peer riêng biệt, khác máy | ZEN P2P graph |

Không có HTTP server. ZEN relay (`lib/server.js`) vẫn là relay thuần — không chạy MCP logic.

### Vì sao tốt hơn HTTP transport

| Yêu cầu | HTTP (Streamable/SSE) | ZEN-native RPC |
|---------|-----------------------|----------------|
| Server cần IP ổn định | ✅ bắt buộc | ❌ không cần |
| NAT traversal | ❌ cần port forward | ✅ tự nhiên |
| Authentication | Cần thêm layer (JWT, API key) | ZEN pair tích hợp sẵn |
| Replay attack | Cần nonce/timestamp riêng | PEN candle tự xử lý |
| Resumability | SSE event ID + Last-Event-ID | ZEN HAM state tự nhiên |
| Streaming | SSE chunked | `.map().on()` |
| Discovery | Cần DNS / service registry | ZEN pub key |
| Phân tán | Hub-and-spoke | Thuần P2P |

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
