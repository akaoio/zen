import assert from "assert";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { report } from "../../lib/tlsdoctor.js";

// `zen doctor` says nothing about TLS today, which is how a relay served an
// expired certificate for five days while every other check on the box was
// green (#88).
//
// The load-bearing check here is the renewal date: acme.sh records when the
// next renewal is due, and a date in the past means it did not happen -- true
// whatever the provider, whatever the challenge method, whatever went wrong.
// Naming the provider is only ever an explanation offered alongside it.
const DAY = 24 * 60 * 60 * 1000;

let PEM;
let TMP;

const CF = async () => ["aleena.ns.cloudflare.com", "derek.ns.cloudflare.com"];

function acmeStore(conf) {
  const dir = path.join(TMP, "acme-" + Math.abs(hash(conf)), "relay.test_ecc");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, "relay.test.conf"), conf);
  return path.join(dir, "..");
}

function hash(s) {
  let h = 0;
  for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0;
  return h;
}

function find(lines, label) {
  return lines.find((l) => l.label === label);
}

describe("what zen doctor says about TLS", function () {
  this.timeout(30 * 1000);

  before(function () {
    TMP = fs.mkdtempSync(path.join(os.tmpdir(), "zen-td-"));
    try {
      execFileSync(
        "openssl",
        ["req", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:prime256v1",
         "-nodes", "-keyout", path.join(TMP, "key.pem"), "-out", path.join(TMP, "cert.pem"),
         "-days", "60", "-subj", "/CN=relay.test"],
        { stdio: ["ignore", "ignore", "ignore"] },
      );
    } catch {
      this.skip();
      return;
    }
    PEM = path.join(TMP, "cert.pem");
  });

  after(function () {
    if (TMP) fs.rmSync(TMP, { recursive: true, force: true });
  });

  const soon = (now) => new Date(now + 30 * DAY).toISOString();
  const past = (now) => new Date(now - 35 * DAY).toISOString();

  it("calls a renewal that was due last month what it is", async function () {
    const now = Date.now();
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_cf'\nLe_NextRenewTimeStr='${past(now)}'\n`),
      certPath: PEM,
      now,
      resolveNs: CF,
    });
    const r = find(lines, "renewal");
    assert.strictEqual(r.level, "bad", "an overdue renewal was not called bad: " + JSON.stringify(r));
    assert.ok(/35 day/.test(r.detail), "does not say how overdue:\n" + r.detail);
  });

  it("is content when the next renewal is still ahead", async function () {
    const now = Date.now();
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_cf'\nLe_NextRenewTimeStr='${soon(now)}'\n`),
      certPath: PEM,
      now,
      resolveNs: CF,
    });
    assert.strictEqual(find(lines, "renewal").level, "ok");
  });

  it("explains an overdue renewal by naming who really holds the zone", async function () {
    const now = Date.now();
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_gd'\nLe_NextRenewTimeStr='${past(now)}'\n`),
      certPath: PEM,
      now,
      resolveNs: CF,
    });
    const d = find(lines, "dns provider");
    assert.ok(d, "offered no explanation at all:\n" + JSON.stringify(lines, null, 1));
    assert.ok(/GoDaddy/.test(d.detail) && /Cloudflare/.test(d.detail), "does not name both sides:\n" + d.detail);
  });

  it("does not go looking for an explanation when nothing is wrong", async function () {
    // The provider table is allowed to be wrong or incomplete precisely because
    // it is never consulted about a healthy relay.
    const now = Date.now();
    let asked = false;
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_gd'\nLe_NextRenewTimeStr='${soon(now)}'\n`),
      certPath: PEM,
      now,
      resolveNs: async () => {
        asked = true;
        return ["aleena.ns.cloudflare.com"];
      },
    });
    assert.strictEqual(asked, false, "looked up nameservers for a healthy relay");
    assert.strictEqual(find(lines, "dns provider"), undefined, "explained a failure that did not happen");
  });

  it("reports the certificate's own expiry", async function () {
    const now = Date.now();
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_cf'\nLe_NextRenewTimeStr='${soon(now)}'\n`),
      certPath: PEM,
      now,
      resolveNs: CF,
    });
    assert.strictEqual(find(lines, "certificate").level, "ok");
  });

  it("calls an expired certificate bad however healthy the renewal looks", async function () {
    const now = Date.now() + 400 * DAY; // long past the 60-day fixture
    const lines = await report({
      domain: "relay.test",
      acmeDir: acmeStore(`Le_Webroot='dns_cf'\nLe_NextRenewTimeStr='${new Date(now + DAY).toISOString()}'\n`),
      certPath: PEM,
      now,
      resolveNs: CF,
    });
    assert.strictEqual(find(lines, "certificate").level, "bad");
  });

  it("says so when acme.sh has never heard of this domain", async function () {
    const lines = await report({
      domain: "relay.test",
      acmeDir: path.join(TMP, "empty-acme"),
      certPath: PEM,
      now: Date.now(),
      resolveNs: CF,
    });
    const r = find(lines, "renewal");
    assert.strictEqual(r.level, "warn", "silent about a certificate nothing will renew");
  });

  it("says nothing about TLS at all when no certificate is configured", async function () {
    const lines = await report({
      domain: "relay.test",
      acmeDir: path.join(TMP, "empty-acme"),
      certPath: path.join(TMP, "no-such-cert.pem"),
      now: Date.now(),
      resolveNs: CF,
    });
    assert.strictEqual(find(lines, "certificate"), undefined, "complained about a relay serving plain HTTP");
  });
});
