import Radix from "../../lib/radix.js";
import assert from "assert";

var esc = String.fromCharCode(27);

// A radisk file found in production held a node with two children where one
// was a prefix of the other:
//
//   "<soul>"      -> { "": <the soul's own value> }
//   "<soul>\x1b"  -> { "data": ..., "updatedAt": ... }
//
// The lookup grows its candidate one character at a time and takes the first
// child it matches, which is always the shorter one -- so everything under the
// longer sibling was invisible. Those keys were on disk, acknowledged, and
// unreadable by any process, forever. Two of them were sitting in the live
// relay's store.
//
// Nothing should build a node in that shape. But one that exists must still
// answer, because the alternative is data that is gone while looking present.
describe("Radix prefix siblings", function () {
  var soul = "kyyvvtgczcw5tt3onkxm";

  function shaped() {
    var rad = Radix();
    var kids = {};
    kids[soul] = { "": { ":": { "#": "chart/" + soul }, ">": 1 } };
    kids[soul + esc] = {
      data: { "": { ":": "payload", ">": 2 } },
      updatedAt: { "": { ":": 1786000000000, ">": 3 } },
    };
    rad.$ = { "chart/": kids };
    return rad;
  }

  it("finds a key that lives under the longer of two prefix siblings", function () {
    var rad = shaped();
    var got = rad("chart/" + soul + esc + "data");
    assert.ok(got, "the field under the longer sibling is unreachable");
    assert.strictEqual(got[":"], "payload");
    var when = rad("chart/" + soul + esc + "updatedAt");
    assert.ok(when && 1786000000000 === when[":"], "the second field is unreachable");
  });

  it("still finds the key on the shorter sibling", function () {
    var rad = shaped();
    var got = rad("chart/" + soul);
    assert.ok(got && got[":"], "the soul's own value went missing");
    assert.strictEqual(got[":"]["#"], "chart/" + soul);
  });

  it("reports a key that genuinely is not there as missing", function () {
    var rad = shaped();
    assert.strictEqual(rad("chart/" + soul + esc + "nothing"), undefined);
    assert.strictEqual(rad("chart/nosuchsoul"), undefined);
  });

  it("leaves a well-formed radix answering exactly as before", function () {
    var rad = Radix();
    rad("a", { ":": 1 });
    rad("ab", { ":": 2 });
    rad("abc", { ":": 3 });
    rad("b", { ":": 4 });
    assert.strictEqual(rad("a")[":"], 1);
    assert.strictEqual(rad("ab")[":"], 2);
    assert.strictEqual(rad("abc")[":"], 3);
    assert.strictEqual(rad("b")[":"], 4);
    assert.strictEqual(rad("abcd"), undefined);
    assert.strictEqual(rad("c"), undefined);
  });
});
