# Compact Wire Format Refactor

**Mục tiêu**: Bỏ JSON wrapper, dùng format chuỗi ngắn gọn cho signed data và encrypted data.  
**Backward compat**: KHÔNG — không hỗ trợ đọc JSON cũ. Tiến lên hoàn toàn.

---

## 1. Thiết kế format mới

### 1.1 Encrypted format

**Cũ** (JSON):
```
{"ct":"...base64...","iv":"...base64...","s":"...base64..."}
```

**Mới** (compact, base62, ~25% nhỏ hơn):
```
<ct_b62>:<iv_b62>:<s_b62>
```

- Dấu `:` không xuất hiện trong base62 (`0-9A-Za-z`) → delimiter an toàn
- 3 phần đúng theo thứ tự: ciphertext, IV, salt
- Detection: `3 phần phân tách bởi ':'`, mỗi phần là base62 thuần

### 1.2 Signed format

**Cũ** (JSON):
```
{"m":"...message...","s":"...base64_sig...","v":0}
```
Hoặc với `ZEN{...}` prefix.

**Mới** (compact):
```
<sig_b62_86chars><v_1digit>:<message>
```

- `sig_b62`: chữ ký ECDSA encode base62 — **luôn đúng 86 ký tự** (64 bytes → base62 = 86 chars)
- `v`: `0` hoặc `1` (recovery bit)
- `<message>`: phần còn lại sau `:` đầu tiên (tính từ index 87)
- Detection: `len >= 88`, char index 87 là `:`, char index 86 là `0|1`, 86 chars đầu là base62

**Nếu có curve tag** (non-secp256k1):
```
<sig_b62_86chars><v_1digit>/<curve_tag>:<message>
```
- Ví dụ: `<sig>0/p256:<message>`
- Dùng `/` (slash) thay vì `:` làm separator trước curve tag — `/` không xuất hiện trong base62, dễ gõ hơn `|`
- Detection: `len >= 88`, char index 86 là `0` hoặc `1`, char index 87 là `/`, theo sau là `[a-z0-9]+:`

**Lý do không dùng `:` làm curve separator**: Nếu message của tầng trong là encrypted compact `"ct:iv:s"` hoặc bất kỳ string nào bắt đầu bằng tên curve (`"p256:..."`), parser sẽ nhận nhầm curve tag. Dùng `/` loại bỏ hoàn toàn ambiguity này, kể cả khi chữ ký lồng nhiều tầng:

```
s1 = sign("hello", secp_pair)       → "<sig1>v1:hello"
s2 = sign(s1, p256_pair)            → "<sig2>v2/p256:<sig1>v1:hello"
s3 = sign(s2, secp_pair)            → "<sig3>v3:<sig2>v2/p256:<sig1>v1:hello"
// parse(s3) → { s: sig3, v: v3, m: "<sig2>v2/p256:<sig1>v1:hello" }  ✓
// parse(m)  → { s: sig2, v: v2, c: "p256", m: "<sig1>v1:hello" }      ✓
// parse(m)  → { s: sig1, v: v1, m: "hello" }                          ✓
```

### 1.3 Secret

Giữ nguyên — output là raw base62 string, không có structure.

---

## 2. Độ dài cố định của chữ ký

ECDSA sig = 64 bytes (32 bytes r + 32 bytes s).  
64 bytes → base62: `ceil(64 * log(256) / log(62))` = **86.08** → luôn pad/truncate thành **86 chars**.

Kiểm tra: `c.base62.bufToB62(64_byte_buf)` → cần đảm bảo output luôn đúng 86 chars.  
Nếu base62 encode ra < 86 chars (leading zero bytes), phải **left-pad bằng `'0'`** đến 86 chars.

---

## 3. File cần thay đổi

### 3.1 `src/base62.js`

Thêm helper `bufToB62Fixed(buf, len)`:
- Encode buf sang base62
- Left-pad `'0'` nếu ngắn hơn `len`
- Throw nếu dài hơn `len`

Dùng: `bufToB62Fixed(sig, 86)` trong `sign.js`.

### 3.2 `src/settings.js`

**`settings.check(t)`** — xác định string có phải signed/encrypted không:

```js
settings.check = function(t) {
  if (typeof t !== "string") return false;
  // Encrypted: 3 base62 parts split by ':'
  const parts = t.split(":");
  if (parts.length === 3 && parts.every(p => /^[0-9A-Za-z]+$/.test(p))) return true;
  // Signed secp256k1: <86 base62><0|1>:
  if (t.length >= 88 && /^[0-9A-Za-z]{86}[01]:/.test(t)) return true;
  // Signed non-secp256k1: <86 base62><0|1>/<curve>:
  if (/^[0-9A-Za-z]{86}[01]\/[a-z0-9]+:/.test(t)) return true;
  return false;
};
```

**`settings.parse(t)`** — parse trả về object `{ct, iv, s}` hoặc `{m, s, v, c?}`:

```js
settings.parse = async function(t) {
  if (typeof t !== "string") return t;
  // Encrypted: ct:iv:s (all base62)
  const parts = t.split(":");
  if (parts.length === 3 && parts.every(p => /^[0-9A-Za-z]+$/.test(p))) {
    return { ct: parts[0], iv: parts[1], s: parts[2], _enc: "base62" };
  }
  // Signed non-secp256k1: <sig86><v>/<curve>:<msg>
  const curveM = t.match(/^([0-9A-Za-z]{86})([01])\/([a-z0-9]+):(.*)$/s);
  if (curveM) {
    return { s: curveM[1], v: parseInt(curveM[2]), c: curveM[3], m: curveM[4] };
  }
  // Signed secp256k1: <sig86><v>:<msg>
  if (t.length >= 88 && /^[0-9A-Za-z]{86}[01]:/.test(t)) {
    return { s: t.slice(0, 86), v: parseInt(t[86]), m: t.slice(88) };
  }
  return t;
};
```

