const BOOT = ["wss://zen.akao.io", "wss://peer0.akao.io", "wss://peer1.akao.io"];

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

export { BOOT, bootstrapDisabled, mergePeers, resolveBootstrapPeers };

export default { BOOT, bootstrapDisabled, mergePeers, resolveBootstrapPeers };
