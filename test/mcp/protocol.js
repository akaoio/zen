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

// ─── helpers ──────────────────────────────────────────────────────────────────

async function kp() { return ZEN.pair(); }

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
  this.timeout(10000);

  it("returns a string starting with !", async function () {
    const alice = await kp();
    const soul  = inboxSoul(alice.pub);
    assert.ok(typeof soul === "string");
    assert.ok(soul.startsWith("!"), "soul must start with !");
  });

  it("is deterministic — same pub → same soul", async function () {
    const alice = await kp();
    assert.strictEqual(inboxSoul(alice.pub), inboxSoul(alice.pub));
  });

  it("different pubs → different souls", async function () {
    const alice = await kp();
    const bob   = await kp();
    assert.notStrictEqual(inboxSoul(alice.pub), inboxSoul(bob.pub));
  });

  it("adding pow option changes the soul (different bytecode)", async function () {
    const alice = await kp();
    const plain = inboxSoul(alice.pub);
    const withPow = inboxSoul(alice.pub, { pow: { unit: "0", difficulty: 1 } });
    assert.notStrictEqual(plain, withPow, "PoW adds a tail opcode, soul must differ");
  });
});

// ─── chanSoul ─────────────────────────────────────────────────────────────────

describe("protocol — chanSoul()", function () {
  this.timeout(10000);

  it("returns a string starting with !", async function () {
    const owner = await kp();
    const soul  = chanSoul("p1", "c1", owner.pub);
    assert.ok(soul.startsWith("!"));
  });

  it("is deterministic", async function () {
    const owner = await kp();
    assert.strictEqual(
      chanSoul("proj-1", "chan-1", owner.pub),
      chanSoul("proj-1", "chan-1", owner.pub)
    );
  });

  it("different chan_id → different souls", async function () {
    const owner = await kp();
    assert.notStrictEqual(
      chanSoul("p1", "c1", owner.pub),
      chanSoul("p1", "c2", owner.pub)
    );
  });

  it("different proj_id → different souls", async function () {
    const owner = await kp();
    assert.notStrictEqual(
      chanSoul("p1", "c1", owner.pub),
      chanSoul("p2", "c1", owner.pub)
    );
  });

  it("different owner_pub → different souls (cert in bytecode)", async function () {
    const o1 = await kp();
    const o2 = await kp();
    assert.notStrictEqual(
      chanSoul("p1", "c1", o1.pub),
      chanSoul("p1", "c1", o2.pub)
    );
  });
});

// ─── dmSoul ───────────────────────────────────────────────────────────────────

describe("protocol — dmSoul()", function () {
  this.timeout(10000);

  it("returns a string starting with !", async function () {
    const bob  = await kp();
    const soul = dmSoul(bob.pub);
    assert.ok(soul.startsWith("!"));
  });

  it("is deterministic", async function () {
    const bob = await kp();
    assert.strictEqual(dmSoul(bob.pub), dmSoul(bob.pub));
  });

  it("different recipients → different souls", async function () {
    const bob   = await kp();
    const carol = await kp();
    assert.notStrictEqual(dmSoul(bob.pub), dmSoul(carol.pub));
  });

  it("default includes PoW (soul differs from pow:false)", async function () {
    const bob = await kp();
    const withPow    = dmSoul(bob.pub);
    const withoutPow = dmSoul(bob.pub, { pow: false });
    assert.notStrictEqual(withPow, withoutPow, "default DM soul must have PoW tail");
  });

  it("pow:false removes PoW policy (shorter soul)", async function () {
    const bob = await kp();
    assert.ok(dmSoul(bob.pub).length > dmSoul(bob.pub, { pow: false }).length);
  });
});

// ─── chanSeed ─────────────────────────────────────────────────────────────────

describe("protocol — chanSeed()", function () {
  this.timeout(10000);

  it("returns a non-empty string", async function () {
    const owner = await kp();
    const seed  = await chanSeed(owner.priv, "p1", "c1", 1);
    assert.ok(typeof seed === "string" && seed.length > 0);
  });

  it("is deterministic for same inputs", async function () {
    const owner = await kp();
    const [s1, s2] = await Promise.all([
      chanSeed(owner.priv, "p1", "c1", 1),
      chanSeed(owner.priv, "p1", "c1", 1),
    ]);
    assert.strictEqual(s1, s2);
  });

  it("different version → different seed", async function () {
    const owner = await kp();
    const [s1, s2] = await Promise.all([
      chanSeed(owner.priv, "p1", "c1", 1),
      chanSeed(owner.priv, "p1", "c1", 2),
    ]);
    assert.notStrictEqual(s1, s2);
  });

  it("different proj_id → different seed", async function () {
    const owner = await kp();
    const [s1, s2] = await Promise.all([
      chanSeed(owner.priv, "proj-a", "c1", 1),
      chanSeed(owner.priv, "proj-b", "c1", 1),
    ]);
    assert.notStrictEqual(s1, s2);
  });

  it("different owner_priv → different seed (security: priv is the secret)", async function () {
    const o1 = await kp();
    const o2 = await kp();
    const [s1, s2] = await Promise.all([
      chanSeed(o1.priv, "p1", "c1", 1),
      chanSeed(o2.priv, "p1", "c1", 1),
    ]);
    assert.notStrictEqual(s1, s2);
  });

  it("SECURITY: public seed (no owner_priv) produces a different result", async function () {
    // An attacker who only knows public data cannot compute the real chan_seed.
    const owner    = await kp();
    const real     = await chanSeed(owner.priv, "p1", "c1", 1);
    // Simulate what attacker would try: hash of public-only data
    const attacker = await ZEN.hash("p1" + "c1" + "v1", null, null, { name: "SHA-256", encode: "base62" });
    assert.notStrictEqual(real, attacker, "public seed must not equal real chan_seed");
  });
});

