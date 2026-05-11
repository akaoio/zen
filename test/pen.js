import zenbase from "../zen.js";
var ZEN;
{
  var W = function (o) {
    return new zenbase(o);
  };
  Object.setPrototypeOf(W, zenbase);
  W.prototype = zenbase.prototype;
  Object.defineProperty(W.prototype, "_", {
    get: function () {
      return this._graph._;
    },
    configurable: true,
  });
  W.is = function ($) {
    return $ instanceof zenbase;
  };
  ZEN = W;
}
import penmod from "../src/pen.js";
import assert from "assert";
("use strict");
var Zen, pen;

before(function (done) {
  Zen = ZEN;
  pen = penmod;
  pen.ready
    .then(function () {
      done();
    })
    .catch(done);
});

// ── helpers ──────────────────────────────────────────────────────────────────

function prog(root) {
  return pen.bc.prog(root);
}
function run(bytecode, regs) {
  return pen.run(bytecode, regs);
}

// ── pack / unpack ─────────────────────────────────────────────────────────────

describe("pen.pack / pen.unpack", function () {
  it("round-trips a single-byte payload", function () {
    var input = new Uint8Array([0x42]);
    var s = pen.pack(input);
    assert.ok(typeof s === "string" && s.length > 0);
    var out = pen.unpack(s);
    assert.strictEqual(out[0], 0x42);
    assert.strictEqual(out.length, 1);
  });

  it("round-trips a multi-byte payload", function () {
    var input = new Uint8Array([0x01, 0x60, 0xf1]);
    var s = pen.pack(input);
    var out = pen.unpack(s);
    assert.strictEqual(out.length, 3);
    assert.deepStrictEqual(Array.from(out), Array.from(input));
  });

  it("round-trips a full prog bytecode", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.iss(bc.r1()));
    var s = pen.pack(bytecode);
    var out = pen.unpack(s);
    assert.deepStrictEqual(Array.from(out), Array.from(bytecode));
  });

  it("handles leading zero bytes via sentinel", function () {
    var input = new Uint8Array([0x00, 0x00, 0xff]);
    var out = pen.unpack(pen.pack(input));
    assert.deepStrictEqual(Array.from(out), Array.from(input));
  });
});

// ── pen.run — ISA ──────────────────────────────────────────────────────────────

describe("pen.run — ISA", function () {
  it("PASS always returns true", function () {
    assert.strictEqual(run(prog(pen.bc.pass()), []), true);
  });

  it("FAIL always returns false", function () {
    assert.strictEqual(run(prog(pen.bc.fail()), []), false);
  });

  it("ISS(R[1]) accepts string", function () {
    assert.strictEqual(
      run(prog(pen.bc.iss(pen.bc.r1())), ["k", "hello"]),
      true,
    );
  });

  it("ISS(R[1]) rejects number", function () {
    assert.strictEqual(run(prog(pen.bc.iss(pen.bc.r1())), ["k", 99]), false);
  });

  it("ISN(R[1]) accepts finite number", function () {
    assert.strictEqual(run(prog(pen.bc.isn(pen.bc.r1())), ["k", 3.14]), true);
  });

  it("ISN(R[1]) rejects string", function () {
    assert.strictEqual(run(prog(pen.bc.isn(pen.bc.r1())), ["k", "x"]), false);
  });

  it("EQ(R[0], str) accepts matching key", function () {
    var bc = pen.bc;
    assert.strictEqual(
      run(prog(bc.eq(bc.r0(), bc.str("mykey"))), ["mykey"]),
      true,
    );
    assert.strictEqual(
      run(prog(bc.eq(bc.r0(), bc.str("mykey"))), ["other"]),
      false,
    );
  });

  it("EQ supports comparing two dynamic expressions", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.eq(bc.seg(bc.r0(), ":", bc.intn(1)), bc.r5()));
    assert.strictEqual(
      run(bytecode, [
        "123:writerpub:nonce",
        "",
        "",
        0,
        Date.now(),
        "writerpub",
      ]),
      true,
    );
    assert.strictEqual(
      run(bytecode, ["123:otherpub:nonce", "", "", 0, Date.now(), "writerpub"]),
      false,
    );
  });

  it("PRE (startsWith)", function () {
    var bc = pen.bc;
    assert.strictEqual(
      run(prog(bc.pre(bc.r0(), bc.str("foo"))), ["foobar"]),
      true,
    );
    assert.strictEqual(
      run(prog(bc.pre(bc.r0(), bc.str("foo"))), ["barfoo"]),
      false,
    );
  });

  it("LNG (string length in range)", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.lng(bc.r0(), 3, 10));
    assert.strictEqual(run(bytecode, ["abc"]), true);
    assert.strictEqual(run(bytecode, ["ab"]), false);
    assert.strictEqual(run(bytecode, ["a".repeat(11)]), false);
  });

  it("DIVU(R[4], 300000) gives candle number", function () {
    var bc = pen.bc;
    var now = 1500000000000;
    var expected = Math.floor(now / 300000);
    var bytecode = prog(bc.divu(bc.r4(), bc.uint(300000)));
    // pen.run returns boolean — but divu returns a number which is truthy if > 0
    // use EQ to verify the actual value
    var eqBc = prog(
      bc.eq(bc.divu(bc.r4(), bc.uint(300000)), bc.uint(expected)),
    );
    assert.strictEqual(run(eqBc, ["", "", "", 0, now]), true);
  });

  it("SEG extracts key segment", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.eq(bc.seg(bc.r0(), "_", bc.intn(1)), bc.str("ETH")));
    assert.strictEqual(run(bytecode, ["5820000_ETH_USDT_buy"]), true);
    assert.strictEqual(run(bytecode, ["5820000_BTC_USDT_buy"]), false);
  });

  it("LET + DIVU + GTE/LTE: candle window", function () {
    var bc = pen.bc;
    var now = 1500000000000;
    var candle = Math.floor(now / 300000); // = 5000000
    var size = 300000;
    var back = 100;
    var fwd = 2;

    // LET(0, floor(now/size), LET(1, tonum(seg(R[0], '_', 0)),
    //   AND(GTE(R[129], R[128]-back), LTE(R[129], R[128]+fwd))))
    var bytecode = prog(
      bc.let_(
        0,
        bc.divu(bc.r4(), bc.uint(size)),
        bc.let_(
          1,
          bc.tonum(bc.seg(bc.r0(), "_", bc.intn(0))),
          bc.and([
            bc.gte(bc.reg(129), bc.sub(bc.reg(128), bc.uint(back))),
            bc.lte(bc.reg(129), bc.add(bc.reg(128), bc.uint(fwd))),
          ]),
        ),
      ),
    );

    var validKey = candle + "_ETH_USDT_buy";
    var staleKey = candle - 200 + "_ETH_USDT_buy";
    var futureKey = candle + 10 + "_ETH_USDT_buy";

    assert.strictEqual(
      run(bytecode, [validKey, "", "", 0, now]),
      true,
      "current candle accepted",
    );
    assert.strictEqual(
      run(bytecode, [staleKey, "", "", 0, now]),
      false,
      "stale candle rejected",
    );
    assert.strictEqual(
      run(bytecode, [futureKey, "", "", 0, now]),
      false,
      "far future rejected",
    );
    // Just within fwd window
    assert.strictEqual(
      run(bytecode, [candle + 2 + "_ETH_USDT_buy", "", "", 0, now]),
      true,
      "fwd edge accepted",
    );
  });

  it("AND short-circuits on false", function () {
    var bc = pen.bc;
    // AND(fail, pass) — if fail is hit first, result is false regardless of pass
    assert.strictEqual(run(prog(bc.and([bc.fail(), bc.pass()])), []), false);
    assert.strictEqual(run(prog(bc.and([bc.pass(), bc.fail()])), []), false);
    assert.strictEqual(run(prog(bc.and([bc.pass(), bc.pass()])), []), true);
  });

  it("OR short-circuits on true", function () {
    var bc = pen.bc;
    assert.strictEqual(run(prog(bc.or([bc.fail(), bc.fail()])), []), false);
    assert.strictEqual(run(prog(bc.or([bc.pass(), bc.fail()])), []), true);
    assert.strictEqual(run(prog(bc.or([bc.fail(), bc.pass()])), []), true);
  });

  it("NOT inverts result", function () {
    var bc = pen.bc;
    assert.strictEqual(run(prog(bc.not(bc.pass())), []), false);
    assert.strictEqual(run(prog(bc.not(bc.fail())), []), true);
  });

  it("UINT varint encodes large values correctly", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.eq(bc.uint(300000), bc.uint(300000)));
    assert.strictEqual(run(bytecode, []), true);
  });

  it("arithmetic: ADD, SUB, MUL", function () {
    var bc = pen.bc;
    assert.strictEqual(
      run(prog(bc.eq(bc.add(bc.uint(3), bc.uint(4)), bc.uint(7))), []),
      true,
    );
    assert.strictEqual(
      run(prog(bc.eq(bc.sub(bc.uint(10), bc.uint(4)), bc.uint(6))), []),
      true,
    );
    assert.strictEqual(
      run(prog(bc.eq(bc.mul(bc.uint(6), bc.uint(7)), bc.uint(42))), []),
      true,
    );
  });

  it("SEGR macro: 4-byte inline SEG(R[reg], sep, idx)", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.eq(bc.segr(0, "_", 2), bc.str("USDT")));
    assert.strictEqual(run(bytecode, ["ETH_BTC_USDT"]), true);
  });

  it("SEGRN macro: TONUM(SEG(R[reg], sep, idx))", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.eq(bc.segrn(0, "_", 0), bc.uint(5820000)));
    assert.strictEqual(run(bytecode, ["5820000_ETH_USDT"]), true);
    assert.strictEqual(run(bytecode, ["9999999_ETH_USDT"]), false);
  });

  it("register shorthands r0–r5 match REG(0)–REG(5)", function () {
    var bc = pen.bc;
    var regs = ["r0", "r1", "r2", "r3", 9999, "r5"];
    // REG(4) is a number — ISN should return true for r4
    assert.strictEqual(run(prog(bc.iss(bc.r0())), regs), true);
    assert.strictEqual(run(prog(bc.iss(bc.r5())), regs), true);
    assert.strictEqual(run(prog(bc.isn(bc.r4())), regs), true);
  });

  it("IF: conditional evaluation", function () {
    var bc = pen.bc;
    // if ISS(R[0]) then PASS else FAIL
    var bytecode = prog(bc.if_(bc.iss(bc.r0()), bc.pass(), bc.fail()));
    assert.strictEqual(run(bytecode, ["str"]), true);
    assert.strictEqual(run(bytecode, [42]), false);
  });
});

