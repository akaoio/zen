// Once a write acknowledges, the key must still be readable from what is on
// disk -- by a process that has never seen it before, with nothing in memory
// to help. Radisk names a file by the first key it holds and routes a lookup
// by those names, so a key written into the wrong file is not stale or late,
// it is gone: no read will ever look where it sits.
//
// Ask by reading, never by comparing key against file name. The routing rule
// is not "the key must be below the next file name" -- a read walks on into
// later files when the range continues there -- so inferring loss from names
// alone reports losses that are not real. (It did: an earlier version of this
// test called 11% of a production store lost, and every sample it named read
// back fine.) The check here is the one that matters: enumerate what is on
// disk, open a cold store over a copy of it, and read every key.
import zenbase from "../../zen.js";
import "../../lib/store.js";
import "../../lib/rfs.js";
import Radisk from "../../lib/radisk.js";
import rfs from "../../lib/rfs.js";
import assert from "assert";
import fs from "fs";
import path from "path";

var CONTACTS = 219;

function makeZEN(dir) {
  return new zenbase({
    peers: [],
    multicast: false,
    axe: false,
    WebSocket: false,
    stats: false,
    localStorage: false,
    radisk: true,
    chunk: 1000,
    file: dir,
  });
}

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

// A radisk file is a radix tree as JSON: concatenating the path down to a ""
// node gives a key the file holds.
function keysOf(file) {
  var tree;
  try {
    tree = JSON.parse(fs.readFileSync(file, "utf8"));
  } catch (e) {
    return [];
  }
  var keys = [];
  (function walk(node, prefix) {
    if (!node || typeof node !== "object") {
      return;
    }
    for (var k in node) {
      if (!node.hasOwnProperty(k)) {
        continue;
      }
      if (k === "") {
        if (node[k] && typeof node[k] === "object" && ":" in node[k]) {
          keys.push(prefix);
        }
        continue;
      }
      walk(node[k], prefix + k);
    }
  })(tree, "");
  return keys;
}

function keysOnDisk(dir) {
  var DIR = String.fromCharCode(28);
  var out = [];
  fs.readdirSync(dir)
    .filter(function (f) {
      return !/\.tmp$/.test(f);
    })
    .forEach(function (f) {
      if (decodeURIComponent(f) === DIR) {
        return;
      }
      keysOf(path.join(dir, f)).forEach(function (k) {
        out.push(k);
      });
    });
  return out;
}

// Radisk memoises one instance per directory, so reading through the same path
// would answer from the writer's memory. Copy the store and open that instead:
// cold caches, disk only, exactly what a restarted relay would see.
function coldStore(dir) {
  var cold = dir + "-cold";
  fs.mkdirSync(cold, { recursive: true });
  fs.readdirSync(dir).forEach(function (f) {
    fs.copyFileSync(path.join(dir, f), path.join(cold, f));
  });
  var store = rfs({ file: cold, log: function () {} });
  return Radisk({ file: cold, store: store });
}

function read(rad, key) {
  return new Promise(function (res) {
    var done = false;
    rad(key, function (err, data) {
      if (!done && undefined !== data) {
        done = true;
        res(true);
      }
    });
    setTimeout(function () {
      if (!done) {
        done = true;
        res(false);
      }
    }, 5000);
  });
}

describe("RAD reachability", function () {
  this.timeout(300 * 1000);

  it("keeps every key on disk readable from a cold store", async function () {
    var dir = "tmp/misfile-" + String(Math.random()).slice(2);
    var zen = makeZEN(dir);
    await writeContacts(zen);

    // Wait for the store to go quiet rather than for a fixed delay: on a busy
    // or starved machine the batches, splits and directory updates take as
    // long as they take, and a fixed sleep just makes this test flaky itself.
    var keys = [],
      steady = 0,
      waited = 0;
    while (waited < 90000 && steady < 4) {
      await new Promise(function (r) {
        setTimeout(r, 500);
      });
      waited += 500;
      var now = keysOnDisk(dir);
      steady = now.length === keys.length && now.length ? steady + 1 : 0;
      keys = now;
    }
    assert.ok(
      keys.length > CONTACTS,
      "the store never settled: " + keys.length + " keys on disk after " + waited + "ms",
    );

    var rad = coldStore(dir);
    var lost = [];
    for (var i = 0; i < keys.length; i++) {
      if (!(await read(rad, keys[i]))) {
        lost.push(keys[i]);
      }
    }

    if (lost.length) {
      console.log("\n===== UNREADABLE in " + dir + "-cold =====");
      lost.slice(0, 8).forEach(function (k) {
        console.log("   " + JSON.stringify(k));
      });
    }
    assert.strictEqual(
      lost.length,
      0,
      lost.length + "/" + keys.length + " keys are on disk but unreadable, e.g. " +
        JSON.stringify(lost.slice(0, 4)),
    );
  });
});
