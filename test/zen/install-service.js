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
