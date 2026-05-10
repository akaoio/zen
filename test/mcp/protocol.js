// test/mcp/protocol.js — unit tests for lib/protocol.js (ZACP helpers)
// Tests soul computation, channel seed security, and group key encryption.
// Run via: npm run test:mcp
import assert from "assert";
import ZEN from "../../zen.js";
import {
  INBOX_CANDLE,
  inboxSoul,
  chanSoul,
  dmSoul,
  chanSeed,
  wrapChanKey,
} from "../../lib/protocol.js";

// ─── shared fixtures — generated once before all tests ───────────────────────
// ZEN.pair() is slow (~300 ms each). Precompute everything up front in parallel
// so the individual it() blocks run as pure assertions with no crypto overhead.

let P;             // pool of 12 key pairs: P[0]…P[11]
let seedV1, seedV2;
let chanPairV1, chanPairV2;
let wrappedOne;    // wrapChanKey(chanPairV1, [P[1].pub])
let wrappedMulti;  // wrapChanKey(chanPairV1, [P[2].pub, P[3].pub])
let wrappedEve;    // wrapChanKey(chanPairV1, [P[4].pub]) — for non-member test
let seedDiffOwner; // chanSeed(P[5].priv, "p1", "c1", 1) — different owner

before(async function () {
  this.timeout(60000);
  // Generate all pairs in parallel to minimise wall time
  P = await Promise.all(Array.from({ length: 12 }, () => ZEN.pair()));
  // Derived seeds and channel pairs
  [seedV1, seedV2, seedDiffOwner] = await Promise.all([
    chanSeed(P[0].priv, "p1", "c1", 1),
    chanSeed(P[0].priv, "p1", "c1", 2),
    chanSeed(P[5].priv, "p1", "c1", 1),
  ]);
  [chanPairV1, chanPairV2] = await Promise.all([
    ZEN.pair(null, { seed: seedV1 }),
    ZEN.pair(null, { seed: seedV2 }),
  ]);
  // Pre-wrap channel keys
  [wrappedOne, wrappedMulti, wrappedEve] = await Promise.all([
    wrapChanKey(chanPairV1, [P[1].pub]),
    wrapChanKey(chanPairV1, [P[2].pub, P[3].pub]),
    wrapChanKey(chanPairV1, [P[4].pub]),
  ]);
});

// ─── INBOX_CANDLE ─────────────────────────────────────────────────────────────

describe("protocol — INBOX_CANDLE", function () {
  it("sep is \":\" (not \"_\")", function () {
    assert.strictEqual(INBOX_CANDLE.sep, ":");
  });

  it("size is 1 hour in ms", function () {
    assert.strictEqual(INBOX_CANDLE.size, 3600000);
  });

  it("allows ±2 segments (back:2, fwd:0)", function () {
    assert.strictEqual(INBOX_CANDLE.back, 2);
    assert.strictEqual(INBOX_CANDLE.fwd, 0);
  });
});

// ─── inboxSoul ────────────────────────────────────────────────────────────────

describe("protocol — inboxSoul()", function () {
  it("returns a string starting with !", function () {
    const soul = inboxSoul(P[0].pub);
    assert.ok(typeof soul === "string");
    assert.ok(soul.startsWith("!"), "soul must start with !");
  });

  it("is deterministic — same pub → same soul", function () {
    assert.strictEqual(inboxSoul(P[0].pub), inboxSoul(P[0].pub));
  });

  it("different pubs → different souls", function () {
    assert.notStrictEqual(inboxSoul(P[0].pub), inboxSoul(P[1].pub));
  });

  it("adding pow option changes the soul (different bytecode)", function () {
    const plain   = inboxSoul(P[0].pub);
    const withPow = inboxSoul(P[0].pub, { pow: { unit: "0", difficulty: 1 } });
    assert.notStrictEqual(plain, withPow, "PoW adds a tail opcode, soul must differ");
  });
});

// ─── chanSoul ─────────────────────────────────────────────────────────────────

describe("protocol — chanSoul()", function () {
  it("returns a string starting with !", function () {
    assert.ok(chanSoul("p1", "c1", P[0].pub).startsWith("!"));
  });

  it("is deterministic", function () {
    assert.strictEqual(
      chanSoul("proj-1", "chan-1", P[0].pub),
      chanSoul("proj-1", "chan-1", P[0].pub)
    );
  });

  it("different chan_id → different souls", function () {
    assert.notStrictEqual(
      chanSoul("p1", "c1", P[0].pub),
      chanSoul("p1", "c2", P[0].pub)
    );
  });

  it("different proj_id → different souls", function () {
    assert.notStrictEqual(
      chanSoul("p1", "c1", P[0].pub),
      chanSoul("p2", "c1", P[0].pub)
    );
  });

  it("different owner_pub → different souls (cert in bytecode)", function () {
    assert.notStrictEqual(
      chanSoul("p1", "c1", P[0].pub),
      chanSoul("p1", "c1", P[1].pub)
    );
  });
});

