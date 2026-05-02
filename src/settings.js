import base62 from "./base62.js";

const settings = {};
settings.pbkdf2 = { hash: { name: "SHA-256" }, iter: 100000, ks: 64 };
settings.ecdsa = {
  pair: { name: "ECDSA", namedCurve: "secp256k1" },
  sign: { name: "ECDSA", hash: { name: "SHA-256" } },
};
settings.ecdh = { name: "ECDH", namedCurve: "secp256k1" };

settings.jwk = function (pub, d) {
  const xy = base62.pubToJwkXY(pub);
  const jwk = { kty: "EC", crv: "secp256k1", x: xy.x, y: xy.y, ext: true };
  jwk.key_ops = d ? ["sign"] : ["verify"];
  if (d) {
    jwk.d =
      d.length === 44 && /^[A-Za-z0-9]{44}$/.test(d) ? base62.b62ToB64(d) : d;
  }
  return jwk;
};

settings.keyToJwk = function (keyBytes) {
  const keyB64 = keyBytes.toString("base64");
  return {
    kty: "oct",
    k: keyB64.replace(/\+/g, "-").replace(/\//g, "_").replace(/\=/g, ""),
    ext: false,
    alg: "A256GCM",
  };
};

// Compact wire format detector.
// Signed secp256k1:   <86 base62 chars><v 0|1>:<message>
// Signed other curve: <86 base62 chars><v 0|1>/<curve>:<message>
// Encrypted:          <ct_b64url>:<iv_b64url>:<s_b64url>  (exactly 3 colon-separated parts)
const _SIG_HEAD = /^[0-9A-Za-z]{86}[01]/;
const _ENC_PART = /^[A-Za-z0-9_-]+$/;

settings.check = function (t) {
  if (typeof t !== "string") {
    return false;
  }
  // Signed: check first (must come before encrypted check)
  if (t.length >= 88 && _SIG_HEAD.test(t) && (t[87] === ":" || t[87] === "/")) {
    return true;
  }
  // Encrypted: exactly 3 non-empty base64url parts
  const parts = t.split(":");
  if (
    parts.length === 3 &&
    parts[0].length > 0 &&
    parts[1].length > 0 &&
    parts[2].length > 0 &&
    _ENC_PART.test(parts[0]) &&
    _ENC_PART.test(parts[1]) &&
    _ENC_PART.test(parts[2])
  ) {
    return true;
  }
  return false;
};

settings.parse = async function (t) {
  if (typeof t !== "string") {
    return t;
  }
  // Signed: check first
  if (t.length >= 88 && _SIG_HEAD.test(t)) {
    if (t[87] === ":") {
      // secp256k1
      return { s: t.slice(0, 86), v: parseInt(t[86]), m: t.slice(88) };
    }
    if (t[87] === "/") {
      // non-secp256k1: <sig86><v>/<curve>:<msg>
      const rest = t.slice(88);
      const ci = rest.indexOf(":");
      if (ci !== -1) {
        return { s: t.slice(0, 86), v: parseInt(t[86]), c: rest.slice(0, ci), m: rest.slice(ci + 1) };
      }
    }
  }
  // Encrypted: exactly 3 non-empty base64url parts
  const parts = t.split(":");
  if (
    parts.length === 3 &&
    parts[0].length > 0 &&
    parts[1].length > 0 &&
    parts[2].length > 0 &&
    _ENC_PART.test(parts[0]) &&
    _ENC_PART.test(parts[1]) &&
    _ENC_PART.test(parts[2])
  ) {
    return { ct: parts[0], iv: parts[1], s: parts[2], _enc: "base64url" };
  }
  // Fallback: try JSON parse (handles serialised objects, numbers, booleans, null)
  try {
    return JSON.parse(t);
  } catch (e) {}
  return t;
};

export default settings;
