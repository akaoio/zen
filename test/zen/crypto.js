import assert from "assert";
import ZEN from "../../zen.js";

function makeZen() {
  return new ZEN({ peers: [] });
}

// ─── Utility ─────────────────────────────────────────────────────────────────
describe("ZEN crypto — quickstart", function () {
  this.timeout(20 * 1000);

  it("pair → encrypt → sign → verify → decrypt → hash roundtrip", async function () {
    var pair = await ZEN.pair();
    var enc = await ZEN.encrypt("hello self", pair);
    var sig = await ZEN.sign(enc, pair);
    var msg = await ZEN.verify(sig, pair.pub);
    var dec = await ZEN.decrypt(msg, pair);
    assert.strictEqual(dec, "hello self");
    var proof = await ZEN.hash(dec, pair);
    var check = await ZEN.hash("hello self", pair);
    assert.strictEqual(proof, check);
  });

  it("shared secret exchange", async function () {
    var alice = await ZEN.pair();
    var bob = await ZEN.pair();
    var aes = await ZEN.secret(bob.pub, alice);
    var enc = await ZEN.encrypt("shared data", aes);
    var aes2 = await ZEN.secret(alice.pub, bob);
    var dec = await ZEN.decrypt(enc, aes2);
    assert.strictEqual(dec, "shared data");
  });
});

describe("ZEN crypto — tamper / wrong key", function () {
  this.timeout(20 * 1000);

  var alice, bob;
  before(async function () {
    [alice, bob] = await Promise.all([ZEN.pair(), ZEN.pair()]);
  });

  it("verifying with wrong pub returns undefined", async function () {
    var sig = await ZEN.sign("asdf", alice);
    var bad = await ZEN.verify(sig, bob.pub).catch(function () {
      return undefined;
    });
    assert.strictEqual(bad, undefined);
  });

  it("verifying tampered payload rejects", async function () {
    var sig = await ZEN.sign("original", alice);
    // Compact format: <86 base62><v>:<message> — tamper the message part
    var tampered = sig.slice(0, 88) + "tampered";
    await assert.rejects(ZEN.verify(tampered, alice.pub));
  });

  it("decrypting with wrong pair returns undefined", async function () {
    var enc = await ZEN.encrypt("secret", alice);
    var bad = await ZEN.decrypt(enc, bob).catch(function () {
      return undefined;
    });
    assert.strictEqual(bad, undefined);
  });
});

describe("ZEN crypto — sign/verify JS types", function () {
  this.timeout(20 * 1000);

  var pair;
  before(async function () {
    pair = await ZEN.pair();
  });

  var cases = [null, true, false, 0, 1, 1.01, "", "a", [], [1], {}, { a: 1 }];

  cases.forEach(function (val) {
    it("roundtrips " + JSON.stringify(val), async function () {
      var sig = await ZEN.sign(val, pair);
      var out = await ZEN.verify(sig, pair.pub);
      assert.deepStrictEqual(out, val);
    });
  });

  it("roundtrips stringified JSON back to object", async function () {
    var sig = await ZEN.sign(JSON.stringify({ a: 1 }), pair);
    var out = await ZEN.verify(sig, pair.pub);
    assert.deepStrictEqual(out, { a: 1 });
  });
});

describe("ZEN crypto — encrypt/decrypt JS types", function () {
  this.timeout(20 * 1000);

  var pair;
  before(async function () {
    pair = await ZEN.pair();
  });

  var cases = [null, true, false, 0, 1, 1.01, "", "a", [], [1], {}, { a: 1 }];

  cases.forEach(function (val) {
    it("roundtrips " + JSON.stringify(val), async function () {
      var enc = await ZEN.encrypt(val, pair);
      var dec = await ZEN.decrypt(enc, pair);
      assert.deepStrictEqual(dec, val);
    });
  });

  it("roundtrips stringified JSON back to object", async function () {
    var enc = await ZEN.encrypt(JSON.stringify({ a: 1 }), pair);
    var dec = await ZEN.decrypt(enc, pair);
    assert.deepStrictEqual(dec, { a: 1 });
  });
});

