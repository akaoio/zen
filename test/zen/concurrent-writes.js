import assert from "assert";
import ZEN from "../../zen.js";

// Concurrent writes that each create a NEW intermediate node under a shared
// parent. Each such put has to resolve the child's soul with an internal GET
// on the shared parent, so this is the shape that exercises read/write
// interlocking. Every write must be acked.
//
// The failure this guards against is probabilistic — it deadlocks on some
// interleavings, not all — so the workload runs several times and any single
// stalled round fails the test.
function makeZEN(label) {
  return new ZEN({
    peers: [],
    multicast: false,
    axe: false,
    WebSocket: false,
    stats: false,
    radisk: true,
    file: "tmp/testzen-" + label + "-" + String(Math.random()).slice(2),
  });
}

function put(chain, data, authenticator, ms) {
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
    }, ms || 5000);
  });
}

describe("test:zen concurrent writes", function () {
  this.timeout(120 * 1000);

  it("acks every concurrent write that creates a new intermediate node", async function () {
    var me = await ZEN.pair();
    var root = "~" + me.pub;
    var ATTEMPTS = 6,
      ROUNDS = 4,
      BATCH = 8;

    for (var attempt = 0; attempt < ATTEMPTS; attempt++) {
      var zen = makeZEN("conc-" + attempt);
      for (var round = 0; round < ROUNDS; round++) {
        var jobs = [];
        for (var i = 0; i < BATCH; i++) {
          // ~pub/p<n>/k — a fresh intermediate node per write, all sharing ~pub
          var n = round * BATCH + i;
          jobs.push(put(zen.get(root).get("p" + n).get("k"), { a: n }, me));
        }
        var acks = await Promise.all(jobs);
        var stalled = acks.filter(function (ack) {
          return !ack || ack.err;
        });
        assert.strictEqual(
          stalled.length,
          0,
          "attempt " +
            attempt +
            " round " +
            round +
            ": " +
            stalled.length +
            "/" +
            BATCH +
            " writes never acked (" +
            JSON.stringify(stalled[0]) +
            ")",
        );
      }
    }
  });
});
