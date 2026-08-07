import zenbase from "../../zen.js";
import "../../lib/store.js";
import "../../lib/rfs.js";
import assert from "assert";

// Radisk batches disk writes: on the first write it schedules a flush
// `opt.until` (250ms) later. While that flush is in flight the data is in
// neither place a read looks -- the in-memory radix for the file has been
// dropped and the file on disk has not been rewritten yet -- so a read landing
// in that window sees the pre-write file, reports "not found", and never
// re-fires when the data does land.
//
// The window moves a little from run to run, so probe a spread of delays
// around opt.until rather than a single one.
function makeZEN(dir) {
  return new zenbase({
    peers: [],
    multicast: false,
    axe: false,
    WebSocket: false,
    stats: false,
    localStorage: false,
    chunk: 1000,
    file: dir,
  });
}

var CONTACTS = 219;

function writeContacts(zen) {
  return new Promise(function (resolve) {
    var left = CONTACTS;
    for (var i = 0; i < CONTACTS; i++) {
      zen
        .get("names")
        .get("n" + i)
        .put({ name: "N" + i, age: i + 1 }, function () {
          if (--left === 0) {
            resolve();
          }
        });
    }
  });
}

function readOne(zen, key, ms) {
  return new Promise(function (resolve) {
    var done = false;
    zen
      .get("names")
      .get(key)
      .on(function (data) {
        if (!data || !data.name || !data.age) {
          return;
        }
        if (done) {
          return;
        }
        done = true;
        resolve(data);
      });
    setTimeout(function () {
      if (!done) {
        done = true;
        resolve(null);
      }
    }, ms || 15000);
  });
}

describe("RAD flush window", function () {
  this.timeout(120 * 1000);

  it("serves reads that land while a batch is being flushed", async function () {
    var DELAYS = [200, 220, 240, 250, 260, 280];

    for (var d = 0; d < DELAYS.length; d++) {
      var delay = DELAYS[d];
      var dir = "tmp/radflush-" + delay + "-" + String(Math.random()).slice(2);
      var zen = makeZEN(dir);
      await writeContacts(zen);
      await new Promise(function (r) {
        setTimeout(r, delay);
      });
      var t0 = Date.now();
      var got = await readOne(makeZEN(dir), "n7");
      var took = Date.now() - t0;
      assert.ok(
        got && got.name === "N7",
        "read " +
          delay +
          "ms after the writes acked returned " +
          JSON.stringify(got) +
          " sau " + took + "ms — the flush window swallowed it",
      );
    }
  });
});
