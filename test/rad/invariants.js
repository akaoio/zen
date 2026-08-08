import { withRadisk, readNow } from "./harness.js";
import assert from "assert";

var esc = String.fromCharCode(27);

// The one thing storage must promise: once a write has been acknowledged, a
// read finds it. Not "usually", not "unless a flush is in progress" -- there is
// no moment a caller can observe where an acknowledged key is absent.
//
// Radisk breaks that promise during a flush, because a flush is not one event.
// It serialises a snapshot, splits it across new files, rewrites the original,
// and edits the directory that every read routes by -- and a read can land
// between any two of those. On a normal machine the gap is microseconds, which
// is why this only ever showed up as a "Windows-only" flaky test. Here the
// scenario is replayed once per step, so every one of those moments is checked.
describe("RAD invariants", function () {
  this.timeout(60 * 1000);

  // Seeded keys, settled and acknowledged, then a second batch whose flush
  // re-partitions the files the first batch lives in.
  function scenario(readAfterSteps, probe) {
    return withRadisk({ chunk: 300, until: 100 }, function (r, ctx) {
      var acked = 0;
      for (var i = 0; i < 40; i++) {
        r("names" + esc + "a" + (100 + i), { name: "A" + i }, function () {
          acked++;
        });
      }
      ctx.settle();
      var seeded = acked;

      // Second batch: interleaved with the first, so its flush has to split
      // the very files the first batch lives in and rewrite the directory
      // across that range.
      for (var j = 0; j < 40; j++) {
        r("names" + esc + "a" + (100 + j) + "x", { name: "B" + j });
      }

      // Stop partway into that flush and read a key from the settled batch.
      var stepped = 0;
      while (stepped < readAfterSteps && ctx.step()) {
        stepped++;
      }
      var got = readNow(r, probe);
      ctx.settle();
      return {
        got: got,
        seeded: seeded,
        stepped: stepped,
        puts: ctx.store.puts,
      };
    });
  }

  it("hands back a key that was acknowledged before the flush started", function () {
    var probe = "names" + esc + "a120";
    var first = scenario(0, probe);
    assert.equal(first.seeded, 40, "the seed batch did not all acknowledge");
    assert.ok(first.puts > 2, "no split happened: " + first.puts + " writes");

    var misses = [];
    for (var k = 0; k <= 80; k++) {
      var res = scenario(k, probe);
      if (res.stepped < k) {
        break; // ran out of flush to step through
      }
      var v = res.got.data;
      if (!v || "A20" !== v.name) {
        misses.push(k + ":" + JSON.stringify(v));
      }
    }
    assert.ok(
      !misses.length,
      misses.length +
        " of the flush's steps hide an acknowledged key -- read at step " +
        JSON.stringify(misses.slice(0, 10)),
    );
  });

  // The other half of the promise: a write that lands *during* a flush must
  // still be findable once everything settles. It cannot merge into the file
  // being flushed, so it waits -- and the flush it waits for is free to split
  // that file and move its key's range elsewhere. Whatever the write does when
  // it wakes up, the key has to end up where reads will look for it.
  it("keeps a write that landed in the middle of a flush", function () {
    var misses = [];
    var reachedFlush = 0;
    var parkedAt = 0;
    for (var k = 1; k <= 60; k++) {
      var res = withRadisk({ chunk: 300, until: 100 }, function (r, ctx) {
        for (var i = 0; i < 40; i++) {
          r("names" + esc + "a" + (100 + i), { name: "A" + i });
        }
        ctx.settle();

        // Overflow one already-named file, so the flush that follows splits
        // *inside* a range the directory already has entries for. Parking on
        // the "!" file proves nothing: the directory keeps routing there
        // afterwards, so a replay against the stale name still lands right.
        var pad = new Array(80).join("p");
        for (var j = 0; j < 30; j++) {
          r("names" + esc + "a120" + String.fromCharCode(97 + (j % 26)) + j, {
            name: "B" + j,
            pad: pad,
          });
        }
        var stepped = 0;
        while (stepped < k && ctx.step()) {
          stepped++;
        }
        if (stepped < k) {
          return null; // ran out of flush
        }

        // Did it actually land mid-flush? Without this the scenario can pass
        // by never exercising the case at all.
        var route = null;
        r.find("names" + esc + "a120y", function (f) {
          route = f;
        });
        var parked = !!r.writing[route];

        // A spread of writes arriving mid-flush, so whichever part of the
        // range this flush's split carries off, some of them are in it.
        var ok = 0,
          late = [];
        for (var m = 0; m < 30; m++) {
          late.push(
            "names" +
              esc +
              "a120" +
              String.fromCharCode(97 + (m % 26)) +
              m +
              "y",
          );
          r(late[m], { name: "LATE" + m }, function () {
            ok++;
          });
        }
        ctx.settle();
        var lost = [];
        for (var n2 = 0; n2 < late.length; n2++) {
          var g2 = readNow(r, late[n2]);
          ctx.settle();
          if (!g2.data || "LATE" + n2 !== g2.data.name) {
            lost.push(late[n2].slice(-6));
          }
        }
        return {
          ok: ok,
          lost: lost,
          count: late.length,
          stepped: stepped,
          parked: parked,
          route: route,
        };
      });
      if (!res) {
        break;
      }
      reachedFlush++;
      if (res.parked) {
        parkedAt++;
      }
      // settle() runs it to quiescence, so an unacknowledged write is itself a
      // failure -- nothing is left that could still deliver it.
      if (res.ok < res.count) {
        misses.push(k + ":ack " + res.ok + "/" + res.count);
      } else if (res.lost.length) {
        misses.push(
          k +
            ":lost " +
            res.lost.length +
            " " +
            JSON.stringify(res.lost.slice(0, 3)),
        );
      }
    }
    assert.ok(
      reachedFlush > 5,
      "the flush was too short to test: " + reachedFlush,
    );
    assert.ok(
      parkedAt > 0,
      "no step in this flush parks the write, so the case is never exercised",
    );
    assert.ok(
      !misses.length,
      misses.length +
        "/" +
        reachedFlush +
        " points in the flush lose a write that landed there -- " +
        JSON.stringify(misses.slice(0, 10)),
    );
  });

  // A file the directory never hears about.
  //
  // Splitting names the new file after the first key moving into it, and that
  // name can be a strict prefix of names the directory already holds --
  // `names<esc>a120` alongside `names<esc>a120a0`. `r.find.add` asked the
  // directory `dir(file)` and treated any truthy answer as "already listed",
  // but a radix answers a prefix lookup with the *subtree* under it, which is
  // truthy whether or not that exact name has an entry. So the entry was never
  // written: the file existed, held the only copy of its keys, and every read
  // routed straight past it.
  //
  // This is the shape behind years of "Windows-only" flakiness -- file names
  // like `names<esc>n7` and `names<esc>n73` are one write apart from it.
  it("lists a file whose name is a prefix of names already listed", function () {
    var res = withRadisk({ chunk: 300, until: 100 }, function (r, ctx) {
      var pad = new Array(120).join("p");
      // Long names under a common prefix, each big enough to split between, so
      // the directory ends up holding names<esc>a120a0, names<esc>a120b1, ...
      for (var j = 0; j < 16; j++) {
        r("names" + esc + "a120" + String.fromCharCode(97 + j) + j, {
          ":": pad,
          ">": 1,
        });
      }
      ctx.settle();

      // Then enough keys sorting *before* that prefix, plus the short name
      // itself, so the next split puts its boundary exactly on
      // `names<esc>a120`.
      for (var m = 0; m < 16; m++) {
        r("names" + esc + "a11" + m, { ":": pad, ">": 2 });
      }
      r("names" + esc + "a120", { ":": "SHORT", ">": 2 });
      ctx.settle();

      var got = readNow(r, "names" + esc + "a120");
      ctx.settle();
      return {
        found: !!(got.data && "SHORT" === got.data[":"]),
        files: Object.keys(ctx.store.files).length,
      };
    });
    assert.ok(
      res.files > 4,
      "no splitting happened, so nothing was tested: " + res.files + " files",
    );
    assert.ok(
      res.found,
      "a file whose name is a prefix of listed names never reached the " +
        "directory, so its keys are unreachable",
    );
  });

  // A write that has to wait must still be readable.
  //
  // When a write lands during a flush it is parked on that flush and replayed
  // when it lands. Everything about it -- key and value -- used to live only in
  // the closure that parked it: not in `r.disk`, not in the snapshot being
  // written, not on disk. So for that whole stretch a read for the key found
  // nothing anywhere and said so, and nothing in the stack asks twice.
  //
  // That is the state the long-running Windows flake kept landing in:
  // acknowledged, and simultaneously nowhere.
  it("answers for a write that is parked on a flush", function () {
    var res = withRadisk({ chunk: 300, until: 100 }, function (r, ctx) {
      var pad = new Array(120).join("p");
      for (var i = 0; i < 30; i++) {
        r("names" + esc + "a" + (100 + i), { ":": pad, ">": 1 });
      }
      ctx.settle();

      // Start a flush and stop while it is still in the store's hands.
      for (var j = 0; j < 30; j++) {
        r("names" + esc + "b" + (100 + j), { ":": pad, ">": 2 });
      }
      var guard = 0;
      while (guard++ < 100000) {
        var busy = false;
        for (var w in r.writing) {
          if (Object.prototype.hasOwnProperty.call(r.writing, w)) {
            busy = true;
            break;
          }
        }
        if (busy) {
          break;
        }
        if (!ctx.step()) {
          break;
        }
      }

      // This write has to wait for that flush. Parking is detected by the
      // queue growing, not by anything the fix introduces -- otherwise the
      // guard would only ever pass on fixed code and this test could never go
      // red for the right reason.
      var late = "names" + esc + "b120";
      var route = null;
      r.find(late, function (f) {
        route = f;
      });
      var before = ((r.writing || {})[route] || []).length;
      r(late, { ":": "PARKED", ">": 3 });
      var parked = ((r.writing || {})[route] || []).length > before;

      // Read it while it is still waiting -- served from memory, so it needs no
      // disk work and cannot accidentally let the flush finish first.
      var got = readNow(r, late);
      return { parked: parked, data: got.data, done: got.done };
    });
    assert.ok(
      res.parked,
      "no write ended up parked, so this invariant was never exercised",
    );
    assert.ok(
      res.data && "PARKED" === res.data[":"],
      "a write parked on a flush is unreadable while it waits: " +
        JSON.stringify(res.data),
    );
  });
});
