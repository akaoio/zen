// lib/status.js — Shared status payload builder
//
// Used by script/server.js (HTTP GET /status) and lib/mcp/server.js (~pub/status graph soul).
// /status returns a compact ZEN signed string — client uses ZEN.recover() + ZEN.verify().
// ~pub/status stores a plain object — ZEN graph auth handles integrity.

import { createRequire } from "node:module";
import ZEN from "../zen.js";

const require = createRequire(import.meta.url);
const { version: pkgVersion } = require("../package.json");

/**
 * buildStatus(fields) → plain object (no `signed` field)
 *
 * fields: { pub, domain, ip4, ip6, port, peers, mcp }
 * Returns canonical status payload. `timestamp` is always Date.now() at call time.
 */
export function buildStatus(fields) {
  return {
    name: "zen",
    version: pkgVersion,
    domain: fields.domain ?? null,
    pub: fields.pub ?? null,
    mcp: fields.mcp ?? false,
    peers: fields.peers ?? [],
    timestamp: Date.now(),
    ip4: fields.ip4 ?? null,
    ip6: fields.ip6 ?? null,
    port: fields.port ?? null,
  };
}

/**
 * signStatus(payload, pair) → Promise<string>
 *
 * Signs the payload as a compact ZEN string.
 * Client decodes with: ZEN.recover(str) → pub, ZEN.verify(str, pub) → JSON string
 */
export async function signStatus(payload, pair) {
  return ZEN.sign(JSON.stringify(payload), pair);
}
