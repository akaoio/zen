# WebSocket — Wire Layer

> **One-liner**: Hai file, hai chiều — `lib/websocket.js` implement WS protocol từ TCP socket lên (server side), `src/websocket.js` quản lý vòng đời outbound connection và tombstone logic (client side).

DAM chỉ cần `peer.wire` có `send(raw)` và `readyState`. Bất kỳ object nào implement interface này đều hoạt động — `ServerWire`, `NativeWebSocket`, UDP adapter, hay mock trong tests.

---

## `lib/websocket.js` — ba class phía server

**`WebSocketServer`** — nhận HTTP `upgrade` event, verify `Sec-WebSocket-Key`, ghi response 101, tạo `ServerWire`.

**`ServerWire`** — WS protocol thuần trên raw TCP socket. Không dùng `ws` npm package để giữ dependency tree tối thiểu.

**`NativeWebSocket`** — wrapper mỏng quanh `globalThis.WebSocket`, bridge `addEventListener` sang Node `EventEmitter` pattern (`onXxx` setter → `on/off`). Dùng khi relay cần connect ra ngoài.

### `ServerWire` — RFC 6455 frame parser

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {
  'fontSize': '14px',
  'primaryColor': '#1e293b',
  'primaryTextColor': '#e2e8f0',
  'primaryBorderColor': '#475569',
  'lineColor': '#94a3b8',
  'secondaryColor': '#0f172a',
  'tertiaryColor': '#1e293b',
  'background': '#0f172a',
  'mainBkg': '#1e293b',
  'nodeBorder': '#475569',
  'clusterBkg': '#0f172a',
  'clusterBorder': '#334155',
  'titleColor': '#94a3b8',
  'edgeLabelBackground': '#1e293b'
}}}%%
graph TB
    CHUNK(["TCP chunk\nsocket.on('data')"])
    CONCAT["concat → _buffer"]
    LOOP{"≥ 2 bytes?"}
    HDR["đọc fin, opcode\nmasked, length"]
    LEN{"length\nfield"}
    L7["≤ 125 — dùng trực tiếp"]
    L16["= 126 — đọc 2 bytes"]
    L64["= 127 — đọc 8 bytes"]
    MASK{"masked?"}
    RMASK["đọc 4-byte mask key"]
    ENOUGH{"đủ bytes?"}
    SLICE["slice payload\ntừ _buffer"]
    UNMASK["XOR: payload[i] ^ mask[i%4]"]
    FRAME["_frame(opcode, fin, payload)"]

    CHUNK --> CONCAT --> LOOP
    LOOP -->|yes| HDR --> LEN
    LEN --> L7 & L16 & L64 --> MASK
    MASK -->|yes| RMASK --> ENOUGH
    MASK -->|no| ENOUGH
    ENOUGH -->|no| LOOP
    ENOUGH -->|yes| SLICE --> UNMASK --> FRAME --> LOOP

    style CHUNK fill:#1c1917,stroke:#78716c,color:#a8a29e
    style CONCAT fill:#1e293b,stroke:#475569,color:#e2e8f0
    style LOOP fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style HDR fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style LEN fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style L7 fill:#1e293b,stroke:#475569,color:#e2e8f0
    style L16 fill:#1e293b,stroke:#475569,color:#e2e8f0
    style L64 fill:#1e293b,stroke:#475569,color:#e2e8f0
    style MASK fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style RMASK fill:#3a2010,stroke:#f97316,color:#fdba74
    style ENOUGH fill:#1e293b,stroke:#475569,color:#e2e8f0
    style SLICE fill:#14532d,stroke:#22c55e,color:#86efac
    style UNMASK fill:#3a2010,stroke:#f97316,color:#fdba74
    style FRAME fill:#14532d,stroke:#22c55e,color:#86efac