// ─── wrapChanKey ─────────────────────────────────────────────────────────────

describe("protocol — wrapChanKey() + ECDH decrypt", function () {
  this.timeout(15000);

  it("returns an object keyed by member pub", async function () {
    const owner   = await kp();
    const member  = await kp();
    const seed    = await chanSeed(owner.priv, "p1", "c1", 1);
    const chanPair = await ZEN.pair(null, { seed });

    const wrapped = await wrapChanKey(chanPair, [member.pub]);
    assert.ok(wrapped[member.pub], "wrapped key must exist for member pub");
    assert.ok(typeof wrapped[member.pub] === "string");
  });

  it("single member: wrap → ECDH decrypt → recovers chan_priv", async function () {
    const owner    = await kp();
    const member   = await kp();
    const seed     = await chanSeed(owner.priv, "p1", "c1", 1);
    const chanPair = await ZEN.pair(null, { seed });

    const wrapped = await wrapChanKey(chanPair, [member.pub]);

    // Member decrypts: ECDH(chan_pair.pub, member.priv) → shared → decrypt
    const shared    = await ZEN.secret(chanPair.pub, member);
    const recovered = await ZEN.decrypt(wrapped[member.pub], { priv: shared });
    assert.strictEqual(recovered, chanPair.priv, "member must recover chan_priv exactly");
  });

  it("multiple members each recover chan_priv independently", async function () {
    const owner    = await kp();
    const alice    = await kp();
    const bob      = await kp();
    const seed     = await chanSeed(owner.priv, "p1", "c1", 1);
    const chanPair = await ZEN.pair(null, { seed });

    const wrapped = await wrapChanKey(chanPair, [alice.pub, bob.pub]);

    const [sharedA, sharedB] = await Promise.all([
      ZEN.secret(chanPair.pub, alice),
      ZEN.secret(chanPair.pub, bob),
    ]);
    const [privA, privB] = await Promise.all([
      ZEN.decrypt(wrapped[alice.pub], { priv: sharedA }),
      ZEN.decrypt(wrapped[bob.pub],   { priv: sharedB }),
    ]);
    assert.strictEqual(privA, chanPair.priv);
    assert.strictEqual(privB, chanPair.priv);
  });

  it("each member's wrapped blob is distinct (different ECDH secret per member)", async function () {
    const owner    = await kp();
    const alice    = await kp();
    const bob      = await kp();
    const seed     = await chanSeed(owner.priv, "p1", "c1", 1);
    const chanPair = await ZEN.pair(null, { seed });

    const wrapped = await wrapChanKey(chanPair, [alice.pub, bob.pub]);
    assert.notStrictEqual(wrapped[alice.pub], wrapped[bob.pub]);
  });

  it("SECURITY: non-member cannot decrypt another member's wrapped key", async function () {
    const owner    = await kp();
    const alice    = await kp();
    const eve      = await kp(); // not a member
    const seed     = await chanSeed(owner.priv, "p1", "c1", 1);
    const chanPair = await ZEN.pair(null, { seed });

    const wrapped = await wrapChanKey(chanPair, [alice.pub]);

    // Eve tries to decrypt alice's wrapped key using her own ECDH
    const eveShared = await ZEN.secret(chanPair.pub, eve);
    let recovered = null;
    try { recovered = await ZEN.decrypt(wrapped[alice.pub], { priv: eveShared }); } catch (_) {}
    assert.notStrictEqual(recovered, chanPair.priv, "non-member must not recover chan_priv");
  });

  it("key rotation: new version produces different chan_pair", async function () {
    const owner = await kp();
    const [s1, s2] = await Promise.all([
      chanSeed(owner.priv, "p1", "c1", 1),
      chanSeed(owner.priv, "p1", "c1", 2),
    ]);
    const [p1, p2] = await Promise.all([
      ZEN.pair(null, { seed: s1 }),
      ZEN.pair(null, { seed: s2 }),
    ]);
    assert.notStrictEqual(p1.pub,  p2.pub);
    assert.notStrictEqual(p1.priv, p2.priv);
  });
});
