// What `zen doctor` has to say about TLS.
//
// A relay served an expired certificate for five days while every other check
// on the box was green (#88). Two silences made that possible: nothing looked
// at the certificate's validity window, and nothing looked at whether renewal
// was still happening.
//
// The load-bearing check here is the renewal date. acme.sh records when the
// next renewal is due, and a date in the past means it did not happen. That is
// true whatever the provider, whatever the challenge method, and whatever went
// wrong -- including the ways nobody has thought of. It needs no table and so
// it cannot go stale.
//
// Naming the DNS provider is only ever an explanation offered *alongside* a
// failure already detected that way. See lib/nscheck.js for why that
// distinction is what makes the provider table safe to keep.

import fs from "fs";
import path from "path";

import { inspect, describe as describeCert } from "./certwatch.js";
import { zoneNs, explain } from "./nscheck.js";

const DAY = 24 * 60 * 60 * 1000;

// acme.sh keeps ECC certificates in <domain>_ecc and RSA ones in <domain>.
function domainConf(acmeDir, domain) {
  for (const dir of [`${domain}_ecc`, domain]) {
    const file = path.join(acmeDir, dir, `${domain}.conf`);
    if (fs.existsSync(file)) return file;
  }
  return null;
}

function confValue(text, key) {
  const m = text.match(new RegExp("^" + key + "='([^']*)'", "m"));
  return m ? m[1] : "";
}

export async function report({ domain, acmeDir, certPath, now = Date.now(), resolveNs }) {
  const lines = [];

  // A relay on plain HTTP is a choice, not a fault. Say nothing.
  if (!certPath || !fs.existsSync(certPath)) return lines;

  const info = inspect(fs.readFileSync(certPath, "utf8"), now);
  const complaint = describeCert("certificate", info);
  if (complaint) {
    lines.push({ level: info.state === "expiring" ? "warn" : "bad", label: "certificate", detail: complaint });
  } else {
    lines.push({ level: "ok", label: "certificate", detail: `${info.subject}, ${info.daysLeft} days left` });
  }

  const conf = domainConf(acmeDir, domain);
  if (!conf) {
    lines.push({
      level: "warn",
      label: "renewal",
      detail: `acme.sh has no record of ${domain} — nothing is going to renew this certificate`,
    });
    return lines;
  }

  const text = fs.readFileSync(conf, "utf8");
  const hook = confValue(text, "Le_Webroot");
  const dueStr = confValue(text, "Le_NextRenewTimeStr");
  const due = Date.parse(dueStr);

  if (!Number.isFinite(due)) {
    lines.push({ level: "warn", label: "renewal", detail: `acme.sh records no next renewal date for ${domain}` });
    return lines;
  }

  if (due > now) {
    lines.push({ level: "ok", label: "renewal", detail: `due ${dueStr}` });
    return lines;
  }

  // Overdue. This is the whole point: renewal had its date and did not happen.
  const late = Math.floor((now - due) / DAY);
  lines.push({
    level: "bad",
    label: "renewal",
    detail: `due ${late} day${late === 1 ? "" : "s"} ago (${dueStr}) and has not happened`,
  });

  // Only now is it worth naming names.
  let ns = null;
  if (resolveNs) {
    const found = await zoneNs(domain, resolveNs);
    ns = found ? found.ns : null;
  }
  const why = explain(hook, ns);
  if (why) lines.push({ level: "bad", label: "dns provider", detail: why });

  lines.push({ level: "fix", label: `${domain}: check the acme.sh log, then re-issue with the provider that holds the zone`, detail: "" });

  return lines;
}
