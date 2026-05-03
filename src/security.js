import ZEN from "./root.js";
import verify from "./verify.js";
import recover from "./recover.js";
import hash from "./hash.js";
import sign from "./sign.js";
import settings from "./settings.js";

var u;
var Zen = ZEN;

// --------------- pack / unpack ---------------

settings.pack = function (d, cb, k, n, s) {
  // pack for verifying
  if (settings.check(d)) {
    return cb(d);
  }
  var f = 0,
    tmp;
  if (d && d["#"] && d["."] && d[">"]) {
    tmp = d[":"];
    f = 1;
  }
  JSON.parseAsync(f ? tmp : d, function (err, meta) {
    var sig = u !== (meta || "")[":"] && (meta || "")["~"];
    if (!sig) {
      cb(d);
      return;
    }
    cb({
      m: {
        "#": s || d["#"],
        ".": k || d["."],
        ":": (meta || "")[":"],
        ">": d[">"] || (Zen.state && Zen.state.is ? Zen.state.is(n, k) : 0),
      },
      s: sig,
      v: (meta || "").v,
      c: (meta || "").c,
    });
  });
};

settings.unpack = function (d, k, n) {
  if (u === d) {
    return;
  }
  if (d && u !== (tmp = d[":"])) {
    return tmp;
  }
  k = k || settings.fall_key;
  if (!n && settings.fall_val) {
    n = {};
    n[k] = settings.fall_val;
  }
  if (!k || !n) {
    return;
  }
  if (d === n[k]) {
    return d;
  }
  if (!settings.check(n[k])) {
    return d;
  }
  var soul = (n && n._ && n._["#"]) || settings.fall_soul;
  var s =
    (Zen.state && Zen.state.is ? Zen.state.is(n, k) : 0) || settings.fall_state;
  if (
    d &&
    4 === d.length &&
    soul === d[0] &&
    k === d[1] &&
    fl(s) === fl(d[3])
  ) {
    return d[2];
  }
  if (s < settings.shuffle_attack) {
    return d;
  }
};
var fl = Math.floor;
var tmp;
settings.shuffle_attack = 1546329600000;

settings.pub = function (s) {
  if (!s) {
    return;
  }
  s = s.split("~")[1];
  if (!s) {
    return;
  }
  if ("@" === (s[0] || "")[0]) {
    return;
  }
  if (/^[A-Za-z0-9]{44}[01]/.test(s)) {
    return s.slice(0, 45);
  }
  var parts = s.split(/[^\w_-]/).slice(0, 2);
  if (!parts || 2 !== parts.length) {
    return;
  }
  return parts.slice(0, 2).join(".");
};

// --------------- zen security middleware ---------------

var valid = Zen && Zen.valid;
var link_is = function (d, l) {
  return "string" == typeof (l = valid && valid(d)) && l;
};

function check(msg) {
  var eve = this,
    at = eve.as,
    put = msg.put,
    soul = put["#"],
    key = put["."],
    val = put[":"],
    state = put[">"],
    id = msg["#"];
  if (!soul || !key) {
    return;
  }

  // In-memory user-graph GET replies tag their internal context with `faith`
  // so signed account data can reuse already-verified graph state without
  // re-entering the async pub check.
  if (
    (msg._ || "").faith &&
    "function" == typeof msg._ &&
    ("~" === soul || "~/" === soul.slice(0, 2) || settings.pub(soul))
  ) {
    check.pipe.faith({ eve: eve, msg: msg, put: put, at: at });
    return;
  }

  var no = function (why) {
    at.on("in", { "@": id, err: (msg.err = why) });
  };

  var ctx = {
    eve: eve,
    msg: msg,
    at: at,
    put: put,
    soul: soul,
    key: key,
    val: val,
    state: state,
    id: id,
    no: no,
    pub: null,
  };
  var pipeline = [check.pipe.forget];

  if ("~" === soul || "~/" === soul.slice(0, 2)) {
    pipeline.push(check.pipe.shard);
  } else if ((tmp = settings.pub(soul))) {
    ctx.pub = tmp;
    pipeline.push(check.pipe.pub);
  } else if (0 <= soul.indexOf("#")) {
    pipeline.push(check.pipe.hash);
  } else {
    pipeline.push(check.pipe.any);
  }

  var required = pipeline[1];
  for (var pi = 0; pi < check.plugins.length; pi++) {
    check.plugins[pi](ctx, pipeline);
  }
  if (required && pipeline.indexOf(required) < 0) {
    return no("Security stage removed.");
  }
  check.run(pipeline, ctx);
}

