/**
 * test/zen/status.js — unit tests for lib/status.js
 *
 * Tests the shared status payload builder and signed status round-trip.
 * Pure unit tests — no running server or network required.
 */
import assert from "assert";
import { createRequire } from "node:module";
import ZEN from "../../zen.js";
import { buildStatus, signStatus } from "../../lib/status.js";

const require = createRequire(import.meta.url);
const { version: pkgVersion } = require("../../package.json");

describe("lib/status.js", function () {
  this.timeout(10 * 1000);

  var pair;

  before(async function () {
    pair = await ZEN.pair();
    assert.ok(pair && pair.pub, "test pair must have pub");
  });

  it("buildStatus() uses dynamic package version (not hardcoded)", function () {
    const payload = buildStatus({ pub: pair.pub, port: 8420 });
    assert.strictEqual(payload.version, pkgVersion,
      "version must match package.json, not be hardcoded");
    assert.ok(pkgVersion && pkgVersion.length > 0, "pkgVersion must be non-empty");
  });

  it("buildStatus() includes all canonical fields with correct defaults", function () {
    const payload = buildStatus({ pub: pair.pub, port: 8420, mcp: false });
    assert.strictEqual(payload.name, "zen");
    assert.strictEqual(payload.pub, pair.pub);
    assert.strictEqual(payload.mcp, false);
    assert.deepStrictEqual(payload.peers, []);
    assert.strictEqual(payload.domain, null);
    assert.strictEqual(payload.ip4, null);
    assert.strictEqual(payload.ip6, null);
    assert.strictEqual(payload.port, 8420);
    assert.ok(typeof payload.timestamp === "number" && payload.timestamp > 0,
      "timestamp must be a positive number");
  });

  it("buildStatus() with pub: null does not throw", function () {
    assert.doesNotThrow(function () {
      const payload = buildStatus({ pub: null });
      assert.strictEqual(payload.pub, null);
    });
  });

  it("signStatus() → ZEN.recover() returns the signing pub", async function () {
    const payload = buildStatus({ pub: pair.pub, port: 8420 });
    const str = await signStatus(payload, pair);
    assert.ok(typeof str === "string" && str.length > 88,
      "signed string must be a compact ZEN signed string");
    const recovered = await ZEN.recover(str);
    assert.strictEqual(recovered, pair.pub,
      "recovered pub must equal the signing key's pub");
  });

  it("signStatus() → ZEN.verify() round-trips the payload", async function () {
    const payload = buildStatus({ pub: pair.pub, port: 8420, mcp: true, peers: ["wss://zen.akao.io:8420/zen"] });
    const str = await signStatus(payload, pair);
    const pub = await ZEN.recover(str);
    const data = await ZEN.verify(str, pub);
    assert.ok(data, "verify must return data");
    // ZEN.verify auto-parses JSON messages — data is already the payload object
    const parsed = (typeof data === "string") ? JSON.parse(data) : data;
    assert.strictEqual(parsed.name, "zen");
    assert.strictEqual(parsed.pub, pair.pub);
    assert.strictEqual(parsed.version, pkgVersion);
    assert.strictEqual(parsed.mcp, true);
    assert.deepStrictEqual(parsed.peers, ["wss://zen.akao.io:8420/zen"]);
  });

  it("buildStatus() with all fields populated", function () {
    const payload = buildStatus({
      pub: pair.pub,
      domain: "zen.akao.io",
      ip4: "1.2.3.4",
      ip6: "2001:db8::1",
      port: 8420,
      peers: ["wss://zen.akao.io:8420/zen"],
      mcp: false,
    });
    assert.strictEqual(payload.domain, "zen.akao.io");
    assert.strictEqual(payload.ip4, "1.2.3.4");
    assert.strictEqual(payload.ip6, "2001:db8::1");
    assert.deepStrictEqual(payload.peers, ["wss://zen.akao.io:8420/zen"]);
  });
});
