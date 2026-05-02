import crv from "./curves.js";
import { cryptoErr, cbOk } from "./err.js";

async function recover(data, cb, opt) {
  try {
    opt = opt || {};
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
