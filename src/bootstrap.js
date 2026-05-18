// Bootstrap peers are intentionally empty — each relay/client declares its own
// peers via PEERS env var or opts.peers. No hardcoded defaults.
const BOOT = [];

// ── Peer URL normalisation ────────────────────────────────────────────────────
// Accepts bare hostname, host:port, or any wss?://…/zen URL.
// Returns a canonical wss:// (or ws:// for localhost) URL.
function normalizePeerUrl(raw) {
  if (typeof raw !== "string") return null;
  raw = raw.trim();
  if (!raw) return null;
  if (/^wss?:\/\/|^https?:\/\//.test(raw)) return raw; // already has scheme
  const colonIdx = raw.lastIndexOf(":");
  let host, port;
  if (colonIdx > 0) {
    host = raw.slice(0, colonIdx);
    port = raw.slice(colonIdx + 1);
  } else {
    host = raw;
    port = "8420";
  }
  if (!host) return null;
  const isLocal = /^(localhost|127\.|::1)/.test(host);
  return (isLocal ? "ws" : "wss") + "://" + host + ":" + port + "/zen";
}

// ── Well-known peer list fetch ────────────────────────────────────────────────
// Fetches /.well-known/peers.json from a relay URL.
// Works in both browser and Node.js 18+ (global fetch).
// Returns normalised wss:// URL array, or [] on any error.
async function wellKnownBootstrap(relayUrl) {
  if (typeof fetch === "undefined") return [];
  try {
    const base = relayUrl
      .replace(/^wss:\/\//, "https://")
      .replace(/^ws:\/\//, "http://")
      .replace(/\/zen\/?$/, "");
    const url = base + "/.well-known/peers.json";
    const ctrl = typeof AbortController !== "undefined" ? new AbortController() : null;
    const timer = ctrl ? setTimeout(() => ctrl.abort(), 5000) : null;
    const res = await fetch(url, ctrl ? { signal: ctrl.signal } : {});
    if (timer) clearTimeout(timer);
    if (!res.ok) return [];
    const data = await res.json();
    if (!Array.isArray(data.peers)) return [];
    return data.peers.map(normalizePeerUrl).filter(Boolean);
  } catch (_) {
    return [];
  }
}

function parseFlag(value) {
  if (typeof value !== "string") return null;
  const normalized = value.trim().toLowerCase();
  if (["1", "true", "yes", "on"].includes(normalized)) return true;
  if (["", "0", "false", "no", "off"].includes(normalized)) return false;
  return null;
}

function bootstrapDisabled(env = {}) {
  const noBootstrap = parseFlag(env.NO_BOOTSTRAP);
  if (noBootstrap !== null) return noBootstrap;
  const bootstrap = parseFlag(env.BOOTSTRAP);
  if (bootstrap !== null) return !bootstrap;
  return false;
}

function mergePeers(...lists) {
  const merged = [];
  const seen = new Set();
  lists.flat().forEach((peer) => {
    if (typeof peer !== "string") return;
    const normalized = peer.trim();
    if (!normalized || seen.has(normalized)) return;
    seen.add(normalized);
    merged.push(normalized);
  });
  return merged;
}

function resolveBootstrapPeers(configuredPeers = [], opt = {}) {
  return mergePeers(opt.includeBootstrap === false ? [] : BOOT, configuredPeers);
}

function parsePeerEnv(value) {
  if (typeof value !== "string") return [];
  return value
    .split(",")
    .map((peer) => peer.trim())
    .filter(Boolean);
}

function resolveEnvPeers(env = {}) {
  const configuredPeers = parsePeerEnv(env.PEERS);
  if (bootstrapDisabled(env)) return configuredPeers;
  if (configuredPeers.length) return resolveBootstrapPeers(configuredPeers);
  return undefined;
}

export {
  BOOT,
  bootstrapDisabled,
  mergePeers,
  normalizePeerUrl,
  parsePeerEnv,
  resolveBootstrapPeers,
  resolveEnvPeers,
  wellKnownBootstrap,
};

export default {
  BOOT,
  bootstrapDisabled,
  mergePeers,
  normalizePeerUrl,
  parsePeerEnv,
  resolveBootstrapPeers,
  resolveEnvPeers,
  wellKnownBootstrap,
};