check.run = function (stages, ctx) {
  var no = ctx.no;
  var i = 0;
  var next = function () {
    if (i >= stages.length) {
      return;
    }
    var stage = stages[i++];
    var spent = false;
    var once = function () {
      if (!spent) {
        spent = true;
        next();
      }
    };
    try {
      stage(ctx, once, no);
    } catch (e) {
      no((e && e.message) || String(e));
    }
  };
  next();
};

check.pipe = {
  faith: function (ctx) {
    var eve = ctx.eve,
      msg = ctx.msg,
      put = ctx.put;
    settings.pack(put, function (raw) {
      verify(raw, false, function (data) {
        put["="] = settings.unpack(data);
        eve.to.next(msg);
      });
    });
  },
  forget: function (ctx, next) {
    var soul = ctx.soul,
      state = ctx.state,
      msg = ctx.msg,
      tmp2;
    if (0 <= soul.indexOf("<?")) {
      tmp2 = parseFloat(soul.split("<?")[1] || "");
      if (tmp2 && state < Zen.state() - tmp2 * 1000) {
        (tmp2 = msg._) && tmp2.stun && tmp2.stun--;
        return;
      }
    }
    next();
  },
  shard: function (ctx, next, reject) {
    check.shard(
      ctx.eve,
      ctx.msg,
      ctx.val,
      ctx.key,
      ctx.soul,
      ctx.at,
      reject,
      ctx.at.user || "",
    );
  },
  pub: function (ctx, next, reject) {
    check.pub(
      ctx.eve,
      ctx.msg,
      ctx.val,
      ctx.key,
      ctx.soul,
      ctx.at,
      reject,
      ctx.at.user || "",
      ctx.pub,
    );
  },
  hash: function (ctx, next, reject) {
    check.hash(ctx.eve, ctx.msg, ctx.val, ctx.key, ctx.soul, ctx.at, reject);
  },
  any: function (ctx, next, reject) {
    check.any(
      ctx.eve,
      ctx.msg,
      ctx.val,
      ctx.key,
      ctx.soul,
      ctx.at,
      reject,
      ctx.at.user || "",
    );
  },
};
Object.freeze(check.pipe);

check.plugins = [];
check.use = function (fn) {
  check.plugins.push(fn);
};

function initZenOpt(msg, ctx) {
  var o = Object.assign({}, ctx);
  try {
    Object.defineProperty(msg._, "zen", {
      value: o,
      enumerable: false,
      configurable: true,
      writable: true,
    });
  } catch (e) {
    msg._.zen = o;
  }
  return o;
}

check.$zen = function (msg, user, pub) {
  var scope = msg._ || {};
  var ctx = scope.opt || (scope.msg || {}).opt || {};
  var opt = scope.zen || initZenOpt(msg, ctx);
  var zen = (user && user._) || {};
  var is = (user && user.is) || {};
  var authenticator = opt.authenticator || zen.zen;
  var upub = opt.pub || (authenticator || {}).pub || is.pub || pub;
  if (!scope.done) {
    delete ctx.authenticator;
    delete ctx.pub;
    scope.done = true;
  }
  return { opt: opt, authenticator: authenticator, upub: upub };
};

check.next = function (eve, msg, no) {
  JSON.stringifyAsync(msg.put[":"], function (err, s) {
    if (err) {
      return no(err || "Stringify error.");
    }
    msg.put[":"] = s;
    return eve.to.next(msg);
  });
};

check.auth = function (msg, no, authenticator, done) {
  settings.pack(msg.put, function (packed) {
    if (!authenticator) {
      return no("Missing authenticator");
    }
    if (typeof authenticator === "function") {
      Promise.resolve(authenticator(packed))
        .then(async function (result) {
          if (u === result) {
            return no("Signature fail.");
          }
          var data =
            typeof result === "string" ? await settings.parse(result) : result;
          if (!data || !data.m || !data.s) {
            return no("Invalid signature format");
          }
          var mObj = typeof data.m === "string" ? JSON.parse(data.m) : data.m;
          var parsed = settings.unpack(mObj);
          msg.put[":"] = { ":": parsed, "~": data.s, v: data.v, c: data.c };
          msg.put["="] = parsed;
          done(parsed);
        })
        .catch(function (e) {
          no(e && e.message ? e.message : "Auth error");
        });
      return;
    }
    sign(
      packed,
      authenticator,
      async function (data) {
        if (u === data) {
          return no("Signature fail.");
        }
        var sigData =
          typeof data === "string" ? await settings.parse(data) : data;
        if (!sigData || !sigData.m || !sigData.s) {
          return no("Invalid signature format");
        }
        var mObj = typeof sigData.m === "string" ? JSON.parse(sigData.m) : sigData.m;
        var parsed = settings.unpack(mObj);
        msg.put[":"] = { ":": parsed, "~": sigData.s, v: sigData.v, c: sigData.c };
        msg.put["="] = parsed;
        done(parsed);
      },
      { raw: 1 },
    );
  });
};

