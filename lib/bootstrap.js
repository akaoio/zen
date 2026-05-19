// lib/bootstrap.js — Node.js bootstrap discovery for ZEN relays.
// Queries DNS TXT records on a seed domain (default: peers.akao.io) to find
// the initial set of relay peers without any hardcoded addresses in source.
// Falls back to well-known HTTP if DNS is unavailable.
//
// DNS record format (one TXT record per peer, or space-separated in one record):
//   peers.akao.io  TXT  "zen.akao.io"
//   peers.akao.io  TXT  "zen0.akao.io"
//   peers.akao.io  TXT  "zen1.akao.io"
//   — or —
//   peers.akao.io  TXT  "zen.akao.io zen0.akao.io zen1.akao.io"

import dns from "node:dns/promises";
import { normalizePeerUrl, wellKnownBootstrap } from "../src/bootstrap.js";

const DNS_SEED = "peers.akao.io";

// Parse DNS TXT records: flatten all character-strings, tokenise on whitespace.
function parseTxtRecords(records) {
  return records
    .flat()
    .join(" ")
    .split(/\s+/)
    .map(normalizePeerUrl)
    .filter(Boolean);
}

// Query DNS TXT on dnsName; returns normalised wss:// URLs or [].
async function dnsBootstrap(dnsName = DNS_SEED) {
  try {
    const records = await dns.resolveTxt(dnsName);
    return parseTxtRecords(records);
  } catch (_) {
    return [];
  }
}

// Full bootstrap sequence for a relay on cold start:
//   1. DNS TXT on opts.dns  (default: "peers.akao.io")
//   2. If DNS returns nothing: well-known HTTP on each URL in opts.fallback[]
//
// opts.dns     {string}          — DNS TXT seed domain (falsy = skip DNS)
// opts.fallback {string|string[]} — relay URL(s) to try /.well-known/peers.json
async function discoverPeers(opts = {}) {
  const dnsName = opts.dns !== undefined ? opts.dns : DNS_SEED;

  if (dnsName) {
    const dnsPeers = await dnsBootstrap(dnsName);
    if (dnsPeers.length) return dnsPeers;
  }

  const fallbacks = [].concat(opts.fallback || []).filter(Boolean);
  for (const url of fallbacks) {
    const peers = await wellKnownBootstrap(url);
    if (peers.length) return peers;
  }

  return [];
}

export { dnsBootstrap, discoverPeers, normalizePeerUrl, wellKnownBootstrap };
export default { dnsBootstrap, discoverPeers, normalizePeerUrl, wellKnownBootstrap };
