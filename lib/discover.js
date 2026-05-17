/**
 * lib/discover.js — ZEN self IP/domain discovery
 *
 * Offline-first priority:
 *   1. Read ~/.config/zen/domain (user-configured)
 *   2. ip route get 8.8.8.8  → outbound LAN/WAN IPv4  (Linux)
 *   3. ip -6 route get / ip -6 addr → outbound global IPv6  (Linux)
 *   4. hostname -I            → first non-loopback IP (v4 or v6)  (Linux)
 *   5. os.networkInterfaces() → cross-platform LAN IP  (Windows / macOS)
 *   6. STUN (stun.l.google.com:19302) → public IPv4 behind NAT
 *   7. STUN over udp6         → public IPv6 behind NAT
 *   8. HTTP fallback (api.ipify.org / api6.ipify.org / ifconfig.me)
 *
 * Returns: { domain, ip, ip6, port, source }
 */

import dgram from "dgram";
import net from "net";
import os from "os";
import { execSync } from "child_process";
import fs from "fs";
import path from "path";
import https from "https";
import http from "http";
import * as xdg from "./xdg.js";

const DOMF  = path.join(xdg.config(), "domain");
const PORTF = path.join(xdg.config(), "port");

// ── helpers ────────────────────────────────────────────────────────────────

function rft(f) {
  try { return fs.readFileSync(f, "utf8").trim() || null; } catch { return null; }
}

function shell(cmd) {
  try { return execSync(cmd, { timeout: 2000, stdio: ["ignore", "pipe", "ignore"] }).toString().trim() || null; } catch { return null; }
}

function hget(url, tout = 3000) {
  return new Promise((ok) => {
    const mod = url.startsWith("https") ? https : http;
    const req = mod.get(url, { timeout: tout }, (res) => {
      let d = "";
      res.on("data", (c) => { d += c; });
      res.on("end",  () => ok(d.trim()));
    });
    req.on("error",   () => ok(null));
    req.on("timeout", () => { req.destroy(); ok(null); });
  });
}

// ── STUN IPv4 (RFC 3489/5389) — minimal STUN binding request ─────────────
function stun(host = "stun.l.google.com", port = 19302, tout = 3000) {
  return new Promise((ok) => {
    const sk   = dgram.createSocket("udp4");
    const done = (ip) => { try { sk.close(); } catch {} ok(ip); };
    const timer  = setTimeout(() => done(null), tout);

    const buf = Buffer.alloc(20);
    buf.writeUInt16BE(0x0001,     0);  // Binding Request
    buf.writeUInt16BE(0,          2);  // length: 0 attrs
    buf.writeUInt32BE(0x2112A442, 4);  // magic cookie
    for (let i = 8; i < 20; i++) buf[i] = Math.floor(Math.random() * 256);

    sk.once("message", (msg) => {
      clearTimeout(timer);
      let off = 20;
      while (off < msg.length - 4) {
        const typ = msg.readUInt16BE(off);
        const len = msg.readUInt16BE(off + 2);
        if (typ === 0x0020 || typ === 0x0001) {
          if (msg[off + 5] === 0x01 && len >= 8) {
            let ip;
            if (typ === 0x0020) {
              ip = [
                (msg[off + 8]  ^ 0x21).toString(),
                (msg[off + 9]  ^ 0x12).toString(),
                (msg[off + 10] ^ 0xA4).toString(),
                (msg[off + 11] ^ 0x42).toString(),
              ].join(".");
            } else {
              ip = [msg[off+8], msg[off+9], msg[off+10], msg[off+11]].join(".");
            }
            done(ip);
            return;
          }
        }
        off += 4 + len + (len % 4 ? 4 - (len % 4) : 0);
      }
      done(null);
    });

    sk.on("error", () => { clearTimeout(timer); done(null); });
    sk.send(buf, 0, buf.length, port, host, (err) => { if (err) { clearTimeout(timer); done(null); } });
  });
}

