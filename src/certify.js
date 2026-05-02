import sign from "./sign.js";
import { cryptoErr, cbOk } from "./err.js";

/*
  The Certify Protocol was made out of love by a Vietnamese code enthusiast.
  Vietnamese people around the world deserve respect!
  IMPORTANT: A Certificate is like a Signature. No one knows who (authority)
  created/signed a cert until you put it into their graph.
*/

// RAD/LEX object key detection
var RAD = ["+", "#", ".", "=", "*", ">", "<"];
function israd(obj) {
  if (!obj || typeof obj !== "object") {
    return false;
  }
  for (var i = 0; i < RAD.length; i++) {
    if (RAD[i] in obj) {
      return true;
    }
  }
  return false;
}

// Normalize certificants → pub_string | [pub_string, ...]
// Wildcard (*) is not supported and will be rejected.
function normcerts(raw) {
  if (!raw) {
    return null;
  }
  if (typeof raw === "string") {
    if (raw.indexOf("*") > -1) {
      console.log("Wildcard certificant (*) is not supported.");
      return null;
    }
    return raw;
  }
  if (!Array.isArray(raw) && typeof raw === "object" && raw.pub) {
    return raw.pub;
  }
  if (Array.isArray(raw)) {
    // single element short-circuit
    if (raw.length === 1 && raw[0]) {
      var x = raw[0];
      if (typeof x === "string") {
        return x;
      }
      if (typeof x === "object" && x.pub) {
        return x.pub;
      }
      return null;
    }
    var list = [];
    var hasWildcard = false;
    raw.forEach(function (x) {
      if (typeof x === "string" && x.indexOf("*") > -1) {
        hasWildcard = true;
      } else if (typeof x === "string") {
        list.push(x);
      } else if (x && typeof x === "object" && x.pub) {
        list.push(x.pub);
      }
    });
    if (hasWildcard) {
      console.log("Wildcard certificant (*) is not supported.");
      return null;
    }
    return list.length > 0 ? list : null;
  }
  return null;
}

async function certify(certs, pol, auth, cb, opt) {
  try {
    opt = opt || {};
    pol = pol || {};

    var c = normcerts(certs);
    if (!c) {
      console.log("No certificant found.");
      return;
    }

    var r = pol.read ? pol.read : null;
    var w = pol.write
      ? pol.write
      : typeof pol === "string" || Array.isArray(pol) || israd(pol)
        ? pol
        : null;

    if (!r && !w) {
      console.log("No policy found.");
      return;
    }

    var expiry =
      opt.expiry !== undefined && opt.expiry !== null
        ? parseFloat(opt.expiry)
        : null;

    var blk = opt.block || opt.blacklist || opt.ban || {};
    var rb =
      blk.read && (typeof blk.read === "string" || (blk.read || {})["#"])
        ? blk.read
        : null;
    var wb =
      typeof blk === "string"
        ? blk
        : blk.write && (typeof blk.write === "string" || (blk.write || {})["#"])
          ? blk.write
          : null;

    var data = JSON.stringify(
      Object.assign(
        { c: c },
        expiry ? { e: expiry } : {},
        r ? { r: r } : {},
        w ? { w: w } : {},
        rb ? { rb: rb } : {},
        wb ? { wb: wb } : {},
      ),
    );

    var cert = await sign(data, auth, null, { raw: 1 });
    // cert is now a compact signed string; no additional wrapping needed
    var out = cert;
    return cbOk(cb, out);
  } catch (e) {
    return cryptoErr(e, cb);
  }
}

export { certify };
export default certify;
