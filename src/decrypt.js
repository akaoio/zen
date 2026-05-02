import crv from "./curves.js";
import { cryptoErr, cbOk } from "./err.js";

async function decrypt(data, pair, cb, opt) {
  try {
    opt = opt || {};
    const c = crv((pair && typeof pair === "object" && pair.curve) || "secp256k1");
    const key = (pair && pair.priv) || pair;
    if (!key) {
      throw new Error("No decryption key.");
    }
    const parsed = await c.settings.parse(data);
    const salt = c.shim.Buffer.from(parsed.s, opt.encode || "base64");
    const iv = c.shim.Buffer.from(parsed.iv, opt.encode || "base64");
    const ct = c.shim.Buffer.from(parsed.ct, opt.encode || "base64");
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
