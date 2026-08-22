import Radisk from "../../lib/radisk.js";
import assert from "assert";

// A read routes to the greatest file name <= the key. When the directory names
// no file covering it, that leaves the root file -- which is also where a store
// with nothing listed sends everything. Finding nothing there is not an answer
// while a flush is in flight: the directory is still moving, and the file that
// would cover the key may be about to be published by the very flush in
// progress. Answering "no such key" then is what made a written key read back
// as missing, in a way nothing asked about again (fa8c84e).
//
// The retry for that shape already existed, but was reachable only when the
// directory was *empty* -- so this test keeps it deliberately non-empty, which
// is the case that had no cover at all.
//
// Held store, so "the flush has not landed" is a switch rather than a race.
describe("a read that finds nothing at the root file", function () {
  this.timeout(30 * 1000);

  var esc = String.fromCharCode(27);

  function store() {
    var files = {};
    var queue = [];
    var holding = false;
    return {
      files: files,
      hold: function () {
        holding = true;
      },
      release: function () {
        holding = false;
        queue.splice(0).forEach(function (fn) {
          setTimeout(fn, 0);
        });
      },
      waiting: function () {
        return queue.length;
      },
      put: function (file, data, cb) {
        var land = function () {
          files[file] = data;
          cb(null, 1);
        };
        holding ? queue.push(land) : setTimeout(land, 0);
      },
      get: function (file, cb) {
        setTimeout(function () {
          cb(null, files[file]);
        }, 0);
      },
      list: function (cb) {
        Object.keys(files).forEach(function (f) {
          cb(f);
        });
        cb();
      },
    };
  }

  function noop() {}

  // Waiting on a clock is what makes a test like this fail on a loaded machine
  // for reasons that have nothing to do with what it checks. Wait on the
  // condition instead, with a bound so a wait that will never end still ends.
  function until(check, ms, cb) {
    var waited = 0;
    (function poll() {
      if (check()) {
        cb(null);
        return;
      }
      if (waited >= ms) {
        cb("waited " + ms + "ms and it never happened");
        return;
      }
      waited += 20;
      setTimeout(poll, 20);
    })();
  }

  it("waits for the flush instead of calling the key missing", function (done) {
    var s = store();
    var r = Radisk({
      file: "tmp/rootmiss-" + String(Math.random()).slice(2, 8),
      until: 20,
      chunk: 120,
      store: s,
    });

    // Enough to publish files, so the directory is not empty. Their names all
    // begin "names", which sorts above the probe below.
    for (var i = 0; i < 24; i++) {
      var n = i < 10 ? "0" + i : "" + i;
      r("names/n" + n + esc + "age", { ":": i, ">": 1 }, noop);
      r("names/n" + n + esc + "name", { ":": "N" + i, ">": 1 }, noop);
    }

    until(function () {
      return !r.busy() && Object.keys(s.files).length > 1;
    }, 20 * 1000, function (err) {
      assert.ok(!err, "the first flush never settled: " + err);
      var listed = 0;
      Radisk.Radix.map(r.list, function () {
        listed++;
      });
      assert.ok(listed > 0, "the directory is empty, which is the case that was already covered");

      // Now a flush that stays in the air.
      s.hold();
      r("names/n99" + esc + "age", { ":": 99, ">": 2 }, noop);

      until(function () {
        return s.waiting() > 0 && r.busy();
      }, 20 * 1000, function (err2) {
        assert.ok(!err2, "no write was caught in flight, so this proves nothing: " + err2);

        // "a" sorts below every file the directory holds, so it routes to the
        // root file, which does not have it either.
        var answered = 0;
        r("a" + esc + "field", function () {
          answered++;
        });

        setTimeout(function () {
          assert.strictEqual(
            answered,
            0,
            "answered while a flush was still in flight, so a key the flush is about to publish reads as missing",
          );
          s.release();
          setTimeout(function () {
            assert.ok(answered > 0, "never answered at all once the flush landed");
            done();
          }, 2500);
        }, 250);
      });
    });
  });
});