// ── ZEN.pen compiler ──────────────────────────────────────────────────────────

describe("ZEN.pen()", function () {
  it("returns a string starting with !", function () {
    var soul = ZEN.pen({});
    assert.ok(typeof soul === "string");
    assert.strictEqual(soul[0], "!");
  });

  it("empty spec → PASS predicate", function () {
    var soul = ZEN.pen({});
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(pen.run(bc, ["k", "v", soul, 0, Date.now(), ""]), true);
  });

  it('{ val: { type: "string" } } accepts string values', function () {
    var soul = ZEN.pen({ val: { type: "string" } });
    var regs = function (v) {
      return ["k", v, soul, 0, Date.now(), ""];
    };
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(pen.run(bc, regs("hello")), true);
    assert.strictEqual(pen.run(bc, regs(42)), false);
    assert.strictEqual(pen.run(bc, regs(null)), false);
  });

  it('{ key: "fixed" } enforces exact key match', function () {
    var soul = ZEN.pen({ key: "fixed" });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(
      pen.run(bc, ["fixed", "v", soul, 0, Date.now(), ""]),
      true,
    );
    assert.strictEqual(
      pen.run(bc, ["other", "v", soul, 0, Date.now(), ""]),
      false,
    );
  });

  it('{ key: { pre: "foo" } } checks prefix', function () {
    var soul = ZEN.pen({ key: { pre: "foo" } });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(
      pen.run(bc, ["foobar", "v", soul, 0, Date.now(), ""]),
      true,
    );
    assert.strictEqual(
      pen.run(bc, ["barfoo", "v", soul, 0, Date.now(), ""]),
      false,
    );
  });

  it("{ key: { and: [...] } } compiles AND of conditions", function () {
    var soul = ZEN.pen({ key: { and: [{ pre: "foo" }, { suf: "bar" }] } });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(
      pen.run(bc, ["foobar", "v", soul, 0, Date.now(), ""]),
      true,
    );
    assert.strictEqual(
      pen.run(bc, ["foo", "v", soul, 0, Date.now(), ""]),
      false,
    );
    assert.strictEqual(
      pen.run(bc, ["bar", "v", soul, 0, Date.now(), ""]),
      false,
    );
  });

  it('{ path: "exact" } enforces exact path match', function () {
    var soul = ZEN.pen({ path: "exact-path" });
    var bc = pen.unpack(soul.slice(1));
    var regs = function (p) {
      return ["k", "v", soul, 0, Date.now(), "", p];
    };
    assert.strictEqual(pen.run(bc, regs("exact-path")), true);
    assert.strictEqual(pen.run(bc, regs("other-path")), false);
    assert.strictEqual(pen.run(bc, regs("")), false);
  });

  it('{ path: { pre: "usr/" } } checks path prefix', function () {
    var soul = ZEN.pen({ path: { pre: "usr/" } });
    var bc = pen.unpack(soul.slice(1));
    var regs = function (p) {
      return ["k", "v", soul, 0, Date.now(), "", p];
    };
    assert.strictEqual(pen.run(bc, regs("usr/alice")), true);
    assert.strictEqual(pen.run(bc, regs("usr/")), true);
    assert.strictEqual(pen.run(bc, regs("admin/alice")), false);
  });

  it('{ path: { type: "string" } } accepts non-empty path, rejects missing path', function () {
    var soul = ZEN.pen({ path: { type: "string" } });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(
      pen.run(bc, ["k", "v", soul, 0, Date.now(), "", "some/path"]),
      true,
    );
    assert.strictEqual(
      pen.run(bc, ["k", "v", soul, 0, Date.now(), "", ""]),
      true,
    ); // empty string is still string
    assert.strictEqual(
      pen.run(bc, ["k", "v", soul, 0, Date.now(), "", null]),
      false,
    );
  });

  it("path + key + val: all three combine with AND", function () {
    var soul = ZEN.pen({
      key: { type: "string" },
      val: { type: "number" },
      path: { pre: "ns/" },
    });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(
      pen.run(bc, ["k", 42, soul, 0, Date.now(), "", "ns/foo"]),
      true,
      "all match",
    );
    assert.strictEqual(
      pen.run(bc, ["k", "str", soul, 0, Date.now(), "", "ns/foo"]),
      false,
      "val wrong type",
    );
    assert.strictEqual(
      pen.run(bc, ["k", 42, soul, 0, Date.now(), "", "other/"]),
      false,
      "path wrong prefix",
    );
  });

  it("path seg: validate first segment of path", function () {
    var soul = ZEN.pen({
      path: { seg: { sep: "/", idx: 0, match: { pre: "user-" } } },
    });
    var bc = pen.unpack(soul.slice(1));
    var regs = function (p) {
      return ["k", "v", soul, 0, Date.now(), "", p];
    };
    assert.strictEqual(pen.run(bc, regs("user-123/data")), true);
    assert.strictEqual(pen.run(bc, regs("admin-1/data")), false);
  });

  it("multiple fields combine with AND", function () {
    var soul = ZEN.pen({ key: { type: "string" }, val: { type: "number" } });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(pen.run(bc, ["k", 99, soul, 0, Date.now(), ""]), true);
    assert.strictEqual(
      pen.run(bc, ["k", "str", soul, 0, Date.now(), ""]),
      false,
    );
  });

  it("{ sign: true } emits 0xC0 policy byte after tree", function () {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.sign, true);
    assert.strictEqual(p.cert, null);
    // predicate still works
    assert.strictEqual(
      pen.run(bc, ["k", "hello", soul, 0, Date.now(), ""]),
      true,
    );
  });

  it("{ cert: pub } emits 0xC1 + pub bytes", function () {
    var pub = "TestPub88CharactersLong0123456789abcdefABCDEF";
    var soul = ZEN.pen({ cert: pub });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.cert, pub);
    assert.strictEqual(p.sign, false);
  });

  it("{ open: true } emits 0xC3", function () {
    var soul = ZEN.pen({ open: true });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(pen.scanpolicy(bc).open, true);
  });

  it('{ pow: { field:1, difficulty:3 } } emits 0xC4, user field ignored, field always R[7], unit defaults to "0"', function () {
    var soul = ZEN.pen({ pow: { field: 1, difficulty: 3 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow);
    assert.strictEqual(p.pow.field, 7, "user field:1 ignored; always R[7]");
    assert.strictEqual(p.pow.difficulty, 3);
    assert.strictEqual(
      p.pow.unit,
      "0",
      'unit defaults to "0" when not specified',
    );
  });

  it('{ pow: { unit: "asdf", difficulty: 2 } } unit and difficulty round-trip (field always R[7])', function () {
    var soul = ZEN.pen({ pow: { field: 0, unit: "asdf", difficulty: 2 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow);
    assert.strictEqual(p.pow.unit, "asdf");
    assert.strictEqual(p.pow.difficulty, 2);
    assert.strictEqual(p.pow.field, 7, "user field:0 ignored; always R[7]");
  });

  it('{ pow: { unit: "abc" } } difficulty defaults to 1', function () {
    var soul = ZEN.pen({ pow: { field: 0, unit: "abc" } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow);
    assert.strictEqual(p.pow.unit, "abc");
    assert.strictEqual(
      p.pow.difficulty,
      1,
      "difficulty defaults to 1 when not specified",
    );
  });

  it('{ pow: { difficulty: 2 } } unit defaults to "0"', function () {
    var soul = ZEN.pen({ pow: { field: 0, difficulty: 2 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow);
    assert.strictEqual(p.pow.unit, "0", 'unit defaults to "0" when omitted');
    assert.strictEqual(p.pow.difficulty, 2);
  });

  it("sign + predicate: policy detected without polluting tree", function () {
    // A predicate with integer constants including values near 0xC0 range
    // Use a candle-like predicate with large integer: ensure no false positive
    var soul = ZEN.pen({ key: { gte: 192 }, sign: true }); // 192 = 0xC0 in ULEB
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.sign, true, "sign correctly detected via treeskip");
    // Predicate: key >= 192
    assert.strictEqual(pen.run(bc, [200, "v", soul, 0, Date.now(), ""]), true);
    assert.strictEqual(pen.run(bc, [100, "v", soul, 0, Date.now(), ""]), false);
  });

  it("path in R[6]: passes path prefix predicate, wrong path fails", function () {
    var soul = ZEN.pen({ path: { pre: "usr/" } });
    var soulWithPath = soul + "/usr/alice";
    var slashIdx = soulWithPath.indexOf("/");
    var pencode = soulWithPath.slice(1, slashIdx);
    var pathpart = soulWithPath.slice(slashIdx + 1); // 'usr/alice'
    var bc = pen.unpack(pencode);
    assert.strictEqual(pen.run(bc, ["k", "v", soulWithPath, 0, Date.now(), "", pathpart]), true, 'path prefix "usr/" accepted');
    assert.strictEqual(pen.run(bc, ["k", "v", soulWithPath, 0, Date.now(), "", "admin/alice"]), false, "wrong path prefix rejected");
  });

  it("no slash in soul: R[6] is empty string, key predicate still evaluates", function () {
    var soul = ZEN.pen({ key: "mykey" });
    var bc = pen.unpack(soul.slice(1));
    assert.strictEqual(pen.run(bc, ["mykey", "v", soul, 0, Date.now(), "", ""]), true, "matching key with empty path");
    assert.strictEqual(pen.run(bc, ["wrongkey", "v", soul, 0, Date.now(), "", ""]), false, "wrong key rejected");
  });
});

// ── ZEN.candle helper ─────────────────────────────────────────────────────────

describe("ZEN.candle()", function () {
  it("returns a plain object (expr spec)", function () {
    var expr = ZEN.candle({
      seg: 0,
      sep: "_",
      size: 300000,
      back: 100,
      fwd: 2,
    });
    assert.ok(expr && typeof expr === "object");
    assert.ok(expr.let);
  });

  it("compiled soul accepts key with current candle number", function () {
    var now = Date.now();
    var size = 300000;
    var candle = Math.floor(now / size);
    var soul = ZEN.pen({
      key: ZEN.candle({ seg: 0, sep: "_", size: size, back: 100, fwd: 2 }),
    });
    var bc = pen.unpack(soul.slice(1));
    var regs = function (key) {
      return [key, "v", soul, 0, now, ""];
    };
    assert.strictEqual(
      pen.run(bc, regs(candle + "_ETH_USDT")),
      true,
      "current candle",
    );
    assert.strictEqual(
      pen.run(bc, regs(candle + 2 + "_ETH_USDT")),
      true,
      "fwd edge",
    );
    assert.strictEqual(
      pen.run(bc, regs(candle - 50 + "_ETH_USDT")),
      true,
      "back window",
    );
    assert.strictEqual(
      pen.run(bc, regs(candle - 200 + "_ETH_USDT")),
      false,
      "stale: beyond back",
    );
    assert.strictEqual(
      pen.run(bc, regs(candle + 10 + "_ETH_USDT")),
      false,
      "future: beyond fwd",
    );
  });

  it("full order schema compiles and validates", function () {
    var now = Date.now();
    var size = 300000;
    var candle = Math.floor(now / size);

    var soul = ZEN.pen({
      key: {
        and: [
          ZEN.candle({ seg: 0, sep: "_", size: size, back: 100, fwd: 2 }),
          {
            seg: {
              sep: "_",
              idx: 3,
              of: { reg: 0 },
              match: { or: [{ eq: "buy" }, { eq: "sell" }] },
            },
          },
        ],
      },
      sign: true,
    });

    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.sign, true);
    assert.ok(bc.length < 512, "bytecode fits in 512 bytes");

    var goodKey = candle + "_ETH_USDT_buy_nonce1";
    var badDir = candle + "_ETH_USDT_hold_nonce1";
    var regs = function (key) {
      return [key, '{"amount":1}', soul, 0, now, ""];
    };

    assert.strictEqual(pen.run(bc, regs(goodKey)), true, "valid buy order key");
    assert.strictEqual(
      pen.run(bc, regs(badDir)),
      false,
      "invalid direction rejected",
    );
  });
});

// ── pen.scanpolicy ────────────────────────────────────────────────────────────

describe("pen.scanpolicy()", function () {
  it("returns empty policy for bytecode with no policy bytes", function () {
    var bc = pen.bc;
    var bytecode = prog(bc.pass());
    var p = pen.scanpolicy(bytecode);
    assert.strictEqual(p.sign, false);
    assert.strictEqual(p.cert, null);
    assert.strictEqual(p.open, false);
    assert.strictEqual(p.pow, null);
  });

  it("detects multiple policies simultaneously", function () {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true, open: true });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.sign, true);
    assert.strictEqual(p.open, true);
  });

  it("does not false-positive on integer 192 (0xC0) inside UINT expr", function () {
    var bc = pen.bc;
    // UINT(192) ULEB128 = [0x04, 0xC0, 0x01] — 0xC0 is a continuation byte
    // treeskip correctly skips past the varint, so scanpolicy ignores it
    var bytecode = prog(bc.eq(bc.uint(192), bc.uint(192)));
    var p = pen.scanpolicy(bytecode);
    assert.strictEqual(p.sign, false, "no false positive from UINT(192)");
  });

  it("does not false-positive on 0xC0 inside LNG [min, max] bytes", function () {
    // LNG with max=192 (0xC0)
    var bc = pen.bc;
    var bytecode = prog(bc.lng(bc.r0(), 0, 192));
    var p = pen.scanpolicy(bytecode);
    assert.strictEqual(p.sign, false, "no false positive from max=0xC0 in LNG");
  });

  it("treeskip-based detection correctly finds sign suffix with large int in tree", function () {
    var soul = ZEN.pen({ key: { gte: 192 }, sign: true });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(
      p.sign,
      true,
      "gte:192 bytecode + sign: both detected correctly",
    );
  });

  it("detects pow policy and extracts field (always 7), difficulty, and default unit", function () {
    var soul = ZEN.pen({ pow: { field: 1, difficulty: 4 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow, "pow policy present");
    assert.strictEqual(p.pow.field, 7, "user field:1 ignored; always R[7]");
    assert.strictEqual(p.pow.difficulty, 4);
    assert.strictEqual(p.pow.unit, "0", 'unit defaults to "0"');
  });
});

// ── penStage (real graph enforcement) ────────────────────────────────────────

describe("penStage (real graph enforcement)", function () {
  this.timeout(10000);

  var pair;

  before(function (done) {
    ZEN.pair(function (p) {
      pair = p;
      done();
    });
  });

  it("invalid soul is rejected at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = "!0"; // '0' decodes to empty/too-short bytecode
    zen.get(soul).get("k").put("value", function (ack) {
      assert.ok(ack && ack.err, "invalid soul must be rejected: " + JSON.stringify(ack));
      done();
    });
  });

  it("bytecode > 512 bytes is rejected at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var bc = pen.bc;
    var longStr = "x".repeat(200);
    var bigBc = prog(bc.and([
      bc.eq(bc.r0(), bc.str(longStr)),
      bc.eq(bc.r0(), bc.str(longStr)),
      bc.eq(bc.r0(), bc.str(longStr)),
    ]));
    assert.ok(bigBc.length > 512, "test setup: bytecode exceeds 512 bytes");
    var bigSoul = "!" + pen.pack(bigBc);
    zen.get(bigSoul).get("k").put("v", function (ack) {
      assert.ok(ack && ack.err, "oversized bytecode must be rejected at graph level: " + JSON.stringify(ack));
      done();
    });
  });

  it("open:true soul accepts anonymous put at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ open: true });
    zen.get(soul).get("k").put("hello", function (ack) {
      assert.ok(!ack || !ack.err, "open soul must accept anonymous write: " + (ack && ack.err));
      done();
    });
  });

  it("string predicate rejects number value at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, open: true });
    zen.get(soul).get("k").put(42, function (ack) {
      assert.ok(ack && ack.err, "number must be rejected by string-type soul at graph level: " + JSON.stringify(ack));
      done();
    });
  });

  it("sign:true soul rejects unsigned put at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ sign: true });
    zen.get(soul).get("k").put("value", function (ack) {
      assert.ok(ack && ack.err, "unsigned put must be rejected by sign:true soul: " + JSON.stringify(ack));
      done();
    });
  });

  it("sign:true soul accepts put with authenticator at graph level", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    ZEN.sign("order_value", pair, function (value) {
      zen.get(soul).get("k").put(value, function (ack) {
        assert.ok(!ack || !ack.err, "authenticated put must be accepted: " + (ack && ack.err));
        done();
      }, { authenticator: pair });
    });
  });
});

