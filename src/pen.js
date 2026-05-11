import SecurityMod from "./security.js";
import base62 from "./base62.js";

const __penWasmURL = new URL("./pen.wasm", import.meta.url);
{
  var runtime = SecurityMod;

  // ── WASM init ───────────────────────────────────────────────────────────────

  var _wasm = null;
  var pen = {};

  function createPenReady() {
    if (
      typeof process !== "undefined" &&
      process.versions &&
      process.versions.node
    ) {
      return import("node:fs/promises")
        .then(function (mod) {
          var readFile = mod.readFile || (mod.default || {}).readFile;
          if (!readFile) {
            throw new Error("pen: fs.readFile unavailable");
          }
          return readFile(__penWasmURL);
        })
        .then(function (bytes) {
          return WebAssembly.instantiate(bytes, {}).then(function (r) {
            _wasm = r;
          });
        });
    }
    if (typeof fetch !== "undefined") {
      return fetch(__penWasmURL)
        .then(function (r) {
          if (!r.ok)
            throw new Error(
              "pen: fetch pen.wasm failed: " + r.status + " " + r.url,
            );
          return r.arrayBuffer();
        })
        .then(function (buf) {
          return WebAssembly.instantiate(buf, {});
        })
        .then(function (r) {
          _wasm = r;
        });
    }
    return Promise.reject(
      new Error("pen: cannot load pen.wasm in this environment"),
    );
  }

  pen.ready = createPenReady();

  function _view() {
    return new Uint8Array(_wasm.instance["exports"].memory.buffer);
  }

  // ── Wire encoding ────────────────────────────────────────────────────────────

  var _enc =
    typeof TextEncoder !== "undefined"
      ? new TextEncoder()
      : {
          encode: function (s) {
            var buf = Buffer.from(s, "utf8");
            return new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
          },
        };

  function _writeReg(view, offset, val) {
    if (val === null || val === undefined) {
      view[offset++] = 0;
      return offset;
    }
    if (typeof val === "boolean") {
      view[offset++] = 1;
      view[offset++] = val ? 1 : 0;
      return offset;
    }
    if (typeof val === "number") {
      if (
        Number.isInteger(val) &&
        val >= -0x8000000000000000 &&
        val <= 0x7fffffffffffffff
      ) {
        view[offset++] = 2;
        var lo = val >>> 0;
        var hi = Math.floor(val / 0x100000000);
        view[offset++] = lo & 0xff;
        view[offset++] = (lo >> 8) & 0xff;
        view[offset++] = (lo >> 16) & 0xff;
        view[offset++] = (lo >> 24) & 0xff;
        var hlo = hi >>> 0;
        view[offset++] = hlo & 0xff;
        view[offset++] = (hlo >> 8) & 0xff;
        view[offset++] = (hlo >> 16) & 0xff;
        view[offset++] = (hlo >> 24) & 0xff;
        return offset;
      } else {
        view[offset++] = 3;
        var dv = new DataView(view.buffer, view.byteOffset + offset, 8);
        dv.setFloat64(0, val, true);
        offset += 8;
        return offset;
      }
    }
    if (typeof val === "string") {
      view[offset++] = 4;
      var encoded = _enc.encode(val);
      var slen = encoded.length;
      view[offset++] = slen & 0xff;
      view[offset++] = (slen >> 8) & 0xff;
      view[offset++] = (slen >> 16) & 0xff;
      view[offset++] = (slen >> 24) & 0xff;
      for (var i = 0; i < slen; i++) view[offset++] = encoded[i];
      return offset;
    }
    view[offset++] = 0;
    return offset;
  }

  // ── run ──────────────────────────────────────────────────────────────────────

  pen.run = function (bytecode, regs) {
    if (!_wasm) throw new Error("pen: not ready. await pen.ready first.");
    var exp = _wasm.instance.exports;
    var view = _view();

    exp.free();

    var base = exp.mem();
    var bclen = bytecode.length;
    view[base + 0] = bclen & 0xff;
    view[base + 1] = (bclen >> 8) & 0xff;
    view[base + 2] = (bclen >> 16) & 0xff;
    view[base + 3] = (bclen >> 24) & 0xff;

    for (var i = 0; i < bclen; i++) view[base + 4 + i] = bytecode[i];

    var regOff = base + 4 + bclen;
    var nregs = regs ? regs.length : 0;
    view[regOff + 0] = nregs & 0xff;
    view[regOff + 1] = (nregs >> 8) & 0xff;
    view[regOff + 2] = (nregs >> 16) & 0xff;
    view[regOff + 3] = (nregs >> 24) & 0xff;

    var off = regOff + 4;
    for (var j = 0; j < nregs; j++) off = _writeReg(view, off, regs[j]);

    var result = exp.run();
    if (result === 1) return true;
    if (result === 0) return false;
    if (result === -2) throw new Error("PEN: bad version byte");
    if (result === -3) throw new Error("PEN: max recursion depth exceeded");
    throw new Error("PEN: runtime error (" + result + ")");
  };

  // ── pack / unpack (bytecode ↔ base62) ────────────────────────────────────────
  // Used to store bytecode as the soul/node-ID in ZEN graph.
  // Soul format: '$' + pen.pack(bytecode)
  // e.g. '$abc123...' (variable length base62)

  var B62_MAP = {};
  for (var _i = 0; _i < base62.ALPHA.length; _i++) B62_MAP[base62.ALPHA[_i]] = _i;

  function b62enc(n) {
    if (n === 0n) return base62.ALPHA[0];
    var s = "";
    while (n > 0n) {
      s = base62.ALPHA[Number(n % 62n)] + s;
      n = n / 62n;
    }
    return s;
  }

  function b62dec(s) {
    var n = 0n;
    for (var i = 0; i < s.length; i++) n = n * 62n + BigInt(B62_MAP[s[i]] || 0);
    return n;
  }

  pen.pack = function (buf) {
    var hex = "01";
    for (var i = 0; i < buf.length; i++)
      hex += ("0" + buf[i].toString(16)).slice(-2);
    return b62enc(BigInt("0x" + hex));
  };

  pen.unpack = function (s) {
    var n = b62dec(s);
    var hex = n.toString(16);
    if (hex.length % 2) hex = "0" + hex;
    var bytes = [];
    for (var i = 0; i < hex.length; i += 2)
      bytes.push(parseInt(hex.slice(i, i + 2), 16));
    return new Uint8Array(bytes.slice(1)); // drop sentinel 0x01
  };

  // ── Bytecode builder ─────────────────────────────────────────────────────────

  var bc = (pen.bc = {});

  bc.uleb = function (n) {
    var bytes = [];
    n = n >>> 0;
    do {
      var b = n & 0x7f;
      n >>>= 7;
      if (n !== 0) b |= 0x80;
      bytes.push(b);
    } while (n !== 0);
    return bytes;
  };
  bc.sleb = function (n) {
    var bytes = [],
      more = true;
    while (more) {
      var b = n & 0x7f;
      n >>= 7;
      if ((n === 0 && (b & 0x40) === 0) || (n === -1 && (b & 0x40) !== 0))
        more = false;
      else b |= 0x80;
      bytes.push(b);
    }
    return bytes;
  };

  bc.prog = function (root) {
    return new Uint8Array([0x01].concat(root));
  };
  bc.null_ = function () {
    return [0x00];
  };
  bc.true_ = function () {
    return [0x01];
  };
  bc.false_ = function () {
    return [0x02];
  };
  bc.str = function (s) {
    var bytes = Array.from(_enc.encode(s));
    return [0x03].concat(bc.uleb(bytes.length)).concat(bytes);
  };
  bc.uint = function (n) {
    return [0x04].concat(bc.uleb(n));
  };
  bc.int = function (n) {
    return [0x07].concat(bc.sleb(n));
  };
  bc.f64 = function (n) {
    var buf = new ArrayBuffer(8);
    new DataView(buf).setFloat64(0, n, false);
    return [0x08].concat(Array.from(new Uint8Array(buf)));
  };
  bc.pass = function () {
    return [0x23];
  };
  bc.fail = function () {
    return [0x24];
  };
  bc.reg = function (n) {
    return [0x10, n];
  };
  bc.r0 = function () {
    return [0xf0];
  };
  bc.r1 = function () {
    return [0xf1];
  };
  bc.r2 = function () {
    return [0xf2];
  };
  bc.r3 = function () {
    return [0xf3];
  };
  bc.r4 = function () {
    return [0xf4];
  };
  bc.r5 = function () {
    return [0xf5];
  };
  bc.r6 = function () {
    return [0x10, 6];
  }; // path register
  bc.local = function (n) {
    return [0xf8 + n];
  };
  bc.intn = function (n) {
    return n >= 0 && n <= 15 ? [0xe0 + n] : bc.uint(n);
  };

  bc.and = function (exprs) {
    return [0x20, exprs.length].concat(...exprs);
  };
  bc.or = function (exprs) {
    return [0x21, exprs.length].concat(...exprs);
  };
  bc.not = function (a) {
    return [0x22].concat(a);
  };

  bc.eq = function (a, b) {
    return [0x30].concat(a, b);
  };
  bc.ne = function (a, b) {
    return [0x31].concat(a, b);
  };
  bc.lt = function (a, b) {
    return [0x32].concat(a, b);
  };
  bc.gt = function (a, b) {
    return [0x33].concat(a, b);
  };
  bc.lte = function (a, b) {
    return [0x34].concat(a, b);
  };
  bc.gte = function (a, b) {
    return [0x35].concat(a, b);
  };

  bc.add = function (a, b) {
    return [0x40].concat(a, b);
  };
  bc.sub = function (a, b) {
    return [0x41].concat(a, b);
  };
  bc.mul = function (a, b) {
    return [0x42].concat(a, b);
  };
  bc.divu = function (a, b) {
    return [0x43].concat(a, b);
  };
  bc.mod = function (a, b) {
    return [0x44].concat(a, b);
  };
  bc.abs = function (a) {
    return [0x46].concat(a);
  };
  bc.neg = function (a) {
    return [0x47].concat(a);
  };

  bc.len = function (a) {
    return [0x50].concat(a);
  };
  bc.slice = function (a, s, e) {
    return [0x51].concat(a, s, e);
  };
  bc.seg = function (a, sep, idx) {
    return [0x52].concat(a, [sep.charCodeAt(0)], idx);
  };
  bc.tonum = function (a) {
    return [0x53].concat(a);
  };
  bc.tostr = function (a) {
    return [0x54].concat(a);
  };
  bc.concat = function (a, b) {
    return [0x55].concat(a, b);
  };
  bc.pre = function (a, b) {
    return [0x56].concat(a, b);
  };
  bc.suf = function (a, b) {
    return [0x57].concat(a, b);
  };
  bc.inc = function (a, b) {
    return [0x58].concat(a, b);
  };
  bc.upper = function (a) {
    return [0x5a].concat(a);
  };
  bc.lower = function (a) {
    return [0x5b].concat(a);
  };

  bc.iss = function (a) {
    return [0x60].concat(a);
  };
  bc.isn = function (a) {
    return [0x61].concat(a);
  };
  bc.isx = function (a) {
    return [0x62].concat(a);
  };
  bc.isb = function (a) {
    return [0x63].concat(a);
  };
  bc.lng = function (a, mn, mx) {
    return [0x64].concat(a, [mn, mx]);
  };

  bc.let_ = function (slot, def, body) {
    return [0x70, slot].concat(def, body);
  };
  bc.if_ = function (c, t, e) {
    return [0x71].concat(c, t, e);
  };
  bc.segr = function (reg, sep, idx) {
    return [0x80, reg, sep.charCodeAt(0), idx];
  };
  bc.segrn = function (reg, sep, idx) {
    return [0x81, reg, sep.charCodeAt(0), idx];
  };

  // ── treeskip: advance pos past one expression node without evaluating ─────────
  // Used by scanpolicy to find where the root expression ends, so policy bytes
  // appended after the tree can be scanned without false positives.

  function treeskip(bytecode, pos) {
    var op = bytecode[pos++];
    if (op === 0x00 || op === 0x01 || op === 0x02 || op === 0x23 || op === 0x24)
      return pos;
    if (op === 0x03) {
      // ULEB128-encoded length
      var len = 0, shift = 0, b;
      do {
        b = bytecode[pos++];
        len |= (b & 0x7f) << shift;
        shift += 7;
      } while (b & 0x80);
      return pos + len;
    } // string
    if (op === 0x04 || op === 0x07) {
      while (bytecode[pos++] & 0x80) {}
      return pos;
    } // varint
    if (op === 0x08) return pos + 8; // f64
    if (op === 0x10) return pos + 1; // reg(n)
    if (op === 0x20 || op === 0x21) {
      // and/or
      var n = bytecode[pos++];
      for (var i = 0; i < n; i++) pos = treeskip(bytecode, pos);
      return pos;
    }
    if (op === 0x22) return treeskip(bytecode, pos); // not
    if (op >= 0x30 && op <= 0x35) {
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // 2-arg compare
    if (op >= 0x40 && op <= 0x45) {
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // 2-arg arith
    if (op === 0x46 || op === 0x47) return treeskip(bytecode, pos); // abs/neg
    if (op === 0x50) return treeskip(bytecode, pos); // len
    if (op === 0x51) {
      pos = treeskip(bytecode, pos);
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // slice
    if (op === 0x52) {
      pos = treeskip(bytecode, pos);
      pos++;
      return treeskip(bytecode, pos);
    } // seg
    if (op === 0x53 || op === 0x54) return treeskip(bytecode, pos); // tonum/tostr
    if (op >= 0x55 && op <= 0x59) {
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // 2-arg string
    if (op === 0x5a || op === 0x5b) return treeskip(bytecode, pos); // upper/lower
    if (op >= 0x60 && op <= 0x63) return treeskip(bytecode, pos); // type checks
    if (op === 0x64) return treeskip(bytecode, pos) + 2; // lng: expr + [min][max]
    if (op === 0x70) {
      pos++;
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // let
    if (op === 0x71) {
      pos = treeskip(bytecode, pos);
      pos = treeskip(bytecode, pos);
      return treeskip(bytecode, pos);
    } // if
    if (op === 0x80 || op === 0x81) return pos + 3; // segr/segrn macros
    if (op >= 0xe0 && op <= 0xef) return pos; // intn shorthands
    if ((op >= 0xf0 && op <= 0xf5) || op >= 0xf8) return pos; // reg shorthands
    return pos; // unknown — stop here (policy byte or EOF)
  }

  function readuleb(bytes, pos) {
    var value = 0,
      shift = 0,
      b = 0;
    do {
      if (pos >= bytes.length) return null;
      b = bytes[pos++];
      value |= (b & 0x7f) << shift;
      shift += 7;
    } while (b & 0x80);
    return { value: value >>> 0, next: pos };
  }

  // ── scanpolicy: extract tail opcodes appended after expression root ───────────
  // Tail bytes (0xC0..) are appended AFTER the complete expression tree.
  // We use treeskip() to find where the tree ends, avoiding false positives from
  // integer/string byte values within the expression that happen to overlap tail opcodes.

  function scanpolicy(bytecode) {
    var p = { sign: false, cert: null, open: false, pow: null };
    if (!bytecode || bytecode.length < 2) return p;
    var pos = treeskip(bytecode, 1); // skip version byte + root expression
    for (var i = pos; i < bytecode.length; ) {
      var op = bytecode[i++];
      if (op === 0xc0) {
        p.sign = true;
        continue;
      }
      if (op === 0xc1) {
        var clen = bytecode[i++] || 0,
          pub = "";
        if (i + clen > bytecode.length) return p;
        for (var cj = 0; cj < clen; cj++)
          pub += String.fromCharCode(bytecode[i++]);
        p.cert = pub;
        continue;
      }
      if (op === 0xc3) {
        p.open = true;
        continue;
      }
      if (op === 0xc4) {
        var pfield = bytecode[i++],
          pdiff = bytecode[i++];
        var ulen = bytecode[i++] || 0,
          unit = "";
        if (i + ulen > bytecode.length) return p;
        for (var ui = 0; ui < ulen; ui++)
          unit += String.fromCharCode(bytecode[i++]);
        p.pow = { field: pfield, difficulty: pdiff, unit: unit || "0" };
        continue;
      }
      break;
    }
    return p;
  }

  pen.scanpolicy = scanpolicy;

  // ── applypolicy: enforce policy after predicate passes ────────────────────────
  // Handles sign (SGN/0xC0), cert (CRT/0xC1), open (NOA/0xC3), and no-policy.
  // PoW (0xC4) is handled in penStage before calling applypolicy.

  function applypolicy(policy, ctx, reject, writer) {
    var eve = ctx.eve,
      msg = ctx.msg;
    var chk = runtime.check;

    if (policy.cert) {
      var raw = {};
      try {
        raw = JSON.parse(ctx.val) || {};
      } catch (e) {}
      if (!raw["+"]) return reject("PEN: cert required");
      function verifyCert(signerPub) {
        if (!signerPub) return reject("PEN: cannot recover signer pub for cert verification");
        chk.$vfy(
          eve,
          msg,
          ctx.key,
          ctx.soul,
          policy.cert,
          reject,
          raw["+"],
          signerPub,
          function () {
            chk.next(eve, msg, reject);
          },
        );
      }
      if (writer) {
        // Writer pub is already known (new write with authenticator)
        verifyCert(writer);
      } else {
        // Peer re-propagation: recover from packed write signature.
        // Use ctx.put (the specific node for this soul/key), not msg.put (the full delta).
        runtime.opt.pack(ctx.put, function (packed) {
          runtime.recover(packed).then(verifyCert).catch(function () {
            reject("PEN: cannot recover signer pub for cert verification");
          });
        });
      }
      return;
    }

    if (policy.sign) {
      // Signature was already verified and msg.put updated in penStage before
      // the predicate ran — just forward.
      return chk.next(eve, msg, reject);
    }

    // open or no policy: forward directly without stringify
    eve.to.next(msg);
  }

  // ── penStage: pipeline stage for !-soul validation ────────────────────────────

  function penStage(ctx, next, reject) {
    // Cache-miss re-propagation: data was already verified on original write.
    // The PoW nonce is stripped by ham(), so re-verification would fail anyway.
    // This mirrors the faith fast-path that security.js uses for ~/* souls.
    var msgCtx = ctx.msg && ctx.msg._;
    var isMiss = msgCtx && msgCtx.miss;
    if (isMiss) {
      return next();
    }
    var soul = ctx.soul;
    var slashIdx = soul.indexOf("/");
    var pencode = slashIdx >= 0 ? soul.slice(1, slashIdx) : soul.slice(1);
    var pathpart = slashIdx >= 0 ? soul.slice(slashIdx + 1) : "";
    var bytecode;
    try {
      bytecode = pen.unpack(pencode);
    } catch (e) {
      return reject("PEN: invalid soul encoding");
    }
    if (!bytecode || bytecode.length < 2) return reject("PEN: empty bytecode");
    if (bytecode.length > 512) return reject("PEN: bytecode too large");

    var policy = scanpolicy(bytecode);
    var sec =
      runtime.check && runtime.check.$zen
        ? runtime.check.$zen(ctx.msg, (ctx.at && ctx.at.user) || "", null)
        : {};
    var writer = sec.upub || (sec.authenticator || {}).pub || "";

    // Auto-mine if opt.pow is a policy object {unit, difficulty} and no nonce yet.
    // Must be called AFTER signing so ctx.val is already the verified plaintext.
    // Peers re-propagating already have the nonce in ctx.put["^"], so this is a no-op for them.
    function mineIfNeeded(cb) {
      var pow = sec.opt && sec.opt.pow;
      if (pow && typeof pow === "object" && !ctx.put["^"]) {
        var proofBlock = JSON.stringify({
          "#": ctx.put["#"] || "",
          ".": ctx.put["."] || "",
          ":": ctx.val,
          ">": ctx.put[">"] || 0,
        });
        runtime.hash(
          function (nonce) { return proofBlock + ":" + nonce; },
          null,
          function (result) {
            ctx.put["^"] = result.nonce;
            cb();
          },
          {
            pow: {
              unit: pow.unit || "0",
              difficulty: pow.difficulty != null ? pow.difficulty : 3,
            },
            name: "SHA-256",
            encode: "hex",
          },
        );
      } else {
        cb();
      }
    }

    // Run bytecode predicates with ctx.val (must be verified plaintext by this point)
    // then hand off to applypolicy for final forwarding.
    function runPredicate() {
      var regs = [
        ctx.key,
        ctx.val,   // R[1]: verified plaintext when sign:true, raw otherwise
        soul,
        ctx.state || 0,
        Date.now(),
        writer,
        pathpart,     // R[6]: path after pencode/ in soul
        ctx.put["^"] || "", // R[7]: PoW nonce
      ];

      pen.ready.then(function () {
        var ok;
        try {
          ok = pen.run(bytecode, regs);
        } catch (e) {
          return reject("PEN VM: " + (e.message || e));
        }
        if (!ok) return reject("PEN: predicate failed");

        if (policy.pow) {
          // Verify SHA-256(canonical_block + ":" + nonce) meets difficulty.
          // The canonical block is {#, ., :, >} — the same data used during signing.
          // Binding nonce to the full block (soul + key + val + state) prevents
          // replay across different keys or values.
          var nonce = regs[7] || ""; // R[7] is always the PoW nonce
          var proofBlock = JSON.stringify({
            "#": ctx.put["#"] || "",
            ".": ctx.put["."] || "",
            ":": ctx.val,
            ">": ctx.put[">"] || 0,
          });
          var proof = proofBlock + ":" + nonce;
          return runtime.hash(
            proof,
            null,
            function (hash) {
              var punit = policy.pow.unit || "0";
              var pdiff =
                policy.pow.difficulty != null ? policy.pow.difficulty : 1;
              var prefix = punit.repeat(pdiff);
              if ((hash || "").indexOf(prefix) !== 0)
                return reject("PEN: PoW insufficient");
              applypolicy(policy, ctx, reject, writer);
            },
            { name: "SHA-256", encode: "hex" },
          );
        }

        applypolicy(policy, ctx, reject, writer);
      });
    }

    // For sign:true, verify and unpack BEFORE running the predicate so that:
    //   - ctx.val is the verified plaintext (not the raw signed blob)
    //   - msg.put[":"] and msg.put["="] are set consistently with check.auth
    if (policy.sign) {
      var chk = runtime.check;
      var msg = ctx.msg;

      if (sec.authenticator) {
        // New write: sign first, check.auth updates msg.put[":"] and msg.put["="]
        chk.auth(msg, reject, sec.authenticator, function (parsed) {
          ctx.val = parsed;
          mineIfNeeded(runPredicate);
        });
        return;
      }
      // Peer re-propagation: verify existing signature then unpack.
      // If check.auth already ran (new-write path), msg.put[":"] is an object
      // with a "~" signature field — use it directly without recovery.
      var putVal = ctx.put[":"];
      if (putVal && typeof putVal === "object" && putVal["~"]) {
        ctx.val = putVal[":"];
        mineIfNeeded(runPredicate);
        return;
      }
      runtime.opt.pack(ctx.put, function (packed) {
        // Fallback: raw signed string in the put — try public key recovery.
        runtime.recover(packed).then(function (signerPub) {
          runtime.verify(packed, signerPub || sec.upub || null, function (data) {
            data = runtime.opt.unpack(data);
            if (data === void 0) return reject("PEN: valid signature required");
            var sig = (packed && packed.s) || "";
            ctx.put[":"] = { ":": data, "~": sig, v: packed && packed.v, c: packed && packed.c };
            ctx.put["="] = data;
            ctx.val = data;
            mineIfNeeded(runPredicate);
          });
        }).catch(function () {
          reject("PEN: cannot recover signer pub");
        });
      });
      return;
    }

    mineIfNeeded(runPredicate);
  }

  if (runtime && runtime.check && runtime.check.use) {
    runtime.check.use(function (ctx, pipeline) {
      if (!ctx.soul || ctx.soul[0] !== "!") return;
      pipeline.splice(1, 0, penStage);
    });
  }

  // ── runtime.pen() — bytecode compiler ─────────────────────────────────────────
  // Compiles a high-level spec to bytecode and returns the soul string '!<base62>'
  //
  // spec.key   — expr to validate ctx.key   (R[0])
  // spec.val   — expr to validate ctx.val   (R[1])
  // spec.soul  — expr to validate ctx.soul  (R[2], full soul string)
  // spec.state — expr to validate ctx.state (R[3])
  // spec.path  — expr to validate path after pencode/ in soul (R[6])
  //              e.g. soul '!abc/foo/bar' → path 'foo/bar'
  //
  // expr formats:
  //   "string"                      → EQ(field, str)
  //   { eq, ne, pre, suf, inc }     → string predicates
  //   { lt, gt, lte, gte }          → numeric comparisons
  //   { and, or, not }              → logical combinators
  //   { type: 'string|number|...' } → ISS/ISN/ISX/ISB
  //   { length: [min, max] }        → LNG
  //   { seg: {sep,idx,of,match} }   → SEG
  //   { let: {bind,def,body} }      → LET
  //   { if: {cond,then,else} }      → IF
  //   { reg: n }                    → REG(n) for LET locals
  //   { divu, mod, add, sub, mul }  → arithmetic
  //   { tonum, tostr }              → coercion

  function compileExpr(x, field_reg) {
    if (x === undefined || x === null) return bc.pass();
    if (typeof x === "boolean") return x ? bc.pass() : bc.fail();
    if (typeof x === "number") return bc.intn(x);
    if (typeof x === "string") return bc.eq(field_reg, bc.str(x));

    var r = field_reg;

    if (x.and)
      return bc.and(
        x.and.map(function (e) {
          return compileExpr(e, r);
        }),
      );
    if (x.or)
      return bc.or(
        x.or.map(function (e) {
          return compileExpr(e, r);
        }),
      );
    if (x.not) return bc.not(compileExpr(x.not, r));

    if (x.eq !== undefined) {
      if (Array.isArray(x.eq))
        return bc.eq(compileVal(x.eq[0]), compileVal(x.eq[1]));
      return bc.eq(r, bc.str(String(x.eq)));
    }
    if (x.ne !== undefined) {
      if (Array.isArray(x.ne))
        return bc.ne(compileVal(x.ne[0]), compileVal(x.ne[1]));
      return bc.ne(r, bc.str(String(x.ne)));
    }
    if (x.pre !== undefined) return bc.pre(r, bc.str(String(x.pre)));
    if (x.suf !== undefined) return bc.suf(r, bc.str(String(x.suf)));
    if (x.inc !== undefined) return bc.inc(r, bc.str(String(x.inc)));

    if (x.lt !== undefined) {
      if (Array.isArray(x.lt))
        return bc.lt(compileVal(x.lt[0]), compileVal(x.lt[1]));
      return bc.lt(r, bc.uint(x.lt));
    }
    if (x.gt !== undefined) {
      if (Array.isArray(x.gt))
        return bc.gt(compileVal(x.gt[0]), compileVal(x.gt[1]));
      return bc.gt(r, bc.uint(x.gt));
    }
    if (x.lte !== undefined) {
      if (Array.isArray(x.lte))
        return bc.lte(compileVal(x.lte[0]), compileVal(x.lte[1]));
      return bc.lte(r, bc.uint(x.lte));
    }
    if (x.gte !== undefined) {
      if (Array.isArray(x.gte))
        return bc.gte(compileVal(x.gte[0]), compileVal(x.gte[1]));
      return bc.gte(r, bc.uint(x.gte));
    }

    if (x.type) {
      var t = x.type;
      if (t === "string") return bc.iss(r);
      if (t === "number") return bc.isn(r);
      if (t === "null") return bc.isx(r);
      if (t === "bool") return bc.isb(r);
    }

    if (x.length) return bc.lng(r, x.length[0], x.length[1]);

    if (x.seg) {
      var s = x.seg;
      var src = s.of ? compileVal(s.of) : r;
      var segexpr = bc.seg(src, s.sep || "_", compileVal({ num: s.idx || 0 }));
      if (s.match) return compileExpr(s.match, segexpr);
      return segexpr;
    }

    if (x.let) {
      var l = x.let;
      return bc.let_(l.bind, compileVal(l.def), compileExpr(l.body, r));
    }

    if (x.if) {
      return bc.if_(
        compileExpr(x.if.cond, r),
        compileExpr(x.if.then, r),
        compileExpr(x.if.else, r),
      );
    }

    if (x.reg !== undefined) return bc.reg(x.reg);

    if (x.divu) return bc.divu(compileVal(x.divu[0]), compileVal(x.divu[1]));
    if (x.mod) return bc.mod(compileVal(x.mod[0]), compileVal(x.mod[1]));
    if (x.add) return bc.add(compileVal(x.add[0]), compileVal(x.add[1]));
    if (x.sub) return bc.sub(compileVal(x.sub[0]), compileVal(x.sub[1]));
    if (x.mul) return bc.mul(compileVal(x.mul[0]), compileVal(x.mul[1]));

    if (x.tonum) return bc.tonum(compileVal(x.tonum));
    if (x.tostr) return bc.tostr(compileVal(x.tostr));

    return bc.pass();
  }

  function compileVal(x) {
    if (typeof x === "number") return bc.uint(x);
    if (typeof x === "string") return bc.str(x);
    if (x && x.reg !== undefined) return bc.reg(x.reg);
    if (x && x.divu)
      return bc.divu(compileVal(x.divu[0]), compileVal(x.divu[1]));
    if (x && x.mod) return bc.mod(compileVal(x.mod[0]), compileVal(x.mod[1]));
    if (x && x.add) return bc.add(compileVal(x.add[0]), compileVal(x.add[1]));
    if (x && x.sub) return bc.sub(compileVal(x.sub[0]), compileVal(x.sub[1]));
    if (x && x.tonum) return bc.tonum(compileVal(x.tonum));
    if (x && x.seg) {
      var s = x.seg;
      var src = s.of ? compileVal(s.of) : bc.r0();
      return bc.seg(src, s.sep || "_", compileVal({ num: s.idx || 0 }));
    }
    if (x && x.num !== undefined)
      return bc.intn(x.num) ? bc.intn(x.num) : bc.uint(x.num);
    return bc.null_();
  }

  runtime.pen = function (spec) {
    var parts = [];

    if (spec.key) parts.push(compileExpr(spec.key, bc.r0()));
    if (spec.val) parts.push(compileExpr(spec.val, bc.r1()));
    if (spec.soul) parts.push(compileExpr(spec.soul, bc.r2()));
    if (spec.state) parts.push(compileExpr(spec.state, bc.r3()));
    if (spec.path) parts.push(compileExpr(spec.path, bc.r6()));

    var root =
      parts.length === 0
        ? bc.pass()
        : parts.length === 1
          ? parts[0]
          : bc.and(parts);

    // Build predicate bytecode, then append tail bytes AFTER expression root.
    // Tail bytes (0xC0..) are unreachable by WASM VM (which stops after root
    // expression), and are extracted by scanpolicy() on the ZEN bridge layer.
    var pred = Array.from(bc.prog(root));

    if (spec.sign) pred.push(0xc0);
    if (spec.cert) {
      var cpub = Array.from(_enc.encode(String(spec.cert).slice(0, 255)));
      pred.push(0xc1, cpub.length);
      for (var ci = 0; ci < cpub.length; ci++) pred.push(cpub[ci]);
    }
    if (spec.open) pred.push(0xc3);
    if (spec.pow) {
      var powfield = 7; // R[7] is always reserved for PoW nonce; field is not user-configurable
      var powdiff =
        (spec.pow.difficulty != null ? spec.pow.difficulty : 1) & 0xff;
      var powunit = spec.pow.unit ? String(spec.pow.unit).slice(0, 255) : "";
      var powubytes = Array.from(_enc.encode(powunit));
      pred.push(0xc4, powfield, powdiff, powubytes.length);
      for (var pi = 0; pi < powubytes.length; pi++) pred.push(powubytes[pi]);
    }
    return "!" + pen.pack(new Uint8Array(pred));
  };

  // ── runtime.candle() — temporal window helper ─────────────────────────────────
  // Returns an expr (for use in spec.key) that validates the candle number
  // embedded at a key segment is within [current - back, current + fwd].
  //
  // opts: { seg: 0, sep: '_', size: 300000, back: 100, fwd: 2 }

  runtime.candle = function (opts) {
    var seg = opts.seg !== undefined ? opts.seg : 0;
    var sep = opts.sep || "_";
    var size = opts.size || 300000;
    var back = opts.back !== undefined ? opts.back : 100;
    var fwd = opts.fwd !== undefined ? opts.fwd : 2;

    return {
      let: {
        bind: 0,
        def: { divu: [{ reg: 4 }, size] }, // local[0] = floor(now / size)
        body: {
          let: {
            bind: 1,
            def: { tonum: { seg: { sep: sep, idx: seg, of: { reg: 0 } } } }, // local[1] = candle from key
            body: {
              and: [
                { gte: [{ reg: 129 }, { sub: [{ reg: 128 }, back] }] },
                { lte: [{ reg: 129 }, { add: [{ reg: 128 }, fwd] }] },
              ],
            },
          },
        },
      },
    };
  };

  pen.pen = function (spec) {
    return runtime.pen(spec);
  };
  pen.candle = function (opts) {
    return runtime.candle(opts);
  };
}
export default pen;
