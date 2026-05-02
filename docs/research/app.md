# ZEN App Primitives — Kế hoạch phát triển

> Research/vision doc — brainstorm và thảo luận, chưa phải spec chính thức.
> Cập nhật lần cuối: 2026-05-02

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
- `ZEN.pair()` — sinh key (secp256k1 / p256), seed-based, additive derivation
- `ZEN.sign/verify` — ECDSA
- `ZEN.encrypt/decrypt` — AES-GCM + ECDH
- `ZEN.certify` — uỷ quyền write cho key khác
- `ZEN.hash` — SHA-256 / KECCAK-256, mining PoW
- `ZEN.pen()` — biên dịch policy spec → soul string

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

### Thiết kế `@<pub>` inbox soul

```
Soul format:  @<recipient_pub>       (@ + 45-char base62)
Key format:   #<hash(content)>       (content-addressed, tự dedup)
Write rule:   Bất kỳ ai, nhưng phải ký (SGN — chứng minh danh tính sender)
Read:         Owner đọc bình thường qua graph API
Private msg:  Sender encrypt content bằng recipient.epub trước khi put
```

#### Ví dụ usage

```js
// Bob gửi message (encrypt để private) cho Alice
const content = await ZEN.encrypt("Hello Alice!", alice.epub)
const key = "#" + await ZEN.hash(content)
zen.get("@" + alice.pub).get(key).put(content, null, { authenticator: bob })

// Alice đọc inbox
zen.get("@" + alice.pub).map().on(async (val, key) => {
  const msg = await ZEN.decrypt(val, alice.epriv)
  console.log("Message:", msg)
})
```

#### Implementation plan (src/security.js)

1. **Routing**: Thêm case `@<pub>` vào switch trong `check()` function — pattern: soul bắt đầu `@` + 45-char base62 — trước khi fall vào `check.pub`

2. **`check.inbox` function**: Tương tự `check.pub` nhưng đơn giản hơn:
   - Extract recipient pub từ soul (loại bỏ `@` prefix)
   - `settings.pack(msg.put, ...)` → `recover(packed)` → `verify(packed, signerPub, ...)`
   - KHÔNG cần cert, KHÔNG cần owner auth
   - Verify xong → `check.next(eve, msg, no)`

3. **`check.pipe.inbox`**: Thêm vào `check.pipe` object

4. **Build**: `npm run buildZEN` sau khi sửa src/

#### Câu hỏi còn mở

- **Storage quota per inbox**: SGN giảm spam nhưng không chặn hoàn toàn. Nên có per-`@<pub>` limit (max N messages, max age)? Hay để lớp application tự xử lý?
- **Blocklist**: Có nên hỗ trợ `~<pub>/inbox-block/<sender_pub>` để owner block sender không?
- **`@<pub>` vs `~<pub>/inbox`**: Hai pattern này tồn tại song song — `@<pub>` cho public permissionless inbox, `~<pub>/inbox` với cert cho trusted-only inbox.
- **Tự động map sang PEN soul**: Về sau có thể `@<pub>` tự động compile sang PEN soul có SGN+POW requirement nếu owner muốn upgrade protection.

---

## 4. Wallet Address (EVM / BTC)

### Vấn đề

`src/format.js` đã tính được EVM address + BTC address + WIF key, nhưng:
- MCP `crypto` tool không expose `format` option
- Developer biết cách convert key nhưng không lấy được wallet address

### Fix (1-line trong lib/mcp.js)

Thêm `"format"` vào array field trong `case "pair":` của MCP handler:

```js
// Trước
["curve","seed","priv","epriv","pub","epub"].forEach(...)
// Sau
["curve","seed","priv","epriv","pub","epub","format"].forEach(...)
```

### Usage sau khi fix

```json
{ "method": "pair", "pairId": "hw", "format": "evm" }
→ { "pub": "0xAbCdEf...", "priv": "0x...", "epub": "0x04..." }

{ "method": "pair", "pairId": "hw", "format": "btc" }
→ { "pub": "1AbcXyz...", "priv": "KwDiBf...", "epub": "0x02..." }
```

### Ý nghĩa

- Mỗi ZEN identity tự nhiên có EVM address tương ứng
- Có thể sign Ethereum transaction bằng cùng key đã dùng trong ZEN
- Bridge giữa ZEN graph và on-chain state

---

## 5. Anti-Spam / DDoS — Phân lớp bảo vệ

Ba lớp bảo vệ, từ nhẹ đến nặng, stack lên nhau:

| Lớp | Cơ chế | Cost for spammer | Overhead cho user thật |
|-----|--------|-----------------|----------------------|
| SGN | Phải ký — lộ danh tính | Phải có keypair | Rất thấp |
| POW | Proof of Work | CPU time | ~50ms/msg |
| Quota | Max N writes/hour per pub | Cần nhiều key | Trung bình |

Với inbox `@<pub>`:
- Default: SGN only
- Owner có thể upgrade lên SGN+POW bằng cách publish yêu cầu trong `~<pub>/inbox-policy`
- Về lâu dài: soul `@<pub>` tự động map sang PEN soul có POW requirement

---

## 6. Storage Limits — Vấn đề chưa giải quyết

**Rủi ro**: Node không giới hạn storage sẽ bị crash khi đầy disk, hoặc bị tấn công bằng cách flood dữ liệu.

**Hướng giải quyết đề xuất**:
- **Per-soul quota**: Mỗi soul có thể có limit khai báo trong meta node
- **TTL (time-to-live)**: Soul format `<soul><?<seconds>` đã có trong code — tận dụng cho inbox
- **LRU eviction**: Khi đầy, drop các node lâu nhất không được access
- **Tiered storage**: Local = hot, S3/IPFS = cold, sync theo demand

Cần benchmark trước khi thiết kế — hiện tại chưa đủ data về quy mô thực tế.

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

- Trả về EVM + BTC address cùng với pub/epub
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
Near-term:
  [ ] @<pub> inbox soul — check.inbox trong security.js + buildZEN
  [ ] MCP crypto: expose format:"evm"/"btc" (1-line fix)
  [ ] Test suite cho inbox soul

Mid-term:
  [ ] MCP list tool
  [ ] MCP subscribe tool (polling mode trước)
  [ ] Per-inbox storage quota / TTL policy
  [ ] Doc: update ch04 với inbox pattern, ch09 với wallet address

Long-term:
  [ ] MCP subscribe tool (SSE/push mode)
  [ ] IPFS storage adapter
  [ ] Cross-agent knowledge sync protocol
  [ ] On-chain bridge (sign ZEN data = sign ETH tx)
  [ ] OpenClaw / multi-IDE integration guide
```

---

*Xem thêm: [ch03-crypto.md](../ch03-crypto.md) · [ch04-authenticated-data.md](../ch04-authenticated-data.md) · [ch07-pen.md](../ch07-pen.md) · [ch09-mcp.md](../ch09-mcp.md)*
