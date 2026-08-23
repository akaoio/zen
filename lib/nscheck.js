// Who actually runs a domain's DNS, and whether acme.sh still agrees.
//
// A certificate expired under #88 because it was issued with `--dns dns_gd`
// while the domain's nameservers had since moved to Cloudflare. acme.sh went on
// writing the challenge into a GoDaddy zone that no longer decided anything,
// and nothing anywhere noticed for 35 days.
//
// IMPORTANT: nothing here decides whether a relay is healthy. An overdue
// renewal decides that, and it needs no table -- overdue is overdue whoever the
// provider is. What is here only *explains* a failure that has already been
// detected some other way.
//
// That distinction is the whole reason this file is safe to have. acme.sh ships
// around 150 dns hooks and this knows four; nameserver hostnames change when
// companies rebrand. A wrong or missing entry below costs a worse explanation.
// It can never turn a healthy relay into a failing check, because it is never
// asked whether the relay is healthy.

const PROVIDERS = [
  { name: "Cloudflare", hooks: ["dns_cf"], ns: /(^|\.)ns\.cloudflare\.com$/ },
  // GoDaddy's nameservers say "domaincontrol", never "godaddy" -- matching on
  // the provider's own name would have missed the very case that prompted this.
  { name: "GoDaddy", hooks: ["dns_gd"], ns: /(^|\.)domaincontrol\.com$/ },
  { name: "Route 53", hooks: ["dns_aws"], ns: /(^|\.)awsdns-\d+\.(com|net|org|co\.uk)$/ },
  { name: "DigitalOcean", hooks: ["dns_dgon"], ns: /(^|\.)digitalocean\.com$/ },
];

// The provider behind an acme.sh hook name, or null for a hook this does not
// know -- including "no" and a webroot path, which are not DNS at all.
export function hookProvider(hook) {
  if (!hook) return null;
  const found = PROVIDERS.find((p) => p.hooks.includes(hook));
  return found ? found.name : null;
}

// The provider behind a set of nameserver hostnames, or null. Anchored at the
// end of the hostname on purpose: a name that merely contains "cloudflare"
// proves nothing about who serves the zone.
export function nsProvider(names) {
  if (!names || !names.length) return null;
  for (const p of PROVIDERS) {
    if (names.some((n) => p.ns.test(String(n).replace(/\.$/, "").toLowerCase()))) {
      return p.name;
    }
  }
  return null;
}

// NS records live at the zone apex, so a relay's own name usually has none.
// Walk up a label at a time until something answers. `resolveNs` is a parameter
// so this is reachable in a test without touching the network.
//
// Stops at two labels: querying the TLD would return the registry's
// nameservers, which say nothing about who runs this zone.
export async function zoneNs(name, resolveNs) {
  let labels = String(name).replace(/\.$/, "").split(".");
  while (labels.length >= 2) {
    const candidate = labels.join(".");
    try {
      const ns = await resolveNs(candidate);
      if (ns && ns.length) return { zone: candidate, ns };
    } catch {
      // No NS at this level, or the lookup failed; try the parent.
    }
    labels = labels.slice(1);
  }
  return null;
}

// One line explaining what acme.sh is talking to versus who holds the zone, or
// null when there is nothing useful to say. Only ever called about a renewal
// already known to be failing -- see the note at the top of this file.
export function explain(hook, ns, zone) {
  const configured = hookProvider(hook);
  if (!configured) return null; // not a DNS hook this knows: nothing to compare
  if (!ns || !ns.length) return null; // no lookup, no claim

  // Naming the zone matters: it is the thing the reader has to go and change,
  // and it is rarely the name they typed -- NS records live at the apex.
  const where = zone || "the zone";
  const actual = nsProvider(ns);
  if (actual === configured) return null;
  if (actual) {
    return `acme.sh validates through ${configured}, but ${actual} holds ${where} — the challenge is being written where nothing reads it`;
  }
  return `acme.sh validates through ${configured}; the nameservers for ${where} are ${ns.join(", ")}`;
}