// ── STUN IPv6 — same request over udp6, parses family 0x02 (16-byte IPv6) ──
function stun6(host = "stun.l.google.com", port = 19302, tout = 3000) {
  return new Promise((ok) => {
    let sk;
    try { sk = dgram.createSocket({ type: "udp6", ipv6Only: true }); } catch { return ok(null); }
    const done = (ip) => { try { sk.close(); } catch {} ok(ip); };
    const timer  = setTimeout(() => done(null), tout);

    const buf = Buffer.alloc(20);
    buf.writeUInt16BE(0x0001,     0);  // Binding Request
    buf.writeUInt16BE(0,          2);  // length: 0 attrs
    buf.writeUInt32BE(0x2112A442, 4);  // magic cookie
    for (let i = 8; i < 20; i++) buf[i] = Math.floor(Math.random() * 256);
    const txid = buf.slice(8, 20);

    sk.once("message", (msg) => {
      clearTimeout(timer);
      let off = 20;
      while (off < msg.length - 4) {
        const typ = msg.readUInt16BE(off);
        const len = msg.readUInt16BE(off + 2);
        // family 0x02 = IPv6
        if ((typ === 0x0020 || typ === 0x0001) && msg[off + 5] === 0x02 && len >= 20) {
          const parts = [];
          for (let i = 0; i < 16; i += 2) {
            let word = msg.readUInt16BE(off + 8 + i);
            if (typ === 0x0020) {
              // RFC 5389 §15.2: XOR-MAPPED-ADDRESS
              // First 4 bytes XORed with magic cookie (0x2112A442),
              // remaining 12 bytes XORed with the 96-bit transaction ID.
              const xor = i < 4
                ? (0x2112A442 >>> ((2 - i) * 8)) & 0xffff
                : txid.readUInt16BE(i - 4);
              word ^= xor;
            }
            parts.push(word.toString(16));
          }
          const ip6 = parts.join(":");
          if (isGlobalIPv6(ip6)) { done(ip6); return; }
        }
        off += 4 + len + (len % 4 ? 4 - (len % 4) : 0);
      }
      done(null);
    });

    sk.on("error", () => { clearTimeout(timer); done(null); });
    sk.send(buf, 0, buf.length, port, host, (err) => { if (err) { clearTimeout(timer); done(null); } });
  });
}

// ── HTTP fallback: IPv6-only endpoint ─────────────────────────────────────
function hget6(tout = 3000) {
  return (
    hget("https://api6.ipify.org", tout)
      .then(r => (r && isGlobalIPv6(r.trim()) ? r.trim() : null))
      .catch(() => null)
  );
}

// ── IPv6 global address helper ─────────────────────────────────────────────
// Uses Node's net.isIPv6 for correct parsing, then checks known non-global prefixes.
// Relies on kernel-level scope filtering (glip6 uses "scope global") as primary guard;
// this is a belt-and-suspenders check for addresses from STUN/HTTP sources.
function isGlobalIPv6(addr) {
  if (!addr || !net.isIPv6(addr)) return false;
  // Expand via URL trick to get the canonical form for prefix checks
  let canonical;
  try { canonical = new URL("http://[" + addr + "]").hostname.slice(1, -1).toLowerCase(); } catch { return false; }
  if (canonical === "::1") return false;
  if (canonical.startsWith("fe80")) return false;
  if (canonical.startsWith("fc") || canonical.startsWith("fd")) return false;
  if (canonical.startsWith("ff")) return false;
  if (canonical === "::") return false;
  return true;
}

// ── offline IP discovery ───────────────────────────────────────────────────

// ── cross-platform local IP discovery via os.networkInterfaces() ─────────
function nics() {
  const ifaces = os.networkInterfaces();
  let v4 = null, v6 = null;
  for (const list of Object.values(ifaces)) {
    for (const a of list) {
      if (a.internal) continue;
      if (!v4 && a.family === "IPv4") v4 = a.address;
      if (!v6 && a.family === "IPv6" && isGlobalIPv6(a.address)) v6 = a.address;
    }
  }
  return { v4, v6 };
}

function glip() {
  const route = shell("ip route get 8.8.8.8 2>/dev/null");
  if (route) {
    const m = route.match(/src\s+(\d+\.\d+\.\d+\.\d+)/);
    if (m && !m[1].startsWith("127.")) return m[1];
  }
  const hostname = shell("hostname -I 2>/dev/null");
  if (hostname) {
    // Only pick plain IPv4 tokens (no colon), skip loopback
    const first = hostname.split(/\s+/).find(ip => ip && /^\d+\.\d+\.\d+\.\d+$/.test(ip) && !ip.startsWith("127."));
    if (first) return first;
  }
  // Cross-platform fallback (Windows, macOS)
  return nics().v4 || null;
}

