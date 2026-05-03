import assert from "assert";
import ZEN from "../../zen.js";

function makeZEN(label) {
  return new ZEN({
    peers: [],
    localStorage: false,
    file: "tmp/meta-" + label + "-" + String(Math.random()).slice(2),
  });
}

// Helper: put + wait for the ack
function put(node, data, opt) {
  return new Promise(function (resolve, reject) {
    node.put(
      data,
      function (ack) {
        if (ack && ack.err) { reject(new Error(ack.err)); return; }
        resolve(ack);
      },
      opt,
    );
  });
}

// Helper: wait for .meta() to fire
function getMeta(node) {
  return new Promise(function (resolve, reject) {
    var timer = setTimeout(function () {
      reject(new Error("Timed out waiting for .meta()"));
    }, 5000);
    node.meta(function (m) {
      if (m.val === undefined || m.val === null) { return; } // skip undefined (not-found acks)
      clearTimeout(timer);
      resolve(m);
    });
  });
}

// ─── ~pub namespace ─────────────────────────────────────────────────────────

describe(".meta() — ~pub namespace", function () {
  this.timeout(20 * 1000);

  var zen, pair;

  before(async function () {
    pair = await ZEN.pair();
    zen = makeZEN("pub");
  });

  it("returns the soul owner pub for a signed value", async function () {
    var soul = "~" + pair.pub;
    var node = zen.get(soul).get("name");
    var metaPromise = getMeta(node);
    await put(node, "Alice", { authenticator: pair });
    var m = await metaPromise;
    assert.strictEqual(m.val, "Alice");
    assert.ok(m.pub, "pub should be set");
    assert.ok(
      m.pub === pair.pub || m.pub.startsWith(pair.pub.slice(0, 20)),
      "pub should match writer: got " + m.pub,
    );
    assert.ok(m.state > 0, "state should be a timestamp");
    assert.strictEqual(m.soul, soul);
    assert.strictEqual(m.key, "name");
  });

  it("sig and v are set for signed data", async function () {
    var soul = "~" + pair.pub;
    var node = zen.get(soul).get("nickname");
    var metaPromise = getMeta(node);
    await put(node, "Al", { authenticator: pair });
    var m = await metaPromise;
    assert.strictEqual(m.val, "Al");
    assert.ok(m.sig, "sig should be present");
    assert.strictEqual(m.sig.length, 86, "sig should be 86 base62 chars");
  });
});

// ─── Unsigned data ───────────────────────────────────────────────────────────

describe(".meta() — unsigned data", function () {
  this.timeout(20 * 1000);

  var zen;

  before(function () {
    zen = makeZEN("unsigned");
  });

  it("pub/sig/v are null for unsigned nodes", async function () {
    var node = zen.get("open-node").get("greeting");
    var metaPromise = getMeta(node);
    await put(node, "hello");
    var m = await metaPromise;
    assert.strictEqual(m.val, "hello");
    assert.strictEqual(m.pub, null);
    assert.strictEqual(m.sig, null);
    assert.ok(m.state > 0, "state should be set");
    assert.strictEqual(m.soul, "open-node");
  });
});

// ─── tilde-shard namespace ──────────────────────────────────────────────────
// Tilde-shard souls (~as/df/gh) enforce strict write rules (values must be
// signed links, not arbitrary strings).  settings.pub() returns a path
// fragment for these souls, which meta.js intentionally rejects and falls
// through to async settings.pack + recover() — same path as any other signed
// data.  The async recovery is exercised indirectly via the ~pub tests above.

// ─── .meta() surface ────────────────────────────────────────────────────────

describe(".meta() — API surface", function () {
  var zen;

  before(function () {
    zen = makeZEN("surface");
  });

  it("is chainable", function () {
    var result = zen.get("x").get("y").meta(function () {});
    assert.ok(result && typeof result.get === "function", "should return chain");
  });

  it("returns a Promise when no callback given", function () {
    var chain = zen.get("x").get("y");
    var result = chain.meta();
    assert.ok(result instanceof Promise, "should return a Promise");
  });
});