check.$vfy = function (
  eve,
  msg,
  key,
  soul,
  pub,
  no,
  certificate,
  certificant,
  cb,
) {
  var certOk = typeof certificate === "string"
    ? settings.check(certificate)
    : (certificate && certificate.m && certificate.s);
  if (!certOk || !certificant || !pub) {
    return;
  }
  return verify(certificate, pub, function (data) {
    if (
      u !== data &&
      u !== data.e &&
      msg.put[">"] &&
      msg.put[">"] > parseFloat(data.e)
    ) {
      return no("Certificate expired.");
    }
    if (
      u !== data &&
      data.c &&
      data.w &&
      (data.c === certificant ||
        (Array.isArray(data.c) && data.c.indexOf(certificant) > -1))
    ) {
      var path =
        soul.indexOf("/") > -1
          ? soul.replace(soul.substring(0, soul.indexOf("/") + 1), "")
          : "";
      String.match = String.match || (Zen.text && Zen.text.match);
      var w = Array.isArray(data.w)
        ? data.w
        : typeof data.w === "object" || typeof data.w === "string"
          ? [data.w]
          : [];
      for (var lex of w) {
        if (
          (String.match(path, lex["#"]) && String.match(key, lex["."])) ||
          (!lex["."] && String.match(path, lex["#"])) ||
          (!lex["#"] && String.match(key, lex["."])) ||
          String.match(path ? path + "/" + key : key, lex["#"] || lex)
        ) {
          if (
            lex["+"] &&
            lex["+"].indexOf("*") > -1 &&
            path &&
            path.indexOf(certificant) == -1 &&
            key.indexOf(certificant) == -1
          ) {
            return no(
              'Path "' +
                path +
                '" or key "' +
                key +
                '" must contain string "' +
                certificant +
                '".',
            );
          }
          if (
            data.wb &&
            (typeof data.wb === "string" || (data.wb || {})["#"])
          ) {
            var root = eve.as.root.$.back(-1);
            if (typeof data.wb === "string" && "~" !== data.wb.slice(0, 1)) {
              root = root.get("~" + pub);
            }
            return root
              .get(data.wb)
              .get(certificant)
              .once(function (value) {
                if (value && (value === 1 || value === true)) {
                  return no("Certificant " + certificant + " blocked.");
                }
                return cb(data);
              });
          }
          return cb(data);
        }
      }
      return no("Certificate verification fail.");
    }
  });
};

check.guard = function (eve, msg, key, soul, at, no, data, next) {
  if (0 > key.indexOf("#")) {
    return next();
  }
  check.hash(eve, msg, data, key, soul, at, no, next);
};

check.hash = function (eve, msg, val, key, soul, at, no, yes) {
  function base64ToHex(data) {
    var binaryStr = atob(data),
      a = [];
    for (var i = 0; i < binaryStr.length; i++) {
      var h = binaryStr.charCodeAt(i).toString(16);
      a.push(h.length === 1 ? "0" + h : h);
    }
    return a.join("");
  }
  var hashKey = key.split("#").pop();
  yes =
    yes ||
    function () {
      eve.to.next(msg);
    };
  if (!hashKey || hashKey === key) {
    return yes();
  }
  hash(
    val,
    null,
    function (b64hash) {
      var hexhash = base64ToHex(b64hash),
        b64slice = b64hash.slice(-20),
        hexslice = hexhash.slice(-20);
      if (
        [b64hash, b64slice, hexhash, hexslice].some(function (item) {
          return item.endsWith(hashKey);
        })
      ) {
        return yes();
      }
      no("Data hash not same as hash!");
    },
    { name: "SHA-256" },
  );
};

check.$sh = {
  pub: 45,
  cut: 2,
  min: 1,
  root: "~",
  pre: "~/",
  bad: /[^0-9a-zA-Z]/,
};
check.$sh.max = Math.ceil(check.$sh.pub / check.$sh.cut);