function glip6() {
  // Try ip -6 route get to a global IPv6 address to find outbound source
  const route = shell("ip -6 route get 2001:4860:4860::8888 2>/dev/null");
  if (route) {
    const m = route.match(/src\s+([0-9a-fA-F:]+)/);
    if (m && isGlobalIPv6(m[1])) return m[1];
  }
  // Try ip -6 addr show scope global
  const a6 = shell("ip -6 addr show scope global 2>/dev/null");
  if (a6) {
    const m = a6.match(/inet6\s+([0-9a-fA-F:]+)(?:\/\d+)?/g);
    if (m) {
      for (const entry of m) {
        const addr = entry.split(/\s+/)[1].split("/")[0];
        if (isGlobalIPv6(addr)) return addr;
      }
    }
  }
  // Fall back to hostname -I: pick first IPv6 global token
  const hostname = shell("hostname -I 2>/dev/null");
  if (hostname) {
    const first = hostname.split(/\s+/).find(t => isGlobalIPv6(t));
    if (first) return first;
  }
  // Cross-platform fallback (Windows, macOS)
  return nics().v6 || null;
}

// ── main export ────────────────────────────────────────────────────────────

/**
 * disc(opt) → Promise<{ domain, ip, ip6, port, source }>
 *
 * opt.port   — configured port
 * opt.domain — override; skip file + network discovery
 * opt.noSave — don't persist discovered domain
 */
export async function disc(opt = {}) {
  const port = opt.port || rft(PORTF) || 8420;
  let dom    = opt.domain || null;
  let src    = "opt";

  if (!dom) {
    dom = rft(DOMF);
    if (dom) src = "config";
  }

  // Run IPv4 and IPv6 local discovery in parallel
  let ip  = glip();
  let ip6 = glip6();

  let pip = null, pip6 = null;
  if (!dom) {
    [pip, pip6] = await Promise.all([stun(), stun6()]);
    if (pip)  { ip  = pip;  src = "stun"; }
    if (pip6) { ip6 = pip6; }
  }

  if (!dom && !pip) {
    const fh =
      await hget("https://api.ipify.org",   3000) ||
      await hget("https://ifconfig.me/ip",  3000) ||
      await hget("http://api.ipify.org",    3000);
    if (fh && /^\d+\.\d+\.\d+\.\d+$/.test(fh)) { ip = fh; src = "http"; }
  }

  if (!dom && !ip6) {
    const fh6 = await hget6(3000);
    if (fh6) { ip6 = fh6; }
  }

  if (!dom && ip) {
    dom = ip;
    src = src === "opt" ? "ip" : src;
  }

  if (dom && !opt.noSave && !opt.domain && !rft(DOMF)) {
    try { xdg.ensure(xdg.config()); fs.writeFileSync(DOMF, dom + "\n"); } catch {}
  }

  return { domain: dom, ip, ip6: ip6 || null, port: Number(port), source: src };
}

// ── hardware identity ─────────────────────────────────────────────────────
/**
 * hwid() → stable entropy string from local hardware (sync, offline).
 *
 * Sources (concatenated with "|"):
 *   1. /etc/machine-id         — 128-bit UUID set at OS install, never changes
 *   2. first non-loopback MAC  — sorted for determinism across renames
 *   3. hostname                — weak fallback; combined with above is sufficient
 *
 * Returns null if nothing is available (e.g., containers without machine-id).
 */
export function hwid() {
  const mid = rft("/etc/machine-id") || rft("/var/lib/dbus/machine-id") || "";
  let mac = "";
  try {
    const ifaces = fs.readdirSync("/sys/class/net").filter(i => i !== "lo");
    mac = ifaces
      .map(i => { try { return rft(`/sys/class/net/${i}/address`); } catch { return ""; } })
      .filter(m => m && m !== "00:00:00:00:00:00")
      .sort()[0] || "";
  } catch {}
  const hostname = shell("hostname") || "";
  const raw = [mid, mac, hostname].filter(Boolean).join("|");
  return raw || null;
}

export { DOMF, PORTF };
