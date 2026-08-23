// Whether the certificate this relay is serving is actually usable.
//
// server.js used to check that the PEM contained "BEGIN CERTIFICATE" and stop
// there. That accepts an expired certificate without a murmur: the process logs
// "SSL certificates found, enabling HTTPS...", starts, and then refuses every
// handshake. One relay did exactly that for five days (#88) while looking
// perfectly healthy in its own logs.
//
// Nothing here throws and nothing here exits. A relay that is up and serving
// bad TLS is worth more than one that is down, so the job of this module is to
// say so loudly and leave the decision to the caller.

import crypto from "crypto";

const DAY = 24 * 60 * 60 * 1000;

// Long enough before expiry that a human has time to act, short enough that the
// warning still means something when it appears. Renewal is due at 30 days out,
// so 14 means renewal has already had two weeks to happen and did not.
const WARN_DAYS = 14;

// `now` is a parameter so a test can reach the whole calendar from a single
// certificate, rather than minting certificates dated into the past.
export function inspect(pem, now = Date.now()) {
  let cert;
  try {
    cert = new crypto.X509Certificate(pem);
  } catch {
    return { state: "unreadable" };
  }

  const validFrom = Date.parse(cert.validFrom);
  const validTo = Date.parse(cert.validTo);
  if (!Number.isFinite(validFrom) || !Number.isFinite(validTo)) {
    return { state: "unreadable" };
  }

  const daysLeft = Math.floor((validTo - now) / DAY);

  let state;
  if (now >= validTo) state = "expired";
  else if (now < validFrom) state = "not-yet-valid";
  else if (validTo - now <= WARN_DAYS * DAY) state = "expiring";
  else state = "ok";

  return { subject: cert.subject, validFrom, validTo, daysLeft, state };
}

function plural(n) {
  return Math.abs(n) === 1 ? "" : "s";
}

function when(ms) {
  return new Date(ms).toISOString().replace(/\.\d+Z$/, "Z");
}

// The line to log about one certificate, or null when there is nothing worth
// saying. It names the certificate, what is wrong with it, and what that means
// for clients -- whoever reads this in a log should not have to infer the
// consequence, because the consequence is the part that went unnoticed.
export function describe(label, info) {
  switch (info.state) {
    case "expired": {
      const d = Math.abs(info.daysLeft);
      return `${label} EXPIRED ${d} day${plural(d)} ago (${when(info.validTo)}) — every TLS handshake is being refused`;
    }
    case "expiring": {
      const d = info.daysLeft;
      return `${label} expires in ${d} day${plural(d)} (${when(info.validTo)}) — renewal has not happened`;
    }
    case "not-yet-valid":
      return `${label} is not valid until ${when(info.validFrom)} — check this machine's clock`;
    case "unreadable":
      return `${label} could not be parsed as a certificate — serving TLS with something unreadable`;
    default:
      return null;
  }
}

// Every certificate the relay is serving, in one pass. Returns the lines worth
// logging, so callers can do this on boot and again on a timer without either
// of them knowing how a certificate is judged.
//
// A certificate that was never configured is not a problem: the SNI one is
// optional, and complaining about its absence would train the reader to ignore
// this whole class of warning.
export function check(certs, now = Date.now()) {
  const lines = [];
  for (const { label, pem } of certs) {
    if (!pem) continue;
    const line = describe(label, inspect(pem, now));
    if (line) lines.push(line);
  }
  return lines;
}