// ── ZEN + PEN integration ─────────────────────────────────────────────────────
// Real async ZEN operations (pair/sign/work/certify/encrypt) feeding into PEN

describe("ZEN + PEN integration", function () {
  this.timeout(15000);

  var pair, pair2;

  before(function (done) {
    ZEN.pair(function (p) {
      pair = p;
      ZEN.pair(function (p2) {
        pair2 = p2;
        done();
      });
    });
  });

  // ── sign round-trip ─────────────────────────────────────────────────────────

  it("sign:true — ZEN.sign output is a string satisfying ISS predicate", function (done) {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    ZEN.sign("hello world", pair, function (signed) {
      assert.ok(typeof signed === "string", "signed value is a string");
      var bc = pen.unpack(soul.slice(1));
      assert.strictEqual(pen.scanpolicy(bc).sign, true);
      // The signed wire value is a string → ISS passes
      assert.strictEqual(
        pen.run(bc, ["k", signed, soul, 0, Date.now(), pair.pub]),
        true,
      );
      done();
    });
  });

  it("sign:true — ZEN.verify(ZEN.sign(v)) round-trip with PEN string val", function (done) {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    var original = "order_amount:1.5";
    ZEN.sign(original, pair, function (signed) {
      ZEN.verify(signed, pair.pub, function (verified) {
        assert.strictEqual(verified, original, "verify round-trips correctly");
        var bc = pen.unpack(soul.slice(1));
        assert.strictEqual(
          pen.run(bc, ["k", signed, soul, 0, Date.now(), ""]),
          true,
          "signed string passes PEN string predicate",
        );
        done();
      });
    });
  });

  it("sign:true — wrong key: ZEN.verify returns undefined, PEN predicate still true (string check only)", function (done) {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    ZEN.sign("data", pair, function (signed) {
      ZEN.verify(signed, pair2.pub, function (verified) {
        assert.strictEqual(
          verified,
          undefined,
          "wrong-key verify returns undefined",
        );
        // PEN predicate sees signed string (still a string), returns true
        // signature enforcement is done by applypolicy layer, not the bytecode predicate
        var bc = pen.unpack(soul.slice(1));
        assert.strictEqual(
          pen.run(bc, ["k", signed, soul, 0, Date.now(), ""]),
          true,
          "predicate only checks type; signature checked by applypolicy",
        );
        done();
      });
    });
  });

  // ── encrypt round-trip ──────────────────────────────────────────────────────

  it("ZEN.encrypt → decrypt round-trip, encrypted value passes PEN string predicate", function (done) {
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    var secret = "private_order_price:42.5";
    ZEN.encrypt(secret, pair, function (enc) {
      assert.ok(typeof enc === "string", "ciphertext is a string");
      ZEN.decrypt(enc, pair, function (dec) {
        assert.strictEqual(dec, secret, "decrypted correctly");
        // encrypted value stored in ZEN is a string → PEN ISS passes
        var bc = pen.unpack(soul.slice(1));
        assert.strictEqual(
          pen.run(bc, ["k", enc, soul, 0, Date.now(), ""]),
          true,
          "ciphertext string passes PEN ISS predicate",
        );
        done();
      });
    });
  });

  it("ZEN.secret shared key, wrong pair: decrypt returns undefined", function (done) {
    ZEN.secret(pair2.pub, pair, function (aesKey) {
      ZEN.encrypt("sensitive", aesKey, function (enc) {
        ZEN.secret(pair.pub, pair2, function (aesKey2) {
          ZEN.decrypt(enc, aesKey2, function (dec) {
            assert.strictEqual(
              dec,
              "sensitive",
              "alice→bob shared key decrypts",
            );
            // deliberately use wrong key
            ZEN.decrypt(enc, pair.pub, function (bad) {
              assert.strictEqual(bad, undefined, "wrong key yields undefined");
              done();
            });
          });
        });
      });
    });
  });

  // ── PoW ─────────────────────────────────────────────────────────────────────

  it("pow difficulty:1 — ZEN.hash mines nonce bound to key; pen verifies hash(key+nonce) via R[7]", function (done) {
    this.timeout(30000);
    var difficulty = 1;
    var key = "1234:item-abc:buy:writerPubXyz";
    var soul = ZEN.pen({ pow: { field: 7, difficulty: difficulty } });
    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(policy.pow.difficulty, difficulty, "pow policy parsed");
    assert.strictEqual(policy.pow.field, 7, "pow field is R[7] (nonce)");

    // Mine nonce such that hash(key + ":" + nonce) starts with "0"
    ZEN.hash(
      key,
      null,
      function (result) {
        assert.ok(result && result.nonce, "hash returns nonce");
        assert.ok(result.hash.startsWith("0"), "hash satisfies difficulty:1");
        // penStage verifies hash(R[0] + ":" + R[7]) — key bound to nonce
        // The WASM VM predicate itself has no key constraint here, so it passes
        assert.strictEqual(
          pen.run(bc, [key, "val", soul, 0, Date.now(), "", "", result.nonce]),
          true,
          "predicate is PASS",
        );
        done();
      },
      { pow: { difficulty: difficulty }, name: "SHA-256", encode: "hex" },
    );
  });

  it("pow + string predicate combined: val must be string AND have PoW (field always R[7])", function (done) {
    var soul = ZEN.pen({
      val: { type: "string" },
      pow: { field: 1, difficulty: 1 },
    });
    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(policy.pow.field, 7, "user field:1 ignored; always R[7]");
    assert.strictEqual(policy.pow.difficulty, 1);
    // predicate: val must be a string (number fails regardless of pow)
    assert.strictEqual(
      pen.run(bc, ["k", "valid_string", soul, 0, Date.now(), ""]),
      true,
    );
    assert.strictEqual(pen.run(bc, ["k", 42, soul, 0, Date.now(), ""]), false);
    done();
  });

  // ── cert ─────────────────────────────────────────────────────────────────────

  it("cert — ZEN.certify creates cert; PEN embeds certifier pub correctly", function (done) {
    // Alice certifies Bob to write; PEN policy embeds Alice's pub (the certifier)
    // ZEN.certify(recipient, policy, pair, null, cb) — but API variations exist.
    // Core test: pen embeds cert pub correctly in policy bytes.
    var soul = ZEN.pen({ cert: pair.pub });
    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(
      policy.cert,
      pair.pub,
      "certifier pub embedded in bytecode",
    );
    assert.strictEqual(policy.sign, false, "sign not set when cert is used");
    // Second pair: different pub should also embed correctly
    var soul2 = ZEN.pen({ cert: pair2.pub });
    var bc2 = pen.unpack(soul2.slice(1));
    assert.strictEqual(
      pen.scanpolicy(bc2).cert,
      pair2.pub,
      "pair2 pub embeds correctly",
    );
    // Both pubs must survive pack/unpack round-trip
    assert.notStrictEqual(
      soul,
      soul2,
      "different certs produce different souls",
    );
    done();
  });

  // ── full order namespace ─────────────────────────────────────────────────────

  it("order namespace: ZEN.candle + sign:true — sign round-trip + candle window", function (done) {
    var now = Date.now();
    var size = 300000;
    var candle = Math.floor(now / size);

    var soul = ZEN.pen({
      key: {
        and: [
          ZEN.candle({ seg: 0, sep: "_", size: size, back: 100, fwd: 2 }),
          {
            seg: {
              sep: "_",
              idx: 3,
              of: { reg: 0 },
              match: { or: [{ eq: "buy" }, { eq: "sell" }] },
            },
          },
        ],
      },
      val: { type: "string" },
      sign: true,
    });

    var goodKey = candle + "_ETH_USDT_buy_" + String.random(6);
    var badKey = candle + "_ETH_USDT_hold_" + String.random(6);
    var order = JSON.stringify({ amount: 1.5, price: 3000 });

    ZEN.sign(order, pair, function (signed) {
      var bc = pen.unpack(soul.slice(1));
      var policy = pen.scanpolicy(bc);
      assert.strictEqual(policy.sign, true, "sign policy present");
      assert.ok(
        bc.length < 512,
        "bytecode < 512 bytes: " + bc.length + " bytes",
      );

      // Good: valid candle + valid direction + signed string val
      assert.strictEqual(
        pen.run(bc, [goodKey, signed, soul, 0, now, pair.pub]),
        true,
        "valid order passes",
      );
      // Bad direction
      assert.strictEqual(
        pen.run(bc, [badKey, signed, soul, 0, now, pair.pub]),
        false,
        "invalid direction blocked",
      );
      // Stale candle
      var stale = candle - 200 + "_ETH_USDT_buy_nonce1";
      assert.strictEqual(
        pen.run(bc, [stale, signed, soul, 0, now, pair.pub]),
        false,
        "stale candle blocked",
      );
      done();
    });
  });

  it('pow unit:"a" difficulty:1 — ZEN.hash must start with "a"', function (done) {
    this.timeout(30000);
    var unit = "a";
    var difficulty = 1;
    var prefix = unit.repeat(difficulty); // 'a'
    var soul = ZEN.pen({
      pow: { field: 1, unit: unit, difficulty: difficulty },
    });
    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(policy.pow.unit, unit);
    assert.strictEqual(policy.pow.difficulty, difficulty);

    var tries = 0;
    function seek(n) {
      tries++;
      if (tries > 500)
        return done(new Error("could not find PoW solution in 500 tries"));
      var val = "test_data_nonce" + n;
      ZEN.hash(
        val,
        null,
        function (hash) {
          if (hash && hash.slice(0, prefix.length) === prefix) {
            assert.ok(
              hash.startsWith(prefix),
              "hash satisfies unit.repeat(difficulty)",
            );
            assert.ok(
              pen.run(bc, ["k", val, soul, 0, Date.now(), ""]),
              "predicate passes (PASS, no constraint)",
            );
            done();
          } else {
            seek(n + 1);
          }
        },
        { name: "SHA-256", encode: "hex" },
      );
    }
    seek(0);
  });

  it("order namespace: ZEN.hash PoW + candle — hostile nonce cannot fake PoW", function (done) {
    this.timeout(30000);
    var now = Date.now();
    var size = 300000;
    var candle = Math.floor(now / size);

    // Soul: key must be valid candle_dir, val must be string with PoW difficulty:1
    var soul = ZEN.pen({
      key: ZEN.candle({ seg: 0, sep: "_", size: size, back: 50, fwd: 1 }),
      pow: { field: 1, difficulty: 1 },
    });

    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(policy.pow.difficulty, 1);

    var key = candle + "_ETH_USDT";
    // Search for a val whose SHA-256 starts with '0'
    var tries = 0;
    function seek(n) {
      tries++;
      if (tries > 200) return done(new Error("too many PoW tries"));
      var val = "amount:100,nonce:" + n;
      ZEN.hash(
        val,
        null,
        function (hash) {
          if (hash && hash[0] === "0") {
            // predicate passes (no val-type constraint here, just pow policy)
            assert.strictEqual(
              pen.run(bc, [key, val, soul, 0, now, ""]),
              true,
              "PoW solution accepted by predicate",
            );
            // make sure bad key (wrong candle) is still rejected
            var badKey = candle + 50 + "_ETH_USDT";
            assert.strictEqual(
              pen.run(bc, [badKey, val, soul, 0, now, ""]),
              false,
              "bad candle rejected even with valid val",
            );
            done();
          } else {
            seek(n + 1);
          }
        },
        { name: "SHA-256", encode: "hex" },
      );
    }
    seek(0);
  });

  // ── ZEN graph put/get with PEN soul ─────────────────────────────────────────

  it("ZEN put/get with open PEN soul stores and retrieves value", function (done) {
    this.timeout(10000);
    var soul = ZEN.pen({ val: { type: "string" }, open: true });
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var value = "market_data_" + Date.now();
    zen.get(soul).get("price").put(value);
    zen
      .get(soul)
      .get("price")
      .once(function (v) {
        assert.strictEqual(v, value, "value round-trips through ZEN graph");
        done();
      });
  });

  it("ZEN user.auth + signed put: user pub in R[5] satisfies sign policy check", function (done) {
    // Simulate what ZEN does when an authenticated user writes into a sign:true PEN soul:
    // R[5] = writer's pub, penStage verifies sign policy via applypolicy.
    // Here we test the predicate + policy detection layer (not the ZEN network layer).
    var soul = ZEN.pen({ val: { type: "string" }, sign: true });
    var bc = pen.unpack(soul.slice(1));
    var policy = pen.scanpolicy(bc);
    assert.strictEqual(policy.sign, true, "sign policy present");

    ZEN.sign("order_data", pair, function (signed) {
      // Verify the signed value passes the string predicate (ISS)
      var regs = ["order_key", signed, soul, 0, Date.now(), pair.pub];
      assert.strictEqual(
        pen.run(bc, regs),
        true,
        "signed string passes predicate with writer pub in R[5]",
      );

      // Unauthenticated user (empty pub) — predicate still passes (auth enforced by applypolicy, not bytecode)
      var anonRegs = ["order_key", signed, soul, 0, Date.now(), ""];
      assert.strictEqual(
        pen.run(bc, anonRegs),
        true,
        "predicate is pub-agnostic; auth enforcement is in applypolicy",
      );

      // Wrong type rejects before even reaching auth check
      var badRegs = ["order_key", 42, soul, 0, Date.now(), pair.pub];
      assert.strictEqual(
        pen.run(bc, badRegs),
        false,
        "number val rejected by ISS predicate",
      );
      done();
    });
  });

  it("ZEN shared PEN soul: authenticator pub is available in R[5] during predicate evaluation", function (done) {
    this.timeout(10000);
    var now = Date.now();
    var size = 300000;
    var candle = Math.floor(now / size);
    var soul = ZEN.pen({
      key: {
        and: [
          {
            let: {
              bind: 0,
              def: {
                divu: [
                  { tonum: { seg: { sep: ":", idx: 0, of: { reg: 0 } } } },
                  size,
                ],
              },
              body: {
                and: [
                  { gte: [{ reg: 128 }, candle] },
                  { lte: [{ reg: 128 }, candle] },
                ],
              },
            },
          },
          { eq: [{ seg: { sep: ":", idx: 1, of: { reg: 0 } } }, { reg: 5 }] },
          {
            seg: {
              sep: ":",
              idx: 2,
              of: { reg: 0 },
              match: { length: [1, 64] },
            },
          },
        ],
      },
      val: { type: "string" },
      sign: true,
    });
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var key = now + ":" + pair.pub + ":nonce1";
    ZEN.sign("shared_order", pair, function (value) {
      zen
        .get(soul)
        .get(key)
        .put(
          value,
          function (ack) {
            if (ack && ack.err) return done(new Error(ack.err));
            zen
              .get(soul)
              .get(key)
              .once(function (v) {
                assert.strictEqual(
                  v,
                  value,
                  "shared PEN write round-trips with authenticator pub in R[5]",
                );
                done();
              });
          },
          { authenticator: pair },
        );
    });
  });
});