describe("ZEN crypto — hash", function () {
  this.timeout(20 * 1000);

  var hashPair;
  before(async function () {
    hashPair = await ZEN.pair();
  });

  it('hash() output contains no "/" (base64url)', async function () {
    function hashAsync(data) {
      return new Promise(function (res) {
        ZEN.hash(
          data,
          null,
          function (r) {
            res(r);
          },
          { name: "SHA-256" },
        );
      });
    }
    var chars =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (var i = 0; i < 1000; i++) {
      var s = Array.from({ length: 32 }, function () {
        return chars[Math.floor(Math.random() * chars.length)];
      }).join("");
      var r = await hashAsync(s);
      assert(!r.includes("/"), 'Found "/" in hash output: ' + r);
    }
  });

  it("hash() supports keccak256", async function () {
    var h = await ZEN.hash("", null, null, {
      name: "keccak256",
      encode: "hex",
    });
    assert.strictEqual(
      h,
      "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470",
    );
  });

  it("hash() handles ArrayBuffer input", async function () {
    var b1 = new Uint8Array(16);
    for (var i = 0; i < 16; i++) b1[i] = i;
    var b2 = new Uint8Array(16);
    for (var i = 0; i < 16; i++) b2[i] = i + 16;
    var h1 = await ZEN.hash(b1.buffer, "salt");
    var h2 = await ZEN.hash(b2.buffer, "salt");
    assert.strictEqual(typeof h1, "string");
    assert.strictEqual(typeof h2, "string");
    assert.notStrictEqual(h1, h2);
  });

  it("hash() is deterministic for same input", async function () {
    var h1 = await ZEN.hash("hello", hashPair);
    var h2 = await ZEN.hash("hello", hashPair);
    assert.strictEqual(h1, h2);
  });

  it("hash() mine: function data — nonce is base62, no underscore", async function () {
    var result = await ZEN.hash(
      function (nonce) { return "order:" + nonce; },
      null, null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
    assert.ok(result && typeof result === "object", "returns object");
    assert.ok(typeof result.hash === "string", "hash is string");
    assert.ok(typeof result.nonce === "string", "nonce is string");
    assert.ok(typeof result.proof === "string", "proof is string");
    assert.ok(/^[0-9A-Za-z]+$/.test(result.nonce), "nonce is base62 (no underscore)");
    assert.ok(result.hash.indexOf("0") === 0, "hash starts with unit");
    assert.strictEqual(result.proof, "order:" + result.nonce, "proof embeds nonce");
  });

  it("hash() mine: string data — colon separator, nonce appended", async function () {
    var result = await ZEN.hash(
      "work",
      null, null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
    assert.ok(result.proof.startsWith("work:"), "colon separator used");
    assert.ok(result.hash.indexOf("0") === 0, "hash meets difficulty");
  });

  it("hash() mine: nonce in middle of key", async function () {
    var result = await ZEN.hash(
      function (nonce) { return "pre:" + nonce + ":suf"; },
      null, null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
    assert.ok(/^pre:[0-9A-Za-z]+:suf$/.test(result.proof), "nonce in middle");
    assert.ok(result.hash.indexOf("0") === 0, "hash meets difficulty");
  });

  it("hash() mine: difficulty 2 requires two-char prefix", async function () {
    this.timeout(60 * 1000);
    var result = await ZEN.hash(
      function (nonce) { return "d2:" + nonce; },
      null, null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 2 } },
    );
    assert.ok(result.hash.indexOf("00") === 0, "hash starts with 00");
  });

  it("hash() mine: proof hashes to same result independently (pen PoW compatible)", async function () {
    var result = await ZEN.hash(
      function (nonce) { return "penkey:" + nonce; },
      null, null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
    // pen.js verifies by calling hash(field, null, cb, { name: "SHA-256", encode: "hex" })
    var verify = await ZEN.hash(result.proof, null, null, { name: "SHA-256", encode: "hex" });
    assert.strictEqual(verify, result.hash, "independent hash matches mined hash");
    assert.ok(verify.indexOf("0") === 0, "verification passes pen PoW prefix check");
  });

  it("hash() mine: callback form delivers { hash, nonce, proof }", function (done) {
    ZEN.hash(
      function (nonce) { return "cb:" + nonce; },
      null,
      function (result) {
        assert.ok(result && typeof result === "object");
        assert.ok(result.hash.indexOf("0") === 0);
        done();
      },
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
  });

  it("hash() mine: pair-as-cb shorthand fires callback exactly once", function (done) {
    // pair param is a function → treated as cb (shorthand). Must NOT fire on every
    // mining attempt, only when the winning hash is found.
    var callCount = 0;
    ZEN.hash(
      "shorthand",
      function (result) {
        callCount++;
        assert.strictEqual(callCount, 1, "callback fires exactly once");
        assert.ok(result && typeof result === "object", "result is object");
        assert.ok(result.hash.indexOf("0") === 0, "hash meets pow requirement");
        done();
      },
      null,
      { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } },
    );
  });
});

describe("ZEN crypto — btoa/atob utf8 roundtrip", function () {
  it("handles multibyte chars via TextEncoder/TextDecoder", function () {
    var samples = [
      "Tiếng Việt có dấu: ắềôữ đặng",
      "中文測試：漢字とかな",
      "Emoji 😀🔥✨",
    ];
    var enc = new TextEncoder();
    var dec = new TextDecoder("utf-8");
    samples.forEach(function (s) {
      var bytes = enc.encode(s);
      var bin = Array.from(bytes, function (b) {
        return String.fromCharCode(b);
      }).join("");
      var b64 = btoa(bin);
      var round = dec.decode(
        new Uint8Array(
          Array.from(atob(b64), function (c) {
            return c.charCodeAt(0);
          }),
        ),
      );
      assert.strictEqual(round, s);
    });
  });
});

describe("ZEN crypto — double sign", function () {
  this.timeout(10 * 1000);

  var alice, bob;
  before(async function () {
    [alice, bob] = await Promise.all([ZEN.pair(), ZEN.pair()]);
  });

  it("signing an already-signed value is idempotent", async function () {
    var sig = await ZEN.sign("hello world", alice);
    var dup = await ZEN.sign(sig, alice);
    assert.strictEqual(dup, sig);
  });

  it("different pair produces different signature", async function () {
    var sig = await ZEN.sign("hello", alice);
    var dup = await ZEN.sign(sig, bob);
    assert.notStrictEqual(dup, sig);
  });
});

describe("ZEN crypto — pair() key format", function () {
  this.timeout(10 * 1000);

  var B62 = /^[A-Za-z0-9]{44}[01]$/;
  var B62_44 = /^[A-Za-z0-9]{44}$/;
  var p;
  before(async function () {
    p = await ZEN.pair();
  });

  it("curve is secp256k1", function () {
    assert.strictEqual(p.curve, "secp256k1");
  });

  it("pub is 45-char base62 compressed", function () {
    assert.match(p.pub, B62);
  });

  it("epub not present (no separate encryption keypair)", function () {
    assert.strictEqual(p.epub, undefined);
    assert.strictEqual(p.epriv, undefined);
  });

  it("priv is 44-char base62", function () {
    assert.match(p.priv, B62_44);
  });

  it("address is EVM checksummed", function () {
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/);
  });

  it("pub and address differ", function () {
    assert.notStrictEqual(p.pub, p.address);
  });

  it("format holds across multiple pairs", async function () {
    var pairs = await Promise.all([ZEN.pair(), ZEN.pair(), ZEN.pair()]);
    pairs.forEach(function (p) {
      assert.match(p.pub, B62);
      assert.match(p.priv, B62_44);
      assert.match(p.address, /^0x[0-9a-fA-F]{40}$/);
      assert.strictEqual(p.epub, undefined);
    });
  });

  it("seed-based pair has same format", async function () {
    var p = await ZEN.pair(null, { seed: "test-seed-format" });
    assert.match(p.pub, B62);
    assert.match(p.priv, B62_44);
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/);
  });
});

