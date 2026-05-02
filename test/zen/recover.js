// zen.recover — TDD tests (written before implementation)
// zen.recover(sig) takes a signature produced by zen.sign (new format with v)
// and returns the pub key of the signer — without needing to supply pub.
import assert from "assert";
import ZEN from "../../zen.js";

describe("ZEN.recover — basic", function () {
  this.timeout(20 * 1000);

  var pair, sig;
  before(async function () {
    pair = await ZEN.pair();
    sig = await ZEN.sign("hello", pair);
  });

  it("ZEN.recover is a function", function () {
    assert.strictEqual(typeof ZEN.recover, "function");
  });

  it("recover returns the signer pub", async function () {
    const pub = await ZEN.recover(sig);
    assert.strictEqual(pub, pair.pub);
  });

  it("recovered pub is 45-char base62 compressed", async function () {
    const pub = await ZEN.recover(sig);
    assert.match(pub, /^[A-Za-z0-9]{44}[01]$/, "pub must be 45-char compressed");
  });

  it("sign output now includes v field (0 or 1)", function () {
    // Compact format: <86-char base62 sig><v>:<msg> — recovery bit is at position 86
    assert.ok(sig[86] === "0" || sig[86] === "1", "v must be 0 or 1");
  });

  it("recover is deterministic — same sig same pub", async function () {
    const pub1 = await ZEN.recover(sig);
    const pub2 = await ZEN.recover(sig);
    assert.strictEqual(pub1, pub2);
  });
});

describe("ZEN.recover — cross-check with verify", function () {
  this.timeout(20 * 1000);

  var alice, bob;
  before(async function () {
    [alice, bob] = await Promise.all([ZEN.pair(), ZEN.pair()]);
  });

  it("verify still works alongside recover (backward compat)", async function () {
    const sig = await ZEN.sign("cross check", alice);
    const recovered = await ZEN.recover(sig);
    const verified = await ZEN.verify(sig, recovered);
    assert.strictEqual(verified, "cross check");
  });

  it("alice and bob produce different recovered pubs", async function () {
    const sigA = await ZEN.sign("msg", alice);
    const sigB = await ZEN.sign("msg", bob);
    const pubA = await ZEN.recover(sigA);
    const pubB = await ZEN.recover(sigB);
    assert.strictEqual(pubA, alice.pub);
    assert.strictEqual(pubB, bob.pub);
    assert.notStrictEqual(pubA, pubB);
  });
});

describe("ZEN.recover — JS types", function () {
  this.timeout(20 * 1000);

  var pair;
  before(async function () {
    pair = await ZEN.pair();
  });

  var cases = [null, true, false, 0, 1, "hello", { a: 1 }, [1, 2]];

  cases.forEach(function (val) {
    it("recover works for " + JSON.stringify(val), async function () {
      const sig = await ZEN.sign(val, pair);
      const pub = await ZEN.recover(sig);
      assert.strictEqual(pub, pair.pub);
    });
  });
});

describe("ZEN.recover — error cases", function () {
  this.timeout(10 * 1000);

  var pair, sig;
  before(async function () {
    pair = await ZEN.pair();
    sig = await ZEN.sign("data", pair);
  });

  it("throws (or returns undefined via cb) when v is missing", async function () {
    // Build a JSON-format input without v so recover() can't find recovery bit
    const sig86 = sig.slice(0, 86);
    const msgPart = sig.slice(88); // message after <sig><v>:
    const noV = JSON.stringify({ s: sig86, m: msgPart });
    await assert.rejects(
      ZEN.recover(noV),
      /recovery bit|v/i,
    );
  });

  it("tampered sig recovers a different (wrong) pub", async function () {
    // Flip the first char of the 86-char base62 sig in the compact string
    const origChar = sig[0];
    const newChar = origChar === "A" ? "B" : "A";
    const tampered = newChar + sig.slice(1);
    // Recovery may succeed but should return a different (wrong) public key
    let result;
    try {
      result = await ZEN.recover(tampered);
    } catch (e) {
      // Throwing is also acceptable
      return;
    }
    assert.notStrictEqual(result, pair.pub);
  });

  it("recover returns undefined via callback on error", function (done) {
    ZEN.recover("not-a-sig", function (pub) {
      assert.strictEqual(pub, undefined);
      done();
    });
  });
});

describe("ZEN.recover — P-256 curve", function () {
  this.timeout(20 * 1000);

  it("recover works for P-256 pair", async function () {
    const pair = await ZEN.pair(null, { curve: "p256" });
    const sig = await ZEN.sign("p256 test", pair);
    const pub = await ZEN.recover(sig);
    assert.strictEqual(pub, pair.pub);
  });

  it("cross-curve: p256 sig forced to recover with secp256k1 gives wrong pub or throws", async function () {
    const pair = await ZEN.pair(null, { curve: "p256" });
    const sig = await ZEN.sign("cross curve test", pair);
    // Compact p256 format: <86><v>/p256:<msg> — strip the curve tag to force secp256k1 recovery
    const rest = sig.slice(88); // "p256:<msg>"
    const ci = rest.indexOf(":");
    const msgPart = rest.slice(ci + 1);
    const noC = sig.slice(0, 87) + ":" + msgPart; // secp256k1 format
    let result;
    try {
      result = await ZEN.recover(noC);
    } catch (e) {
      return;
    }
    assert.notStrictEqual(result, pair.pub);
  });

  it("cross-curve: secp256k1 sig forced to recover with p256 gives wrong pub or throws", async function () {
    const pair = await ZEN.pair(); // secp256k1
    const sig = await ZEN.sign("cross curve test", pair);
    // Compact secp256k1 format: <86><v>:<msg> — inject curve tag to force p256 recovery
    const withC = sig.slice(0, 87) + "/p256:" + sig.slice(88);
    let result;
    try {
      result = await ZEN.recover(withC);
    } catch (e) {
      // Throwing is acceptable — wrong curve, wrong curve constants
      return;
    }
    // If it doesn't throw, it must return a different (wrong) pub
    assert.notStrictEqual(result, pair.pub);
  });
});

describe("ZEN instance — recover method", function () {
  it("zen.recover mirrors ZEN.recover", async function () {
    const zen = new ZEN({ peers: [] });
    const pair = await ZEN.pair();
    const sig = await ZEN.sign("instance test", pair);
    const pub = await zen.recover(sig);
    assert.strictEqual(pub, pair.pub);
  });
});