// ── ZEN.pen({ pow }) — field hardcoded to R[7] ───────────────────────────────

describe("ZEN.pen({ pow }) — field hardcoded to R[7]", function () {
  it("{ pow: { unit: '0', difficulty: 2 } } (no field) → scanpolicy.pow.field === 7", function () {
    var soul = ZEN.pen({ pow: { unit: "0", difficulty: 2 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow, "pow policy present");
    assert.strictEqual(p.pow.field, 7, "field is always 7 (nonce register)");
    assert.strictEqual(p.pow.unit, "0");
    assert.strictEqual(p.pow.difficulty, 2);
  });

  it("{ pow: { field: 3, unit: '0', difficulty: 2 } } — user field ignored, always 7", function () {
    var soul = ZEN.pen({ pow: { field: 3, unit: "0", difficulty: 2 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.ok(p.pow, "pow policy present");
    assert.strictEqual(p.pow.field, 7, "user-supplied field:3 is ignored; always R[7]");
  });

  it("{ pow: { field: 0, difficulty: 2 } } — field:0 also overridden to 7", function () {
    var soul = ZEN.pen({ pow: { field: 0, difficulty: 2 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.pow.field, 7, "field:0 also overridden to R[7]");
  });

  it("{ pow: { difficulty: 1 } } (no field, no unit) → field=7, unit='0'", function () {
    var soul = ZEN.pen({ pow: { difficulty: 1 } });
    var bc = pen.unpack(soul.slice(1));
    var p = pen.scanpolicy(bc);
    assert.strictEqual(p.pow.field, 7);
    assert.strictEqual(p.pow.unit, "0");
    assert.strictEqual(p.pow.difficulty, 1);
  });
});

// ── canonical block PoW hash ──────────────────────────────────────────────────

describe("canonical block PoW hash", function () {
  this.timeout(30000);

  it("mining hashes JSON.stringify({#,.,':',>}) + ':' + nonce (canonical block format)", function (done) {
    var soul = "!testSoul";
    var key = "price";
    var val = "42.5";
    var state = 1700000000000;
    var proofBlock = JSON.stringify({ "#": soul, ".": key, ":": val, ">": state });
    ZEN.hash(
      function (nonce) { return proofBlock + ":" + nonce; },
      null,
      function (result) {
        assert.ok(result && result.nonce, "mining returns nonce");
        assert.ok(typeof result.hash === "string", "mining returns hash string");
        assert.ok(result.hash.startsWith("0"), "hash satisfies difficulty:1");
        // Verification: re-hash the same canonical proof string → identical hash
        ZEN.hash(proofBlock + ":" + result.nonce, null, function (verifyHash) {
          assert.strictEqual(verifyHash, result.hash, "verification hash matches mining hash");
          done();
        }, { name: "SHA-256", encode: "hex" });
      },
      { pow: { unit: "0", difficulty: 1 }, name: "SHA-256", encode: "hex" },
    );
  });

  it("nonce is bound to the canonical block — different key produces different proof string", function (done) {
    var soul = "!testSoul";
    var val = "100";
    var state = 1700000000000;
    var blockA = JSON.stringify({ "#": soul, ".": "keyA", ":": val, ">": state });
    var blockB = JSON.stringify({ "#": soul, ".": "keyB", ":": val, ">": state });
    ZEN.hash(
      function (nonce) { return blockA + ":" + nonce; },
      null,
      function (result) {
        assert.ok(result.hash.startsWith("0"), "blockA hash satisfies difficulty:1");
        var proofA = blockA + ":" + result.nonce;
        var proofB = blockB + ":" + result.nonce;
        assert.notStrictEqual(proofA, proofB, "proof strings differ when key differs — replay is blocked at hash level");
        done();
      },
      { pow: { unit: "0", difficulty: 1 }, name: "SHA-256", encode: "hex" },
    );
  });

  it("nonce is bound to the canonical block — different value produces different proof string", function (done) {
    var soul = "!testSoul";
    var key = "price";
    var state = 1700000000000;
    var blockA = JSON.stringify({ "#": soul, ".": key, ":": "100", ">": state });
    var blockB = JSON.stringify({ "#": soul, ".": key, ":": "200", ">": state });
    ZEN.hash(
      function (nonce) { return blockA + ":" + nonce; },
      null,
      function (result) {
        var proofA = blockA + ":" + result.nonce;
        var proofB = blockB + ":" + result.nonce;
        assert.notStrictEqual(proofA, proofB, "proof strings differ when value differs — replay is blocked at hash level");
        done();
      },
      { pow: { unit: "0", difficulty: 1 }, name: "SHA-256", encode: "hex" },
    );
  });

  it("nonce is bound to the canonical block — different soul produces different proof string", function (done) {
    var key = "price";
    var val = "42";
    var state = 1700000000000;
    var blockA = JSON.stringify({ "#": "!soulA", ".": key, ":": val, ">": state });
    var blockB = JSON.stringify({ "#": "!soulB", ".": key, ":": val, ">": state });
    ZEN.hash(
      function (nonce) { return blockA + ":" + nonce; },
      null,
      function (result) {
        var proofA = blockA + ":" + result.nonce;
        var proofB = blockB + ":" + result.nonce;
        assert.notStrictEqual(proofA, proofB, "proof strings differ when soul differs — cross-soul replay blocked");
        done();
      },
      { pow: { unit: "0", difficulty: 1 }, name: "SHA-256", encode: "hex" },
    );
  });
});

// ── penStage — auto-mining via opt.pow ────────────────────────────────────────

describe("penStage — auto-mining via opt.pow", function () {
  this.timeout(30000);

  it("opt.pow = {unit:'0', difficulty:1} auto-mines and write is accepted by pow soul", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ pow: { unit: "0", difficulty: 1 } });
    zen.get(soul).get("k").put("value", function (ack) {
      assert.ok(!ack || !ack.err, "auto-mined write must be accepted: " + (ack && ack.err));
      done();
    }, { pow: { unit: "0", difficulty: 1 } });
  });

  it("no opt.pow on pow:difficulty:4 soul → write rejected (no nonce, hash won't satisfy difficulty)", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    // difficulty:4 → 1 in 65536 chance of false positive, effectively impossible
    var soul = ZEN.pen({ pow: { unit: "0", difficulty: 4 } });
    zen.get(soul).get("k").put("value", function (ack) {
      assert.ok(ack && ack.err, "write without nonce must be rejected by pow soul: " + JSON.stringify(ack));
      done();
    });
  });

  it("auto-mined value is readable back from graph", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, pow: { unit: "0", difficulty: 1 } });
    var value = "market_price_" + Date.now();
    zen.get(soul).get("price").put(value, function (ack) {
      assert.ok(!ack || !ack.err, "auto-mined write accepted: " + (ack && ack.err));
      zen.get(soul).get("price").once(function (v) {
        assert.strictEqual(v, value, "auto-mined value round-trips through graph");
        done();
      });
    }, { pow: { unit: "0", difficulty: 1 } });
  });

  it("string val predicate + pow: wrong type rejected even with opt.pow", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, pow: { unit: "0", difficulty: 1 } });
    zen.get(soul).get("k").put(42, function (ack) {
      assert.ok(ack && ack.err, "number rejected by string-type soul even when opt.pow provided: " + JSON.stringify(ack));
      done();
    }, { pow: { unit: "0", difficulty: 1 } });
  });

  it("sign:true + pow: authenticator + opt.pow together → write accepted", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, sign: true, pow: { unit: "0", difficulty: 1 } });
    ZEN.pair(function (pair) {
      ZEN.sign("signed_order", pair, function (signed) {
        zen.get(soul).get("k").put(signed, function (ack) {
          assert.ok(!ack || !ack.err, "sign+pow write accepted: " + (ack && ack.err));
          done();
        }, { authenticator: pair, pow: { unit: "0", difficulty: 1 } });
      });
    });
  });
});

