import assert from "assert";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { inspect, describe as describeCert, check } from "../../lib/certwatch.js";

// A relay served an expired certificate for five days and looked healthy the
// whole time: server.js checked that the PEM said "BEGIN CERTIFICATE" and never
// looked at the validity window (#88). From the logs the process was fine; only
// a client could tell you every handshake was being refused.
//
// inspect() takes `now` so the whole calendar is reachable from one fixture --
// no clock games, no certificates minted in the past.
const DAY = 24 * 60 * 60 * 1000;

let PEM;
let NOT_BEFORE;
let NOT_AFTER;

describe("what a certificate's validity window says", function () {
  this.timeout(30 * 1000);

  before(function () {
    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "zen-cw-"));
    const cert = path.join(tmp, "cert.pem");
    try {
      execFileSync(
        "openssl",
        [
          "req", "-x509", "-newkey", "ec",
          "-pkeyopt", "ec_paramgen_curve:prime256v1",
          "-nodes", "-keyout", path.join(tmp, "key.pem"),
          "-out", cert, "-days", "30", "-subj", "/CN=relay.test",
        ],
        { stdio: ["ignore", "ignore", "ignore"] },
      );
    } catch {
      this.skip(); // no openssl here; the suite still means something elsewhere
      return;
    }
    PEM = fs.readFileSync(cert, "utf8");
    const info = inspect(PEM, Date.now());
    NOT_BEFORE = info.validFrom;
    NOT_AFTER = info.validTo;
    fs.rmSync(tmp, { recursive: true, force: true });
  });

  it("reads the window out of the certificate", function () {
    const info = inspect(PEM, NOT_BEFORE + DAY);
    assert.ok(info.validTo > info.validFrom, "window makes no sense: " + JSON.stringify(info));
    assert.strictEqual(info.state, "ok");
  });

  it("counts the days left", function () {
    const info = inspect(PEM, NOT_AFTER - 3 * DAY);
    assert.strictEqual(info.daysLeft, 3);
  });

  it("calls a certificate past its notAfter expired", function () {
    assert.strictEqual(inspect(PEM, NOT_AFTER + DAY).state, "expired");
  });

  it("calls it expired the moment the window closes, not a day later", function () {
    assert.strictEqual(inspect(PEM, NOT_AFTER + 1000).state, "expired");
  });

  it("warns before it expires, not after", function () {
    assert.strictEqual(inspect(PEM, NOT_AFTER - 5 * DAY).state, "expiring");
  });

  it("says nothing while the expiry is still far off", function () {
    assert.strictEqual(inspect(PEM, NOT_AFTER - 29 * DAY).state, "ok");
  });

  it("notices a certificate that is not valid yet", function () {
    assert.strictEqual(inspect(PEM, NOT_BEFORE - DAY).state, "not-yet-valid");
  });

  it("does not throw on something that is not a certificate", function () {
    // A truncated or half-written PEM must not take the relay down on boot.
    const info = inspect("-----BEGIN CERTIFICATE-----\nnonsense\n-----END CERTIFICATE-----\n", Date.now());
    assert.strictEqual(info.state, "unreadable");
  });
});

describe("what the relay says about its certificate", function () {
  it("stays quiet when there is nothing to report", function () {
    assert.strictEqual(describeCert("HTTPS_CERT", { state: "ok", daysLeft: 60 }), null);
  });

  it("names the file it is complaining about", function () {
    const line = describeCert("HTTPS_CERT2", { state: "expired", daysLeft: -5, validTo: Date.now() });
    assert.ok(line.indexOf("HTTPS_CERT2") >= 0, "does not say which certificate:\n" + line);
  });

  it("says how long a certificate has been expired", function () {
    const line = describeCert("HTTPS_CERT", { state: "expired", daysLeft: -5, validTo: Date.now() });
    assert.ok(/5 day/.test(line), "does not say how long:\n" + line);
  });

  it("says how long is left on one that is expiring", function () {
    const line = describeCert("HTTPS_CERT", { state: "expiring", daysLeft: 9, validTo: Date.now() });
    assert.ok(/9 day/.test(line), "does not say how long:\n" + line);
  });

  it("speaks up about a certificate it could not read", function () {
    assert.ok(describeCert("HTTPS_CERT", { state: "unreadable" }), "said nothing about an unreadable cert");
  });
});

describe("checking every certificate at once", function () {
  this.timeout(30 * 1000);

  it("says nothing at all while every certificate is fine", function () {
    assert.deepStrictEqual(check([{ label: "HTTPS_CERT", pem: PEM }], NOT_AFTER - 29 * DAY), []);
  });

  it("reports the expired one and stays quiet about the good one", function () {
    const lines = check(
      [
        { label: "HTTPS_CERT", pem: PEM },
        { label: "HTTPS_CERT2", pem: PEM },
      ],
      NOT_AFTER - 29 * DAY,
    );
    assert.deepStrictEqual(lines, [], "complained about two healthy certificates");

    const bad = check(
      [
        { label: "HTTPS_CERT", pem: PEM },
        { label: "HTTPS_CERT2", pem: "not a certificate" },
      ],
      NOT_AFTER - 29 * DAY,
    );
    assert.strictEqual(bad.length, 1, "expected exactly one complaint:\n" + bad.join("\n"));
    assert.ok(bad[0].indexOf("HTTPS_CERT2") >= 0, "blamed the wrong certificate:\n" + bad[0]);
  });

  it("ignores a certificate that was never configured", function () {
    // The SNI certificate is optional; an absent one is not a problem to report.
    assert.deepStrictEqual(check([{ label: "HTTPS_CERT2", pem: undefined }], Date.now()), []);
  });
});
