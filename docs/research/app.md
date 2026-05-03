# ZEN App Primitives & ZACP — Design Spec

> Cập nhật lần cuối: 2026-05-02

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

```
v1.1.x:
  [ ] lib/protocol.js — ZACP helpers (inboxSoul, chanSoul, dmSoul, wrapChanKey, unwrapChanKey, chanSeed)
  [ ] lib/identity.js — seed-based mode (ZEN_IDENTITY_SEED / --identity-file / stdin)
  [ ] lib/mcp.js — add "protocol" tool delegating to lib/protocol.js
  [ ] Test suite cho inbox soul + ZACP group encryption

v1.2.x:
  [ ] Cert expiry support trong MCP certify tool
  [ ] Channel key rotation trong lib/protocol.js
  [ ] MCP list tool (enumerate keys trong một soul)
  [ ] MCP subscribe tool (polling mode)

Long-term:
  [ ] IPFS storage adapter (pattern như rs3.js)
  [ ] MCP subscribe SSE push mode
  [ ] On-chain bridge (secp256k1 = ETH key, sign ZEN data = sign ETH tx)
  [ ] BIP39 mnemonic support trong lib/identity.js
```

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
