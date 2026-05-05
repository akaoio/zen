import PEN from "./pen.js";
import settings from "./settings.js";
import pair from "./pair.js";
import sign from "./sign.js";
import verify from "./verify.js";
import encrypt from "./encrypt.js";
import decrypt from "./decrypt.js";
import secret from "./secret.js";
import shim from "./shim.js";
import hash from "./hash.js";
import certify from "./certify.js";
import keyid from "./keyid.js";
import recover from "./recover.js";
import security from "./security.js";
import graph from "./graph.js";
import { BOOT } from "./bootstrap.js";

var hasOwn = Object.prototype.hasOwnProperty;
var STATIC_SKIP = { length: 1, name: 1, prototype: 1 };
var CHAIN_SKIP = { constructor: 1 };
async function finalizeSigned(result, opt, cb) {
  try {
    if (!(opt || {}).raw) {
      result = await shim.stringify(result);
    }
    if (cb) {
      try {
        cb(result);
      } catch (e) {
        console.log(e);
      }
    }
    return result;
  } catch (e) {
    return result;
  }
}
function mirrorStatics(target, source) {
  Object.getOwnPropertyNames(source).forEach(function (name) {
    if (STATIC_SKIP[name] || hasOwn.call(target, name)) {
      return;
    }
    var desc = Object.getOwnPropertyDescriptor(source, name);
    if (!desc) {
      return;
    }
    Object.defineProperty(target, name, desc);
  });
}

function mirrorMethods(target, source, pick) {
  Object.getOwnPropertyNames(source).forEach(function (name) {
    if ((pick && !pick(name)) || hasOwn.call(target, name)) {
      return;
    }
    var desc = Object.getOwnPropertyDescriptor(source, name);
    if (!desc || "function" !== typeof desc.value) {
      return;
    }
    Object.defineProperty(target, name, {
      configurable: true,
      writable: true,
      value: function (...args) {
        return this.constructor[name](...args);
      },
    });
  });
}

function mirrorChain(target, source) {
  Object.getOwnPropertyNames(source).forEach(function (name) {
    if (CHAIN_SKIP[name] || hasOwn.call(target, name)) {
      return;
    }
    var desc = Object.getOwnPropertyDescriptor(source, name);
    if (!desc || "function" !== typeof desc.value) {
      return;
    }
    Object.defineProperty(target, name, {
      configurable: true,
      writable: true,
      value: function (...args) {
        return this._graph[name](...args);
      },
    });
  });
}

class ZEN {
  constructor(opt = {}) {
    this._opt = opt;
    // Auto-connect to bootstrap peers unless caller explicitly provides peers
    if (opt.peers === undefined) {
      opt.peers = BOOT.slice();
    }
    this._graphInstance = opt.graph || graph.create(opt);
  }

  static pen(spec = {}) {
    return PEN.pen(spec);
  }
  static candle(opts = {}) {
    return PEN.candle(opts);
  }
  static pair(...args) {
    return pair(...args);
  }
  static async sign(data, pairLike, cb, opt) {
    opt = opt || {};
    if (data === undefined) {
      throw new Error("`undefined` not allowed.");
    }
    var parsed = await settings.parse(data);
    var check = (opt.check = opt.check || parsed);
    if (
      typeof pairLike !== "function" &&
      (settings.check(check) || (check && check.s && check.m))
    ) {
      try {
        if (undefined !== (await verify(check, pairLike))) {
          return finalizeSigned(data, { raw: true }, cb);
        }
      } catch (e) {}
    }
    if (typeof pairLike === "function") {
      var signed = await pairLike(data);
      if (signed && signed.authenticatorData) {
        return finalizeSigned(
          {
            m: parsed,
            s: signed.signature
              ? shim.Buffer.from(signed.signature, "binary").toString(
                  opt.encode || "base64",
                )
              : undefined,
            a: shim.Buffer.from(signed.authenticatorData, "binary").toString(
              "base64",
            ),
            c: shim.Buffer.from(signed.clientDataJSON, "binary").toString(
              "base64",
            ),
          },
          opt,
          cb,
        );
      }
      return finalizeSigned(
        {
          m: parsed,
          s:
            typeof signed === "string"
              ? signed
              : signed && signed.signature
                ? shim.Buffer.from(signed.signature, "binary").toString(
                    opt.encode || "base64",
                  )
                : undefined,
        },
        opt,
        cb,
      );
    }
    return sign(data, pairLike, cb, opt);
  }
  static verify(...args) {
    return verify(...args);
  }
  static encrypt(...args) {
    return encrypt(...args);
  }
  static decrypt(...args) {
    return decrypt(...args);
  }
  static secret(...args) {
    return secret(...args);
  }
  static hash(...args) {
    return hash(...args);
  }
  static certify(...args) {
    return certify(...args);
  }
  static recover(...args) {
    return recover(...args);
  }

  get _graph() {
    return this._graphInstance;
  }

  get ready() {
    return PEN.ready;
  }

  use(runtime) {
    this._graphInstance = runtime;
    return this;
  }

  chain() {
    return this._graph;
  }
  pen(spec = {}) {
    return this.constructor.pen(spec);
  }
  candle(opts = {}) {
    return this.constructor.candle(opts);
  }
  pair(...args) {
    return this.constructor.pair(...args);
  }
  sign(...args) {
    return this.constructor.sign(...args);
  }
  verify(...args) {
    return this.constructor.verify(...args);
  }
  encrypt(...args) {
    return this.constructor.encrypt(...args);
  }
  decrypt(...args) {
    return this.constructor.decrypt(...args);
  }
  secret(...args) {
    return this.constructor.secret(...args);
  }
  hash(...args) {
    return this.constructor.hash(...args);
  }
  certify(...args) {
    return this.constructor.certify(...args);
  }
  recover(...args) {
    return this.constructor.recover(...args);
  }

  get(...args) {
    return this._graph.get(...args);
  }
  put(...args) {
    return this._graph.put(...args);
  }
  on(...args) {
    return this._graph.on(...args);
  }
  once(...args) {
    return this._graph.once(...args);
  }
  map(...args) {
    return this._graph.map(...args);
  }
  set(...args) {
    return this._graph.set(...args);
  }
  back(...args) {
    return this._graph.back(...args);
  }
}

mirrorStatics(ZEN, graph.core);
mirrorStatics(ZEN, PEN);
mirrorMethods(ZEN.prototype, PEN, function (name) {
  return !CHAIN_SKIP[name];
});
mirrorChain(ZEN.prototype, graph.chain);

ZEN.Buffer = shim.Buffer;
ZEN.random = shim.random;
ZEN.keyid = keyid;
ZEN.graph = graph;
ZEN.security = security;
ZEN.check = security.check;
ZEN.opt = security.opt;

export { ZEN };
export default ZEN;
