import crv from "./curves.js";
import applyFormat from "./format.js";
import { cryptoErr, cbOk } from "./err.js";

async function derivepriv(c, priv, seed, label) {
  for (let i = 0; i < 100; i++) {
    const off = await c.hashToScalar(seed, label + (i ? i : ""));
    const d = c.mod(priv + off, c.N);
    if (d !== 0n) {
      return d;
    }
  }
  throw new Error("Failed to derive non-zero private key");
}

async function derivepub(c, pt, seed, label) {
  for (let i = 0; i < 100; i++) {
    const off = await c.hashToScalar(seed, label + (i ? i : ""));
    const d = c.pointAdd(pt, c.pointMultiply(off, c.G));
    if (d) {
      return d;
    }
  }
  throw new Error("Failed to derive valid public key");
}

async function pair(cb, opt) {
  try {
    opt = opt || {};
    const curveName = opt.curve || "secp256k1";
    const c = crv(curveName);
    if (opt.curve && c.curve !== curveName && curveName !== "secp256r1") {
      throw new Error("Unknown curve: " + curveName);
    }
    const format = opt.format || "zen";
    // Use c.curve as the canonical name for deterministic labels so that
    // aliases (secp256r1 → p256) produce the same key from the same seed.
    const labelCurve = c.curve;

    let spriv = null,
      spub = null;

    if (opt.seed && (opt.priv || opt.pub)) {
      // Additive derivation from existing key + seed
      if (opt.priv) {
        const basePriv = opt.priv;
        spriv = await derivepriv(
          c,
          c.parseScalar(basePriv, "Signing key"),
          opt.seed,
          "ZEN.DERIVE|sign|",
        );
        spub = c.publicFromPrivate(spriv);
      }
      if (opt.pub) {
        const basePub = opt.pub;
        spub = await derivepub(
          c,
          c.parsePub(basePub),
          opt.seed,
          "ZEN.DERIVE|sign|",
        );
      }
    } else {
      // Generate fresh or restore from private / seed
      const rawPriv = opt.priv;
      spriv = rawPriv ? c.parseScalar(rawPriv, "Signing key") : null;

      // Seed labels use canonical c.curve so aliases (secp256r1 ≡ p256) share the same key.
      // For secp256k1: 'ZEN|secp256k1|sign|' matches the original hardcoded value — backward compat.
      if (!spriv && opt.seed) {
        spriv = await c.hashToScalar(opt.seed, "ZEN|" + labelCurve + "|sign|");
      }
      const rawPub = opt.pub;
      if (!spriv && !rawPub) {
        spriv = await c.randomScalar();
      }

      if (spriv) {
        spub = c.publicFromPrivate(spriv);
      } else if (rawPub) {
        spub = c.parsePub(rawPub);
      }
    }

    // Single keypair — sign and encrypt use the same key
    const out = await applyFormat(format, labelCurve, c, {
      signPriv: spriv,
      signPub: spub,
      encPriv: spriv,
      encPub: spub,
    });
    return cbOk(cb, out);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { pair };
export default pair;
