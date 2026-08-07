import assert from "assert";
import ZEN from "../../zen.js";

// A ZEN with no storage adapter and no peers: nothing in the process can
// answer a GET. Reads and writes must still resolve instead of hanging.
function makeZEN() {
  return new ZEN({
    peers: [],
    WebSocket: false,
    localStorage: false,
    radisk: false,
    rfs: false,
    opfs: false,
    axe: false,
    multicast: false,
    stats: false,
  });
}

function settle(fn, ms) {
  return new Promise(function (resolve) {
    var done = false;
    var fin = function (v) {
      if (!done) {
        done = true;
        resolve(v);
      }
    };
    fn(fin);
    setTimeout(function () {
      fin("<never called back>");
    }, ms || 4000);
  });
}

describe("test:zen no acknowledger", function () {
  this.timeout(20 * 1000);

  it("calls back once() on an unknown soul instead of hanging", async function () {
    var zen = makeZEN();
    var got = await settle(function (fin) {
      zen.get("nostore-unknown-" + Math.random()).once(function (data) {
        fin({ data: data });
      });
    });
    assert.notStrictEqual(got, "<never called back>", "once() never called back");
    assert.strictEqual(got.data, undefined, "unknown soul should read as undefined");
  });

  it("acks a put that has to create a child node instead of going silent", async function () {
    var zen = makeZEN();
    var ack = await settle(function (fin) {
      zen
        .get("nostore-obj-" + Math.random())
        .get("x")
        .put({ a: 1 }, fin);
    });
    assert.notStrictEqual(ack, "<never called back>", "put() never acked");
  });

  it("acks a nested put instead of going silent", async function () {
    var zen = makeZEN();
    var ack = await settle(function (fin) {
      zen
        .get("nostore-deep-" + Math.random())
        .get("a")
        .get("b")
        .put({ a: 1 }, fin);
    });
    assert.notStrictEqual(ack, "<never called back>", "nested put() never acked");
  });
});