check.$seg = function (seg, short) {
  if ("string" != typeof seg || !seg) {
    return;
  }
  if (short) {
    if (seg.length < check.$sh.min || seg.length > check.$sh.cut) {
      return;
    }
  } else {
    if (seg.length !== check.$sh.cut) {
      return;
    }
  }
  if (check.$sh.bad.test(seg)) {
    return;
  }
  return 1;
};
check.$path = function (soul) {
  if (check.$sh.root === soul) {
    return [];
  }
  if (check.$sh.pre !== (soul || "").slice(0, 2)) {
    return;
  }
  if ("/" === soul.slice(-1) || 0 <= soul.indexOf("//")) {
    return;
  }
  var path = soul.slice(2).split("/");
  for (var i = 0; i < path.length; i++) {
    if (!check.$seg(path[i])) {
      return;
    }
  }
  return path;
};
check.$kid = function (soul, key) {
  if (check.$sh.root === soul) {
    return check.$sh.pre + key;
  }
  return soul + "/" + key;
};
check.$pub = function (soul, key) {
  var path = check.$path(soul);
  if (!path) {
    return;
  }
  return path.join("") + key;
};
check.$leaf = function (soul, key) {
  var pub = check.$pub(soul, key);
  if (!pub || pub.length !== check.$sh.pub) {
    return;
  }
  if (settings.pub("~" + pub) !== pub) {
    return;
  }
  return pub;
};

check.$tag = async function (msg, cert, upub, $verify, next) {
  var _cert = await settings.parse(cert);
  if (_cert && _cert.m && _cert.s) {
    $verify(_cert, upub, function (_) {
      msg.put[":"]["+"] = _cert;
      next();
    });
  }
};

check.pass = function (eve, msg, raw, data, $verify, writerPub) {
  if (raw["+"] && raw["+"]["m"] && raw["+"]["s"] && writerPub) {
    return $verify(raw["+"], writerPub, function (_) {
      msg.put["="] = data;
      return eve.to.next(msg);
    });
  }
  msg.put["="] = data;
  return eve.to.next(msg);
};

check.pub = async function (eve, msg, val, key, soul, at, no, user, pub, conf) {
  conf = conf || {};
  var $verify = function (certificate, certificant, cb) {
    return check.$vfy(
      eve,
      msg,
      key,
      soul,
      pub,
      no,
      certificate,
      certificant,
      cb,
    );
  };
  var $next = function () {
    return check.next(eve, msg, no);
  };
  var sec = check.$zen(msg, user, pub);
  var opt = sec.opt,
    authenticator = sec.authenticator,
    upub = sec.upub;
  var cert = conf.nocert ? u : opt.cert;
  var $expect = function (data) {
    if (u === conf.want) {
      return 1;
    }
    if (data === conf.want) {
      return 1;
    }
    no(conf.err || "Unexpected payload.");
  };
  var raw = (await settings.parse(val)) || {};
  var $hash = function (data, next2) {
    check.guard(eve, msg, key, soul, at, no, data, next2);
  };

  if ("pub" === key && "~" + pub === soul) {
    if (val === pub) {
      return eve.to.next(msg);
    }
    return no("Account not same!");
  }

  if (
    ((user && user.is) || authenticator) &&
    upub &&
    !raw["~"] &&
    (pub === upub || (pub !== upub && cert))
  ) {
    check.auth(msg, no, authenticator, function (data) {
      if (!$expect(data)) {
        return;
      }
      $hash(data, function () {
        if (pub !== upub && cert) {
          return check.$tag(msg, cert, upub, $verify, $next);
        }
        $next();
      });
    });
    return;
  }

  settings.pack(msg.put, function (packed) {
    recover(packed).then(function (signerPub) {
      verify(packed, signerPub, function (data) {
        data = settings.unpack(data);
        if (u === data) {
          return no("Unverified data.");
        }
        if (!$expect(data)) {
          return;
        }
        var lnk = link_is(data);
        if (lnk && pub === settings.pub(lnk)) {
          (at.zen.own[lnk] = at.zen.own[lnk] || {})[pub] = 1;
        }
        $hash(data, function () {
          check.pass(eve, msg, raw, data, $verify, signerPub);
        });
      });
    }).catch(function () {
      no("Cannot recover signer identity.");
    });
  });
};

