import assert from "assert";

import { hookProvider, nsProvider, zoneNs, explain } from "../../lib/nscheck.js";

// The certificate that expired under #88 was issued with `--dns dns_gd` while
// the domain's NS had moved to Cloudflare. acme.sh kept writing the challenge
// into a zone that no longer decided anything, and said nothing, for 35 days.
//
// None of this decides whether a relay is healthy -- an overdue renewal does
// that, and it needs no table. What is here only explains a failure that has
// already been detected, so a wrong entry below costs a worse explanation, not
// a false alarm on a machine that is fine.

describe("naming the provider behind an acme.sh hook", function () {
  it("knows the hooks ssl.sh can configure", function () {
    assert.strictEqual(hookProvider("dns_cf"), "Cloudflare");
    assert.strictEqual(hookProvider("dns_gd"), "GoDaddy");
    assert.strictEqual(hookProvider("dns_aws"), "Route 53");
    assert.strictEqual(hookProvider("dns_dgon"), "DigitalOcean");
  });

  it("admits when it does not know a hook", function () {
    // acme.sh ships around 150 dns hooks. Claiming to recognise one it does
    // not is how a table like this starts lying.
    assert.strictEqual(hookProvider("dns_namecheap"), null);
  });

  it("is not confused by a webroot or standalone certificate", function () {
    assert.strictEqual(hookProvider("no"), null);
    assert.strictEqual(hookProvider("/var/www/html"), null);
    assert.strictEqual(hookProvider(""), null);
  });
});

describe("naming the provider behind a zone's nameservers", function () {
  it("recognises Cloudflare", function () {
    assert.strictEqual(nsProvider(["aleena.ns.cloudflare.com", "derek.ns.cloudflare.com"]), "Cloudflare");
  });

  it("recognises GoDaddy, whose nameservers do not say godaddy", function () {
    // The reason this table exists at all: matching on the provider's own name
    // would never match here.
    assert.strictEqual(nsProvider(["ns01.domaincontrol.com", "ns02.domaincontrol.com"]), "GoDaddy");
  });

  it("recognises Route 53", function () {
    assert.strictEqual(nsProvider(["ns-1234.awsdns-56.org", "ns-78.awsdns-90.co.uk"]), "Route 53");
  });

  it("admits when it does not recognise the nameservers", function () {
    assert.strictEqual(nsProvider(["ns1.somehost.example", "ns2.somehost.example"]), null);
  });

  it("is not fooled by a nameserver that merely contains the name", function () {
    assert.strictEqual(nsProvider(["cloudflare.attacker.example"]), null);
  });

  it("says nothing about an empty answer", function () {
    assert.strictEqual(nsProvider([]), null);
  });
});

describe("finding the zone that actually holds the nameservers", function () {
  // NS records live at the zone apex, so a relay's own name usually has none.
  const zone = { "akao.io": ["aleena.ns.cloudflare.com"] };
  const resolver = async (name) => {
    if (zone[name]) return zone[name];
    const err = new Error("no data");
    err.code = "ENODATA";
    throw err;
  };

  it("walks up from the relay's name to the zone apex", async function () {
    assert.deepStrictEqual(await zoneNs("zen.akao.io", resolver), {
      zone: "akao.io",
      ns: ["aleena.ns.cloudflare.com"],
    });
  });

  it("stops at the name itself when that is the apex", async function () {
    assert.deepStrictEqual(await zoneNs("akao.io", resolver), {
      zone: "akao.io",
      ns: ["aleena.ns.cloudflare.com"],
    });
  });

  it("gives up quietly when nothing answers", async function () {
    assert.strictEqual(await zoneNs("nowhere.example", resolver), null);
  });

  it("does not walk past a two-label name", async function () {
    // Querying the TLD would return the registry's nameservers, which say
    // nothing about who runs this zone.
    const seen = [];
    await zoneNs("a.b.c.example", async (n) => {
      seen.push(n);
      const err = new Error("no data");
      err.code = "ENODATA";
      throw err;
    });
    assert.ok(!seen.includes("example"), "asked the TLD: " + seen.join(", "));
  });
});

describe("explaining a renewal that is not happening", function () {
  it("names the zone it is talking about", function () {
    // "Cloudflare holds the zone" leaves the reader to work out which zone.
    const line = explain("dns_gd", ["aleena.ns.cloudflare.com"], "akao.io");
    assert.ok(/akao\.io/.test(line), "does not say which zone:\n" + line);
  });

  it("names the provider acme.sh uses and the one the zone is on", function () {
    const line = explain("dns_gd", ["aleena.ns.cloudflare.com"], "akao.io");
    assert.ok(/GoDaddy/.test(line), "does not name the configured provider:\n" + line);
    assert.ok(/Cloudflare/.test(line), "does not name the zone's provider:\n" + line);
  });

  it("stays quiet when the configured provider still runs the zone", function () {
    assert.strictEqual(explain("dns_cf", ["aleena.ns.cloudflare.com"]), null);
  });

  it("still shows the nameservers when it cannot name their provider", function () {
    // No verdict is possible, but the reader may well recognise them.
    const line = explain("dns_cf", ["ns1.somehost.example"]);
    assert.ok(/ns1\.somehost\.example/.test(line), "hid the nameservers:\n" + line);
  });

  it("says nothing at all about a webroot certificate", function () {
    // No DNS provider is configured, so there is no mismatch to have.
    assert.strictEqual(explain("no", ["aleena.ns.cloudflare.com"]), null);
  });

  it("says nothing when the nameservers could not be looked up", function () {
    assert.strictEqual(explain("dns_gd", null), null);
  });
});
