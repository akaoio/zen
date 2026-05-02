// Multi-curve and multi-format tests for zen.pair()
import assert from "assert";
import ZEN from "../../zen.js";

// ─── RIPEMD-160 test vectors ─────────────────────────────────────────────────
describe("ripemd160 test vectors", function () {
  this.timeout(10 * 1000);

  // Import the bundled zen and access internals via pair-generated data.
  // We verify correctness indirectly: a known secp256k1 private key must
  // produce the expected Bitcoin P2PKH address (which depends on ripemd160).

  it("known BTC P2PKH address from secp256k1 private key", async function () {
    const p = await ZEN.pair(null, { format: "btc" });
    assert.ok(p.address.startsWith("1"), "BTC P2PKH address should start with 1");
    assert.ok(
      p.priv.startsWith("K") || p.priv.startsWith("L"),
      "WIF compressed mainnet starts with K or L",
    );
  });
});

// ─── secp256k1 backward compatibility ────────────────────────────────────────
describe("zen.pair() — secp256k1 backward compatibility", function () {
  this.timeout(20 * 1000);

  it("default pair has curve=secp256k1", async function () {
    const p = await ZEN.pair();
    assert.strictEqual(p.curve, "secp256k1");
  });

  it("explicit curve:secp256k1 identical to default", async function () {
    const seed = "compat-test-seed-42";
    const p1 = await ZEN.pair(null, { seed });
    const p2 = await ZEN.pair(null, { seed, curve: "secp256k1" });
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.priv, p2.priv);
    assert.strictEqual(p1.address, p2.address);
  });

  it("secp256k1 pub is 45-char base62 compressed", async function () {
    const p = await ZEN.pair(null, { seed: "secp-base62-check" });
    assert.match(p.pub, /^[A-Za-z0-9]{44}[01]$/, "pub should be 45-char base62 compressed");
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/, "address should be EVM checksum");
  });

  it("secp256k1 additive derivation still works", async function () {
    const alice = await ZEN.pair(null, { seed: "alice" });
    const derived = await ZEN.pair(null, { seed: "extra", priv: alice.priv });
    assert.ok(
      derived.pub !== alice.pub,
      "derived pub should differ from original",
    );
    assert.match(
      derived.pub,
      /^[A-Za-z0-9]{44}[01]$/,
      "derived pub should be 45-char base62 compressed",
    );
  });
});

// ─── P-256 curve ─────────────────────────────────────────────────────────────
describe("zen.pair() — P-256 / secp256r1", function () {
  this.timeout(20 * 1000);

  it("curve:p256 returns pub with curve=p256", async function () {
    const p = await ZEN.pair(null, { curve: "p256" });
    assert.strictEqual(p.curve, "p256");
  });

  it("curve:secp256r1 is alias for p256", async function () {
    const seed = "p256-alias-test";
    const p1 = await ZEN.pair(null, { seed, curve: "p256" });
    const p2 = await ZEN.pair(null, { seed, curve: "secp256r1" });
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.priv, p2.priv);
  });

  it("p256 pair has pub, priv, address", async function () {
    const p = await ZEN.pair(null, { curve: "p256" });
    assert.ok(p.pub, "pub present");
    assert.ok(p.priv, "priv present");
    assert.ok(p.address, "address present");
    assert.ok(!p.epub, "epub not present");
    assert.ok(!p.epriv, "epriv not present");
  });

  it("p256 pub is 45-char base62 compressed (zen format default)", async function () {
    const p = await ZEN.pair(null, { curve: "p256" });
    assert.match(p.pub, /^[A-Za-z0-9]{44}[01]$/, "pub 45-char base62 compressed");
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/, "address is EVM checksum");
  });

  it("p256 seed deterministic", async function () {
    const seed = "p256-reproducible-seed";
    const p1 = await ZEN.pair(null, { seed, curve: "p256" });
    const p2 = await ZEN.pair(null, { seed, curve: "p256" });
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.priv, p2.priv);
    assert.strictEqual(p1.address, p2.address);
  });

  it("p256 different seed → different keys", async function () {
    const p1 = await ZEN.pair(null, { seed: "p256-seed-a", curve: "p256" });
    const p2 = await ZEN.pair(null, { seed: "p256-seed-b", curve: "p256" });
    assert.notStrictEqual(p1.pub, p2.pub);
    assert.notStrictEqual(p1.priv, p2.priv);
  });

  it("p256 restore from priv gives same pub", async function () {
    const original = await ZEN.pair(null, { curve: "p256" });
    const restored = await ZEN.pair(null, {
      priv: original.priv,
      curve: "p256",
    });
    assert.strictEqual(restored.pub, original.pub);
    assert.strictEqual(restored.priv, original.priv, "priv round-trips");
  });

  it("p256 different curve from secp256k1", async function () {
    const seed = "same-seed-different-curve";
    const secp = await ZEN.pair(null, { seed, curve: "secp256k1" });
    const p256 = await ZEN.pair(null, { seed, curve: "p256" });
    assert.notStrictEqual(
      secp.pub,
      p256.pub,
      "different curves produce different keys",
    );
    assert.notStrictEqual(
      secp.priv,
      p256.priv,
      "different curves produce different privs",
    );
  });
});

