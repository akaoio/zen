import crv from "./curves.js";
import { cryptoErr } from "./err.js";

async function sign(data, pair, cb, opt) {
  try {
    opt = opt || {};
    if (data === undefined) {
      throw new Error("`undefined` not allowed.");
    }
    if (!pair || typeof pair === "function" || !pair.priv) {
      throw new Error("No signing key.");
    }
    const c = crv(pair.curve);
    let msg, h;
    if (opt.prehash) {
      // Accept pre-hashed bytes (Uint8Array or 0x-prefixed hex string).
      // Used for EVM transaction signing where the hash is keccak256(rlpEncoded).
      if (data instanceof Uint8Array) {
        h = data;
      } else {
        const hex = typeof data === "string" && data.startsWith("0x") ? data.slice(2) : data;
        h = new Uint8Array(hex.match(/.{2}/g).map((b) => parseInt(b, 16)));
      }
    } else {
      msg = await c.normalizeMessage(data);
      h = await c.shaBytes(msg);
    }
    const priv = c.parseScalar(pair.priv, "Signing key");
    for (let i = 0; i < 16; i++) {
      const k = await c.deterministicK(priv, h, i);
      const pt = c.pointMultiplyG(k);
      if (!pt) {
        continue;
      }
      const r = c.mod(pt.x, c.N);
      if (!r) {
        continue;
      }
      let s = c.mod(
        c.modInv(k, c.N) * (c.mod(c.bytesToBigInt(h), c.N) + r * priv),
        c.N,
      );
      if (!s) {
        continue;
      }
      let v = Number(pt.y & 1n);
      if (s > c.HALF_N) {
        s = c.N - s;
        v ^= 1;
      }
      if (opt.encode === "raw") {
        // Return raw {r, s, v} for EVM RLP transaction construction.
        return c.finalize(
          {
            r: "0x" + r.toString(16).padStart(64, "0"),
            s: "0x" + s.toString(16).padStart(64, "0"),
            v,
          },
          { raw: true },
          cb,
        );
      }
      const sig = c.concatBytes(c.bigIntToBytes(r, 32), c.bigIntToBytes(s, 32));
      const sigB62 = c.base62.bufToB62Fixed(sig, 86);
      const msgStr = typeof msg === "string" ? msg : await c.shim.stringify(msg);
      const msgB62 = c.base62.bufToB62Ct(new c.shim.TextEncoder().encode(msgStr));
      const out =
        c.curve !== "secp256k1"
          ? sigB62 + v + "/" + c.curve + ":" + msgB62
          : sigB62 + v + ":" + msgB62;
      return c.finalize(out, Object.assign({}, opt, { raw: true }), cb);
    }
    throw new Error("Failed to sign");
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { sign };
export default sign;