describe("ZEN crypto — wire format (no ZEN prefix)", function () {
  it("sign() and encrypt() omit ZEN prefix", async function () {
    var pair = await ZEN.pair(null, { seed: "zen-wire-format" });
    var sig = await ZEN.sign({ hello: "zen" }, pair);
    var enc = await ZEN.encrypt("secret", pair);
    assert.strictEqual(sig.startsWith("ZEN"), false);
    assert.strictEqual(enc.startsWith("ZEN"), false);
  });
});

describe("ZEN crypto — seed-based key generation", function () {
  this.timeout(10 * 1000);

  it("same seed → same pair", async function () {
    var p1 = await ZEN.pair(null, { seed: "my secret seed" });
    var p2 = await ZEN.pair(null, { seed: "my secret seed" });
    assert.strictEqual(p1.priv, p2.priv);
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.address, p2.address);
  });

  it("different seed → different pair", async function () {
    var p1 = await ZEN.pair(null, { seed: "seed-a" });
    var p2 = await ZEN.pair(null, { seed: "seed-b" });
    assert.notStrictEqual(p1.pub, p2.pub);
  });

  it("ArrayBuffer seed deterministic", async function () {
    var enc = new TextEncoder();
    var b1 = enc.encode("my secret seed").buffer;
    var b2 = enc.encode("my secret seed").buffer;
    var b3 = enc.encode("other seed").buffer;
    var p1 = await ZEN.pair(null, { seed: b1 });
    var p2 = await ZEN.pair(null, { seed: b2 });
    var p3 = await ZEN.pair(null, { seed: b3 });
    assert.strictEqual(p1.pub, p2.pub);
    assert.notStrictEqual(p1.pub, p3.pub);
  });

  it("pair from priv restores pub", async function () {
    var base = await ZEN.pair(null, { seed: "priv-restore" });
    var rebuilt = await ZEN.pair(null, { priv: base.priv });
    assert.strictEqual(rebuilt.priv, base.priv);
    assert.strictEqual(rebuilt.pub, base.pub);
  });

  it("pair from priv restores pub", async function () {
    var base = await ZEN.pair(null, { seed: "priv-restore" });
    var rebuilt = await ZEN.pair(null, { priv: base.priv });
    assert.strictEqual(rebuilt.priv, base.priv);
    assert.strictEqual(rebuilt.pub, base.pub);
  });

  it("seed types: empty, numeric, special chars, unicode all produce valid pairs", async function () {
    var seeds = ["", "12345", "!@#$%^&*()", "a".repeat(1000), "😀🔑🔒👍"];
    var pairs = await Promise.all(
      seeds.map(function (s) {
        return ZEN.pair(null, { seed: s });
      }),
    );
    pairs.forEach(function (p) {
      assert.strictEqual(typeof p.pub, "string");
      assert.ok(p.pub.length > 0);
    });
    var pubs = new Set(
      pairs.map(function (p) {
        return p.pub;
      }),
    );
    assert.strictEqual(pubs.size, pairs.length);
  });

  it("full workflow with seeded pair", async function () {
    var pair = await ZEN.pair(null, { seed: "workflow-seed" });
    var enc = await ZEN.encrypt("hello self", pair);
    var sig = await ZEN.sign(enc, pair);
    var msg = await ZEN.verify(sig, pair.pub);
    var dec = await ZEN.decrypt(msg, pair);
    assert.strictEqual(dec, "hello self");
    var h1 = await ZEN.hash(dec, pair);
    var h2 = await ZEN.hash("hello self", pair);
    assert.strictEqual(h1, h2);
  });
});

