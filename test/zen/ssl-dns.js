import assert from "assert";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

// ssl.sh picks a DNS provider and builds the acme.sh command from it. Getting
// that wrong is not loud: a provider whose name is misspelled used to fall
// through to manual DNS, which acme.sh itself says "can not renew
// automatically" -- so the certificate simply stops renewing one day, months
// later, with nothing in any log. A relay served an expired certificate for
// five days that way (#88).
//
// So these run the real parsing and the real command building, in --dry-run,
// and read the acme.sh command that comes out.
const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const ssl = path.join(root, "script", "ssl.sh");

// cleanup_ssl runs on `trap 0` and does `rm -rf "$ACME_DIR/$DOMAIN"`, so a test
// that exits nonzero with the real defaults would delete the live certificate
// out of ~/.acme.sh. Every run is pinned to a throwaway domain and a temp
// ACME_DIR, and the trap is cleared the moment the script is sourced -- no test
// gets to forget.
const SAFE = ["--domain", "example.test", "--email", "nobody@example.test"];

function run(args, opts = {}) {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "zen-ssl-"));
  const quote = (s) => "'" + String(s).replace(/'/g, "'\\''") + "'";
  const argv = [
    ...SAFE,
    "--acme-dir",
    path.join(tmp, "acme"),
    "--key-file",
    path.join(tmp, "key.pem"),
    "--cert-file",
    path.join(tmp, "cert.pem"),
    "--dry-run",
    ...args,
  ]
    .map(quote)
    .join(" ");

  // Strip the call to main so sourcing runs parsing and validation but does not
  // go install acme.sh, then call the one function under test.
  const script = `
    TMP=$(mktemp)
    sed '/^main "$@"$/d' ${quote(ssl)} > "$TMP"
    set -- ${argv}
    . "$TMP"
    rc=$?
    trap - 0
    rm -f "$TMP"
    [ $rc -eq 0 ] || exit $rc
    issue_certificate || exit $?
    echo "---ENV---"
    echo "CF_Token=\${CF_Token:-}"
    echo "CF_Key=\${CF_Key:-}"
    echo "CF_Email=\${CF_Email:-}"
    echo "CF_Zone_ID=\${CF_Zone_ID:-}"
  `;

  try {
    const out = execFileSync("sh", ["-c", script], {
      encoding: "utf8",
      stdio: ["pipe", "pipe", "pipe"],
      ...opts,
    });
    return { status: 0, out };
  } catch (err) {
    return {
      status: err.status === undefined ? 1 : err.status,
      out: (err.stdout || "") + (err.stderr || ""),
    };
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
}

// The one line --dry-run prints, with the acme.sh command in it.
function acmeCommand(res) {
  const line = res.out.split("\n").find((l) => l.indexOf("Would issue certificate with:") >= 0);
  assert.ok(line, "ssl.sh issued no command (exit " + res.status + "):\n" + res.out);
  return line;
}

function env(res, name) {
  const line = res.out.split("\n").find((l) => l.indexOf(name + "=") === 0);
  return line ? line.slice(name.length + 1) : "";
}

// Sources ssl.sh with the given arguments and reports back on the shell state
// it left behind, without issuing anything.
function probe(args, snippet) {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "zen-ip6-"));
  const quote = (s) => "'" + String(s).replace(/'/g, "'\\''") + "'";
  const argv = [...SAFE, "--acme-dir", path.join(tmp, "acme"), "--dry-run", ...args].map(quote).join(" ");
  const script = `
    exec 2>&1
    TMP=$(mktemp)
    sed '/^main "$@"$/d' ${quote(ssl)} > "$TMP"
    set -- ${argv}
    . "$TMP"
    trap - 0
    rm -f "$TMP"
    ACME_DIR=${quote(path.join(tmp, "acme"))}
    ${snippet}
  `;
  try {
    return { tmp, out: execFileSync("sh", ["-c", script], { encoding: "utf8", stdio: ["pipe", "pipe", "pipe"] }) };
  } catch (err) {
    return { tmp, out: (err.stdout || "") + (err.stderr || "") };
  }
}

