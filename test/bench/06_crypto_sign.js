/**
 * test/bench/06_crypto_sign.js — Benchmark the signing pipeline.
 *
 * ZEN.sign() hot path:
 *   1. normalizeMessage(data)  → JSON.stringify
 *   2. shaBytes(msg)           → WebCrypto SHA-256
 *   3. deterministicK(priv,h)  → WebCrypto HMAC (RFC 6979)
 *   4. pointMultiply(k, G)     → V8 BigInt (double-and-add)
 *   5. mod arithmetic          → V8 BigInt
 *   6. encodeBase64(sig)       → base64 string
 *
 * Steps 3+4 dominate. We already know WASM loses here.
 * This bench confirms the baseline and helps detect regressions.
 *
 * Run:
 *   node --experimental-vm-modules test/bench/06_crypto_sign.js
 */

import { suite, bench, run } from "./harness.js";
import ZEN from "../../zen.js";

// ── Generate keypairs for testing ─────────────────────────────────────────────
const K1_PAIR = await ZEN.pair(null, { curve: "secp256k1" });
const P2_PAIR = await ZEN.pair(null, { curve: "p256" });
const DEFAULT_PAIR = await ZEN.pair(); // default curve

const SHORT_MSG = "hello";
const MEDIUM_MSG = "x".repeat(256);

suite("ZEN.pair() key generation", () => {
  bench("pair() default curve", () => ZEN.pair());
  bench("pair() secp256k1", () => ZEN.pair(null, { curve: "secp256k1" }));
  bench("pair() p256", () => ZEN.pair(null, { curve: "p256" }));
});

suite("ZEN.sign() + verify()", () => {
  bench("sign short msg (default)", () => ZEN.sign(SHORT_MSG, DEFAULT_PAIR));
  bench("sign medium msg (default)", () => ZEN.sign(MEDIUM_MSG, DEFAULT_PAIR));
  bench("sign short msg (k1)", () => ZEN.sign(SHORT_MSG, K1_PAIR));
  bench("sign short msg (p256)", () => ZEN.sign(SHORT_MSG, P2_PAIR));
  bench("verify short msg (default)", async () => {
    const sig = await ZEN.sign(SHORT_MSG, DEFAULT_PAIR);
    return ZEN.verify(sig, DEFAULT_PAIR.pub);
  });
});

suite("ZEN.encrypt() + decrypt()", () => {
  bench("encrypt short string", () => ZEN.encrypt(SHORT_MSG, DEFAULT_PAIR));
  bench("encrypt medium string", () => ZEN.encrypt(MEDIUM_MSG, DEFAULT_PAIR));
  bench("decrypt (round-trip)", async () => {
    const enc = await ZEN.encrypt(SHORT_MSG, DEFAULT_PAIR);
    return ZEN.decrypt(enc, DEFAULT_PAIR);
  });
});

suite("ZEN.secret() ECDH", () => {
  bench("secret(pub, pair)", async () => {
    return ZEN.secret(K1_PAIR.pub, P2_PAIR).catch(() => null);
  });
  bench("secret + encrypt", async () => {
    const sec = await ZEN.secret(DEFAULT_PAIR.pub, DEFAULT_PAIR);
    return ZEN.encrypt(SHORT_MSG, sec);
  });
});

// ── Component-level benchmarks ───────────────────────────────────────────────
import secp256k1Mod from "../../src/curves/secp256k1.js";
const k1 = secp256k1Mod.default ?? secp256k1Mod;

suite("secp256k1 primitives", () => {
  const scalar = 0xdeadbeefcafebabedeadbeefcafebabe_deadbeefcafebabedeadbeefcafebabebn;
  bench("pointMultiply k*G", () => k1.pointMultiply(scalar, k1.G));
  bench("modInv (N)", () => k1.modInv(scalar, k1.N));
  bench("mod (P)", () => k1.mod(scalar * scalar, k1.P));
});

await run({ warmup: 20, iters: 100 });
