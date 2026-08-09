import zenbase from "../../zen.js";
import "../../lib/store.js";
import "../../lib/rfs.js";
import assert from "assert";
import fs from "node:fs";
import path from "node:path";

// <?N souls are EPHEMERAL by design (Mark's original marker): the security
// pipeline's `forget` stage already refuses STALE copies on sync — these
// tests pin down the other half, which the rewrite had lost: the data must
// never touch the disk at all. RAM is its only home; a restart forgets it.

const DIR = "tmp/ephemeral-test-store";

function makeZEN(dir) {
  return new zenbase({
    peers: [],
    multicast: false,
    axe: false,
    WebSocket: false,
    stats: false,
    localStorage: false,
    radisk: true,
    file: dir,
  });
}

function put(chain, data) {
  return new Promise(function (resolve) {
    var done = false;
    var fin = function (ack) {
      if (!done) {
        done = true;
        resolve(ack || {});
      }
    };
    chain.put(data, fin);
    setTimeout(function () {
      fin({ err: "TIMEOUT" });
    }, 10000);
  });
}

function read(chain, ms) {
  return new Promise(function (resolve) {
    var done = false;
    chain.once(function (v) {
      if (!done) {
        done = true;
        resolve(v);
      }
    });
    setTimeout(function () {
      if (!done) {
        done = true;
        resolve(undefined);
      }
    }, ms);
  });
}

function sleep(ms) {
  return new Promise(function (r) {
    setTimeout(r, ms);
  });
}

// Every byte the store wrote, concatenated — radata files are radix-packed
// text, so a soul or value that reached the disk is findable as a substring.
function diskBytes(dir) {
  var out = "";
  if (!fs.existsSync(dir)) {
    return out;
  }
  var walk = function (p) {
    for (var name of fs.readdirSync(p)) {
      var full = path.join(p, name);
      if (fs.statSync(full).isDirectory()) {
        walk(full);
      } else {
        out += fs.readFileSync(full, "utf8");
      }
    }
  };
  walk(dir);
  return out;
}

describe("<?N ephemeral souls — RAM only, never disk", function () {
  this.timeout(30000);

  before(function () {
    fs.rmSync(DIR, { recursive: true, force: true });
  });

  it("a put on an ephemeral soul ACKS but leaves no bytes in the store", async function () {
    var zen = makeZEN(DIR);
    var ack = await put(zen.get("whisper<?60").get("note"), "vanishes");
    assert.ok(!ack.err, "ephemeral put must still ack: " + ack.err);
    var control = await put(zen.get("ledger").get("note"), "persists");
    assert.ok(!control.err, "control put must ack: " + control.err);
    // reads must serve from RAM within the same process
    assert.strictEqual(await read(zen.get("whisper<?60").get("note"), 3000), "vanishes");

    await sleep(1500); // outlive the radisk batching window
    var blob = diskBytes(DIR);
    assert.ok(blob.indexOf("ledger") >= 0, "the durable control soul must reach the disk");
    assert.ok(blob.indexOf("persists") >= 0, "the durable control value must reach the disk");
    assert.strictEqual(blob.indexOf("whisper"), -1, "an ephemeral soul must never reach the disk");
    assert.strictEqual(blob.indexOf("vanishes"), -1, "an ephemeral value must never reach the disk");
  });

  it("a restart forgets ephemeral data and keeps the durable control", async function () {
    var again = makeZEN(DIR); // fresh RAM over the same store
    assert.strictEqual(await read(again.get("whisper<?60").get("note"), 3000), undefined, "ephemeral data dies with the process");
    assert.strictEqual(await read(again.get("ledger").get("note"), 8000), "persists", "durable data survives the restart");
  });
});
