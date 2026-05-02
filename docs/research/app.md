# ZEN App Primitives — Kế hoạch phát triển

> Research/vision doc — brainstorm và thảo luận, chưa phải spec chính thức.
> Cập nhật lần cuối: 2026-05-03

---

## 1. Tổng quan hệ thống primitives hiện tại

ZEN hiện có đủ các building block cốt lõi:

| Soul type | Pattern | Ý nghĩa |
|-----------|---------|---------|
| Shard namespace | `~` hoặc `~/path` | Hierarchical namespace theo path |
| User namespace | `~<pub>` | Owner-only write, signed data |
| Content address | `#<hash>` | Nội dung bất biến, tự xác thực |
| PEN policy | `!<bytecode>` | Write có điều kiện qua bytecode VM |
| Open node | `<any>` | Không kiểm soát quyền ghi |

Các primitive crypto:
- `ZEN.pair()` — sinh key (secp256k1 / p256), seed-based, additive derivation. Schema: `{curve, pub, priv, address}`
- `ZEN.sign/verify` — ECDSA compact wire format
- `ZEN.encrypt/decrypt` — AES-GCM + ECDH, dùng `pub`/`priv` trực tiếp
- `ZEN.certify` — uỷ quyền write, trả về compact signed string
- `ZEN.hash` — SHA-256 / KECCAK-256, mining PoW
- `ZEN.pen()` — biên dịch policy spec → soul string `!<base62>`
- `ZEN.candle()` — temporal window expression để nhúng vào pen spec

**Pair schema đã thay đổi (v1.0.9)**: Không còn `epub`/`epriv`. Encrypt/decrypt dùng `pair.pub`/`pair.priv` trực tiếp. `pair.address` = EVM checksum address.

**Vấn đề**: Các primitive đã đủ mạnh nhưng chưa có lớp application xây lên trên. Developer phải tự lắp ghép, không có pattern rõ ràng cho các use case phổ biến.

---

## 2. Vision: ZEN MCP cho AI Agent Ecosystem

### Bài toán

AI agents hiện nay:
- Bị khoá trong một IDE / một session — không có persistent memory xuyên suốt
- Không giao tiếp được với agent khác (đa IDE, đa provider)
- Khi offline thì mất hết context
- Không có cơ chế identity — không biết agent nào đang nói chuyện

### ZEN MCP giải quyết được gì

```
┌───────────────────────────────────────────────────────────────┐
│  AI Agent A (VS Code)  ←──── ZEN graph ────→  AI Agent B (Cursor) │
│                                    ↕                           │
│                            AI Agent C (Claude)                 │
│                              + Human peers                     │
└───────────────────────────────────────────────────────────────┘
```

- **Offline-first**: Agent ghi vào local graph, sync khi online — không mất dữ liệu
- **Persistent identity**: Hardware-derived keypair, cùng identity trên cùng machine qua `lib/identity.js`
- **Authenticity**: Mọi message đều có thể verify được bằng `ZEN.verify`
- **Real-time sync**: Graph CRDT tự resolve conflict khi nhiều agent ghi đồng thời

### Ứng dụng lớn hơn

Nếu tích hợp IPFS hoặc hệ lưu trữ lớn: ZEN có thể trở thành lớp đồng bộ knowledge toàn cầu giữa các AI agent — mọi insight được verify và persist bất kể agent nào viết.

