import crv from "./curves.js";
import applyFormat from "./format.js";
import { cryptoErr, cbOk } from "./err.js";

// Parse a raw ECDSA signature for prehash mode.
// Accepts: { r, s, v } object  OR  65-byte "0x..." hex string (r32 + s32 + v1).
function parseRawSig(data) {
  if (data && typeof data === "object" && !Array.isArray(data)) {
    let v = typeof data.v === "bigint" ? Number(data.v) : Number(data.v);
    if (v >= 27) v -= 27;
    const r = typeof data.r === "bigint" ? data.r : BigInt(data.r);
    const s = typeof data.s === "bigint" ? data.s : BigInt(data.s);
    return { r, s, v };
  }
  const hex = typeof data === "string" && data.startsWith("0x") ? data.slice(2) : String(data);
  if (hex.length !== 130) throw new Error("ZEN.recover prehash: expected 65-byte hex sig (130 hex chars)");
  const bytes = new Uint8Array(hex.match(/.{2}/g).map((b) => parseInt(b, 16)));
  let v = bytes[64];
  if (v >= 27) v -= 27;
  const toBigInt = (arr) => arr.reduce((acc, b) => (acc << 8n) | BigInt(b), 0n);
  return { r: toBigInt(bytes.slice(0, 32)), s: toBigInt(bytes.slice(32, 64)), v };
}

// Parse a pre-hashed digest — accepts 32-byte Uint8Array or "0x..." hex string.
function parseHashBytes(hash) {
  if (hash instanceof Uint8Array) return hash;
  const hex = typeof hash === "string" && hash.startsWith("0x") ? hash.slice(2) : String(hash);
  return new Uint8Array(hex.match(/.{2}/g).map((b) => parseInt(b, 16)));
}

async function recover(data, cb, opt) {
  try {
    opt = opt || {};

    if (opt.prehash) {
      // Prehash mode: data = raw sig {r,s,v} or 65-byte hex; opt.hash = pre-hashed digest bytes/hex.
      // Mirrors ZEN.sign(digest, pair, null, { prehash: true, encode: "raw" }) on the verify side.
      if (!opt.hash) throw new Error("ZEN.recover prehash: opt.hash (pre-hashed digest) is required");
      const curveName = opt.curve || "secp256k1";
      const c = crv(curveName);
      const hashBytes = parseHashBytes(opt.hash);
      const { r, s, v } = parseRawSig(data);
      const point = c.recoverPub(v, r, s, hashBytes);
      if (opt.format === "evm" || opt.format === "btc") {
        const out = await applyFormat(opt.format, c.curve, c, { signPriv: null, signPub: point });
        return cbOk(cb, out.address);
      }
      const pub = c.pointToPub(point);
      return cbOk(cb, pub);
    }

    // Original ZEN-format path: data is a ZEN signed string (base62 + embedded message).
    const c0 = crv();
    const msg = await c0.settings.parse(data);
    if (!msg || msg.v === undefined || msg.v === null) {
      throw new Error("No recovery bit (v) in signature");
    }
    const c = crv((msg && msg.c) || opt.curve);
    const h = await c.shaBytes(msg.m);
    const sigBytes = new Uint8Array(c.base62.b62ToBuf(msg.s || "0".repeat(86), 64));
    const { r, s } = c.parseSignature(sigBytes);
    const point = c.recoverPub(msg.v, r, s, h);
    const pub = c.pointToPub(point);
    return cbOk(cb, pub);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { recover };
export default recover;
