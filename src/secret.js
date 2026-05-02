import crv from "./curves.js";
import { cryptoErr, cbOk } from "./err.js";

async function secret(peer, pair, cb, opt) {
  try {
    opt = opt || {};
    const privKey = (pair && pair.priv) || null;
    if (!pair || !privKey) {
      throw new Error("No secret mix.");
    }
    const c = crv(pair.curve);
    const peerPub = (peer && peer.pub) || peer;
    const pt = c.parsePub(peerPub);
    const priv = c.parseScalar(privKey, "Signing key");
    const shared = c.pointMultiply(priv, pt);
    if (!shared) {
      throw new Error("Could not derive shared secret");
    }
    const h = await c.shaBytes(c.compactPoint(shared));
    const out = c.base62.bufToB62(h);
    return cbOk(cb, out);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { secret };
export default secret;