Các hệ như [OpenClaw](https://github.com/openclaw/openclaw) và các multi-agent framework khác có thể dùng ZEN MCP như transport layer chung, thay thế cho memory ad-hoc không đồng bộ.

---

## 3. Inbox `@<pub>` — P2P Messaging không cần Certify

### Vấn đề hiện tại

Cách duy nhất để peer B gửi message cho peer A là:
1. A cấp certificate cho B (`ZEN.certify`) để B được write vào `~<A_pub>/inbox`
2. Hoặc dùng open node — không có bảo mật

Cả hai đều không tự nhiên. Certificate yêu cầu handshake trước, open node thì bị spam.

### Vấn đề key overflow (mới — quan trọng)

Nếu thiết kế đơn giản: soul `@<alice.pub>` với SGN requirement, key = `#<hash(content)>` — thì vấn đề xảy ra:
- Mỗi message là một key mới trên cùng một soul
- Sau N tháng: soul có hàng ngàn keys
- Graph engine phải load tất cả keys khi query node
- **Database chết** khi đủ lớn — đây là vấn đề thực sự

**Convention-only solution không đủ**: Nếu chỉ quy ước "client chỉ ghi vào candle hiện tại", peer độc hại có thể bỏ qua quy ước và flood 1 triệu keys với timestamps cũ. Peer honest sẽ reject (theo convention), nhưng peer storage vẫn nhận write từ peer khác theo protocol ZEN.

**Giải pháp**: Soul phải được tạo từ `ZEN.pen()` với candle expression nhúng vào bytecode. PEN WASM VM chạy trên **mọi peer** khi nhận write — không có peer nào có thể bypass. Đây là lý do bắt buộc phải dùng PEN soul.

### Kiến trúc: PEN Soul với Candle Constraint

#### `ZEN.candle(opts)` — cơ chế

`ZEN.candle()` trả về một **expression object** (không phải số) để dùng trong `spec.key` của `ZEN.pen()`. Expression này khi compile sang bytecode sẽ validate: *số segment đầu tiên của key phải nằm trong cửa sổ thời gian hiện tại.*

```
candle_segment = Math.floor(Date.now() / size)
```

Ví dụ với `size: 3600000` (1 giờ): vào lúc 2026-05-03 14:xx UTC:
```
candle_segment = Math.floor(1746274000000 / 3600000) = 485076
```

Key hợp lệ: `485076.abc123`, `485075.xyz789` (back=1)
Key bị PEN reject: `485070.old` (quá cũ), `485090.future` (tương lai)

#### Soul architecture

```
Soul = "!" + pen.pack(bytecode)  +  "/" + alice.pub
         ↑ encoded policy                ↑ path component

PEN registers khi validate:
  R[0] = key         ("485076.abc123hash")
  R[2] = full soul   ("!<bytecode>/alice45charPub...")
  R[5] = writer pub  (bob.pub — ai đang ghi)
  R[6] = path        ("alice45charPub..." — extracted từ soul)
  R[7] = PoW nonce   (nếu có)

Bytecode validates:
  AND(
    EQ(R[6], alice.pub),           ← path constraint: đây là inbox của alice
    candle_expr(R[0]),             ← key phải trong cửa sổ thời gian hiện tại
    [SGN 0xC0]                     ← phải ký (tail byte, không vào WASM)
  )
```

#### Soul computation — deterministic từ alice.pub

```js
// Standard inbox candle parameters (protocol constant)
const INBOX_CANDLE = { seg: 0, sep: ".", size: 3600000, back: 2, fwd: 0 }
// back:2 = chấp nhận 2 candle windows trước (2 giờ trước)
// fwd:0  = không chấp nhận timestamp tương lai

// Tính inbox soul của alice — deterministic, không cần prior contact
const aliceInboxSoul = ZEN.pen({
  path: alice.pub,                    // R[6] phải bằng alice.pub
  key: ZEN.candle(INBOX_CANDLE),      // key phải trong candle window hiện tại
  sign: true,                         // phải ký (SGN opcode 0xC0)
})
// aliceInboxSoul = "!<base62bytecode>" — cùng input → cùng output
// Bất kỳ peer nào cũng tính được kết quả này từ alice.pub

// Soul path khi write:
const inboxPath = aliceInboxSoul + "/" + alice.pub
// = "!abc123.../alice45charPub..."
```

#### Key format

```
key = <candle_segment> + "." + <hash(content)>

Ví dụ: "485076.7kNmQpRsTvXw..."
  - "485076"  = Math.floor(Date.now() / 3600000) — hourly segment
  - ".7kNm..."= hash của nội dung để dedup
```

Trong cùng một candle window, mỗi unique content là một key khác nhau. PEN kiểm tra segment đầu (trước dấu `.`) phải nằm trong `[current-2, current]`.

#### Ví dụ đầy đủ

```js
// Standard parameters (dùng trong cả client và server)
const INBOX_CANDLE = { seg: 0, sep: ".", size: 3600000, back: 2, fwd: 0 }

// Bob gửi private message cho Alice
async function sendInbox(bob, alice, message) {
  // Tính inbox soul của alice (deterministic)
  const soul = ZEN.pen({
    path: alice.pub,
    key: ZEN.candle(INBOX_CANDLE),
    sign: true,
  })

  // Encrypt nội dung bằng alice.pub (chỉ alice.priv mới decrypt được)
  const encrypted = await ZEN.encrypt(message, alice.pub)  // ← dùng pub, không phải epub

  // Tính key = <candle>.<hash>
  const segment = Math.floor(Date.now() / INBOX_CANDLE.size)
  const contentHash = await ZEN.hash(encrypted, null, null, { name: "SHA-256" })
  const key = segment + "." + contentHash.slice(0, 16)

  // Write — PEN tự động validate trên mọi peer
  zen.get(soul + "/" + alice.pub).get(key).put(encrypted, null, {
    authenticator: bob   // ← ký bằng bob.priv, prove danh tính sender
  })
}

// Alice đọc inbox
async function readInbox(alice) {
  const soul = ZEN.pen({
    path: alice.pub,
    key: ZEN.candle(INBOX_CANDLE),
    sign: true,
  })

  zen.get(soul + "/" + alice.pub).map().on(async (val, key) => {
    if (!val) return
    const message = await ZEN.decrypt(val, alice.priv)  // ← dùng priv, không phải epriv
    // Biết ai gửi: recover từ signature
    const senderPub = await ZEN.recover(val)    // nếu val là signed string
    console.log("From:", senderPub, "→", message)
  })
}
```

### Tại sao cần PEN (không phải chỉ convention)

| | Convention-only `@<pub>` | PEN soul `!<bytecode>/<pub>` |
|--|--|--|
| Peer honest tuân thủ? | Có | Có — PEN enforce |
| Peer độc hại bypass? | **Có thể** — flood old keys | **Không thể** — PEN reject ở write time |
| Cần tất cả peer hiểu? | Không enforce | Tự động — WASM chạy ở mọi peer |
| Key space bounded? | Không (nếu bị attack) | Có — tối đa `back+1+fwd` windows writable |
| Implementation phức tạp? | Đơn giản hơn | Cần ZEN.pen + ZEN.candle — nhưng đã có sẵn |

### Soul `@<pub>` cũ vs soul mới

`security.js` hiện tại có guard: `if ("@" === (s[0] || "")[0]) return;` trong `settings.pub()` — `@`-prefixed souls fall through sang `check.pipe.any` (open, không auth). Soul mới với format `!<bytecode>/<alice.pub>` không có vấn đề này — được route qua PEN pipeline (`check.plugins`).

Không cần implement `check.pipe.inbox` nữa. PEN đã handle hoàn toàn.

### Câu hỏi còn mở

- **Anti-spam layer 2 — PoW**: Thêm `pow: { unit: "0", difficulty: 1 }` vào pen spec? Với difficulty 1 (~100ms/message), flood 1000 messages/giờ mất ~100 giây CPU. Reasonable cho legitimate users, đắt cho spammer. Có nên bật default không?

- **Metadata privacy**: Khi SGN bật, signature lưu lại trong graph — bất kỳ ai cũng `ZEN.recover()` được bob.pub từ message. Tức metadata "bob wrote to alice" là public. Nếu cần sender anonymity: không dùng SGN, chỉ dùng PoW. Tradeoff: mất accountability.

- **Key window size**: Hourly (3600000ms) → tối đa ~3h backlog. Nếu agent offline 4h thì miss window. Daily (86400000ms)? 5-minute (300000ms)? Tuỳ use case.

- **Pruning old data**: CRDT không delete, nhưng sau khi candle window đóng, messages vẫn readable mãi. Application layer có muốn TTL? ZEN đã có `<?<seconds>` soul suffix cho forget policy — có thể stack lên.

- **Discovery nếu alice dùng custom params**: Nếu alice muốn khác standard INBOX_CANDLE (vd: daily candle, PoW required), cần publish soul spec tại `~<alice.pub>/@`. Bob phải lookup trước. Với standard params thì không cần lookup.

---

## 4. Wallet Address (EVM / BTC)

### Status: Đã implement trong v1.0.9

`pair.address` trong schema mới chính là EVM checksum address, tự động có khi gọi `ZEN.pair()`. Không cần `format` option thêm nữa.

```js
const p = await ZEN.pair()
console.log(p.address)  // "0xAbCdEf..." — EVM checksum address
// p = { curve, pub, priv, address }
// Không có epub, epriv — encrypt/decrypt dùng pub/priv trực tiếp
```

Không còn `epub`/`epriv` trong schema. Tất cả crypto operations dùng `pub`/`priv`:

```js
// Encrypt cho alice
const enc = await ZEN.encrypt(message, alice.pub)   // ← pub (không phải epub)

// Decrypt bởi alice
const msg = await ZEN.decrypt(enc, alice.priv)       // ← priv (không phải epriv)
```

### Identity bridge

Mỗi ZEN identity tự nhiên có EVM address tương ứng. Có thể sign Ethereum transaction bằng cùng secp256k1 key đã dùng trong ZEN. Bridge giữa ZEN graph và on-chain state là tự nhiên, không cần conversion layer.

---

## 5. Anti-Spam / DDoS — Phân lớp bảo vệ

Ba lớp bảo vệ, từ nhẹ đến nặng, stack lên nhau:

| Lớp | Cơ chế | Cost for spammer | Overhead cho user thật |
|-----|--------|-----------------|----------------------|
| Candle window | PEN enforce time-bounded keys | Phải mine trong window | Tự nhiên — key có timestamp |
| SGN | Phải ký — lộ danh tính | Phải có keypair | Rất thấp (~1ms) |
| POW | Proof of Work | CPU time per write | ~100ms/msg (difficulty 1) |
| Quota | Max N writes/hour per pub | Cần nhiều key | Trung bình |

**Candle là lớp nền quan trọng nhất**: Không có nó, mọi lớp khác đều có thể bypass bằng cách flood keys cũ (SGN + timestamp cũ vẫn technically valid trong CRDT). Với PEN candle, peer không thể nhồi old-dated keys vào graph.

### Combination đề xuất cho inbox

```js
// Level 1: Minimum viable (SGN only — không anonymous, free)
ZEN.pen({
  path: alice.pub,
  key: ZEN.candle({ seg: 0, sep: ".", size: 3600000, back: 2, fwd: 0 }),
  sign: true,
})

// Level 2: Anti-spam (SGN + light PoW — ~100ms/msg)
ZEN.pen({
  path: alice.pub,
  key: ZEN.candle({ seg: 0, sep: ".", size: 3600000, back: 2, fwd: 0 }),
  sign: true,
  pow: { unit: "0", difficulty: 1 },
})

// Level 3: High security (SGN + heavy PoW — ~10s/msg)
ZEN.pen({
  path: alice.pub,
  key: ZEN.candle({ seg: 0, sep: ".", size: 86400000, back: 1, fwd: 0 }),
  sign: true,
  pow: { unit: "0", difficulty: 2 },
})
```

Alice có thể publish preferred policy tại `~<alice.pub>/@`. Bob đọc để biết dùng spec nào trước khi gửi.

---

## 6. Storage Limits — Candle giải quyết key overflow

**Vấn đề cốt lõi**: Graph engine khi load một soul phải enumerate tất cả keys. Soul với N=10,000 keys sẽ gây OOM hoặc timeout. Đây là attack vector thực sự, không chỉ là lý thuyết.

**Candle giải quyết ở protocol level**:
- PEN candle constraint → chỉ keys có timestamp trong cửa sổ `[current-back, current+fwd]` được accept
- Old candle windows đóng lại: không có writes mới, không có key space explosion
- Key count per soul = `(back + 1 + fwd) × (messages per window)` — bounded

**Ví dụ**: Inbox với hourly candle, back=2:
- Tại bất kỳ thời điểm nào, tối đa 3 candle windows là writable
- Nếu user nhận 100 messages/giờ: tối đa ~300 active keys
- Historical messages vẫn readable nhưng space được capped về phía trước

**Các vấn đề còn lại**:

| Problem | Status | Giải pháp |
|---------|--------|-----------|
| Key overflow | ✅ PEN candle giải quyết | Enforced at write time |
| Storage full disk | ⬜ Chưa có | LRU eviction, per-node quota |
| Flood during window | ⬜ Partial | PoW tăng cost per write |
| Data bất tử (CRDT no-delete) | ⬜ Chưa có | TTL suffix `<?<ms>` (đã có code) |

**TTL pattern** (`<soul><?<seconds>` format, đã có trong codebase):
```js
// Soul expire sau 7 ngày không access
zen.get("!<bytecode>/<alice.pub><?604800").get(key).put(...)
```

Cân nhắc default TTL cho inbox: 30 ngày? 90 ngày? Application-configurable.

---

## 7. MCP Tool Extensions (cần cho AI Agent)

### `subscribe` tool

Hiện tại MCP là request-response. AI agent không nhận được notification khi có message mới vào inbox.

**Hướng A — Polling**: Agent tự poll `@<pub>` mỗi N giây. Đơn giản, không cần thay đổi server.

**Hướng B — SSE (push)**: MCP server stream JSON events qua stdout khi graph thay đổi. Cần thêm notification protocol vào JSON-RPC handler. Phức tạp hơn nhưng real-time thật sự.

```json
{ "jsonrpc": "2.0", "method": "notifications/graph",
  "params": { "soul": "@alice_pub", "key": "#abc", "value": "..." } }
```

### `list` tool

List các keys trong một soul — hiện tại chỉ có `get` (single key) và implicit `map` qua graph API.

```json
{ "soul": "@alice_pub", "limit": 20, "after": "#lastKey" }
→ [{ "key": "#abc", "state": 1234567890 }, ...]
```

### `identity` tool improvements

- Trả về `address` (EVM) cùng với `pub`/`priv` — đã có trong pair schema mới
- Support rotate identity (generate new pair, link về pub cũ)

---

## 8. Long-term: IPFS Integration

ZEN graph nodes là content-addressed (`#hash`) — tự nhiên tương thích với IPFS CID.

**Hướng tích hợp**:
- `rfs.js` / `radisk.js` → thêm IPFS adapter tương tự `rs3.js`
- Node có thể pin/unpin theo demand thay vì lưu tất cả local
- `#<hash>` souls tự động resolve qua IPFS nếu không có local

**Tiềm năng**: AI agent memory được persist lên IPFS → immutable, verifiable, accessible từ bất kỳ đâu trên mạng. Kết hợp với ZEN identity và signature, có thể xây knowledge graph toàn cầu không cần server trung tâm.

---

## 9. Roadmap đề xuất

```
Near-term (v1.1.x):
  [ ] Inbox PEN soul — implement ZEN.inbox(pub, opts) helper function
      → ZEN.pen({ path: pub, key: ZEN.candle(INBOX_CANDLE), sign: true })
      → Return soul + write helpers
  [ ] Test suite cho inbox soul (candle validation, SGN, key dedup)
  [ ] Standard INBOX_CANDLE constant trong src/index.js

Mid-term:
  [ ] MCP list tool (enumerate keys in soul)
  [ ] MCP subscribe tool (polling mode trước, SSE sau)
  [ ] Per-inbox TTL policy via soul suffix
  [ ] Doc: update ch04 với inbox pattern + candle architecture
  [ ] Doc: ch09 update với ZEN.inbox() MCP usage

Long-term:
  [ ] MCP subscribe tool (SSE/push mode)
  [ ] IPFS storage adapter
  [ ] Cross-agent knowledge sync protocol
  [ ] On-chain bridge (sign ZEN data = sign ETH tx)
  [ ] OpenClaw / multi-IDE integration guide
  [ ] PoW difficulty auto-calibration (based on network congestion)
```

### Đã hoàn thành (v1.0.9)

```
[x] Compact wire format — signed/encrypted strings không wrap JSON
[x] Pair schema: {curve, pub, priv, address} — không còn epub/epriv
[x] EVM address tích hợp vào pair (pair.address)
[x] Certify trả về compact signed string trực tiếp
[x] PEN WASM VM + candle helper sẵn sàng sử dụng
[x] 503/503 tests passing
```

---

*Xem thêm: [ch03-crypto.md](../ch03-crypto.md) · [ch04-authenticated-data.md](../ch04-authenticated-data.md) · [ch07-pen.md](../ch07-pen.md) · [ch09-mcp.md](../ch09-mcp.md)*
