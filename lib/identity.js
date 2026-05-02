// lib/identity.js — Shared hardware-based identity for ZEN server and MCP
// Generates a deterministic keypair from hardware entropy (machine-id + MAC + hostname)
// and persists it to XDG state directory for reuse across processes.

import fs from "fs";
import path from "path";
import ZEN from "../index.js";
import * as xdg from "./xdg.js";
import { hwid } from "./discover.js";

/**
 * getOrCreateIdentity() → { pair, seed, hwid }
 *
 * Generates a stable hardware-derived keypair and persists it to:
 *   ~/.local/state/zen/identity.json
 *
 * The keypair is deterministically derived from hwid() and will be identical
 * across server.js and mcp.js on the same hardware.
 *
 * Returns:
 *   - pair: { pub, priv, epub, epriv } — full keypair for signing/encryption
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

  // Check if we have a persisted identity
  const statePath = path.join(xdg.state(), "identity.json");

  try {
    // Try to load existing identity
    if (fs.existsSync(statePath)) {
      const raw = fs.readFileSync(statePath, "utf8");
      const stored = JSON.parse(raw);

      // Verify that the stored identity matches current hardware
      if (stored.hwid === hraw && stored.seed === seed) {
        // Re-derive the pair from seed to ensure consistency
        // (We don't store private keys in plaintext, we regenerate from seed)
        const pair = await ZEN.pair(null, { seed });

        // Verify pub key matches
        if (pair.pub === stored.pub) {
          return { pair, seed, hwid: hraw };
        }
      }
    }
  } catch (e) {
    // Corrupt or missing file, regenerate
  }

  // Generate new identity from hardware seed
  const pair = await ZEN.pair(null, { seed });

  // Persist identity metadata (not private keys, just verification data)
  try {
    xdg.ensure(xdg.state());
    const identity = {
      hwid: hraw,
      seed: seed,
      pub: pair.pub,
      epub: pair.epub,
      created: Date.now(),
    };
    fs.writeFileSync(statePath, JSON.stringify(identity, null, 2), "utf8");
  } catch (e) {
    // Non-fatal: identity still works, just won't be verified on next load
    console.warn("Warning: could not persist identity:", e.message);
  }

  return { pair, seed, hwid: hraw };
}

/**
 * getIdentity() → { pair, seed, hwid } | null
 *
 * Gets the hardware-based identity without creating it if it doesn't exist.
 * Useful for read-only contexts.
 */
export async function getIdentity() {
  const hraw = hwid();
  if (!hraw) return null;

  const seed = await ZEN.hash(hraw, null, null, { name: "SHA-256", encode: "base62" });
  const pair = await ZEN.pair(null, { seed });

  return { pair, seed, hwid: hraw };
}
