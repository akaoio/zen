import bridge from "./crypto.js";

let _wasmReady = false;
bridge.ready.then(() => { _wasmReady = true; }).catch(() => {});

function bito(n) {
  const hex = n.toString(16).padStart(64, "0");
  const out = new Uint8Array(32);
  for (let i = 0; i < 32; i++) out[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  return out;
}

function _32ToBi(arr) {
  let hex = "";
  for (let i = 0; i < 32; i++) hex += arr[i].toString(16).padStart(2, "0");
  return BigInt("0x" + hex);
}

const ALPHA = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const ALPHA_MAP = {};
for (let i = 0; i < ALPHA.length; i++) {
  ALPHA_MAP[ALPHA[i]] = i;
}
const PUB_LEN = 44;

function biToB62(n) {
  if (typeof n !== "bigint" || n < 0n) {
    throw new Error("biToB62: input must be non-negative BigInt");
  }
  if (_wasmReady) {
    const out = bridge.b62Encode(bito(n));
    let s = "";
    for (let i = 0; i < 44; i++) s += String.fromCharCode(out[i]);
    return s;
  }
  let out = "";
  let value = n;
  while (value > 0n) {
    out = ALPHA[Number(value % 62n)] + out;
    value = value / 62n;
  }
  while (out.length < PUB_LEN) {
    out = "0" + out;
  }
  if (out.length > PUB_LEN) {
    throw new Error("biToB62: value too large for " + PUB_LEN + "-char base62");
  }
  return out;
}

function b62ToBI(s) {
  if (typeof s !== "string" || s.length !== PUB_LEN) {
    throw new Error("b62ToBI: expected " + PUB_LEN + "-char base62 string");
  }
  if (!/^[A-Za-z0-9]+$/.test(s)) {
    throw new Error("b62ToBI: invalid base62 characters");
  }
  if (_wasmReady) {
    const decoded = bridge.b62Decode(s);
    if (!decoded) throw new Error("b62ToBI: invalid base62");
    return _32ToBi(decoded);
  }
  let n = 0n;
  for (let i = 0; i < s.length; i++) {
    const c = ALPHA_MAP[s[i]];
    if (c === undefined) {
      throw new Error("b62ToBI: unknown char " + s[i]);
    }
    n = n * 62n + BigInt(c);
  }
  return n;
}

// Fixed-length constants for AES-GCM wire format fields.
// IV  = 15 bytes → 21 base62 chars  (62^21 > 256^15)
// salt =  9 bytes → 13 base62 chars  (62^13 > 256^9)
const IV_B62_LEN = 21;
const S_B62_LEN  = 13;

// Encode an AES-GCM ciphertext buffer (arbitrary length) into base62.
// Format: 1-char prefix = last-chunk byte count (1-32, ALPHA-indexed),
//         then ceil(buf.length/32) × 44-char base62 chunks (each chunk is
//         32 bytes zero-left-padded and encoded as biToB62).
// The prefix lets the decoder reconstruct exact byte length without any extra metadata.
function bufToB62Ct(buf) {
  if (!buf || buf.length === 0) return ALPHA[0]; // edge: 0-byte output
  const lastLen = ((buf.length - 1) % 32) + 1;  // 1..32
  let out = ALPHA[lastLen];                       // 1-char prefix
  for (let i = 0; i < buf.length; i += 32) {
    const srcLen = Math.min(32, buf.length - i);
    const chunk = new Uint8Array(32);             // zero-padded 32 bytes
    for (let j = 0; j < srcLen; j++) chunk[32 - srcLen + j] = buf[i + j]; // left-pad
    out += biToB62(_32ToBi(chunk));
  }
  return out;
}

// Decode a bufToB62Ct-encoded string back to a Uint8Array.
function b62CtToBuf(s) {
  if (!s || s.length < 2) return new Uint8Array(0);
  const lastLen = ALPHA_MAP[s[0]];  // 1..32
  if (lastLen === undefined || lastLen === 0) return new Uint8Array(0);
  const data = s.slice(1);
  const numChunks = data.length / 44;
  const result = [];
  for (let i = 0; i < numChunks; i++) {
    const chunk44 = data.slice(i * 44, (i + 1) * 44);
    const bytes = bito(b62ToBI(chunk44));   // 32 bytes, big-endian
    if (i < numChunks - 1) {
      for (let j = 0; j < 32; j++) result.push(bytes[j]);
    } else {
      // Last chunk: the real data occupies the rightmost lastLen bytes.
      for (let j = 32 - lastLen; j < 32; j++) result.push(bytes[j]);
    }
  }
  return new Uint8Array(result);
}

// Encode an arbitrary-length buffer as a single base62 BigInt, padded to exactly `len` chars.
function bufToB62Fixed(buf, len) {
  let hex = "";
  for (let i = 0; i < buf.length; i++) hex += ("0" + buf[i].toString(16)).slice(-2);
  let n = BigInt("0x" + (hex || "0"));
  let out = "";
  while (n > 0n) { out = ALPHA[Number(n % 62n)] + out; n = n / 62n; }
  while (out.length < len) out = "0" + out;
  if (out.length > len) throw new Error("bufToB62Fixed: value overflows " + len + " chars");
  return out;
}

// Decode a base62 string back to a fixed-length Uint8Array.
function b62ToBuf(s, byteLen) {
  let n = 0n;
  for (let i = 0; i < s.length; i++) {
    const c = ALPHA_MAP[s[i]];
    if (c === undefined) throw new Error("b62ToBuf: invalid base62 char '" + s[i] + "'");
    n = n * 62n + BigInt(c);
  }
  const hex = n.toString(16).padStart(byteLen * 2, "0");
  const result = new Uint8Array(byteLen);
  for (let i = 0; i < byteLen; i++) result[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  return result;
}

function bufToB62(buf) {
  if (_wasmReady) {
    let out = "";
    for (let i = 0; i < buf.length; i += 32) {
      const chunk = new Uint8Array(32);
      const slice = buf.slice(i, Math.min(i + 32, buf.length));
      chunk.set(slice, 32 - slice.length);
      const enc = bridge.b62Encode(chunk);
      for (let j = 0; j < 44; j++) out += String.fromCharCode(enc[j]);
    }
    return out;
  }
  let out = "";
  for (let i = 0; i < buf.length; i += 32) {
    const end = Math.min(i + 32, buf.length);
    let hex = "";
    for (let p = 0; p < 32 - (end - i); p++) {
      hex += "00";
    }
    for (let j = i; j < end; j++) {
      hex += ("0" + buf[j].toString(16)).slice(-2);
    }
    out += biToB62(BigInt("0x" + hex));
  }
  return out;
}

const base62 = {
  ALPHA,
  biToB62,
  b62ToBI,
  bufToB62,
  bufToB62Fixed,
  bufToB62Ct,
  b62ToBuf,
  b62CtToBuf,
  IV_B62_LEN,
  S_B62_LEN,
  PUB_LEN,
};
export default base62;
export { bito };
