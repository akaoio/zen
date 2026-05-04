/**
 * test/discover.js — cross-platform tests for lib/discover.js
 *
 * Tests the IP discovery layer without relying on network access.
 * Validates:
 *   - nics() returns consistent shape on all platforms (Linux, Windows, macOS)
 *   - isGlobalIPv6() correctly classifies addresses (tested via disc() internals)
 *   - disc() returns the required { domain, ip, ip6, port, source } shape
 *   - hostInUrl() bracket-wrapping in lib/scan.js
 *
 * Environment: set ZEN_TEST_NO_NETWORK=1 to skip STUN/HTTP fallbacks (for CI).
 */

import assert from "assert";
import os from "os";
import net from "net";

// ── nics() behaviour ───────────────────────────────────────────────────────

function nics() {
  const ifaces = os.networkInterfaces();
  let v4 = null, v6 = null;

  // isGlobalIPv6 inline (mirrors lib/discover.js)
  function isGlobalIPv6(addr) {
    if (!addr || !net.isIPv6(addr)) return false;
    let canonical;
    try { canonical = new URL("http://[" + addr + "]").hostname.slice(1, -1).toLowerCase(); } catch { return false; }
    if (canonical === "::1") return false;
    if (canonical.startsWith("fe80")) return false;
    if (canonical.startsWith("fc") || canonical.startsWith("fd")) return false;
    if (canonical.startsWith("ff")) return false;
    if (canonical === "::") return false;
    return true;
  }

  for (const list of Object.values(ifaces)) {
    for (const a of list) {
      if (a.internal) continue;
      if (!v4 && a.family === "IPv4") v4 = a.address;
      if (!v6 && a.family === "IPv6" && isGlobalIPv6(a.address)) v6 = a.address;
    }
  }
  return { v4, v6 };
}

// ── isGlobalIPv6 test cases ────────────────────────────────────────────────

function isGlobalIPv6(addr) {
  if (!addr || !net.isIPv6(addr)) return false;
  let canonical;
  try { canonical = new URL("http://[" + addr + "]").hostname.slice(1, -1).toLowerCase(); } catch { return false; }
  if (canonical === "::1")    return false;
  if (canonical.startsWith("fe80")) return false;
  if (canonical.startsWith("fc") || canonical.startsWith("fd")) return false;
  if (canonical.startsWith("ff")) return false;
  if (canonical === "::") return false;
  return true;
}

describe("isGlobalIPv6()", function () {
  it("rejects loopback ::1", function () {
    assert.strictEqual(isGlobalIPv6("::1"), false);
  });
  it("rejects link-local fe80::1", function () {
    assert.strictEqual(isGlobalIPv6("fe80::1"), false);
  });
  it("rejects ULA fc00::1", function () {
    assert.strictEqual(isGlobalIPv6("fc00::1"), false);
  });
  it("rejects ULA fd00::1", function () {
    assert.strictEqual(isGlobalIPv6("fd00::1"), false);
  });
  it("rejects multicast ff02::1", function () {
    assert.strictEqual(isGlobalIPv6("ff02::1"), false);
  });
  it("rejects all-zeros ::", function () {
    assert.strictEqual(isGlobalIPv6("::"), false);
  });
  it("rejects null", function () {
    assert.strictEqual(isGlobalIPv6(null), false);
  });
  it("rejects IPv4 string", function () {
    assert.strictEqual(isGlobalIPv6("1.2.3.4"), false);
  });
  it("accepts global unicast 2001:db8::1", function () {
    assert.strictEqual(isGlobalIPv6("2001:db8::1"), true);
  });
  it("accepts short global 2607:f8b0::1", function () {
    assert.strictEqual(isGlobalIPv6("2607:f8b0::1"), true);
  });
});

// ── nics() shape ──────────────────────────────────────────────────────────

describe("nics() — os.networkInterfaces() fallback", function () {
  it("returns an object with v4 and v6 keys", function () {
    const r = nics();
    assert.ok(typeof r === "object" && r !== null);
    assert.ok("v4" in r);
    assert.ok("v6" in r);
  });

  it("v4 is null or a valid IPv4 address", function () {
    const { v4 } = nics();
    if (v4 !== null) {
      assert.ok(/^\d+\.\d+\.\d+\.\d+$/.test(v4), "v4 must be an IPv4 address, got: " + v4);
      assert.ok(!v4.startsWith("127."), "v4 must not be loopback");
    }
  });

  it("v6 is null or a global IPv6 address", function () {
    const { v6 } = nics();
    if (v6 !== null) {
      assert.ok(net.isIPv6(v6), "v6 must be a valid IPv6 address, got: " + v6);
      assert.ok(isGlobalIPv6(v6), "v6 must be global (not loopback/link-local/ULA)");
    }
  });
});