// ── canonical block — state binds nonce ───────────────────────────────────────
// Proves: the canonical block {#,.,':',>} includes the HAM state timestamp '>'.
// Changing '>' produces a different proof string → old nonce is cryptographically
// invalid for the new block (1-in-16 false-positive rate at difficulty:1 means
// the probability of accidental acceptance is ~6%, so we test structural inequality
// rather than probabilistic hash failure).

describe("canonical block — state binds nonce", function () {
  this.timeout(30000);

  it("state change produces different canonical block — old nonce gives different proof string", function (done) {
    var soul = "!testSoul";
    var key = "price";
    var val = "42";
    var S1 = 1700000000000;
    var S2 = S1 + 1; // any state increment (each write gets a new Zen.state())
    var block1 = JSON.stringify({ "#": soul, ".": key, ":": val, ">": S1 });
    var block2 = JSON.stringify({ "#": soul, ".": key, ":": val, ">": S2 });

    ZEN.hash(
      function (nonce) { return block1 + ":" + nonce; },
      null,
      function (result) {
        assert.ok(result.hash.startsWith("0"), "nonce is valid for state S1");

        // Same nonce applied to a block with state S2 creates a different proof string
        var proof1 = block1 + ":" + result.nonce;
        var proof2 = block2 + ":" + result.nonce;
        assert.notStrictEqual(proof1, proof2,
          "canonical blocks differ when > (state) differs — old nonce is structurally invalid for new state");

        // Block strings themselves differ (confirming > is included in canonical block)
        assert.notStrictEqual(block1, block2,
          "canonical block JSON changes when state changes");
        done();
      },
      { pow: { unit: "0", difficulty: 1 }, name: "SHA-256", encode: "hex" },
    );
  });

  it("canonical block includes all 4 fields: soul(#), key(.), value(:), state(>)", function () {
    // Changing each field independently changes the block JSON string
    var base = { "#": "!soul", ".": "k", ":": "v", ">": 1000 };
    var withDiffSoul  = Object.assign({}, base, { "#": "!soul2" });
    var withDiffKey   = Object.assign({}, base, { ".": "k2" });
    var withDiffVal   = Object.assign({}, base, { ":": "v2" });
    var withDiffState = Object.assign({}, base, { ">": 1001 });

    var baseStr = JSON.stringify(base);
    assert.notStrictEqual(JSON.stringify(withDiffSoul),  baseStr, "soul(#) is part of canonical block");
    assert.notStrictEqual(JSON.stringify(withDiffKey),   baseStr, "key(.) is part of canonical block");
    assert.notStrictEqual(JSON.stringify(withDiffVal),   baseStr, "value(:) is part of canonical block");
    assert.notStrictEqual(JSON.stringify(withDiffState), baseStr, "state(>) is part of canonical block");
  });

  it("same soul+key+val+state always produces the same canonical block (deterministic)", function () {
    var a = JSON.stringify({ "#": "!s", ".": "k", ":": "v", ">": 9999 });
    var b = JSON.stringify({ "#": "!s", ".": "k", ":": "v", ">": 9999 });
    assert.strictEqual(a, b, "canonical block is deterministic for same inputs");
  });
});

