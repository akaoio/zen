import os from "os";
import path from "path";
import fs from "fs";

// ZEN_NET controls which network this process belongs to.
// "mainnet" (default): ~/.config/zen, ~/.local/share/zen, port 8420
// "testnet":           ~/.config/zen-testnet, ~/.local/share/zen-testnet, port 8421
// Any other value X:  ~/.config/zen-X, port 8422+
export function net() { return process.env.ZEN_NET || "mainnet"; }

const NET_SUFFIX = (function () {
  const n = net();
  return n === "mainnet" ? "" : "-" + n;
})();

// Default relay port per network (used when no port config file exists).
export const DEFAULT_PORT = (function () {
  const n = net();
  if (n === "mainnet") return 8420;
  if (n === "testnet") return 8421;
  return 8422; // future nets
})();

function zenDir(xdgEnv) {
  var fallbacks = { XDG_CONFIG_HOME: [".config"], XDG_DATA_HOME: [".local", "share"], XDG_STATE_HOME: [".local", "state"] };
  var base = process.env[xdgEnv] || path.join.apply(path, [os.homedir()].concat(fallbacks[xdgEnv]));
  return path.join(base, "zen" + NET_SUFFIX);
}

export function config() { return zenDir("XDG_CONFIG_HOME"); }
export function data()   { return zenDir("XDG_DATA_HOME"); }
export function state()  { return zenDir("XDG_STATE_HOME"); }

export function ensure(dirPath) {
  try { if (!fs.existsSync(dirPath)) { fs.mkdirSync(dirPath, { recursive: true }); } } catch (e) {}
  return dirPath;
}
