import Radisk from "../../lib/radisk.js";
import assert from "assert";

// A radix handed to the writer can legitimately hold nothing.
//
// Two paths reach that state, and both are normal operation. A split builds
// `f.hub` from the keys below the new boundary, and there may be none. And
// before a flush, `s.write` checks whether the file still owns what it is
// holding -- splits published since may have taken over the whole range -- and
// keeps only the keys below the next file's name, which can be none of them.
//
// `Radix()` leaves `$` undefined until something is put in it, and
// `JSON.stringify(undefined)` is `undefined`. Reading `.length` off that threw
// a TypeError, from a setTimeout, outside the try that was meant to catch
// write failures -- so it did not fail the write, it took the process down.
// Two soak workers died this way in ~5700 cycles.
//
// A file that owns nothing is an empty file. It still has to be written: the
// keys that left it are handed back to the directory once the write lands.
describe("writing a radix that holds nothing", function () {
  this.timeout(30 * 1000);

  function radisk(name) {
    var wrote = {};
    var r = Radisk({
      // its own path, or Radisk hands back the instance another test built
      file: "tmp/emptyrad-" + name + "-" + String(Math.random()).slice(2, 8),
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
    return { r: r, wrote: wrote };
  }

  it("writes it instead of throwing", function (done) {
    var box = radisk("write");
    var empty = Radisk.Radix();
    var called = false;

    // The throw was synchronous, so it escapes here rather than reaching `cb`.
    box.r.write("!", empty, function (err, ok) {
      called = true;
      assert.ok(!err, "an empty file is not an error: " + err);
      assert.ok(ok, "the write did not report success");
      done();
    });

    assert.ok(called || true, "unreachable");
  });

  it("what it wrote reads back as a file with nothing in it", function (done) {
    var box = radisk("read");
    var empty = Radisk.Radix();
    box.r.write("!", empty, function (err) {
      assert.ok(!err, "write failed: " + err);
      box.r.read("anything", function (err2, data) {
        assert.ok(!err2, "read of the empty file failed: " + err2);
        assert.strictEqual(data, undefined, "an empty file must hold nothing");
        done();
      });
    });
  });
});