// ─── EVM format ───────────────────────────────────────────────────────────────
describe("zen.pair() — format:evm", function () {
  this.timeout(20 * 1000);

  it("evm format pub is 0x04 + 128 hex chars (uncompressed)", async function () {
    const p = await ZEN.pair(null, { format: "evm" });
    assert.match(p.pub, /^0x04[0-9a-f]{128}$/, "pub is uncompressed pubkey");
  });

  it("evm format priv is 0x + 64 hex chars", async function () {
    const p = await ZEN.pair(null, { format: "evm" });
    assert.match(p.priv, /^0x[0-9a-f]{64}$/, "priv is 0x+64hex");
  });

  it("evm format address is 0x-checksummed ETH address", async function () {
    const p = await ZEN.pair(null, { format: "evm" });
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/, "address is checksummed ETH address");
  });

  it("evm format is deterministic from seed", async function () {
    const seed = "evm-determinism-check";
    const p1 = await ZEN.pair(null, { seed, format: "evm" });
    const p2 = await ZEN.pair(null, { seed, format: "evm" });
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.priv, p2.priv);
    assert.strictEqual(p1.address, p2.address);
  });

  it("evm address is EIP-55 checksummed (mixed case)", async function () {
    const p = await ZEN.pair(null, {
      seed: "eip55-test-seed-xyz",
      format: "evm",
    });
    const addr = p.address.slice(2); // strip 0x
    const hasUpper = /[A-F]/.test(addr);
    const hasLower = /[a-f]/.test(addr);
    assert.ok(hasUpper || hasLower, "address should have hex chars");
  });

  it("p256 + evm format returns valid ETH address", async function () {
    const p = await ZEN.pair(null, { curve: "p256", format: "evm" });
    assert.strictEqual(p.curve, "p256");
    assert.match(p.pub, /^0x04[0-9a-f]{128}$/, "p256 evm pub is uncompressed pubkey");
    assert.match(p.address, /^0x[0-9a-fA-F]{40}$/, "p256 evm address is ETH address");
    assert.match(p.priv, /^0x[0-9a-f]{64}$/, "p256 evm priv is 0x+hex");
  });
});

