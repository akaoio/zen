// Bootstrap peers are intentionally empty — each relay/client declares its own
// peers via PEERS env var or opts.peers. No hardcoded defaults.
const BOOT = [];

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
  parsePeerEnv,
  resolveBootstrapPeers,
  resolveEnvPeers,
};

export default {
  BOOT,
  bootstrapDisabled,
  mergePeers,
  parsePeerEnv,
  resolveBootstrapPeers,
  resolveEnvPeers,
};