// ─── dmSoul ───────────────────────────────────────────────────────────────────

describe("protocol — dmSoul()", function () {
  it("returns a string starting with !", function () {
    assert.ok(dmSoul(P[0].pub).startsWith("!"));
  });

  it("is deterministic", function () {
    assert.strictEqual(dmSoul(P[0].pub), dmSoul(P[0].pub));
  });

  it("different recipients → different souls", function () {
    assert.notStrictEqual(dmSoul(P[0].pub), dmSoul(P[1].pub));
  });

  it("default includes PoW (soul differs from pow:false)", function () {
    const withPow    = dmSoul(P[0].pub);
    const withoutPow = dmSoul(P[0].pub, { pow: false });
    assert.notStrictEqual(withPow, withoutPow, "default DM soul must have PoW tail");
  });

  it("pow:false removes PoW policy (shorter soul)", function () {
    assert.ok(dmSoul(P[0].pub).length > dmSoul(P[0].pub, { pow: false }).length);
  });
});

// ─── chanSeed ─────────────────────────────────────────────────────────────────

describe("protocol — chanSeed()", function () {
  it("returns a non-empty string", function () {
    assert.ok(typeof seedV1 === "string" && seedV1.length > 0);
  });

  it("is deterministic for same inputs", async function () {
    const again = await chanSeed(P[0].priv, "p1", "c1", 1);
    assert.strictEqual(seedV1, again);
  });

  it("different version → different seed", function () {
    assert.notStrictEqual(seedV1, seedV2);
  });

  it("different proj_id → different seed", async function () {
    const s = await chanSeed(P[0].priv, "proj-b", "c1", 1);
    assert.notStrictEqual(seedV1, s);
  });

  it("different owner_priv → different seed (security: priv is the secret)", function () {
    assert.notStrictEqual(seedV1, seedDiffOwner);
  });

  it("SECURITY: public seed (no owner_priv) produces a different result", async function () {
    const attacker = await ZEN.hash("p1" + "c1" + "v1", null, null, { name: "SHA-256", encode: "base62" });
    assert.notStrictEqual(seedV1, attacker, "public seed must not equal real chan_seed");
  });
});

// ─── wrapChanKey ─────────────────────────────────────────────────────────────

describe("protocol — wrapChanKey() + ECDH decrypt", function () {
  this.timeout(15000);

  it("returns an object keyed by member pub", function () {
    assert.ok(wrappedOne[P[1].pub], "wrapped key must exist for member pub");
    assert.ok(typeof wrappedOne[P[1].pub] === "string");
  });

  it("single member: wrap → ECDH decrypt → recovers chan_priv", async function () {
    const shared    = await ZEN.secret(chanPairV1.pub, P[1]);
    const recovered = await ZEN.decrypt(wrappedOne[P[1].pub], { priv: shared });
    assert.strictEqual(recovered, chanPairV1.priv, "member must recover chan_priv exactly");
  });

  it("multiple members each recover chan_priv independently", async function () {
    const [sharedA, sharedB] = await Promise.all([
      ZEN.secret(chanPairV1.pub, P[2]),
      ZEN.secret(chanPairV1.pub, P[3]),
    ]);
    const [privA, privB] = await Promise.all([
      ZEN.decrypt(wrappedMulti[P[2].pub], { priv: sharedA }),
      ZEN.decrypt(wrappedMulti[P[3].pub], { priv: sharedB }),
    ]);
    assert.strictEqual(privA, chanPairV1.priv);
    assert.strictEqual(privB, chanPairV1.priv);
  });

  it("each member's wrapped blob is distinct (different ECDH secret per member)", function () {
    assert.notStrictEqual(wrappedMulti[P[2].pub], wrappedMulti[P[3].pub]);
  });

  it("SECURITY: non-member cannot decrypt another member's wrapped key", async function () {
    const eveShared = await ZEN.secret(chanPairV1.pub, P[6]); // P[6] is eve — not in wrappedEve members
    let recovered = null;
    try { recovered = await ZEN.decrypt(wrappedEve[P[4].pub], { priv: eveShared }); } catch (_) {}
    assert.notStrictEqual(recovered, chanPairV1.priv, "non-member must not recover chan_priv");
  });

  it("key rotation: new version produces different chan_pair", function () {
    assert.notStrictEqual(chanPairV1.pub,  chanPairV2.pub);
    assert.notStrictEqual(chanPairV1.priv, chanPairV2.priv);
  });
});