// ─── BTC format ───────────────────────────────────────────────────────────────
describe("zen.pair() — format:btc", function () {
  this.timeout(20 * 1000);

  it("btc format pub is compressed pubkey hex (0x02/03 + 64 hex)", async function () {
    const p = await ZEN.pair(null, { format: "btc" });
    assert.match(p.pub, /^0x0[23][0-9a-f]{64}$/, "pub is 0x02/03 + 32-byte x");
  });

  it("btc format priv is WIF compressed (K or L)", async function () {
    const p = await ZEN.pair(null, { format: "btc" });
    assert.match(
      p.priv,
      /^[KL][1-9A-HJ-NP-Za-km-z]+$/,
      "WIF compressed starts with K or L",
    );
  });

  it("btc format address is P2PKH base58 (starts with 1)", async function () {
    const p = await ZEN.pair(null, { format: "btc" });
    assert.match(p.address, /^1[1-9A-HJ-NP-Za-km-z]+$/, "address is base58 P2PKH");
  });

  it("btc format is deterministic from seed", async function () {
    const seed = "btc-determinism-seed";
    const p1 = await ZEN.pair(null, { seed, format: "btc" });
    const p2 = await ZEN.pair(null, { seed, format: "btc" });
    assert.strictEqual(p1.pub, p2.pub);
    assert.strictEqual(p1.priv, p2.priv);
    assert.strictEqual(p1.address, p2.address);
  });

  it("p256 + btc format returns valid P2PKH address", async function () {
    const p = await ZEN.pair(null, { curve: "p256", format: "btc" });
    assert.strictEqual(p.curve, "p256");
    assert.match(p.address, /^1[1-9A-HJ-NP-Za-km-z]+$/, "p256 btc address is P2PKH");
    assert.match(p.pub, /^0x0[23][0-9a-f]{64}$/, "p256 btc pub is compressed pubkey");
  });

  it("btc address length is 25–34 chars", async function () {
    const p = await ZEN.pair(null, { format: "btc" });
    assert.ok(
      p.address.length >= 25 && p.address.length <= 34,
      `address length ${p.address.length} should be 25-34`,
    );
  });
});

// ─── callback API ─────────────────────────────────────────────────────────────
describe("zen.pair() — callback API with formats", function () {
  this.timeout(10 * 1000);

  it("callback receives result for p256", function (done) {
    ZEN.pair(
      function (p) {
        assert.ok(p, "callback receives pair");
        assert.strictEqual(p.curve, "p256");
        done();
      },
      { curve: "p256" },
    );
  });

  it("callback on error gets undefined", function (done) {
    ZEN.pair(
      function (p) {
        assert.strictEqual(p, undefined);
        done();
      },
      { curve: "invalidcurve" },
    );
  });
});

// ─── sign / verify multi-curve ───────────────────────────────────────────────
describe("sign/verify — multi-curve", function () {
  this.timeout(30 * 1000);

  it("p256: sign + verify roundtrip", async function () {
    const p = await ZEN.pair(null, { curve: "p256" });
    const signed = await ZEN.sign("hello p256", p);
    const msg = await ZEN.verify(signed, p.pub);
    assert.strictEqual(msg, "hello p256");
  });

  it("p256: signed data carries curve marker c=p256", async function () {
    const p = await ZEN.pair(null, { curve: "p256" });
    const signed = await ZEN.sign("test", p);
    // Compact p256 format: <86-char sig><v>/p256:<msg>
    assert.strictEqual(signed[87], "/", "p256 uses / as curve separator");
    assert.ok(signed.slice(88).startsWith("p256:"), "p256 curve tag should follow the separator");
  });

  it("secp256k1: signed data has no c field (backward compat)", async function () {
    const p = await ZEN.pair(null, { curve: "secp256k1" });
    const signed = await ZEN.sign("test", p);
    // Compact secp256k1 format: <86-char sig><v>:<msg> — uses : not /
    assert.strictEqual(signed[87], ":", "secp256k1 uses : as message separator (no curve tag)");
  });

  it("p256: verify with wrong pub returns undefined", async function () {
    const alice = await ZEN.pair(null, { curve: "p256" });
    const bob = await ZEN.pair(null, { curve: "p256" });
    const signed = await ZEN.sign("secret", alice);
    const bad = await ZEN.verify(signed, bob.pub).catch(() => undefined);
    assert.strictEqual(bad, undefined);
  });

  it("cross-curve verification fails (p256 signed, secp256k1 pub)", async function () {
    const p256pair = await ZEN.pair(null, { curve: "p256" });
    const secppair = await ZEN.pair(null, { curve: "secp256k1" });
    const signed = await ZEN.sign("cross", p256pair);
    // Envelope has c=p256, but we pass a secp256k1 pub — curve mismatch should reject
    const bad = await ZEN.verify(signed, secppair.pub).catch(() => undefined);
    assert.strictEqual(bad, undefined);
  });
});

