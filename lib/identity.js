// lib/identity.js — Shared hardware-based identity for ZEN server and MCP
// Generates a deterministic keypair from hardware entropy (machine-id + MAC + hostname)
// SECURITY: Identity is NEVER persisted to disk. Always calculated on demand from hardware.

import ZEN from "../index.js";
import { hwid } from "./discover.js";

/**
 * getOrCreateIdentity(opt?) → { pair, seed, hwid, mode } | null
 *
 * Returns the agent's keypair. Priority order:
 *   1. opt.seed             — explicit passphrase (portable, cross-machine)
 *   2. ZEN_IDENTITY_SEED    — env var
 *   3. --identity-file flag — path in process.argv
 *   4. opt.identityFile     — explicit file path
 *   5. Hardware fingerprint — machine-locked default
 *
 * SECURITY: Nothing is persisted to disk. Keypair exists in runtime memory only.
 *
 * Returns:
 *   - pair:  { pub, priv, address } — keypair for signing
 *   - seed:  base62-encoded hash of source entropy
 *   - hwid:  raw hardware identity string (null for seed mode)
 *   - mode:  "seed" | "hardware"
 *
 * Returns null if hardware entropy is unavailable and no seed is provided.
 */
export async function getOrCreateIdentity(opt = {}) {
  // Priority 1: explicit opt.seed
  let seed_input = opt.seed || null;

  // Priority 2: ZEN_IDENTITY_SEED env var
  if (!seed_input) seed_input = process.env.ZEN_IDENTITY_SEED || null;

  // Priority 3: --identity-file <path> in process.argv
  if (!seed_input) {
    const flagIdx = process.argv.indexOf("--identity-file");
    if (flagIdx !== -1 && process.argv[flagIdx + 1]) {
      try {
        const { readFileSync } = await import("node:fs");
        seed_input = readFileSync(process.argv[flagIdx + 1], "utf8").trim();
      } catch (_) {}
    }
  }

  // Priority 4: opt.identityFile
  if (!seed_input && opt.identityFile) {
    try {
      const { readFileSync } = await import("node:fs");
      seed_input = readFileSync(opt.identityFile, "utf8").trim();
    } catch (_) {}
  }

  // Seed-based mode (portable)
  if (seed_input) {
    const seed = await ZEN.hash(seed_input, null, null, { name: "SHA-256", encode: "base62" });
    const pair = await ZEN.pair(null, { seed });
    return { pair, seed, hwid: null, mode: "seed" };
  }

  // Priority 5: hardware fingerprint (machine-locked default)
  const hraw = hwid();
  if (!hraw) return null;
  const seed = await ZEN.hash(hraw, null, null, { name: "SHA-256", encode: "base62" });
  const pair = await ZEN.pair(null, { seed });
  return { pair, seed, hwid: hraw, mode: "hardware" };
}

/**
 * getIdentity() → { pair, seed, hwid, mode } | null
 * Alias for getOrCreateIdentity() for backward compatibility.
 */
export async function getIdentity() {
  return getOrCreateIdentity();
}

