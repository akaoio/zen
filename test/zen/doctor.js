import assert from "assert";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

// `zen doctor` exists because nothing on a host errors when the installation
// drifts away from the checkout. The auto-update timer here sat enabled,
// active and dead for eight days without a log line, and every deploy in that
// window silently needed a human. So what matters about this command is not
// that it prints something when all is well -- it is that it says so, loudly
// and with a non-zero exit, when it is not.
const here = path.dirname(fileURLToPath(import.meta.url));
const cli = path.join(here, "..", "..", "script", "zen.sh");

function doctor(env) {
  try {
    return {
      code: 0,
      out: execFileSync("sh", [cli, "doctor"], {
        env: { ...process.env, ...env },
        encoding: "utf8",
        stdio: ["ignore", "pipe", "pipe"],
      }),
    };
  } catch (e) {
    return { code: e.status, out: (e.stdout || "") + (e.stderr || "") };
  }
}

function sandbox(name) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "zen-doctor-" + name + "-"));
  fs.mkdirSync(path.join(root, "cfg", "zen"), { recursive: true });
  fs.mkdirSync(path.join(root, "install", "script"), { recursive: true });
  fs.writeFileSync(
    path.join(root, "install", "package.json"),
    JSON.stringify({ name: "@akaoio/zen", version: "0.0.0-test" }),
  );
  fs.writeFileSync(path.join(root, "cfg", "zen", "install_dir"), path.join(root, "install"));
  fs.writeFileSync(path.join(root, "cfg", "zen", "service_name"), "zen-doctor-test");
  return root;
}

describe("zen doctor", function () {
  this.timeout(60 * 1000);

  it("reports the installation it found", function () {
    const root = sandbox("found");
    const r = doctor({ XDG_CONFIG_HOME: path.join(root, "cfg") });
    assert.ok(/ZEN doctor/.test(r.out), "no report at all:\n" + r.out);
    assert.ok(
      r.out.indexOf(path.join(root, "install")) >= 0,
      "did not name the install it was pointed at:\n" + r.out,
    );
  });

  it("says so, and fails, when the install is not a checkout", function () {
    const root = sandbox("nogit");
    const r = doctor({ XDG_CONFIG_HOME: path.join(root, "cfg") });
    assert.ok(
      /not a git checkout|no record of what was deployed/.test(r.out),
      "a directory with no git and no deploy record passed unremarked:\n" + r.out,
    );
    assert.notStrictEqual(r.code, 0, "it found something wrong and still exited 0:\n" + r.out);
  });

  it("fails when it cannot find an installation at all", function () {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), "zen-doctor-none-"));
    fs.mkdirSync(path.join(root, "cfg", "zen"), { recursive: true });
    // HOME too, or it finds the real one through the fallback path
    const r = doctor({ XDG_CONFIG_HOME: path.join(root, "cfg"), HOME: root });
    assert.notStrictEqual(r.code, 0, "no installation anywhere, and it exited 0:\n" + r.out);
    assert.ok(/not found/.test(r.out), "did not say what was missing:\n" + r.out);
  });

  it("rejects an option it does not know instead of ignoring it", function () {
    const root = sandbox("badopt");
    let r;
    try {
      r = {
        code: 0,
        out: execFileSync("sh", [cli, "doctor", "--wat"], {
          env: { ...process.env, XDG_CONFIG_HOME: path.join(root, "cfg") },
          encoding: "utf8",
          stdio: ["ignore", "pipe", "pipe"],
        }),
      };
    } catch (e) {
      r = { code: e.status, out: (e.stdout || "") + (e.stderr || "") };
    }
    assert.notStrictEqual(r.code, 0, "unknown option accepted silently");
    assert.ok(/Unknown option/.test(r.out), "no word about the option:\n" + r.out);
  });
});