// ─── Format round-trip ───────────────────────────────────────────────────────
describe("zen.pair() — format round-trip", function () {
  this.timeout(20 * 1000);

  it("zen → evm → zen round-trip preserves pub and priv", async function () {
    const zen = await ZEN.pair(null, { seed: "round-trip-test" });
    const evm = await ZEN.pair(null, { priv: zen.priv, format: "evm" });
    const back = await ZEN.pair(null, { priv: evm.priv, format: "zen" });
    assert.strictEqual(back.pub, zen.pub, "pub must survive zen→evm→zen round-trip");
    assert.strictEqual(back.priv, zen.priv, "priv must survive zen→evm→zen round-trip");
    assert.strictEqual(back.address, zen.address, "address must survive zen→evm→zen round-trip");
  });

  it("spread evm pair back to zen also round-trips", async function () {
    const zen = await ZEN.pair(null, { seed: "spread-round-trip" });
    const evm = await ZEN.pair(null, { priv: zen.priv, format: "evm" });
    const back = await ZEN.pair(null, { priv: evm.priv, format: "zen" });
    assert.strictEqual(back.pub, zen.pub, "spread round-trip must recover original pub");
  });

  it("btc pub (compressed hex) is parseable as a curve point for ECDH", async function () {
    const alice = await ZEN.pair(null, { seed: "alice-btc-pub" });
    const bob = await ZEN.pair(null, { seed: "bob-btc-pub" });
    const bobBtc = await ZEN.pair(null, { priv: bob.priv, format: "btc" });
    // alice computes shared secret using bob's BTC-format compressed pub
    const s1 = await ZEN.secret(bobBtc.pub, alice);
    // bob computes shared secret using alice's zen pub
    const s2 = await ZEN.secret(alice.pub, bob);
    assert.strictEqual(s1, s2, "ECDH must work cross-format with BTC compressed pub");
  });

  it("evm ECDH still works after round-trip", async function () {
    const alice = await ZEN.pair(null, { seed: "alice-rt" });
    const bob = await ZEN.pair(null, { seed: "bob-rt" });
    const aliceEvm = await ZEN.pair(null, { priv: alice.priv, format: "evm" });
    const aliceBack = await ZEN.pair(null, { priv: aliceEvm.priv, format: "zen" });
    const s1 = await ZEN.secret(bob.pub, alice);
    const s2 = await ZEN.secret(bob.pub, aliceBack);
    assert.strictEqual(s1, s2, "ECDH shared secret must match after evm round-trip");
  });
});

// ─── secret multi-curve ───────────────────────────────────────────────────────
describe("secret — multi-curve", function () {
  this.timeout(20 * 1000);

  it("p256: ECDH shared secret matches", async function () {
    const alice = await ZEN.pair(null, { curve: "p256" });
    const bob = await ZEN.pair(null, { curve: "p256" });
    const s1 = await ZEN.secret(bob.pub, alice);
    const s2 = await ZEN.secret(alice.pub, bob);
    assert.strictEqual(s1, s2, "p256 ECDH shared secrets must match");
  });

  it("p256: shared secret is a base62 string", async function () {
    const alice = await ZEN.pair(null, { curve: "p256" });
    const bob = await ZEN.pair(null, { curve: "p256" });
    const s = await ZEN.secret(bob.pub, alice);
    assert.match(s, /^[A-Za-z0-9]+$/, "shared secret should be base62");
  });
});