describe("certificates for a raw IPv6 address", function () {
  if ("linux" !== process.platform) {
    it.skip("only meaningful on linux", function () {});
    return;
  }
  this.timeout(30 * 1000);

  it("does not go asking for one unless asked", function () {
    // Let's Encrypt only issues shortlived certificates for a bare IP, and a
    // residential prefix rotates out from under one. On the box this was found
    // on it had produced nothing but a failed attempt's leftovers.
    assert.ok(/AUTO_IP6=false/.test(probe([], 'echo "AUTO_IP6=$AUTO_IP6"').out));
  });

  it("still does when it is", function () {
    assert.ok(/AUTO_IP6=true/.test(probe(["--auto-ip6"], 'echo "AUTO_IP6=$AUTO_IP6"').out));
  });

  it("keeps taking --no-auto-ip6 from scripts that already pass it", function () {
    assert.ok(/AUTO_IP6=false/.test(probe(["--no-auto-ip6"], 'echo "AUTO_IP6=$AUTO_IP6"').out));
  });

  it("clears away an attempt that produced no certificate", function () {
    // acme.sh leaves a key, a csr and a conf behind when issuance fails. Left
    // there they look like a certificate that expired rather than one that was
    // never obtained -- which is how #88 came to describe a certificate that
    // does not exist.
    const r = probe([], `
      mkdir -p "$ACME_DIR/dead_ecc"
      : > "$ACME_DIR/dead_ecc/dead.key"
      : > "$ACME_DIR/dead_ecc/dead.csr"
      cleanup_ip6_attempt dead
      [ -d "$ACME_DIR/dead_ecc" ] && echo LEFTOVERS-KEPT || echo LEFTOVERS-GONE
    `);
    assert.ok(/LEFTOVERS-GONE/.test(r.out), "left a failed attempt on disk:\n" + r.out);
  });

  it("leaves an attempt that did produce one alone", function () {
    const r = probe([], `
      mkdir -p "$ACME_DIR/live_ecc"
      : > "$ACME_DIR/live_ecc/live.key"
      : > "$ACME_DIR/live_ecc/fullchain.cer"
      cleanup_ip6_attempt live
      [ -d "$ACME_DIR/live_ecc" ] && echo KEPT || echo DELETED
    `);
    assert.ok(/KEPT/.test(r.out), "deleted a certificate it should have kept:\n" + r.out);
  });
});

