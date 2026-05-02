// lib/identity.js — Shared hardware-based identity for ZEN server and MCP
// Generates a deterministic keypair from hardware entropy (machine-id + MAC + hostname)
// SECURITY: Identity is NEVER persisted to disk. Always calculated on demand from hardware.

import ZEN from "../index.js";
import { hwid } from "./discover.js";

/**
 * getOrCreateIdentity() → { pair, seed, hwid }
 *
 * Generates a stable hardware-derived keypair from hardware entropy.
 * The keypair is deterministically derived from hwid() and will be identical
 * across server.js and mcp.js on the same hardware.
 *
 * SECURITY: Identity is calculated on every call. Nothing is persisted to disk.
 * The seed and keypair exist only in runtime memory.
 *
 * Returns:
 *   - pair: { pub, priv, address } — keypair for signing
 *   - seed: base62-encoded hash of hwid
 *   - hwid: raw hardware identity string
 *
 * If hardware entropy is unavailable, returns null (fallback to random identity).
 */
export async function getOrCreateIdentity() {
  const hraw = hwid();
  if (!hraw) {
    return null; // No hardware entropy available
  }

  // Derive seed from hardware using SHA-256 (deterministic, no random salt)
  const seed = await ZEN.hash(hraw, null, null, { name: "SHA-256", encode: "base62" });

  // Generate keypair from seed (calculated every time, never persisted)
  const pair = await ZEN.pair(null, { seed });

  return { pair, seed, hwid: hraw };
}

/**
 * getIdentity() → { pair, seed, hwid } | null
 *
 * Alias for getOrCreateIdentity() for backward compatibility.
 * Gets the hardware-based identity by calculating it from hardware entropy.
 */
export async function getIdentity() {
  return getOrCreateIdentity();
}

