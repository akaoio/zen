import ZEN from "../../zen.js";
import "../../lib/store.js";
import assert from "assert";

// A read through a link delivers twice: the pointer first, then the node it
// points at once that has loaded. `root.pass` is a recursion guard -- "this
// listener already called back in this pass" -- and the pointer used to spend
// that slot, so the delivery that actually carried the node looked like a
// repeat and was dropped. The listener was left holding `{"#": soul}` and the
// node never arrived, however long it waited.
//
// The window is ordinary: it opens whenever the parent node is already in
// memory while the node it links to still has to be loaded. Here the store is
// under the test's control, so that is a switch rather than a race -- no
// pinning, no timing, no flake.
describe("read through a link", function () {
  this.timeout(30 * 1000);

  function harness() {
    var files = {};
    var held = [];
    var store = {
      hold: false,
      put: function (file, data, cb) {
        files[file] = data;
        setTimeout(function () {
          cb(null, 1);
        }, 0);
      },
      get: function (file, cb) {
        var serve = function () {
          cb(null, files[file]);
        };
        store.hold ? held.push(serve) : setTimeout(serve, 0);
      },
      list: function (cb) {
        Object.keys(files).forEach(function (f) {
          cb(f);
        });
        cb();
      },
      release: function () {
        store.hold = false;
        held.splice(0).forEach(function (fn) {
          setTimeout(fn, 0);
        });
      },
    };
    return store;
  }

  function instance(store) {
    return new ZEN({
      peers: [],
      multicast: false,
      axe: false,
      WebSocket: false,
      stats: false,
      localStorage: false,
      file: "tmp/linkread-" + String(Math.random()).slice(2),
      store: store,
    });
  }

  it("delivers the node, not the pointer, when the link resolves first", async function () {
    var store = harness();
    var writer = instance(store);

    await new Promise(function (res) {
      writer.get("names").get("n7").put({ name: "N7", age: 8 }, res);
    });
    await new Promise(function (r) {
      setTimeout(r, 800);
    });

    // A reader with nothing of this in memory.
    var reader = instance(store);

    // Warm only the parent, so the link is in memory when the listener lands
    // but the node it points at is not.
    await new Promise(function (res) {
      var done = false;
      reader.get("names").on(function (d) {
        if (!done && d) {
          done = true;
          res(d);
        }
      });
      setTimeout(function () {
        if (!done) {
          done = true;
          res(null);
        }
      }, 5000);
    });

    store.hold = true; // the store answers nothing until released
    var seen = [];
    var got = await new Promise(function (res) {
      var done = false;
      reader
        .get("names")
        .get("n7")
        .on(function (d) {
          seen.push(d);
          if (!d || !d.name || !d.age || done) {
            return;
          }
          done = true;
          res(d);
        });
      setTimeout(function () {
        store.release();
      }, 300);
      setTimeout(function () {
        if (!done) {
          done = true;
          res(null);
        }
      }, 8000);
    });

    assert.ok(
      got && "N7" === got.name && 8 === got.age,
      "the listener never got the node it asked for, only " + JSON.stringify(seen),
    );
  });
});