check.shard = async function (eve, msg, val, key, soul, at, no, user) {
  var path = check.$path(soul),
    link = link_is(val),
    expected,
    leaf,
    raw;
  if (!path) {
    return no("Invalid shard soul path.");
  }
  if (!check.$seg(key, 1)) {
    return no("Invalid shard key.");
  }
  if (path.length + 1 > check.$sh.max) {
    return no("Invalid shard depth.");
  }
  leaf = check.$leaf(soul, key);
  if (leaf) {
    if (!link) {
      return no("Shard leaf value must be a link.");
    }
    if (link !== "~" + leaf) {
      return no("Shard leaf link must point to ~pub.");
    }
    var lsec = check.$zen(msg, user, leaf);
    var lauthenticator = lsec.authenticator,
      lupub = lsec.upub || (lauthenticator || {}).pub;
    if (!lauthenticator) {
      return no("Shard leaf requires authenticator.");
    }
    if (lupub !== leaf) {
      return no("Shard leaf authenticator pub mismatch.");
    }
    check.auth(msg, no, lauthenticator, function (data) {
      if (link_is(data) !== link) {
        return no("Shard leaf signed payload mismatch.");
      }
      msg.put["="] = { "#": link };
      check.next(eve, msg, no);
    });
    return;
  }
  expected = check.$kid(soul, key);
  var prefix = check.$pub(soul, key);
  raw = link ? {} : (await settings.parse(val)) || {};
  var hasSignature = !link && raw && typeof raw === "object" && raw["~"];
  if (!hasSignature) {
    if (!link) {
      return no("Shard intermediate value must be link.");
    }
    if (link !== expected) {
      return no("Invalid shard link target.");
    }
    var sec2 = check.$zen(msg, user);
    var authenticator2 = sec2.authenticator;
    var authPub = sec2.upub || (authenticator2 || {}).pub;
    if (!authenticator2) {
      return no("Shard intermediate requires authenticator.");
    }
    if ("string" !== typeof authPub || authPub.length !== check.$sh.pub) {
      return no("Invalid shard intermediate pub.");
    }
    if (settings.pub("~" + authPub) !== authPub) {
      return no("Invalid shard intermediate pub format.");
    }
    if (0 !== authPub.indexOf(prefix)) {
      return no("Shard pub prefix mismatch.");
    }
    check.auth(msg, no, authenticator2, function (data) {
      if (link_is(data) !== expected) {
        return no("Shard intermediate signed payload mismatch.");
      }
      msg.put["="] = { "#": expected };
      check.next(eve, msg, no);
    });
    return;
  }
  var existing = ((at.graph || {})[soul] || {})[key];
  if (existing) {
    var existingParsed = await settings.parse(existing);
    if (existingParsed && link_is(existingParsed[":"]) === expected) {
      msg.put["="] = { "#": expected };
      return eve.to.next(msg);
    }
  }
  if (link_is(raw[":"]) !== expected) {
    return no("Invalid shard link target.");
  }
  settings.pack(msg.put, function (packed) {
    recover(packed).then(function (signerPub) {
      if ("string" !== typeof signerPub || signerPub.length !== check.$sh.pub) {
        return no("Invalid shard intermediate pub.");
      }
      if (settings.pub("~" + signerPub) !== signerPub) {
        return no("Invalid shard intermediate pub format.");
      }
      if (0 !== signerPub.indexOf(prefix)) {
        return no("Shard pub prefix mismatch.");
      }
      verify(packed, signerPub, function (data) {
        data = settings.unpack(data);
        if (u === data) {
          return no("Invalid shard intermediate signature.");
        }
        if (link_is(data) !== expected) {
          return no("Shard intermediate payload mismatch.");
        }
        msg.put["="] = data;
        eve.to.next(msg);
      });
    }).catch(function () {
      no("Cannot recover shard signer pub.");
    });
  });
};

check.any = function (eve, msg, val, key, soul, at, no) {
  if (at.opt.secure) {
    return no("Soul missing public key at '" + key + "'.");
  }
  at.on("secure", function (msg2) {
    this.off();
    if (!at.opt.secure) {
      return eve.to.next(msg2);
    }
    no("Data cannot be changed.");
  }).on.on("secure", msg);
};

// --------------- zen plugin ---------------

Zen.on("opt", function (at) {
  if (!at.zen) {
    at.zen = { own: {} };
    at.on("put", check, at);
  }
  this.to.next(at);
});

// --------------- exports ---------------

var security = {
  check: check,
  opt: settings,
  verify: verify,
  recover: recover,
  hash: hash,
  sign: sign,
};

export default security;
