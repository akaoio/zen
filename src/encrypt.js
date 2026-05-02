import crv from "./curves.js";
import { cryptoErr } from "./err.js";

async function encrypt(data, pair, cb, opt) {
  try {
    opt = opt || {};
    const c = crv((pair && typeof pair === "object" && pair.curve) || "secp256k1");
    const key = (pair && pair.priv) || pair;
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
      c.shim.Buffer.from(ct, "binary").toString("base64url") +
      ":" +
      rand.iv.toString("base64url") +
      ":" +
      rand.s.toString("base64url");
    return c.finalize(out, Object.assign({}, opt, { raw: true }), cb);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { encrypt };
export default encrypt;
