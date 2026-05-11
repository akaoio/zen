import assert from "assert";
import ZEN from "../../zen.js";

function makeZEN(label) {
  return new ZEN({
    peers: [],
    localStorage: false,
    file: "tmp/testzen-" + label + "-" + String(Math.random()).slice(2),
  });
}

function put(node, data, opt) {
  return new Promise(function (resolve, reject) {
    node.put(
      data,
      function (ack) {
        if (ack && ack.err) {
          reject(new Error(ack.err));
          return;
        }
        resolve(ack);
      },
      opt,
    );
  });
}

function once(node, opt) {
  opt = opt || {};
  return new Promise(function (resolve, reject) {
    var timer = setTimeout(function () {
      reject(new Error("Timed out waiting for once()"));
    }, opt.timeout || 5000);
    node.once(function (data, key) {
      clearTimeout(timer);
      resolve({ data: data, key: key });
    });
  });
}

describe("test:zen", function () {
  this.timeout(20 * 1000);

  it("creates a ZEN instance with graph and crypto methods", function () {
    var zen = makeZEN("surface");
    [
      "get",
      "put",
      "once",
      "map",
      "set",
      "back",
      "opt",
      "pair",
      "sign",
      "verify",
      "encrypt",
      "decrypt",
      "secret",
      "hash",
      "pen",
      "candle",
      "pack",
      "unpack",
      "run",
    ].forEach(function (name) {
      assert.strictEqual(typeof zen[name], "function", name);
    });
    assert.strictEqual(typeof zen.certify, "function");
  });

  it("writes and reads nested graph data through a ZEN instance", async function () {
    var zen = makeZEN("nested");
    await put(zen.get("profile").get("name"), "Zen");
    await put(zen.get("profile").get("nested").get("level"), 1);
    var name = await once(zen.get("profile").get("name"));
    var nested = await once(zen.get("profile").get("nested").get("level"));
    assert.strictEqual(name.data, "Zen");
    assert.strictEqual(nested.data, 1);
  });

  it("awaits put ack without losing read-then semantics", async function () {
    var zen = makeZEN("put-then");
    var node = zen.get("profile").get("status");
    var write = await node.put("ready");
    assert.strictEqual(typeof write, "object");
    var read = await node;
    assert.strictEqual(read, "ready");
  });

  it("iterates set members through map on the same ZEN instance", async function () {
    var zen = makeZEN("set-map");
    var alice = zen.get("users/alice");
    var bob = zen.get("users/bob");
    await put(alice, { name: "Alice" });
    await put(bob, { name: "Bob" });
    zen.get("users").set(alice);
    zen.get("users").set(bob);

    var seen = {};
    await new Promise(function (resolve, reject) {
      var timer = setTimeout(function () {
        reject(new Error("Timed out waiting for mapped set items"));
      }, 5000);
      zen
        .get("users")
        .map()
        .once(function (node) {
          if (!node || !node.name) {
            return;
          }
          seen[node.name] = true;
          if (seen.Alice && seen.Bob) {
            clearTimeout(timer);
            resolve();
          }
        });
    });
  });

  it("keeps separate ZEN instances isolated", async function () {
    var alpha = makeZEN("alpha");
    var beta = makeZEN("beta");
    await put(alpha.get("profile"), { owner: "alpha" });
    await put(beta.get("profile"), { owner: "beta" });
    var a = await once(alpha.get("profile").get("owner"));
    var b = await once(beta.get("profile").get("owner"));
    assert.strictEqual(a.data, "alpha");
    assert.strictEqual(b.data, "beta");
    assert.notStrictEqual(alpha.chain(), beta.chain());
  });

  it("signs, verifies, encrypts, and decrypts through a ZEN instance", async function () {
    var zen = makeZEN("crypto");
    var pair = await zen.pair();
    var signed = await zen.sign({ hello: "zen" }, pair);
    var verified = await zen.verify(signed, pair.pub);
    var encrypted = await zen.encrypt("secret", pair);
    var decrypted = await zen.decrypt(encrypted, pair);
    assert.strictEqual(signed.startsWith("ZEN"), false);
    assert.strictEqual(encrypted.startsWith("ZEN"), false);
    assert.deepStrictEqual(verified, { hello: "zen" });
    assert.strictEqual(decrypted, "secret");
    assert.strictEqual(pair.curve, "secp256k1");
  });

  it("derives a shared secret through ZEN instance helpers", async function () {
    var zen = makeZEN("secret");
    var alice = await zen.pair();
    var bob = await zen.pair();
    var toBob = await zen.secret(bob.pub, alice);
    var toAlice = await zen.secret(alice.pub, bob);
    var encrypted = await zen.encrypt("shared data", toBob);
    var decrypted = await zen.decrypt(encrypted, toAlice);
    assert.strictEqual(decrypted, "shared data");
  });

  it("runs deterministic hash and PEN pack/unpack through ZEN instance methods", async function () {
    var zen = makeZEN("pen");
    var pair = await zen.pair();
    var proof1 = await zen.hash("hello zen", pair);
    var proof2 = await zen.hash("hello zen", pair);
    var soul = zen.pen({ key: "fixed" });
    var raw = Uint8Array.from([1, 2, 3, 4]);
    var packed = zen.pack(raw);
    var unpacked = zen.unpack(packed);
    assert.strictEqual(proof1, proof2);
    assert.strictEqual(proof1.includes("/"), false);
    assert.strictEqual(typeof soul, "string");
    assert.strictEqual(soul.startsWith("!"), true);
    assert.deepStrictEqual(Array.from(unpacked), Array.from(raw));
  });

  it("exposes hash() and supports keccak256", async function () {
    var zen = makeZEN("hash");
    var shaViaHash = await zen.hash("hello zen", null, null, {
      name: "SHA-256",
      encode: "hex",
    });
    var keccak = await zen.hash("", null, null, {
      name: "keccak256",
      encode: "hex",
    });
    assert.strictEqual(typeof zen.work, "undefined");
    assert.strictEqual(
      keccak,
      "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470",
    );
    assert.strictEqual(shaViaHash.length, 64);
  });
});
