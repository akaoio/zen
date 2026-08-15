import ZEN from "../../zen.js";
import assert from "assert";

// A node written twice must be delivered twice, even when the second write
// lands while the first batch is still open.
//
// Deliveries are coalesced per batch: the first one for a node parks on the
// batch and later messages for that node fold into it, on the understanding
// that they all say the same thing. The parked delivery carries no value of its
// own -- when it finally runs it reads whatever the node holds by then. So a
// node written again before the batch ended used to lose its earlier value
// completely: the messages carrying it had been folded away as repeats, and the
// parked delivery now spoke for the newer write.
//
// In the suite this surfaced as `unlink deeply nested` and three of its
// neighbours timing out about once in sixty runs on a loaded machine -- load is
// what stretches a batch long enough for the next write to land inside it. The
// traces showed six messages carrying the old value dropped one after another,
// then the parked delivery waking 70ms later and handing over the new one.
//
// Nothing here waits on timing. A listener that opts out of coalescing fires
// while the batch is still open, and the second write is made from inside it,
// so the two writes are always in that order and always inside one batch.
describe("a batch must not swallow a value", function () {
  this.timeout(30 * 1000);

  function instance() {
    return new ZEN({
      peers: [],
      multicast: false,
      axe: false,
      WebSocket: false,
      stats: false,
      localStorage: false,
      radisk: false,
      // its own store path: a shared one leaves the next file's tests talking
      // to this instance's leftovers
      file: "tmp/hatchswallow-" + String(Math.random()).slice(2, 8),
    });
  }

  it("delivers both values of a node written twice inside one batch", function (done) {
    var zen = instance();
    var tag = "hatchswallow" + String(Math.random()).slice(2, 8);
    var leaf = tag + "/LEAF";
    var seen = [];

    zen
      .get(tag)
      .get("leaf")
      .on(function (d) {
        if (d && d.code) {
          seen.push(d.code);
        }
      });

    // This one is not coalesced, so it runs while the batch is still open, and
    // the write it makes lands in the middle of that batch.
    var fired = false;
    zen
      .get(tag)
      .get("trigger")
      .on(
        function (d) {
          if (fired || !d) {
            return;
          }
          fired = true;
          zen.get(leaf).put({ code: "BB" });
        },
        { hatch: 1 },
      );

    zen.get(tag).put({
      leaf: { _: { "#": leaf }, code: "AA", county: { A1: "first" } },
      trigger: { go: 1 },
    });

    setTimeout(function () {
      assert.ok(fired, "the mid-batch write never happened, so this proves nothing");
      assert.deepStrictEqual(
        seen,
        ["AA", "BB"],
        "both values must arrive, in the order they were written; got " +
          JSON.stringify(seen),
      );
      done();
    }, 2000);
  });
});