describe("ZEN crypto — derive (additive)", function () {
  this.timeout(10 * 1000);

  var base;
  before(async function () {
    base = await ZEN.pair();
  });

  it("deterministic for same priv + seed", async function () {
    var d1 = await ZEN.pair(null, { priv: base.priv, seed: "derive-det" });
    var d2 = await ZEN.pair(null, { priv: base.priv, seed: "derive-det" });
    assert.strictEqual(d1.priv, d2.priv);
    assert.strictEqual(d1.pub, d2.pub);
  });

  it("Bob (priv+seed) and Alice (pub+seed) produce same derived pub", async function () {
    var bob = await ZEN.pair(null, { priv: base.priv, seed: "derive-sign" });
    var alice = await ZEN.pair(null, { pub: base.pub, seed: "derive-sign" });
    assert.strictEqual(bob.pub, alice.pub);
  });

  it("Bob (priv+seed) and Alice (pub+seed) produce same derived pub", async function () {
    var bob = await ZEN.pair(null, { priv: base.priv, seed: "derive-enc" });
    var alice = await ZEN.pair(null, { pub: base.pub, seed: "derive-enc" });
    assert.strictEqual(bob.pub, alice.pub);
  });

  it("partial output: priv+seed gives priv+pub+address (no epriv/epub)", async function () {
    var d = await ZEN.pair(null, { priv: base.priv, seed: "derive-partial" });
    assert.ok(d.priv);
    assert.ok(d.pub);
    assert.strictEqual(d.epriv, undefined);
    assert.strictEqual(d.epub, undefined);
  });

  it("partial output: pub+seed gives pub+address only (no priv)", async function () {
    var d = await ZEN.pair(null, { pub: base.pub, seed: "derive-pub-partial" });
    assert.ok(d.pub);
    assert.ok(!d.priv);
    assert.strictEqual(d.epriv, undefined);
    assert.strictEqual(d.epub, undefined);
  });

  it("rejects invalid pub format", async function () {
    await assert.rejects(
      ZEN.pair(null, { pub: "invalid", seed: "derive-invalid" }),
    );
  });

  it("rejects pub not on curve", async function () {
    await assert.rejects(
      ZEN.pair(null, { pub: "AA.AA", seed: "derive-off-curve" }),
    );
  });

  it("rejects priv out of range", async function () {
    var n = BigInt(
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141",
    );
    var priv = Buffer.from(n.toString(16).padStart(64, "0"), "hex")
      .toString("base64")
      .replace(/\+/g, "-")
      .replace(/\//g, "_")
      .replace(/=/g, "");
    await assert.rejects(ZEN.pair(null, { priv, seed: "derive-priv-oor" }));
  });
});

describe("ZEN user graph — authenticator", function () {
  this.timeout(45 * 1000);

  it("put to user graph without being authenticated (provide pair)", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen
        .get("~" + bob.pub)
        .get("test")
        .put(
          "this is Bob",
          function (ack) {
            assert.ok(!ack.err);
            zen
              .get("~" + bob.pub)
              .get("test")
              .once(function (data) {
                assert.strictEqual(data, "this is Bob");
                done();
              });
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("put deep path with authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var enc = await ZEN.encrypt("secret", bob);
      var ref = zen.get("~" + bob.pub).get("a").get("b");
      ref.put(
        enc,
        function (ack) {
          if (ack && ack.err) {
            return done(new Error("put failed: " + ack.err));
          }
          ref.once(function (data) {
            try {
              assert.strictEqual(data, enc);
              done();
            } catch (e) {
              done(e);
            }
          });
        },
        { authenticator: bob },
      );
    })().catch(done);
  });

  it("accepts top-level authenticator options without mutating caller input", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var opts = { authenticator: bob };
      zen
        .get("~" + bob.pub)
        .get("top")
        .put(
          "level",
          function (ack) {
            if (ack && ack.err) {
              return done(new Error("put failed: " + ack.err));
            }
            try {
              assert.strictEqual(opts.authenticator, bob);
              assert.strictEqual(opts.pub, undefined);
            } catch (e) {
              return done(e);
            }
            zen
              .get("~" + bob.pub)
              .get("top")
              .once(function (data) {
                try {
                  assert.strictEqual(data, "level");
                  done();
                } catch (e) {
                  done(e);
                }
              });
          },
          opts,
        );
    })().catch(done);
  });

  it("does not leak authenticator on out", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen.on("out", function (msg) {
        if (msg.put) {
          try {
            assert.ok(
              !Object.prototype.propertyIsEnumerable.call(msg._ || {}, "zen"),
            );
            assert.ok(!(msg.opt || {}).authenticator);
          } catch (e) {
            done(e);
            return;
          }
        }
        this.to.next(msg);
      });
      zen
        .get("~" + bob.pub)
        .get("a")
        .get("b")
        .put(
          "x",
          function (ack) {
            if (ack && ack.err) {
              return done(new Error("put failed: " + ack.err));
            }
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects write without authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen
        .get("~" + bob.pub)
        .get("test")
        .put("no auth", function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });
});

