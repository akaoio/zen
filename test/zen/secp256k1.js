import assert from "assert";
import ZEN from "../../zen.js";

describe("ZEN secp256k1", function () {
  this.timeout(20 * 1000);

  it("derives deterministic secp256k1 pairs from the same seed", async function () {
    var first = await ZEN.pair(null, { seed: "same-seed" });
    var second = await ZEN.pair(null, { seed: "same-seed" });
    assert.strictEqual(first.curve, "secp256k1");
    assert.strictEqual(first.priv, second.priv);
    assert.strictEqual(first.pub, second.pub);
    assert.strictEqual(first.address, second.address);
    assert.strictEqual(first.epub, undefined);
    assert.strictEqual(first.epriv, undefined);
  });

  it("emits public keys that are valid secp256k1 points", async function () {
    var pair = await ZEN.pair();
    assert.strictEqual(pair.curve, "secp256k1");
    assert.strictEqual(typeof pair.pub, "string");
    assert.strictEqual(pair.pub.length, 45);
  });

  it("signs deterministically and rejects tampering", async function () {
    var pair = await ZEN.pair(null, { seed: "sign-seed" });
    var one = await ZEN.sign({ ok: true }, pair);
    var two = await ZEN.sign({ ok: true }, pair);
    var verified = await ZEN.verify(one, pair.pub);
    assert.strictEqual(one, two);
    assert.strictEqual(one.startsWith("ZEN"), false);
    assert.deepStrictEqual(verified, { ok: true });

    var tampered = JSON.parse(one);
    tampered.m = { ok: false };
    await assert.rejects(
      ZEN.verify(JSON.stringify(tampered), pair.pub),
      /Signature did not match/,
    );
  });

  it("rejects verification with the wrong public key", async function () {
    var alice = await ZEN.pair(null, { seed: "alice-seed" });
    var bob = await ZEN.pair(null, { seed: "bob-seed" });
    var signed = await ZEN.sign("hello", alice);
    await assert.rejects(
      ZEN.verify(signed, bob.pub),
      /Signature did not match/,
    );
  });

  it("derives symmetric shared secrets for secp256k1 encryption keys", async function () {
    var alice = await ZEN.pair(null, { seed: "alice-secret" });
    var bob = await ZEN.pair(null, { seed: "bob-secret" });
    var toBob = await ZEN.secret(bob.pub, alice);
    var toAlice = await ZEN.secret(alice.pub, bob);
    assert.strictEqual(toBob, toAlice);
    assert.strictEqual(typeof toBob, "string");
    assert.strictEqual(toBob.length, 44);
  });

  it("hashes with keccak256", async function () {
    var byHash = await ZEN.hash("hello", null, null, {
      name: "keccak256",
      encode: "hex",
    });
    assert.strictEqual(
      byHash,
      "1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8",
    );
  });
});
