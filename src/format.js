// Format converters for zen.pair() output.
// Receives raw BigInt scalars and {x,y} curve points; returns {curve, pub, priv, address}.
import keccak256 from "./keccak256.js";
import ripemd160 from "./ripemd160.js";
import shim from "./shim.js";
import { bito } from "./base62.js";

// ── shared helpers ────────────────────────────────────────────────────────────

function toHex(bytes) {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

// Binary SHA-256 — uses WebCrypto directly on raw bytes (NOT via sha256.js JSON path)
async function sha256Bytes(bytes) {
  const hash = await shim.subtle.digest(
    "SHA-256",
    bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes),
  );
  return new Uint8Array(hash);
}

// Base58Check encode
const B58_ALPHA = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

function base58Encode(bytes) {
  const digits = [0];
  for (let i = 0; i < bytes.length; i++) {
    let carry = bytes[i];
    for (let j = 0; j < digits.length; j++) {
      carry += digits[j] * 256;
      digits[j] = carry % 58;
      carry = Math.floor(carry / 58);
    }
    while (carry) {
      digits.push(carry % 58);
      carry = Math.floor(carry / 58);
    }
  }
  let result = "";
  for (let i = 0; i < bytes.length && bytes[i] === 0; i++) result += "1";
  for (let i = digits.length - 1; i >= 0; i--) result += B58_ALPHA[digits[i]];
  return result;
}

async function base58Check(payload) {
  const h1 = await sha256Bytes(payload);
  const h2 = await sha256Bytes(h1);
  const out = new Uint8Array(payload.length + 4);
  out.set(payload);
  out.set(h2.slice(0, 4), payload.length);
  return base58Encode(out);
}

// ── EVM format ────────────────────────────────────────────────────────────────

async function evmAddress(pub) {
  const xBytes = bito(pub.x);
  const yBytes = bito(pub.y);
  const raw = new Uint8Array(64);
  raw.set(xBytes, 0);
  raw.set(yBytes, 32);
  // keccak256 of raw 64-byte uncompressed key (without 04 prefix)
  const hash = await keccak256(raw);
  const addrHex = toHex(hash.slice(-20));
  // EIP-55 checksum
  const ckHash = toHex(await keccak256(addrHex));
  let addr = "0x";
  for (let i = 0; i < 40; i++) {
    addr +=
      parseInt(ckHash[i], 16) >= 8 ? addrHex[i].toUpperCase() : addrHex[i];
  }
  return addr;
}

function evmPrivHex(priv) {
  return "0x" + toHex(bito(priv));
}

function evmEncPub(pub) {
  // Uncompressed pubkey: 0x04 + 32-byte x + 32-byte y
  const out = new Uint8Array(65);
  out[0] = 0x04;
  out.set(bito(pub.x), 1);
  out.set(bito(pub.y), 33);
  return "0x" + toHex(out);
}

// ── BTC format ────────────────────────────────────────────────────────────────

function compressedPubBytes(pub) {
  const out = new Uint8Array(33);
  out[0] = pub.y & 1n ? 0x03 : 0x02;
  out.set(bito(pub.x), 1);
  return out;
}

async function btcAddress(pub) {
  // P2PKH mainnet: Base58Check(0x00 + RIPEMD160(SHA256(compressed_pubkey)))
  const compressed = compressedPubBytes(pub);
  const sha = await sha256Bytes(compressed);
  const ripd = ripemd160(sha);
  const payload = new Uint8Array(21);
  payload[0] = 0x00;
  payload.set(ripd, 1);
  return base58Check(payload);
}

async function btcWIF(priv) {
  // WIF mainnet compressed: Base58Check(0x80 + 32-byte-priv + 0x01)
  const privBytes = bito(priv);
  const payload = new Uint8Array(34);
  payload[0] = 0x80;
  payload.set(privBytes, 1);
  payload[33] = 0x01;
  return base58Check(payload);
}

function btcCompressedHex(pub) {
  return "0x" + toHex(compressedPubBytes(pub));
}

// ── main export ───────────────────────────────────────────────────────────────

export default async function applyFormat(format, curveName, core, raw) {
  const { signPriv, signPub } = raw;
  const out = { curve: curveName };

  if (format === "zen") {
    if (signPriv) out.priv = core.scalarToString(signPriv);
    if (signPub)  out.pub  = core.pointToPub(signPub);
    if (signPub)  out.address = await evmAddress(signPub);
    return out;
  }

  if (format === "evm") {
    if (signPub)  out.pub     = evmEncPub(signPub);       // 0x04 + 128 hex (uncompressed)
    if (signPriv) out.priv    = evmPrivHex(signPriv);     // 0x + 64 hex
    if (signPub)  out.address = await evmAddress(signPub); // 0x EIP-55 checksum
    return out;
  }

  if (format === "btc") {
    if (signPub)  out.pub     = btcCompressedHex(signPub); // 0x02/03 + 64 hex
    if (signPriv) out.priv    = await btcWIF(signPriv);    // WIF compressed
    if (signPub)  out.address = await btcAddress(signPub); // P2PKH base58
    return out;
  }

  throw new Error("Unknown format: " + format + ". Supported: zen, evm, btc");
}
