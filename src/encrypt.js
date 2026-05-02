import crv from "./curves.js";
import { cryptoErr } from "./err.js";

async function encrypt(data, pair, cb, opt) {
  try {
    opt = opt || {};
    const c = crv((pair && typeof pair === "object" && pair.curve) || "secp256k1");
    // Normalize pair.priv to canonical base62 so that different format
    // representations of the same scalar (zen, evm) produce the same key.
    // Falls back to raw string for formats parseScalar can't handle (e.g. BTC WIF).
    const rawKey = (pair && pair.priv) || pair;
    let key = rawKey;
    if (pair && pair.priv) {
      try {
        key = c.scalarToString(c.parseScalar(pair.priv, "Encryption key"));
      } catch (_) {
        key = pair.priv;
      }
    }
    if (data === undefined) {
      throw new Error("`undefined` not allowed.");
    }
    if (!key) {
      throw new Error("No encryption key.");
    }
    const message =
      typeof data === "string" ? data : await c.shim.stringify(data);
    const rand = { s: c.shim.random(9), iv: c.shim.random(15) };
    const aes = await c.aeskey(key, rand.s, opt);
    const ct = await c.shim.subtle.encrypt(
      {
        name: opt.name || "AES-GCM",
        iv: new Uint8Array(rand.iv),
      },
      aes,
      new c.shim.TextEncoder().encode(message),
    );
    const out =
      c.base62.bufToB62Ct(new Uint8Array(ct)) +
      "." +
      c.base62.bufToB62Fixed(rand.iv, c.base62.IV_B62_LEN) +
      "." +
      c.base62.bufToB62Fixed(rand.s, c.base62.S_B62_LEN);
    return c.finalize(out, Object.assign({}, opt, { raw: true }), cb);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { encrypt };
export default encrypt;
