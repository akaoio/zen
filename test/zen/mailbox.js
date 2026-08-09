import zenbase from "../../zen.js";
import "../../lib/store.js";
import "../../lib/rfs.js";
import assert from "assert";
import fs from "node:fs";
import path from "node:path";

// The PEN mailbox (akao#189 §2): ONE public policy soul, every participant
// FORCED into their own slot — key = <writerPub><?N — with the pub taken
// from the RECOVERED signature (R5, sign:true), never from a claim. The
// <?N marker in the KEY makes each slot ephemeral: acked, served from RAM,
// never persisted, forgotten on restart. This is the private-namespace
// problem solved without certs: policy does the fencing, signatures do the
// identity, the marker does the forgetting.

const DIR = "tmp/mailbox-test-store";

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

function put(chain, data, opt) {
  return new Promise(function (resolve) {
    var done = false;
    var fin = function (ack) {
      if (!done) {
        done = true;
        resolve(ack || {});
      }
    };
    chain.put(data, fin, opt);
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

describe("PEN mailbox — policy-fenced slots with ephemeral keys", function () {
  this.timeout(30000);
  var soul, pairA, pairB, zen;

  before(async function () {
    fs.rmSync(DIR, { recursive: true, force: true });
    pairA = await zenbase.pair();
    pairB = await zenbase.pair();
    // signed writes only; key must END with <?60 and BEGIN with the
    // recovered writer pub — the fence every mailbox slot lives behind
    soul = await zenbase.pen({
      sign: true,
      key: {
        and: [
          { suf: "<?60" },
          {
            let: {
              bind: 0,
              def: { seg: { of: { reg: 0 }, sep: "<", idx: 0 } },
              body: { eq: [{ reg: 128 }, { reg: 5 }] },
            },
          },
        ],
      },
    });
    assert.ok(typeof soul === "string" && soul[0] === "!", "pen soul compiles to a !bytecode string");
    zen = makeZEN(DIR);
  });

  it("a writer lands in their OWN ephemeral slot: acked, served from RAM, zero bytes on disk", async function () {
    var ack = await put(zen.get(soul).get(pairA.pub + "<?60"), "mail-for-A", { authenticator: pairA });
    assert.ok(!ack.err, "own-slot write must ack: " + ack.err);
    assert.strictEqual(await read(zen.get(soul).get(pairA.pub + "<?60"), 3000), "mail-for-A");
    await sleep(1500);
    var blob = diskBytes(DIR);
    assert.strictEqual(blob.indexOf("mail-for-A"), -1, "an ephemeral slot value must never reach the disk");
  });

  it("the policy rejects a slot that is not the writer's own pub", async function () {
    var ack = await put(zen.get(soul).get(pairB.pub + "<?60"), "forged", { authenticator: pairA });
    assert.ok(ack.err, "writing into someone else's slot must be rejected");
    assert.strictEqual(await read(zen.get(soul).get(pairB.pub + "<?60"), 1500), undefined);
  });

  it("the policy rejects a slot without the ephemeral marker", async function () {
    var ack = await put(zen.get(soul).get(pairA.pub), "durable-sneak", { authenticator: pairA });
    assert.ok(ack.err, "a markerless key must be rejected — the mailbox is ephemeral by LAW");
  });

  it("a second writer gets their own independent slot", async function () {
    var ack = await put(zen.get(soul).get(pairB.pub + "<?60"), "mail-for-B", { authenticator: pairB });
    assert.ok(!ack.err, "B's own slot must ack: " + ack.err);
    assert.strictEqual(await read(zen.get(soul).get(pairB.pub + "<?60"), 3000), "mail-for-B");
    assert.strictEqual(await read(zen.get(soul).get(pairA.pub + "<?60"), 3000), "mail-for-A", "A's slot is untouched");
  });

  it("a restart forgets every slot", async function () {
    var again = makeZEN(DIR);
    assert.strictEqual(await read(again.get(soul).get(pairA.pub + "<?60"), 2500), undefined, "A's slot dies with the process");
    assert.strictEqual(await read(again.get(soul).get(pairB.pub + "<?60"), 2500), undefined, "B's slot dies with the process");
  });
});