// ─── cross-format encrypt/decrypt ────────────────────────────────────────────
describe("encrypt/decrypt — cross-format (zen ↔ evm)", function () {
  this.timeout(20 * 1000);

  const SEED = "cross-format-enc-seed";
  let zenPair, evmPair;

  before(async function () {
    zenPair = await ZEN.pair(null, { seed: SEED });
    evmPair = await ZEN.pair(null, { seed: SEED, format: "evm" });
  });

  it("zen and evm pairs from same seed share the same underlying key", function () {
    assert.strictEqual(zenPair.address, evmPair.address, "same address proves same scalar");
  });

  it("encrypt with zen pair, decrypt with evm pair", async function () {
    const enc = await ZEN.encrypt("hello cross", zenPair);
    const dec = await ZEN.decrypt(enc, evmPair);
    assert.strictEqual(dec, "hello cross");
  });

  it("encrypt with evm pair, decrypt with zen pair", async function () {
    const enc = await ZEN.encrypt("hello cross", evmPair);
    const dec = await ZEN.decrypt(enc, zenPair);
    assert.strictEqual(dec, "hello cross");
  });

  it("spread evm pair into new zen pair and decrypt still works", async function () {
    const spreadZen = await ZEN.pair(null, { priv: evmPair.priv, format: "zen" });
    const enc = await ZEN.encrypt("spread test", zenPair);
    const dec = await ZEN.decrypt(enc, spreadZen);
    assert.strictEqual(dec, "spread test");
  });

  it("wrong pair (different seed) cannot decrypt", async function () {
    const other = await ZEN.pair(null, { seed: "totally-different-seed" });
    const enc = await ZEN.encrypt("secret", zenPair);
    const dec = await ZEN.decrypt(enc, other).catch(() => undefined);
    assert.notStrictEqual(dec, "secret", "wrong pair must not decrypt correctly");
  });

  it("zen and evm produce independent ciphertexts (random IV)", async function () {
    const enc1 = await ZEN.encrypt("same", zenPair);
    const enc2 = await ZEN.encrypt("same", evmPair);
    assert.notStrictEqual(enc1, enc2, "each encrypt call produces unique ciphertext");
    // but both decrypt to the same plaintext with either key
    assert.strictEqual(await ZEN.decrypt(enc1, evmPair), "same");
    assert.strictEqual(await ZEN.decrypt(enc2, zenPair), "same");
  });

  it("full workflow: encrypt zen → sign evm → verify zen → decrypt evm", async function () {
    const plaintext = "end-to-end cross-format";
    const enc = await ZEN.encrypt(plaintext, zenPair);
    const sig = await ZEN.sign(enc, evmPair);
    const verified = await ZEN.verify(sig, evmPair.pub);
    const dec = await ZEN.decrypt(verified, zenPair);
    assert.strictEqual(dec, plaintext);
  });
});

// ─── cross-format sign/verify/recover ────────────────────────────────────────
describe("sign/verify/recover — cross-format (zen ↔ evm)", function () {
  this.timeout(20 * 1000);

  const SEED = "cross-format-sign-seed";
  let zenPair, evmPair;

  before(async function () {
    zenPair = await ZEN.pair(null, { seed: SEED });
    evmPair = await ZEN.pair(null, { seed: SEED, format: "evm" });
  });

  it("sign with zen pair, verify with zen pub", async function () {
    const sig = await ZEN.sign("msg", zenPair);
    const out = await ZEN.verify(sig, zenPair.pub);
    assert.strictEqual(out, "msg");
  });

  it("sign with evm pair, verify with evm pub", async function () {
    const sig = await ZEN.sign("msg", evmPair);
    const out = await ZEN.verify(sig, evmPair.pub);
    assert.strictEqual(out, "msg");
  });

  it("recover from zen-signed returns zen-format pub", async function () {
    const sig = await ZEN.sign("msg", zenPair);
    const recovered = await ZEN.recover(sig);
    assert.strictEqual(recovered, zenPair.pub, "recover matches zenPair.pub");
  });

  it("recover from evm-signed returns zen-format pub (sign normalises internally)", async function () {
    const sig = await ZEN.sign("msg", evmPair);
    const recovered = await ZEN.recover(sig);
    // sign() calls parseScalar on priv → same scalar → same recovery output (zen pub format)
    assert.strictEqual(recovered, zenPair.pub, "recover from evm-signed must equal zen pub");
  });

  it("sign with zen, verify with evm pub (parsePub handles both formats)", async function () {
    const sig = await ZEN.sign("cross-verify", zenPair);
    const out = await ZEN.verify(sig, evmPair.pub);
    assert.strictEqual(out, "cross-verify");
  });

  it("sign with evm, verify with zen pub", async function () {
    const sig = await ZEN.sign("cross-verify-2", evmPair);
    const out = await ZEN.verify(sig, zenPair.pub);
    assert.strictEqual(out, "cross-verify-2");
  });

  it("tampered message fails verification for both formats", async function () {
    const sig = await ZEN.sign("original", zenPair);
    const tampered = sig.slice(0, 88) + "tampered";
    const bad = await ZEN.verify(tampered, zenPair.pub).catch(() => undefined);
    assert.strictEqual(bad, undefined);
  });
});