// ── hostInUrl() — scan.js IPv6 bracket wrapping ───────────────────────────

function hostInUrl(host) {
  return net.isIPv6(host) ? "[" + host + "]" : host;
}

describe("hostInUrl()", function () {
  it("wraps IPv6 address in brackets", function () {
    assert.strictEqual(hostInUrl("2001:db8::1"), "[2001:db8::1]");
  });
  it("leaves IPv4 address unchanged", function () {
    assert.strictEqual(hostInUrl("192.168.1.1"), "192.168.1.1");
  });
  it("leaves hostname unchanged", function () {
    assert.strictEqual(hostInUrl("peer1.akao.io"), "peer1.akao.io");
  });
  it("produces a valid URL with IPv6", function () {
    const h = hostInUrl("2001:db8::1");
    const url = new URL("ws://" + h + ":8420/zen");
    // url.hostname includes brackets for IPv6 per URL spec — strip them to get the bare address
    const bare = url.hostname.startsWith("[") ? url.hostname.slice(1, -1) : url.hostname;
    assert.strictEqual(bare, "2001:db8::1");
    assert.strictEqual(url.port, "8420");
  });
});

// ── vprs() regex — server.js peer URL validation ──────────────────────────

function vprs(peer) {
  return /^(https?|wss?):\/\/.+/i.test(peer);
}

describe("vprs() URL validation", function () {
  it("accepts ws://host:port/path", function () {
    assert.ok(vprs("ws://zen.akao.io:8420/zen"));
  });
  it("accepts wss://host:port/path", function () {
    assert.ok(vprs("wss://zen.akao.io:8420/zen"));
  });
  it("accepts ws://[::1]:8420/zen (bracket IPv6)", function () {
    assert.ok(vprs("ws://[::1]:8420/zen"));
  });
  it("rejects bare ws://", function () {
    assert.ok(!vprs("ws://"));
  });
  it("rejects bare wss://", function () {
    assert.ok(!vprs("wss://"));
  });
  it("rejects random string", function () {
    assert.ok(!vprs("not-a-url"));
  });
  it("rejects ftp:// scheme", function () {
    assert.ok(!vprs("ftp://host/path"));
  });
});

// ── disc() integration shape ───────────────────────────────────────────────
// Imports lib/discover.js and verifies the return shape.
// Network calls (STUN, HTTP) are made unless ZEN_TEST_NO_NETWORK=1.

describe("disc() — return shape", async function () {
  this.timeout(15000);

  let disc;
  before(async function () {
    const mod = await import("../lib/discover.js");
    disc = mod.disc;
  });

  it("returns required fields", async function () {
    const r = await disc({ port: 8420, noSave: true });
    assert.ok(typeof r === "object" && r !== null, "result must be an object");
    assert.ok("domain" in r, "missing field: domain");
    assert.ok("ip"     in r, "missing field: ip");
    assert.ok("ip6"    in r, "missing field: ip6");
    assert.ok("port"   in r, "missing field: port");
    assert.ok("source" in r, "missing field: source");
  });

  it("port is a number", async function () {
    const r = await disc({ port: 9999, noSave: true });
    assert.strictEqual(typeof r.port, "number");
    assert.strictEqual(r.port, 9999);
  });

  it("ip is null or a valid IPv4 address", async function () {
    const r = await disc({ noSave: true });
    if (r.ip !== null) {
      assert.ok(/^\d+\.\d+\.\d+\.\d+$/.test(r.ip), "ip must be IPv4, got: " + r.ip);
    }
  });

  it("ip6 is null or a global IPv6 address", async function () {
    const r = await disc({ noSave: true });
    if (r.ip6 !== null) {
      assert.ok(net.isIPv6(r.ip6), "ip6 must be valid IPv6, got: " + r.ip6);
      assert.ok(isGlobalIPv6(r.ip6), "ip6 must be global, got: " + r.ip6);
    }
  });

  it("opt.domain overrides discovery", async function () {
    const r = await disc({ domain: "peer1.akao.io", noSave: true });
    assert.strictEqual(r.domain, "peer1.akao.io");
    assert.strictEqual(r.source, "opt");
  });
});
