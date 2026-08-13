// A reader that opens while a batch is being flushed used to be blind for the
// rest of its life. Radisk loads its directory of files once; if that load
// lands while the store has nothing to give -- a flush still in the air, a slow
// disk, another process mid-write -- the empty listing is kept, and every
// lookup afterwards falls back to the root file, which does not hold the key.
// The reader finds nothing again, ever, while one opened a moment later finds
// everything at once.
//
// The store here is the test's, so "the flush has not landed yet" is a switch
// rather than a race: no pinning, no timing, no luck.

import ZEN from "../../zen.js";
import "../../lib/store.js";
import assert from "assert";

const CONTACTS = 80;
const files = {};
let hold = false;
const held = [];

const store = {
  put(file, data, cb) {
    const land = () => { files[file] = data; cb(null, 1); };
    hold ? held.push(land) : setTimeout(land, 0);
  },
  get(file, cb) {
    const serve = () => cb(null, files[file]);
    hold ? held.push(serve) : setTimeout(serve, 0);
  },
  list(cb) { Object.keys(files).forEach((f) => cb(f)); cb(); },
};
const release = () => { hold = false; held.splice(0).forEach((fn) => setTimeout(fn, 0)); };

function zen() {
  return new ZEN({
    peers: [], multicast: false, axe: false, WebSocket: false, stats: false,
    localStorage: false, chunk: 1000,
    file: "tmp/flushrepro-" + String(Math.random()).slice(2), store,
  });
}

describe("RAD flush window", function () {
  this.timeout(120 * 1000);
  it("serves a reader that opened while the batch was still being flushed", async function () {
const A = zen();
await new Promise((res) => {
  let left = CONTACTS;
  for (let i = 0; i < CONTACTS; i++) {
    A.get("names").get("n" + i).put({ name: "N" + i, age: i + 1 }, () => { if (--left === 0) res(); });
  }
});

// Hold the disk right after the acks, the way a starved core does: the flush
// is scheduled and cannot complete.
hold = true;
await new Promise((r) => setTimeout(r, 400));

const B = zen();
const seen = [];
const got = await new Promise((res) => {
  let done = false;
  B.get("names").get("n7").on((d) => {
    seen.push(d && d.name ? "NODE" : JSON.stringify(d));
    if (!d || !d.name || !d.age || done) return;
    done = true; res(d);
  });
  setTimeout(release, 900);                      // let the flush land
  setTimeout(() => { if (!done) { done = true; res(null) } }, 20000);
});
    assert.ok(
      got && "N7" === got.name && 8 === got.age,
      "the reader never got the record; it saw " + JSON.stringify(seen),
    );
  });
});

// Second failure mode, still open as of 2026-08-13 -- this test does not cover
// it, and it is what keeps test/rad/flush-read.js red under a pinned core.
//
// Caught in a loaded pinned run: radisk answers a single-field read for
// `names<ESC>n7` with a BRANCH -- the children n70..n79 -- and lib/store.js
// discards a branch when one field was asked for, so the caller is told
// nothing and never asks again. At that moment the value is in no radix in
// memory at all: not r.disk, not r.writing[*].rad, not .pend. The branches came
// from the root file "!" and from "names<ESC>n79" -- note that n7 sorts BEFORE
// n79, so routing handed the read a file it should never have been given.
//
// Tried and did not help (1/4 pinned): on a branch, walk back through earlier
// files looking for the entry. So the entry is probably not in an earlier file
// either -- chase where routing got n79 from first.