describe("ZEN user graph — external authenticator", function () {
  this.timeout(20 * 1000);

  it("put to user graph using external async authenticator (nested ZEN.sign)", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      async function authenticator(data) {
        return ZEN.sign(data, bob);
      }
      zen
        .get("~" + bob.pub)
        .get("test")
        .put(
          "this is Bob",
          function (ack) {
            if (ack && ack.err) {
              return done(new Error("put failed: " + ack.err));
            }
            zen
              .get("~" + bob.pub)
              .get("test")
              .once(function (data) {
                try {
                  assert.strictEqual(data, "this is Bob");
                  done();
                } catch (e) {
                  done(e);
                }
              });
          },
          { authenticator: authenticator },
        );
    })().catch(done);
  });
});

describe("ZEN user graph — shard intermediate", function () {
  this.timeout(20 * 1000);

  it("accepts valid shard intermediate", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      var expected = "~/" + key;
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects shard intermediate when link target mismatches", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      zen
        .get("~")
        .get(key)
        .put(
          { "#": "~/zz" },
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects shard intermediate without authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      zen
        .get("~")
        .get(key)
        .put({ "#": "~/" + key }, function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });

  it("rejects shard path with double slash", function (done) {
    var zen = makeZen();
    zen
      .get("~/ab//cd")
      .get("ef")
      .put({ "#": "~/ab//cd/ef" }, function (ack) {
        assert.ok(ack.err);
        done();
      });
  });

  it("rejects shard with invalid key length", function (done) {
    var zen = makeZen();
    zen
      .get("~")
      .get("abc")
      .put({ "#": "~/abc" }, function (ack) {
        assert.ok(ack.err);
        done();
      });
  });

  it("rejects hash mismatch inside user graph", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen
        .get("~" + bob.pub)
        .get("payload#deadbeef")
        .put(
          "hello world",
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects hash mismatch at depth 2 under ~pub", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen
        .get("~" + bob.pub)
        .get("parent")
        .get("payload#deadbeef")
        .put(
          "hello world",
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects hash mismatch at depth 3 under ~pub", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      zen
        .get("~" + bob.pub)
        .get("parent")
        .get("child")
        .get("payload#deadbeef")
        .put(
          "hello world",
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects shard intermediate when value is not link", function (done) {
    var zen = makeZen();
    zen
      .get("~")
      .get("ab")
      .put("no-link", function (ack) {
        assert.ok(ack.err);
        done();
      });
  });

  it("accepts shard intermediate with external async authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      var expected = "~/" + key;
      async function authenticator(data) {
        return ZEN.sign(data, bob);
      }
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: authenticator, pub: bob.pub },
        );
    })().catch(done);
  });

  it("accepts shard intermediate with top-level function authenticator and pub", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      var expected = "~/" + key;
      async function authenticator(data) {
        return ZEN.sign(data, bob);
      }
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: authenticator, pub: bob.pub },
        );
    })().catch(done);
  });

  it("rejects shard intermediate with function authenticator but missing opt.pub", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      var expected = "~/" + key;
      async function authenticator(data) {
        return ZEN.sign(data, bob);
      }
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: authenticator },
        ); // no pub → claim undefined
    })().catch(done);
  });

  it("rejects shard intermediate with state too far in future", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      var expected = "~/" + key;
      var futureState = ZEN.state() + 1000 * 60 * 60 * 24 * 14; // 2 weeks ahead
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(ack.err);
            done();
          },
          { authenticator: bob },
          { state: futureState },
        );
    })().catch(done);
  });

  it("rejects shard intermediate with wrong pub prefix", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var alice = await ZEN.pair();
      var key = bob.pub.slice(0, 2);
      while (alice.pub.slice(0, 2) === key) {
        alice = await ZEN.pair();
      }
      var expected = "~/" + key;
      zen
        .get("~")
        .get(key)
        .put(
          { "#": expected },
          function (ack) {
            assert.ok(ack.err); // alice.pub doesn't start with key
            done();
          },
          { authenticator: alice },
        );
    })().catch(done);
  });

  it("rejects shard write when depth exceeds limit", function (done) {
    var zen = makeZen();
    var segs = Array(44).fill("ab");
    var soul = "~/" + segs.join("/");
    zen
      .get(soul)
      .get("cd")
      .put({ "#": soul + "/cd" }, function (ack) {
        assert.ok(ack.err);
        done();
      });
  });

  it("rejects shard path with trailing slash", function (done) {
    var zen = makeZen();
    zen
      .get("~/ab/cd/")
      .get("ef")
      .put({ "#": "~/ab/cd//ef" }, function (ack) {
        assert.ok(ack.err);
        done();
      });
  });
});

