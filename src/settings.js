import base62 from "./base62.js";

const settings = {};
settings.pbkdf2 = { hash: { name: "SHA-256" }, iter: 100000, ks: 64 };
settings.ecdsa = {
  pair: { name: "ECDSA", namedCurve: "secp256k1" },
  sign: { name: "ECDSA", hash: { name: "SHA-256" } },
};
settings.ecdh = { name: "ECDH", namedCurve: "secp256k1" };

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
// Encrypted:          <ct_b62>.<iv_b62_21>.<s_b62_13>
const _SIG_HEAD = /^[0-9A-Za-z]{86}[01]/;
const _B62_PART = /^[A-Za-z0-9]+$/;
const IV_B62_LEN = 21;
const S_B62_LEN  = 13;

settings.check = function (t) {
  if (typeof t !== "string") {
    return false;
  }
  // Signed: check first (must come before encrypted check)
  if (t.length >= 88 && _SIG_HEAD.test(t) && (t[87] === ":" || t[87] === "/")) {
    return true;
  }
  // Encrypted base62: 3 dot-separated parts; iv is always 21 chars, s always 13 chars
  const dparts = t.split(".");
  if (
    dparts.length === 3 &&
    dparts[0].length > 0 &&
    dparts[1].length === IV_B62_LEN &&
    dparts[2].length === S_B62_LEN &&
    _B62_PART.test(dparts[0]) &&
    _B62_PART.test(dparts[1]) &&
    _B62_PART.test(dparts[2])
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
    const decodeMsg = (b62) => new TextDecoder().decode(base62.b62CtToBuf(b62));
    if (t[87] === ":") {
      // secp256k1: message is base62-encoded
      return { s: t.slice(0, 86), v: parseInt(t[86]), m: decodeMsg(t.slice(88)) };
    }
    if (t[87] === "/") {
      // non-secp256k1: <sig86><v>/<curve>:<msg_b62>
      const rest = t.slice(88);
      const ci = rest.indexOf(":");
      if (ci !== -1) {
        return { s: t.slice(0, 86), v: parseInt(t[86]), c: rest.slice(0, ci), m: decodeMsg(rest.slice(ci + 1)) };
      }
    }
  }
  // Encrypted base62 (new): 3 dot-separated parts; iv=21 chars, s=13 chars
  const dparts = t.split(".");
  if (
    dparts.length === 3 &&
    dparts[0].length > 0 &&
    dparts[1].length === IV_B62_LEN &&
    dparts[2].length === S_B62_LEN &&
    _B62_PART.test(dparts[0]) &&
    _B62_PART.test(dparts[1]) &&
    _B62_PART.test(dparts[2])
  ) {
    return { ct: dparts[0], iv: dparts[1], s: dparts[2], _enc: "base62" };
  }
  // Fallback: try JSON parse (handles serialised objects, numbers, booleans, null)
  try {
    return JSON.parse(t);
  } catch (e) {}
  return t;
};

export default settings;
