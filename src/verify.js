import crv from "./curves.js";
import { cryptoErr, cbOk } from "./err.js";

async function verify(data, pair, cb, opt) {
  try {
    opt = opt || {};
    const c0 = crv(); // secp256k1 — for settings.parse (curve-independent)
    const msg = await c0.settings.parse(data);
    if (pair === false) {
      const raw = await c0.settings.parse(msg.m);
      return cbOk(cb, raw);
    }
    const pub = pair && pair.pub ? pair.pub : pair;
    // Curve priority: embedded in signed data → pair.curve → opt.curve → secp256k1
    const c = crv((msg && msg.c) || (pair && pair.curve) || opt.curve);
    const h = await c.shaBytes(msg.m);
    const sigBytes = new Uint8Array(c.base62.b62ToBuf(msg.s || "0".repeat(86), 64));
    const { r, s } = c.parseSignature(sigBytes);
    let pt;
    if (pub) {
      pt = c.parsePub(pub);
    } else {
      if (msg.v === undefined || msg.v === null) {
        throw new Error("Public key or recovery bit (v) required");
      }
      if (typeof c.recoverPub !== "function") {
        throw new Error("Curve does not support public key recovery");
      }
      pt = c.recoverPub(msg.v, r, s, h);
    }
    const z = c.mod(c.bytesToBigInt(h), c.N);
    const w = c.modInv(s, c.N);
    const u1 = c.mod(z * w, c.N);
    const u2 = c.mod(r * w, c.N);
    const res = c.pointAdd(c.pointMultiplyG(u1), c.pointMultiply(u2, pt));
    if (!res || c.mod(res.x, c.N) !== r) {
      throw new Error("Signature did not match");
    }
    const out =
      typeof msg.m === "string" && c.settings.check(msg.m)
        ? msg.m
        : await c.settings.parse(msg.m);
    return cbOk(cb, out);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { verify };
export default verify;