describe("ZEN user graph — shard leaf", function () {
  this.timeout(20 * 1000);

  it("rejects shard leaf when value is raw pub string", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put(bob.pub, function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });

  it("rejects shard leaf when value is null", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put(null, function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });

  it("rejects shard leaf when value is a number", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put(42, function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });

  it("rejects shard leaf when link points to wrong soul", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put({ "#": soul + "/" + key }, function (ack) {
          assert.ok(ack.err); // link must point to ~pub not intermediate path
          done();
        });
    })().catch(done);
  });

  it("rejects shard leaf without authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put({ "#": "~" + bob.pub }, function (ack) {
          assert.ok(ack.err);
          done();
        });
    })().catch(done);
  });

  it("put to shard leaf with authenticator pair", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put(
          { "#": "~" + bob.pub },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("put to shard leaf with external authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      async function authenticator(data) {
        return ZEN.sign(data, bob);
      }
      zen
        .get(soul)
        .get(key)
        .put(
          { "#": "~" + bob.pub },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: authenticator },
        );
    })().catch(done);
  });
});

describe("ZEN user graph — full shard chain", function () {
  this.timeout(20 * 1000);

  it("registers full shard path (soul+key) with authenticator pair", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put(
          { "#": "~" + bob.pub },
          function (ack) {
            assert.ok(!ack.err);
            done();
          },
          { authenticator: bob },
        );
    })().catch(done);
  });

  it("rejects full shard chain when no authenticator", function (done) {
    var zen = makeZen();
    (async function () {
      var bob = await ZEN.pair();
      var chunks = bob.pub.match(/.{1,2}/g) || [];
      var key = chunks.pop();
      var soul = chunks.length ? "~/" + chunks.join("/") : "~";
      zen
        .get(soul)
        .get(key)
        .put({ "#": "~" + bob.pub }, function (ack) {
          assert.ok(ack.err);
          done();
        }); // no authenticator
    })().catch(done);
  });
});

describe("ZEN — hash-based namespace routing (Frozen/Across spaces)", function () {
  this.timeout(20 * 1000);

  it("stores and retrieves data under a hash-keyed namespace", function (done) {
    var zen = makeZen();
    (async function () {
      var alice = await ZEN.pair();
      // Write to user graph
      await new Promise(function (res, rej) {
        zen.get("~" + alice.pub).put(
          { name: "Alice", country: "USA" },
          function (ack) {
            if (ack && ack.err) {
              return rej(new Error(ack.err));
            }
            res();
          },
          { authenticator: alice },
        );
      });
      // Write to hash namespace
      var data = "hello world";
      var hash = await ZEN.hash(data, null, null, { name: "SHA-256" });
      hash = hash.slice(-20);
      await new Promise(function (res, rej) {
        zen
          .get("#users")
          .get(hash)
          .put(data, function (ack) {
            if (ack && ack.err) {
              return rej(new Error(ack.err));
            }
            res();
          });
      });
      var result = await new Promise(function (res) {
        zen
          .get("#users")
          .get(hash)
          .once(function (v) {
            res(v);
          });
      });
      assert.strictEqual(result, data);
      done();
    })().catch(done);
  });
});