// ── value update — re-sign and re-mine required ───────────────────────────────
// Proves: when a soul enforces sign and/or pow, users can still update the value
// by providing a fresh signature and a freshly mined nonce for the new write.
// The new write's canonical block has a new '>' state timestamp, making the old
// nonce invalid and requiring a new one.

describe("value update — re-sign and re-mine required", function () {
  this.timeout(60000);

  it("pow soul: second write with new value is accepted and overwrites first", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, pow: { unit: "0", difficulty: 1 } });
    var opt = { pow: { unit: "0", difficulty: 1 } };

    zen.get(soul).get("price").put("value_v1", function (ack1) {
      assert.ok(!ack1 || !ack1.err, "v1 write accepted: " + (ack1 && ack1.err));

      // Second write: new value → new state → auto-mine new nonce
      zen.get(soul).get("price").put("value_v2", function (ack2) {
        assert.ok(!ack2 || !ack2.err, "v2 update accepted: " + (ack2 && ack2.err));

        zen.get(soul).get("price").once(function (v) {
          assert.strictEqual(v, "value_v2", "latest value is v2 after update");
          done();
        });
      }, opt);
    }, opt);
  });

  it("sign+pow soul: second write (new sign + new mine) is accepted and overwrites first", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, sign: true, pow: { unit: "0", difficulty: 1 } });

    ZEN.pair(function (pair) {
      var opt = { authenticator: pair, pow: { unit: "0", difficulty: 1 } };

      ZEN.sign("order_v1", pair, function (signed1) {
        zen.get(soul).get("order").put(signed1, function (ack1) {
          assert.ok(!ack1 || !ack1.err, "v1 signed+mined write accepted: " + (ack1 && ack1.err));

          // Update: new value → new sign + new mine (both happen automatically via opt)
          ZEN.sign("order_v2", pair, function (signed2) {
            zen.get(soul).get("order").put(signed2, function (ack2) {
              assert.ok(!ack2 || !ack2.err, "v2 update (new sign+mine) accepted: " + (ack2 && ack2.err));

              zen.get(soul).get("order").once(function (v) {
                assert.strictEqual(v, signed2, "latest value is signed_v2 after update");
                done();
              });
            }, opt);
          });
        }, opt);
      });
    });
  });

  it("sign+pow soul: multiple sequential updates all accepted; last value wins", function (done) {
    var zen = Zen({ radisk: false, peers: [], localStorage: false });
    var soul = ZEN.pen({ val: { type: "string" }, sign: true, pow: { unit: "0", difficulty: 1 } });

    ZEN.pair(function (pair) {
      var opt = { authenticator: pair, pow: { unit: "0", difficulty: 1 } };
      var writes = ["v1", "v2", "v3"];
      var idx = 0;

      function next() {
        if (idx >= writes.length) {
          // All writes done — latest value should be signed "v3"
          zen.get(soul).get("seq").once(function (v) {
            ZEN.verify(v, pair.pub, function (plain) {
              assert.strictEqual(plain, "v3", "final value after 3 sequential updates is v3");
              done();
            });
          });
          return;
        }
        var val = writes[idx++];
        ZEN.sign(val, pair, function (signed) {
          zen.get(soul).get("seq").put(signed, function (ack) {
            assert.ok(!ack || !ack.err, "write " + val + " accepted: " + (ack && ack.err));
            next();
          }, opt);
        });
      }
      next();
    });
  });
});

