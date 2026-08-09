import zenbase from "../../zen.js";
import "../../lib/store.js";
import assert from "assert";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

// A library must never write into its own package directory. zen's stats
// writer used to resolve its state next to the module under GUN_TEST_TMP,
// which lands inside node_modules for every consumer — and akao vendors zen by
// symlinking src/core/ZEN at the package, so those writes appeared inside its
// SOURCE tree and its dev server rebuilt in a loop over them. Test mode means
// <cwd>/tmp, the same convention lib/tpath.js uses for every other data path.

const PACKAGE_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const TMP_DIR = path.resolve(process.cwd(), process.env.GUN_TMP_DIR || "tmp");

function statsFiles(dir) {
  try {
    return fs
      .readdirSync(dir)
      .filter((name) => name.startsWith("stats."))
      .map((name) => {
        const full = path.join(dir, name);
        return { name, mtime: fs.statSync(full).mtimeMs };
      });
  } catch (e) {
    return [];
  }
}

describe("stats state never lands in the package directory", function () {
  this.timeout(30000);

  it("writes under <cwd>/tmp in test mode, and touches nothing in the package root", async function () {
    assert.strictEqual(process.env.GUN_TEST_TMP, "1", "this test only means anything in test mode");

    const before = statsFiles(PACKAGE_ROOT);
    const zen = new zenbase({
      peers: [],
      multicast: false,
      axe: false,
      WebSocket: false,
      localStorage: false,
      radisk: true,
      file: "stats-path-probe",
    });
    assert.ok(zen);

    // the writer runs on a 5s interval; give it one full turn plus slack
    await new Promise(function (resolve) {
      setTimeout(resolve, 7000);
    });

    const after = statsFiles(PACKAGE_ROOT);
    const changed = after.filter((entry) => {
      const previous = before.find((each) => each.name === entry.name);
      return !previous || previous.mtime !== entry.mtime;
    });
    assert.deepStrictEqual(
      changed.map((entry) => entry.name),
      [],
      "no stats file may be created or rewritten inside the package directory"
    );

    assert.ok(
      statsFiles(TMP_DIR).some((entry) => entry.name.includes("stats-path-probe")),
      `test-mode stats belong in ${TMP_DIR} — found ${JSON.stringify(statsFiles(TMP_DIR).map((e) => e.name))}`
    );
  });
});
