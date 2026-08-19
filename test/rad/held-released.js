import Radisk from "../../lib/radisk.js";
import assert from "assert";

// `r.held` is the index of writes that are acknowledged but not yet on disk.
// It exists so a read landing in that window is not told the key is missing,
// and it is only meant to hold what is still in flight.
//
// It was added to on every save and taken from on every `s.ack` -- but `s.ack`
// runs once per flush, from the closure of whichever save happened to start
// the timer. Every other save for that file returns early: its callback goes
// on the queue and its own `s.ack` never runs. So its entry stayed in the
// index for the life of the process.
//
// On a relay that is every value ever written, held in memory for ever. It
// sits outside the graph, so none of the GRAPH_GC_* eviction reaches it.
describe("the acked-but-not-durable index", function () {
  this.timeout(30 * 1000);

  function radisk(name) {
    var wrote = {};
    return Radisk({
      // its own path, or Radisk hands back the instance another test built
      file: "tmp/held-" + name + "-" + String(Math.random()).slice(2, 8),
      until: 20,
      store: {
        get: function (file, cb) {
          cb(null, wrote[file]);
        },
        put: function (file, data, cb) {
          wrote[file] = data;
          cb(null, 1);
        },
        list: function (cb) {
          cb();
        },
      },
    });
  }

  function held(r) {
    return Object.keys(r.held || {});
  }

  function writeAll(r, keys) {
    return new Promise(function (resolve) {
      var left = keys.length;
      keys.forEach(function (k) {
        r(k, { ":": k.toUpperCase(), ">": 1 }, function (err) {
          assert.ok(!err, "write failed: " + err);
          if (!--left) resolve();
        });
      });
    });
  }

  it("lets go of every key the flush made durable, not just one", async function () {
    var r = radisk("release");
    await writeAll(r, ["a/1", "a/2", "a/3", "a/4", "a/5"]);
    assert.deepStrictEqual(
      held(r),
      [],
      "these are on disk and still in the index: " + held(r).join(", "),
    );
  });

  it("does not grow with every batch written", async function () {
    var r = radisk("grow");
    for (var batch = 0; batch < 5; batch++) {
      var keys = [];
      for (var i = 0; i < 10; i++) keys.push("c/" + batch + "/" + i);
      await writeAll(r, keys);
    }
    // 50 keys written and all of them durable. Anything left is a value the
    // process will hold until it exits.
    assert.strictEqual(
      held(r).length,
      0,
      "the index kept " + held(r).length + " of 50 keys after they landed",
    );
  });
});