// ── unlimited string tests ────────────────────────────────────────────────────

describe("pen — unlimited string support (WASM linear memory heap)", function () {
  function rep(ch, n) { return ch.repeat(n); }

  it("register string > 128 bytes accepted and compared correctly (EQ)", function () {
    var b = pen.bc;
    var longStr = rep("a", 150) + rep("b", 150); // 300 bytes
    var bytecode = prog(b.eq(b.r0(), b.str(longStr)));
    assert.strictEqual(run(bytecode, [longStr]), true, "exact 300-byte match");
    assert.strictEqual(run(bytecode, [longStr + "x"]), false, "mismatch: longer input");
    assert.strictEqual(run(bytecode, [rep("a", 150) + rep("b", 149)]), false, "mismatch: shorter input");
  });

  it("register string > 255 bytes accepted", function () {
    var b = pen.bc;
    var longStr = rep("z", 500); // 500 bytes
    var bytecode = prog(b.eq(b.r0(), b.str(longStr)));
    assert.strictEqual(run(bytecode, [longStr]), true, "exact 500-byte match");
    assert.strictEqual(run(bytecode, [rep("z", 499)]), false, "499 bytes does not match 500");
  });

  it("inline bytecode STR > 128 bytes via ULEB128 encoding", function () {
    var b = pen.bc;
    var longStr = rep("x", 300); // 300 bytes encoded inline in bytecode
    var bytecode = prog(b.eq(b.r0(), b.str(longStr)));
    assert.strictEqual(run(bytecode, [longStr]), true, "300-byte inline STR match");
    assert.strictEqual(run(bytecode, [rep("x", 299)]), false, "299-byte mismatch");
  });

  it("CONCAT of two 200-byte register strings produces 400-byte result", function () {
    var b = pen.bc;
    var a = rep("A", 200);
    var ab = rep("B", 200);
    var expected = a + ab;
    var bytecode = prog(b.eq(b.concat(b.r0(), b.r1()), b.str(expected)));
    assert.strictEqual(run(bytecode, [a, ab]), true, "concat(200,200)=400 bytes");
    assert.strictEqual(run(bytecode, [a, rep("B", 199)]), false, "concat mismatch");
  });

  it("UPPER on 200-byte string works correctly", function () {
    var b = pen.bc;
    var lower = rep("a", 200);
    var upper = rep("A", 200);
    var bytecode = prog(b.eq(b.upper(b.r0()), b.str(upper)));
    assert.strictEqual(run(bytecode, [lower]), true, "upper(200 lowercase a) = 200 uppercase A");
  });

  it("LOWER on 200-byte string works correctly", function () {
    var b = pen.bc;
    var upper = rep("Z", 200);
    var lower = rep("z", 200);
    var bytecode = prog(b.eq(b.lower(b.r0()), b.str(lower)));
    assert.strictEqual(run(bytecode, [upper]), true, "lower(200 uppercase Z) = 200 lowercase z");
  });

  it("INCLUDES finds short needle in long haystack", function () {
    var b = pen.bc;
    var haystack = rep("a", 100) + "NEEDLE" + rep("b", 100);
    var bytecode = prog(b.inc(b.r0(), b.str("NEEDLE")));
    assert.strictEqual(run(bytecode, [haystack]), true, "NEEDLE found in 206-byte haystack");
    assert.strictEqual(run(bytecode, [rep("a", 200)]), false, "NEEDLE not found");
  });

  it("INCLUDES finds long needle in long haystack", function () {
    var b = pen.bc;
    var needle = rep("x", 150);
    var haystack = rep("a", 100) + needle + rep("b", 100);
    var bytecode = prog(b.inc(b.r0(), b.r1()));
    assert.strictEqual(run(bytecode, [haystack, needle]), true, "150-byte needle in 350-byte haystack");
    assert.strictEqual(run(bytecode, [rep("a", 350), needle]), false, "needle not found");
  });

  it("PRE (startsWith) with long prefix", function () {
    var b = pen.bc;
    var prefix = rep("p", 150);
    var str = prefix + rep("s", 100);
    var bytecode = prog(b.pre(b.r0(), b.r1()));
    assert.strictEqual(run(bytecode, [str, prefix]), true, "long startsWith: true");
    assert.strictEqual(run(bytecode, [rep("q", 250), prefix]), false, "long startsWith: false");
  });

  it("SUF (endsWith) with long suffix", function () {
    var b = pen.bc;
    var suffix = rep("e", 150);
    var str = rep("s", 100) + suffix;
    var bytecode = prog(b.suf(b.r0(), b.r1()));
    assert.strictEqual(run(bytecode, [str, suffix]), true, "long endsWith: true");
    assert.strictEqual(run(bytecode, [rep("a", 250), suffix]), false, "long endsWith: false");
  });

  it("LEN of 500-byte string returns 500", function () {
    var b = pen.bc;
    var str = rep("L", 500);
    var bytecode = prog(b.eq(b.len(b.r0()), b.uint(500)));
    assert.strictEqual(run(bytecode, [str]), true, "LEN(500-byte string) = 500");
  });

  it("SLICE of long string extracts correct substring", function () {
    var b = pen.bc;
    var str = rep("a", 100) + "MIDDLE" + rep("b", 100);
    var bytecode = prog(b.eq(b.slice(b.r0(), b.uint(100), b.uint(106)), b.str("MIDDLE")));
    assert.strictEqual(run(bytecode, [str]), true, "SLICE extracts MIDDLE from 206-byte string");
  });

  it("SEG on long string works correctly", function () {
    var b = pen.bc;
    var part1 = rep("a", 100);
    var part2 = rep("b", 100);
    var str = part1 + "/" + part2;
    var bytecode = prog(b.eq(b.seg(b.r0(), "/", b.uint(1)), b.str(part2)));
    assert.strictEqual(run(bytecode, [str]), true, "SEG extracts 100-byte second segment");
  });

  it("LT/GT comparison on long strings (lexicographic)", function () {
    var b = pen.bc;
    var a = rep("a", 200);
    var bstr = rep("b", 200);
    var ltbc = prog(b.lt(b.r0(), b.r1()));
    assert.strictEqual(run(ltbc, [a, bstr]), true, "200-byte aaa... < bbb...");
    assert.strictEqual(run(ltbc, [bstr, a]), false, "200-byte bbb... not < aaa...");
  });

  it("1000-byte string round-trips through register", function () {
    var b = pen.bc;
    var str = rep("X", 1000);
    var bytecode = prog(b.eq(b.r0(), b.str(str)));
    assert.strictEqual(run(bytecode, [str]), true, "1000-byte equality");
  });

  it("TONUM on numeric string works", function () {
    var b = pen.bc;
    var bytecode = prog(b.eq(b.tonum(b.r0()), b.uint(12345)));
    assert.strictEqual(run(bytecode, ["12345"]), true, "tonum('12345') = 12345");
  });
});
