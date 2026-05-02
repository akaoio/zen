import crv from "./curves.js";
import { cryptoErr, cbOk } from "./err.js";

async function decrypt(data, pair, cb, opt) {
  try {
    opt = opt || {};
    const c = crv((pair && typeof pair === "object" && pair.curve) || "secp256k1");
    // Normalize pair.priv to canonical base62 so that different format
    // representations of the same scalar (zen, evm) can decrypt each other.
    // Falls back to raw string for formats parseScalar can't handle (e.g. BTC WIF).
    const rawKey = (pair && pair.priv) || pair;
    let key = rawKey;
    if (pair && pair.priv) {
      try {
        key = c.scalarToString(c.parseScalar(pair.priv, "Decryption key"));
      } catch (_) {
        key = pair.priv;
      }
    }
    if (!key) {
      throw new Error("No decryption key.");
    }
    const parsed = await c.settings.parse(data);
    const enc = parsed._enc || opt.encode || "base64";
    let salt, iv, ct;
    if (enc === "base62") {
      salt = c.shim.Buffer.from(c.base62.b62ToBuf(parsed.s, 9));
      iv   = c.shim.Buffer.from(c.base62.b62ToBuf(parsed.iv, 15));
      ct   = c.base62.b62CtToBuf(parsed.ct);
    } else {
      salt = c.shim.Buffer.from(parsed.s, enc);
      iv   = c.shim.Buffer.from(parsed.iv, enc);
      ct   = c.shim.Buffer.from(parsed.ct, enc);
    }
    const aes = await c.aeskey(key, salt, opt);
    const decrypted = await c.shim.subtle.decrypt(
      {
        name: opt.name || "AES-GCM",
        iv: new Uint8Array(iv),
        tagLength: 128,
      },
      aes,
      new Uint8Array(ct),
    );
    const out = await c.settings.parse(
      new c.shim.TextDecoder("utf8").decode(decrypted),
    );
    return cbOk(cb, out);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { decrypt };
export default decrypt;
