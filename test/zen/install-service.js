import assert from "assert";
import { execFileSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

// install.sh used to carry its own copy of the systemd unit in a heredoc, while
// script/zen.service sat in the repo looking like the source of that unit and
// being read by nothing. It drifted, of course, and reading it sent people
// looking for problems that were not there. Now there is one template and
// install.sh renders it -- which is only an improvement while the rendering
// actually puts everything in.
//
// So this runs create_service the way install.sh runs it, with the options an
// install can be given, and reads the unit that comes out.
const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const installer = path.join(root, "script", "install.sh");

function render(opts) {
  const set = Object.entries({
    DRY_RUN: "true",
    SKIP_SERVICE: "false",
    SUDO: "",
    SERVICE_NAME: "zen",
    INSTALL_DIR: root,
    REAL_USER: "someone",
    PORT: "",
    DOMAIN: "",
    PEERS: "",
    HTTPS_KEY: "",
    HTTPS_CERT: "",
    FMB: "",
    FRAT: "",
    EVICT: "",
    GC_MB: "",
    GC_SEC: "",
    GC_KEEP: "",
    UDP_PORT: "",
    ...opts,
  })
    .map(([k, v]) => `${k}='${v}'`)
    .join("\n");

  // strip the call to main, source the rest, then call the one function
  const script = `
    set -eu
    TMP=$(mktemp)
    sed '/^main "$@"$/d' '${installer}' > "$TMP"
    set --
    . "$TMP"
    rm -f "$TMP"
    ${set}
    create_service 2>/dev/null | sed -n '/^\\[Unit\\]/,/^WantedBy=/p'
  `;
  return execFileSync("sh", ["-c", script], { encoding: "utf8" });
}

// Runs one function out of install.sh with no controlling terminal -- which is
// how cron, systemd, and a good many `curl | bash` invocations reach it. CI and
// the harness are already in that state, so there is nothing to simulate.
function callWithNoTty(snippet) {
  const script = `
    exec 2>&1
    set -eu
    TMP=$(mktemp)
    sed '/^main "$@"$/d' '${installer}' > "$TMP"
    set --
    . "$TMP"
    rm -f "$TMP"
    ${snippet}
  `;
  try {
    return execFileSync("sh", ["-c", script], { encoding: "utf8", stdio: ["pipe", "pipe", "pipe"] });
  } catch (err) {
    return (err.stdout || "") + (err.stderr || "");
  }
}

describe("install.sh with nobody to ask", function () {
  if ("linux" !== process.platform) {
    it.skip("only meaningful on linux", function () {});
    return;
  }
  this.timeout(30 * 1000);

  it("gets through the questions instead of dying on them", function () {
    // /dev/tty is a device node that exists even where opening it cannot
    // succeed. install.sh tested for the node and then wrote to it, which
    // under `set -e` ends the install part-way through with a message about a
    // device rather than anything an operator can act on.
    const out = callWithNoTty('SKIP_SERVICE=false\nYES=false\nDOMAIN=""\nPORT=8420\nprompt_missing_options\necho "STILL-ALIVE domain=[$DOMAIN] port=[$PORT]"');
    assert.ok(/STILL-ALIVE/.test(out), "install.sh died on a question it could not ask:\n" + out);
    assert.ok(/domain=\[\] port=\[8420\]/.test(out), "unanswered questions did not keep their defaults:\n" + out);
  });

  it("leaves an answer it was given alone", function () {
    const out = callWithNoTty('SKIP_SERVICE=false\nYES=false\nDOMAIN="given.test"\nPORT=9999\nprompt_missing_options\necho "domain=[$DOMAIN] port=[$PORT]"');
    assert.ok(/domain=\[given\.test\] port=\[9999\]/.test(out), "overwrote what it was told:\n" + out);
  });

  it("declines a confirmation it has no way to ask", function () {
    const out = callWithNoTty('YES=false\nif confirm "proceed?"; then echo ANSWERED-yes; else echo ANSWERED-no; fi');
    assert.ok(/ANSWERED-no/.test(out), "an unanswerable question was not declined:\n" + out);
    assert.ok(!/dev\/tty/.test(out), "leaked a device error at the operator:\n" + out);
  });

  it("still takes yes for an answer without asking anything", function () {
    const out = callWithNoTty('YES=true\nif confirm "proceed?"; then echo ANSWERED-yes; else echo ANSWERED-no; fi');
    assert.ok(/ANSWERED-yes/.test(out), "--yes stopped working:\n" + out);
  });
});

describe("the systemd unit install.sh writes", function () {
  // install.sh builds a systemd unit with a POSIX shell -- neither exists on macOS or Windows, where these assert
  // things about a deployment that cannot happen. Linux is where they mean
  // something; elsewhere they only made CI red.
  if ("linux" !== process.platform) {
    it.skip("only meaningful on linux", function () {});
    return;
  }
  this.timeout(30 * 1000);

  it("leaves no placeholder behind", function () {
    const unit = render({ PORT: "8420" });
    assert.ok(!/__ZEN_[A-Z]+__/.test(unit), "a placeholder survived rendering:\n" + unit);
  });

  it("puts the install it was given into the unit", function () {
    const unit = render({ PORT: "8420", REAL_USER: "someone" });
    assert.ok(unit.indexOf("User=someone") >= 0, "wrong user:\n" + unit);
    assert.ok(unit.indexOf("WorkingDirectory=" + root) >= 0, "wrong directory:\n" + unit);
    assert.ok(
      new RegExp("ExecStart=\\S+ " + root.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") + "/script/server.js").test(unit),
      "wrong ExecStart:\n" + unit,
    );
  });

  it("writes every option it was given, and only those", function () {
    const unit = render({
      PORT: "8420",
      DOMAIN: "example.test",
      HTTPS_KEY: "/k.pem",
      HTTPS_CERT: "/c.pem",
      FMB: "200",
      FRAT: "0.10",
      EVICT: "1",
      GC_MB: "400",
      GC_SEC: "60",
      GC_KEEP: "120",
      UDP_PORT: "8421",
    });
    for (const line of [
      "Environment=PORT=8420",
      "Environment=DOMAIN=example.test",
      "Environment=HTTPS_KEY=/k.pem",
      "Environment=HTTPS_CERT=/c.pem",
      "Environment=FMB=200",
      "Environment=FRAT=0.10",
      "Environment=EVICT=1",
      "Environment=GRAPH_GC_MB=400",
      "Environment=GRAPH_GC_SEC=60",
      "Environment=GRAPH_GC_KEEP=120",
      "Environment=UDP_PORT=8421",
    ]) {
      assert.ok(unit.indexOf(line) >= 0, "missing " + line + " from:\n" + unit);
    }
    // PEERS was not given, so nothing about it should appear
    assert.ok(!/Environment=PEERS/.test(unit), "wrote an option it was never given:\n" + unit);
  });

  it("writes no Environment lines at all when given no options", function () {
    const unit = render({});
    const live = unit.split("\n").filter((l) => /^Environment=/.test(l));
    assert.deepStrictEqual(live, [], "expected a bare unit, got:\n" + live.join("\n"));
  });

  it("is a unit systemd would accept: one section each, in order", function () {
    const unit = render({ PORT: "8420" });
    assert.ok(/\[Unit\][\s\S]*\[Service\][\s\S]*\[Install\]/.test(unit), "sections out of order:\n" + unit);
    assert.strictEqual((unit.match(/^\[Service\]$/gm) || []).length, 1, "duplicate [Service]:\n" + unit);
    assert.ok(/^WantedBy=multi-user\.target$/m.test(unit), "nothing would start it at boot:\n" + unit);
  });
});
