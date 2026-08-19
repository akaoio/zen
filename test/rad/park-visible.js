import { withRadisk, readNow } from "./harness.js";
import assert from "assert";

// From the moment a write is handed to radisk until it is durable, a read for
// that key must never be told it is missing. That is the one promise storage
// makes, and a flush in flight is exactly when it is hard to keep: the file on
// disk is still the pre-flush version and the in-memory radix has been dropped.
//
// A write landing in that window is parked, and there are two places it can
// park. `s.mix` parks it in the flush's queue AND records it in `r.held`, the
// index a read falls back on. `s.find` parks it in the queue only.
//
// The queue is per file, which is enough right up until the flush splits. A
// split publishes a new file name mid-flush, the directory starts routing the
// key there, and a read then looks in a file that has never heard of it --
// while the value sits in the queue of the file it used to belong to. Nothing
// is left to catch it, because `r.held` was never told.
//
// Rather than pick a moment and hope, this walks the flush: every step of it,
// crossed with a probe in each part of the key space, since a write only parks
// if it routes to the file being written.
describe("a write parked on a flush stays visible", function () {
  this.timeout(120 * 1000);

  var STEPS = 40;
  var KEYS = 24;
  var PROBES = ["names/n00x", "names/n05x", "names/n09x", "names/n13x", "names/n17x", "names/n21x", "names/n25x"];

  function key(i) {
    return "names/n" + (i < 10 ? "0" + i : i);
  }
  function noop() {}

  function scenario(k, probe, j) {
    // chunk is small so the flush splits, which is what moves the directory
    // out from under a parked write
    return withRadisk({ until: 250, chunk: 120 }, function (r, ctx) {
      for (var i = 0; i < KEYS; i++) {
        r(key(i), { ":": "V" + i, ">": 1 }, noop);
      }
      for (var s = 0; s < k && ctx.step(); s++) {}
      var busy = r.busy();
      r(probe, { ":": "PROBE", ">": 2 }, noop);
      // let the flush get further along -- a split publishes new file names
      // as it goes, and that is what moves the key's route
      for (var m = 0; m < j && ctx.step(); m++) {}
      var got = readNow(r, probe);
      ctx.settle();
      return { k: k, j: j, probe: probe, busy: busy, called: got.called, data: got.data };
    });
  }

  it("is never answered 'no such key', at any point of the flush", function () {
    var lost = [];
    var parked = 0;
    for (var k = 0; k <= STEPS; k++) {
      for (var p = 0; p < PROBES.length; p++) {
        for (var j = 0; j <= 20; j++) {
          var out = scenario(k, PROBES[p], j);
          if (out.busy) {
            parked++;
          }
          if (!out.called || undefined === out.data) {
            lost.push(out);
          }
        }
      }
    }
    // A sweep that never parks a write proves nothing about parked writes.
    assert.ok(
      parked > 0,
      "no case in this sweep wrote while a flush was in flight; it tests nothing",
    );
    assert.deepStrictEqual(
      lost.map(function (o) {
        return "park@" + o.k + " read@+" + o.j + " " + o.probe + (o.called ? " answered missing" : " never answered");
      }),
      [],
      "a parked write went invisible",
    );
  });
});
