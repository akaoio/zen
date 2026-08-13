#!/usr/bin/env node

// radcheck — is anything in this radisk store unreachable?
//
// Radisk names a file by the first key it holds and routes a lookup to the
// greatest file name <= the key, looking nowhere else. So a key stored in an
// earlier file than the one that now covers it cannot be read back, by this
// process or any other, however long you wait: it acknowledged, it is on disk,
// and it is gone.
//
// This walks a radata directory and reports every key in that state. Read
// only -- it opens files and nothing else. On a live relay it is a snapshot,
// so a handful of keys may be mid-flush; run it twice if a count looks odd.
//
// Usage: node script/radcheck.js [dir] [--all]

import fs from "node:fs";
import path from "node:path";

var DIR = String.fromCharCode(28); // the directory file, not data
var args = process.argv.slice(2);
var all = args.indexOf("--all") >= 0;
var dir = args.filter(function (a) {
  return a.indexOf("--") !== 0;
})[0];

if (!dir) {
  dir = path.join(
    process.env.XDG_DATA_HOME || path.join(process.env.HOME || ".", ".local/share"),
    "zen/radata",
  );
}

if (!fs.existsSync(dir)) {
  console.error("no such store: " + dir);
  process.exit(2);
}

// A radisk file is a radix tree as JSON: concatenating the path down to a ""
// node gives the key it holds.
function keysOf(file) {
  var tree;
  try {
    tree = JSON.parse(fs.readFileSync(file, "utf8"));
  } catch (e) {
    return null; // unreadable or mid-write
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

function show(s) {
  return JSON.stringify(s).replace(/\\u001b/g, "<ESC>");
}

var names = fs
  .readdirSync(dir)
  .filter(function (f) {
    return !/\.tmp$/.test(f);
  })
  .map(decodeURIComponent)
  .filter(function (n) {
    return n !== DIR;
  })
  .sort();

var onDisk = [],
  unreadable = 0;
names.forEach(function (name) {
  var keys = keysOf(path.join(dir, encodeURIComponent(name).replace(/\*/g, "%2A")));
  if (!keys) {
    unreadable++;
    return;
  }
  keys.forEach(function (k) {
    onDisk.push(k);
  });
});

// Ground truth, not inference. Comparing a key against the next file name is
// not the routing rule -- a read walks on into later files when the range it
// wants continues there -- so guessing from names alone reports losses that
// are not real. Ask radisk for every key instead and believe the answer.
var rfs = (await import("../lib/rfs.js")).default;
var Radisk = (await import("../lib/radisk.js")).default;
var store = rfs({ file: dir, log: function () {} });
var rad = Radisk({ file: dir, store: store });

function read(key) {
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

var lost = [];
for (var i = 0; i < onDisk.length; i++) {
  if (!(await read(onDisk[i]))) {
    lost.push(onDisk[i]);
  }
  if (0 === i % 500) {
    process.stderr.write("  checked " + i + "/" + onDisk.length + "\r");
  }
}
process.stderr.write("                              \r");

console.log("store        " + dir);
console.log("files        " + names.length + (unreadable ? " (" + unreadable + " unreadable)" : ""));
console.log("keys         " + onDisk.length);
console.log(
  "unreachable  " +
    lost.length +
    (onDisk.length ? "  (" + ((lost.length / onDisk.length) * 100).toFixed(2) + "%)" : ""),
);
if (lost.length) {
  console.log("");
  (all ? lost : lost.slice(0, 20)).forEach(function (k) {
    console.log("  " + show(k));
  });
  if (!all && lost.length > 20) {
    console.log("  ... " + (lost.length - 20) + " more (--all to list)");
  }
}
process.exit(lost.length ? 1 : 0);