describe("the DNS provider ssl.sh hands to acme.sh", function () {
  // ssl.sh is a POSIX shell script driving a Linux deployment; on macOS and
  // Windows these would only make CI red.
  if ("linux" !== process.platform) {
    it.skip("only meaningful on linux", function () {});
    return;
  }
  this.timeout(30 * 1000);

  it("uses the Cloudflare hook when asked for Cloudflare", function () {
    const cmd = acmeCommand(run(["--dns", "cloudflare", "--dns-api-token", "tkn"]));
    assert.ok(cmd.indexOf("--dns dns_cf") >= 0, "not the Cloudflare hook:\n" + cmd);
  });

  it("carries an API token as CF_Token, never as CF_Key", function () {
    const res = run(["--dns", "cloudflare", "--dns-api-token", "tkn"]);
    assert.strictEqual(env(res, "CF_Token"), "tkn", "token did not reach CF_Token:\n" + res.out);
    assert.strictEqual(env(res, "CF_Key"), "", "token was sent as a global api key:\n" + res.out);
  });

  it("accepts a Cloudflare token with no account email", function () {
    // The token path never reads CF_Email; demanding --dns-email made the
    // credential this deployment actually has impossible to use.
    const res = run(["--dns", "cloudflare", "--dns-api-token", "tkn"]);
    assert.strictEqual(res.status, 0, "refused a token-only Cloudflare run:\n" + res.out);
  });

  it("passes a zone id through for tokens that cannot list zones", function () {
    const res = run(["--dns", "cloudflare", "--dns-api-token", "tkn", "--dns-zone-id", "zone123"]);
    assert.strictEqual(env(res, "CF_Zone_ID"), "zone123", "zone id did not reach CF_Zone_ID:\n" + res.out);
  });

  it("still accepts a global api key and email", function () {
    const res = run(["--dns", "cloudflare", "--dns-api-key", "k", "--dns-email", "a@b.test"]);
    assert.strictEqual(env(res, "CF_Key"), "k", "key did not reach CF_Key:\n" + res.out);
    assert.strictEqual(env(res, "CF_Email"), "a@b.test", "email did not reach CF_Email:\n" + res.out);
  });

  // These two pass only credentials that ssl.sh already accepts, so the only
  // thing left to fail on is the provider name itself. Reaching for a flag that
  // does not exist yet would make them pass on "Unknown option" instead, which
  // proves nothing about providers.
  it("refuses a provider name it does not know", function () {
    const res = run(["--dns", "cloudlfare", "--dns-api-key", "k", "--dns-email", "a@b.test"]);
    assert.notStrictEqual(res.status, 0, "a misspelled provider was accepted:\n" + res.out);
  });

  it("names the providers it does know when refusing one", function () {
    const res = run(["--dns", "cloudlfare", "--dns-api-key", "k", "--dns-email", "a@b.test"]);
    assert.ok(res.out.indexOf("Unknown DNS provider") >= 0, "no such error was reported:\n" + res.out);
    assert.ok(res.out.indexOf("cloudflare") >= 0, "the error does not say what is valid:\n" + res.out);
  });

  it("refuses Cloudflare with no credential at all", function () {
    const res = run(["--dns", "cloudflare"]);
    assert.notStrictEqual(res.status, 0, "ran Cloudflare with no credential:\n" + res.out);
  });

  it("gives acme.sh a log file so a failed renewal leaves a trail", function () {
    // acme.sh writes no log unless asked, and the cron it installs sends output
    // to /dev/null. Renewal failed daily for 35 days with nowhere to be seen.
    const cmd = acmeCommand(run(["--dns", "cloudflare", "--dns-api-token", "tkn"]));
    assert.ok(/\s--log(\s|$)/.test(cmd), "no --log in the acme command:\n" + cmd);
  });

  it("logs webroot issuance too", function () {
    const cmd = acmeCommand(run(["--webroot", "/tmp"]));
    assert.ok(/\s--log(\s|$)/.test(cmd), "no --log in the acme command:\n" + cmd);
  });

  it("aims a staging run at the staging CA", function () {
    // acme.sh consults --staging only when no server was named (_initAPI:
    // `if [ -z "$ACME_DIRECTORY" ]; then if [ "$STAGE" ]`). Passing --server
    // letsencrypt alongside --staging sent every "safe test" run to production
    // and spent a real certificate.
    const cmd = acmeCommand(run(["--dns", "cloudflare", "--dns-api-token", "tkn", "--staging"]));
    assert.ok(/--server letsencrypt_test(\s|$)/.test(cmd), "a staging run points at production:\n" + cmd);
  });

  it("aims an ordinary run at the production CA", function () {
    const cmd = acmeCommand(run(["--dns", "cloudflare", "--dns-api-token", "tkn"]));
    assert.ok(/--server letsencrypt(\s|$)/.test(cmd), "not the production CA:\n" + cmd);
  });

  it("keeps meaning manual DNS when --dns is bare and nothing can be asked", function () {
    // With no terminal there is nobody to show a menu to, so the old meaning
    // has to survive for cron and curl|bash.
    const cmd = acmeCommand(run(["--dns"]));
    assert.ok(cmd.indexOf("yes-I-know-dns-manual-mode") >= 0, "bare --dns stopped meaning manual:\n" + cmd);
  });
});
