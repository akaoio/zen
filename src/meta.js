import ZEN from "./root.js";
import settings from "./settings.js";
import recover from "./recover.js";

var u;
var Zen = ZEN;

/**
 * .meta(cb) — subscribe to a node and receive full signature metadata.
 *
 * Callback signature: cb({ val, key, pub, sig, v, state, soul })
 *
 *   val   — verified plaintext value
 *   key   — graph key
 *   pub   — public key of the writer (recovered from signature or extracted from soul)
 *   sig   — 86-char base62 ECDSA signature, or null if unsigned
 *   state — HAM state timestamp (ms)
 *   soul  — node soul (address)
 *
 * For ~pub namespaces the soul owner is returned as pub when no other signer
 * can be determined.  For !pen and other namespaces the pub key is recovered
 * asynchronously from the embedded signature.
 * For unsigned data pub/sig/v are null.
 */
Zen.chain.meta = function (cb, opt) {
  if (!cb) {
    var zen = this;
    return new Promise(function (resolve) {
      zen.meta(resolve);
    });
  }
  var zen = this;
  (opt = opt || {}).not = 1;
  opt.on = 1;
  zen.get(function (val, key, msg) {
    var put = msg && msg.put;
    if (u === put) {
      return;
    }
    var soul = put["#"];
    var state = put[">"];

    // Extract raw signed slot
    var raw = put[":"];
    var rawObj = raw && typeof raw === "object" ? raw : null;
    if (!rawObj && typeof raw === "string") {
      try {
        rawObj = JSON.parse(raw);
      } catch (e) {}
    }
    var sig = rawObj ? rawObj["~"] || null : null;

    // Fast path: extract pub from soul only when it is an actual public key
    // (45-char base62 ending in 0|1).  Tilde-shard souls like ~as/df/gh return
    // a path fragment from settings.pub() which is NOT a real pub key — reject those.
    var rawPub = settings.pub(soul);
    var pub = (rawPub && /^[A-Za-z0-9]{44}[01]/.test(rawPub)) ? rawPub : null;

    var done = function (resolvedPub) {
      cb.call(zen, {
        val: val,
        key: key,
        pub: resolvedPub !== u ? resolvedPub : null,
        sig: sig,
        state: state,
        soul: soul,
      });
    };

    if (pub) {
      return done(pub);
    }

    // Async fallback: recover pub from embedded signature via settings.pack
    if (sig !== null) {
      settings.pack(put, function (packed) {
        if (packed && packed.s) {
          recover(packed)
            .then(function (recovered) {
              done(recovered);
            })
            .catch(function () {
              done(null);
            });
        } else {
          done(null);
        }
      });
    } else {
      done(null);
    }
  }, opt);
  return zen;
};