> Lưu ý: `settings.parse` bây giờ là **sync-capable** (không cần async cho compact format) nhưng giữ `async` để không breaking existing callers với `await`.

### 3.3 `src/encrypt.js`

Thay output encoding:
```js
// Cũ:
const out = {
  ct: c.shim.Buffer.from(ct, "binary").toString(opt.encode || "base64"),
  iv: rand.iv.toString(opt.encode || "base64"),
  s: rand.s.toString(opt.encode || "base64"),
};
return c.finalize(out, opt, cb);  // → JSON string

// Mới:
const ctB62  = c.base62.bufToB62(c.shim.Buffer.from(ct, "binary"));
const ivB62  = c.base62.bufToB62(rand.iv);
const sB62   = c.base62.bufToB62(rand.s);
const out = ctB62 + ":" + ivB62 + ":" + sB62;
return c.finalize(out, opt, cb);  // → compact string trực tiếp
```

> `c.finalize` với string input: cần kiểm tra behavior — có thể cần pass `{raw: out}` hoặc return trực tiếp.

### 3.4 `src/decrypt.js`

Thay decode encoding từ `"base64"` sang `parsed._enc`:
```js
// Cũ:
const salt = c.shim.Buffer.from(parsed.s,  opt.encode || "base64");
const iv   = c.shim.Buffer.from(parsed.iv, opt.encode || "base64");
const ct   = c.shim.Buffer.from(parsed.ct, opt.encode || "base64");

// Mới:
const enc  = parsed._enc || opt.encode || "base64";
const salt = c.shim.Buffer.from(parsed.s,  enc);
const iv   = c.shim.Buffer.from(parsed.iv, enc);
const ct   = c.shim.Buffer.from(parsed.ct, enc);
```

### 3.5 `src/sign.js`

Thay build output object:
```js
// Cũ:
const out = { m: msg, s: c.encodeBase64(sig, opt.encode || "base64"), v };
if (c.curve !== "secp256k1") { out.c = c.curve; }
return c.finalize(out, opt, cb);

// Mới:
const sigB62 = c.base62.bufToB62Fixed(sig, 86);  // pad to 86 chars
let out;
if (c.curve !== "secp256k1") {
  out = sigB62 + v + "/" + c.curve + ":" + msg;  // slash before curve tag
} else {
  out = sigB62 + v + ":" + msg;
}
return c.finalize(out, opt, cb);
```

### 3.6 `src/verify.js`

Thay decode sig:
```js
// Cũ:
const sigBytes = new Uint8Array(
  c.shim.Buffer.from(msg.s || "", opt.encode || "base64"),
);

// Mới:
const sigBytes = new Uint8Array(c.base62.b62ToBuf(msg.s || ""));
```

> `msg.s` từ `settings.parse` đã là 86-char base62 string, `b62ToBuf` → 64 bytes.

---

## 4. Thay đổi trong `c.finalize`

Cần kiểm tra `src/curves.js` (hoặc nơi định nghĩa `finalize`) — hiện tại nó có thể JSON-stringify object trước khi return. Nếu `out` là string, `finalize` phải trả thẳng string, không stringify.

Cần verify behavior của `finalize(stringValue, opt, cb)`.

---

## 5. Tests cần update

| File | Thay đổi |
|------|---------|
| `test/zen/crypto.js` | Assert format mới — compact string thay vì JSON |
| `test/zen/multicurve.js` | Tương tự, kể cả p256 curve tag |
| `test/zen/instance.js` | Nếu test encrypt/decrypt round-trip |
| `test/pen.js` | Nếu test signed values |

Tất cả **round-trip tests** (sign→verify, encrypt→decrypt) vẫn phải pass mà không sửa logic.

---

## 6. Thứ tự thực hiện

1. **`src/base62.js`** — thêm `bufToB62Fixed(buf, len)`
2. **`src/settings.js`** — viết lại `check()` và `parse()`
3. **`src/encrypt.js`** — output compact
4. **`src/decrypt.js`** — dùng `parsed._enc`
5. **`src/sign.js`** — output compact
6. **`src/verify.js`** — decode sig từ base62
7. **Build + test** — `npm run buildZEN && npm run clean && npm run test:all`
8. **Update tests** nếu có assertion về format

---

## 7. Rủi ro & cần kiểm tra trước

- [ ] `c.finalize(string, opt, cb)` có stringify không? → đọc `curves.js`
- [ ] `c.base62.bufToB62(buf)` có left-pad không? → test 64 bytes all-zero → cần đúng 86 chars
- [ ] Encrypted detection regex: `parts.length === 3` — có false-positive với message chứa `:` không? → message nằm TRONG encrypted (đã được encrypt), không phải raw → không bao giờ là compact string trực tiếp → safe
- [ ] Signed detection: message có thể chứa `:` → `t.slice(88)` lấy toàn bộ phần sau `:` đầu → đúng
- [ ] `normalizeMessage` trong `sign.js` — nếu message đã là JSON object, nó stringify thành gì? → cần đảm bảo không có `:` conflict