```

Parser không giả định mỗi TCP chunk = một WS frame — TCP có thể split hoặc merge tùy Nagle algorithm. `_buffer` accumulate đến khi đủ bytes mới parse.

**Opcode dispatch:**

| Opcode | Xử lý |
| --- | --- |
| `0x0` continuation | append `_fragments[]`, flush khi `fin=true` |
| `0x1` text | `emit('message', utf8 string)` |
| `0x2` binary | `emit('message', Buffer)` |
| `0x8` close | echo close frame + `socket.end()` |
| `0x9` ping | echo pong `0xa` |
| `0xa` pong | no-op — DAM dùng DAM-level ping/pong riêng |

RFC 6455 yêu cầu browser luôn mask frame gửi lên server. Server → client không mask. `ServerWire` unmask khi `masked bit = 1`.

---

## `src/websocket.js` — outbound lifecycle và tombstone

```mermaid
%%{init: {'theme': 'base', 'themeVariables': {
  'fontSize': '14px',
  'primaryColor': '#1e293b',
  'primaryTextColor': '#e2e8f0',
  'primaryBorderColor': '#475569',
  'lineColor': '#94a3b8',
  'secondaryColor': '#0f172a',
  'tertiaryColor': '#1e293b',
  'background': '#0f172a',
  'mainBkg': '#1e293b',
  'nodeBorder': '#475569',
  'clusterBkg': '#0f172a',
  'clusterBorder': '#334155',
  'titleColor': '#94a3b8',
  'edgeLabelBackground': '#1e293b'
}}}%%
graph TB
    CALL(["mesh.wire(peer) → open(peer)"])
    TOMB{"_noReconnect\nor _tombUrls?"}
    ABORT(["return silent"])
    NEW["new WebSocket(url)\n_isOutbound = true"]

    OPEN["onopen:\n_openAt = now\nkeepalive ping 30s\nmesh.hi(peer)"]

    CLOSE["onclose:\nclearInterval(_keepalive)"]
    AXE{"_isOutbound\n&& !peer.met?"}
    AXE_INC["_axeGuess++\n≥ 5 → tombstone"]
    HI{"met && _openAt\n< 8s ago?"}
    HI_INC["_hiGuess++\n≥ 3 → tombstone"]
    RECON["reconnect() ~2s\nmesh.bye(peer)"]

    CALL --> TOMB
    TOMB -->|yes| ABORT
    TOMB -->|no| NEW --> OPEN & CLOSE
    CLOSE --> AXE
    AXE -->|yes| AXE_INC --> HI
    AXE -->|no| HI
    HI -->|yes| HI_INC --> RECON
    HI -->|no| RECON

    style CALL fill:#1e3a5f,stroke:#3b82f6,color:#93c5fd
    style TOMB fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style ABORT fill:#450a0a,stroke:#ef4444,color:#fca5a5
    style NEW fill:#1e3a5f,stroke:#3b82f6,color:#93c5fd
    style OPEN fill:#14532d,stroke:#22c55e,color:#86efac
    style CLOSE fill:#3a2010,stroke:#f97316,color:#fdba74
    style AXE fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style AXE_INC fill:#450a0a,stroke:#ef4444,color:#fca5a5
    style HI fill:#3b1f5e,stroke:#a855f7,color:#d8b4fe
    style HI_INC fill:#450a0a,stroke:#ef4444,color:#fca5a5
    style RECON fill:#422006,stroke:#f97316,color:#fdba74
```

### Tombstone — hai counter độc lập

**`_axeGuess`** — tăng khi connect thành công nhưng đóng trước khi HI exchange xong (`peer.met = false`). Nguyên nhân thường gặp: AXE PID-sort drop một phía ngay sau TCP handshake. Sau 5 lần → tombstone.

**`_hiGuess`** — tăng khi peer đã qua HI (`peer.met = true`) nhưng đóng trong vòng 8s. Nguyên nhân: AXE drop sau handshake nhưng trước khi connection ổn định. Sau 3 lần → tombstone.

Tombstone lưu URL theo 3 dạng vì DAM và AXE dùng cả `wss://` lẫn `https://` làm key cho cùng relay:

```js
opt._tombUrls.add(peer.url);                          // wss://relay.x
opt._tombUrls.add(peer.url.replace(/^wss?/, 'http')); // https://relay.x
opt._tombUrls.add(peer.url.replace(/^https?/, 'ws')); // ws://relay.x
```

**Tombstone không persist qua restart** — chỉ tồn tại trong `opt._tombUrls` (in-memory `Set`). Relay bị tombstone được thử lại sau tab reload hoặc process restart.

### Reconnect và keepalive

Reconnect delay cố định ~2s, không có exponential backoff. Khi network partition, nhiều node reconnect cùng lúc → thundering herd vào relay. Đây là known trade-off ưu tiên simplicity.

Trên browser khi tab bị ẩn (`document.hidden`), reconnect loop pause cho đến khi tab visible — tránh wakeup không cần thiết trên mobile.

Keepalive ping (`setInterval(mesh.ping, 30s)`) dùng `peer.wire === wire` để check identity — đảm bảo interval chỉ ping đúng wire instance đã tạo ra nó, không phải wire mới sau reconnect. Proxy thường idle timeout 60s; ping 30s giữ connection khỏi bị close silently ở tầng network.

---

## Tham khảo

| Điểm yếu / trade-off | |
| --- | --- |
| **Fixed 2s reconnect** | Thundering herd sau partition |
| **Tombstone không reset** | `_axeGuess`/`_hiGuess` không clear khi connection ổn định trở lại |
| **Buffer unbounded** | `_buffer` trong `ServerWire` không có size cap — fragmented message lớn tích lũy RAM |
| **No compression** | Không support `permessage-deflate` extension |

| File | Vai trò |
| --- | --- |
| [lib/websocket.js](../../lib/websocket.js) | WS protocol — `NativeWebSocket`, `ServerWire`, `WebSocketServer` |
| [src/websocket.js](../../src/websocket.js) | Outbound lifecycle, tombstone, reconnect — side-effect only |
| [src/mesh.js](../../src/mesh.js) | DAM — gọi `peer.wire.send()` và `mesh.wire(peer)` |
