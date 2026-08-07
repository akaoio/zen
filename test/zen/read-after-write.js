import zenbase from "../../zen.js";
import "../../lib/store.js";
import "../../lib/rfs.js";
import assert from "assert";

// Write a batch, let everything settle, then read it back in the same process.
// Every record is in the in-memory graph and on disk, so every read must
// deliver. Reads that park on an in-flight write's stun list can be added to
// that list just after the write finished draining it, and are then never
// resumed -- once() simply never fires for them.
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

function put(chain, data, authenticator) {
  return new Promise(function (resolve) {
    var done = false;
    var fin = function (ack) {
      if (!done) {
        done = true;
        resolve(ack);
      }
    };
    chain.put(data, fin, { authenticator: authenticator });
    setTimeout(function () {
      fin({ err: "TIMEOUT" });
    }, 15000);
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
    }, ms || 3000);
  });
}

describe("read after write", function () {
  this.timeout(300 * 1000);

  it("delivers every record written earlier in the same process", async function () {
    var N = 200;
    var me = await zenbase.pair();
    var root = "~" + me.pub;
    var zen = makeZEN(
      "tmp/rawrite-" + String(Math.random()).slice(2),
    );

    var ackErr = 0;
    for (var i = 0; i < N; i++) {
      var ack = await put(zen.get(root).get("c").get("k" + i), { o: i }, me);
      if (ack && ack.err) {
        ackErr++;
      }
    }
    assert.strictEqual(ackErr, 0, ackErr + " writes failed to ack");

    await new Promise(function (r) {
      setTimeout(r, 3000);
    });

    var miss = [];
    for (var j = 0; j < N; j++) {
      var v = await read(zen.get(root).get("c").get("k" + j));
      if (!v || v.o !== j) {
        miss.push(j);
      }
    }
    assert.strictEqual(
      miss.length,
      0,
      miss.length + "/" + N + " reads never delivered: " + JSON.stringify(miss.slice(0, 10)),
    );
  });
});
