#!/usr/bin/env node

import cluster from "cluster";
import crypto from "crypto";
import dgram from "dgram";
import fs from "fs";
import path from "path";
import http from "http";
import https from "https";
import tls from "tls";
import { fileURLToPath } from "node:url";
import { dirname } from "node:path";
import ZEN from "../index.js";
import {
  bootstrapDisabled,
  resolveBootstrapPeers,
} from "../src/bootstrap.js";
import * as xdg from "../lib/xdg.js";
import { disc, hwid, DOMF, PORTF } from "../lib/discover.js";
import { scanbg, mkpat, scanip6 } from "../lib/scan.js";
import { getOrCreateIdentity } from "../lib/identity.js";
import { buildStatus, signStatus } from "../lib/status.js";
import { attach as attachMcp } from "../lib/mcp/server.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const main = !!process.argv[1] && __filename === process.argv[1];

const nver = process.versions.node.split(".").map(Number);
if (nver[0] < 14) {
  console.error(
    "ERROR: Node.js 14+ required. Current version:",
    process.version,
  );
  process.exit(1);
}

process.on("SIGTERM", () => {
  console.log("Received SIGTERM, shutting down gracefully");
  process.exit(0);
});

process.on("SIGINT", () => {
  console.log("Received SIGINT, shutting down gracefully");
  process.exit(0);
});

process.on("uncaughtException", (err) => {
  console.error("Uncaught Exception:", err);
  process.exit(1);
});

process.on("unhandledRejection", (reason, promise) => {
  console.error("Unhandled Rejection at:", promise, "reason:", reason);
  process.exit(1);
});

function vport(port) {
  const portNum = parseInt(port, 10);
  if (isNaN(portNum) || portNum < 1 || portNum > 65535) {
    throw new Error("Invalid port: " + port + ". Must be between 1-65535");
  }
  return portNum;
}

function vpath(filePath) {
  if (filePath.includes("../") || filePath.includes("..\\")) {
    throw new Error("Path traversal detected: " + filePath);
  }
  if (!path.isAbsolute(filePath)) {
    throw new Error("Absolute path required for security files: " + filePath);
  }
  return filePath;
}

function vprs(peers) {
  if (!peers) return [];
  return peers.split(",").map((peer) => {
    const trimmed = peer.trim();
    // Accept http(s) and ws(s) schemes, including bracket-IPv6 format: ws://[::1]:8420/zen
    if (!/^(https?|wss?):\/\/.+/i.test(trimmed)) {
      throw new Error("Invalid peer URL: " + trimmed);
    }
    return trimmed;
  });
}

let zen;

if (main && cluster.isPrimary) {
  console.log("Master process " + process.pid + " starting...");
  cluster.setupPrimary({ exec: __filename });

  cluster.on("exit", (worker, code, signal) => {
    console.log(
      "Worker " +
        worker.process.pid +
        " died with code " +
        code +
        " and signal " +
        signal,
    );
    if (code === 1) {
      console.error("Worker died due to configuration error, not restarting");
      process.exit(1);
    }
    if (code !== 0 && !worker.exitedAfterDisconnect) {
      console.log("Restarting worker...");
      cluster.fork();
    }
  });

  const worker = cluster.fork();
  process.on("SIGTERM", () => {
    console.log("Master received SIGTERM, shutting down wkr...");
    worker.disconnect();
    setTimeout(() => {
      worker.kill();
    }, 5000);
  });
} else if (main) {
  (async () => {
    const env = process.env;
    let port;
    let hport;
    let peers;
    let domain;

  try {
    port = vport(env.PORT || process.argv[2] || 8420);
    hport = env.HTTPS_PORT ? vport(env.HTTPS_PORT) : null;
    peers = resolveBootstrapPeers(vprs(env.PEERS), {
      includeBootstrap: !bootstrapDisabled(env),
    });
    // Domain: env var > XDG config file
    domain = env.DOMAIN || null;
    if (!domain) {
      try { domain = fs.readFileSync(DOMF, "utf8").trim() || null; } catch {}
    }
    // Filter own URLs from bootstrap peers to prevent self-connection loops.
    if (domain) {
      const selfUrls = new Set();
      for (const scheme of ["https", "http", "wss", "ws"]) {
        selfUrls.add(`${scheme}://${domain}:${port}/zen`);
        selfUrls.add(`${scheme}://${domain}/zen`);
      }
      peers = peers.filter(p => !selfUrls.has(p));
    }
  } catch (err) {
    console.error("Configuration Error:", err.message);
    process.exit(1);
  }

  const opt = {
    port,
    peers,
    domain,  // used by axe.js stay-restore to filter self-connections
  };

  const cfgd = xdg.config();
  const dkey = path.join(cfgd, "key.pem");
  const dcrt = path.join(cfgd, "cert.pem");

  if (env.HTTPS_KEY) {
    try {
      env.HTTPS_KEY = vpath(env.HTTPS_KEY);
    } catch (err) {
      console.error("HTTPS_KEY validation failed:", err.message);
      process.exit(1);
    }
  }

  if (env.HTTPS_CERT) {
    try {
      env.HTTPS_CERT = vpath(env.HTTPS_CERT);
    } catch (err) {
      console.error("HTTPS_CERT validation failed:", err.message);
      process.exit(1);
    }
  }

  if (env.HTTPS_KEY2) {
    try {
      env.HTTPS_KEY2 = vpath(env.HTTPS_KEY2);
    } catch (err) {
      console.error("HTTPS_KEY2 validation failed:", err.message);
      process.exit(1);
    }
  }

  if (env.HTTPS_CERT2) {
    try {
      env.HTTPS_CERT2 = vpath(env.HTTPS_CERT2);
    } catch (err) {
      console.error("HTTPS_CERT2 validation failed:", err.message);
      process.exit(1);
    }
  }

  if (fs.existsSync(dcrt)) {
    env.HTTPS_KEY = env.HTTPS_KEY || dkey;
    env.HTTPS_CERT = env.HTTPS_CERT || dcrt;
  }

  // Auto-load IPv6 cert if present alongside the primary cert (no env vars needed).
  // ssl.sh writes the IPv6 cert here automatically when --auto-ip6 fires.
  const dkey2 = path.join(cfgd, "ip6-key.pem");
  const dcrt2 = path.join(cfgd, "ip6-cert.pem");
  if (fs.existsSync(dcrt2)) {
    env.HTTPS_KEY2 = env.HTTPS_KEY2 || dkey2;
    env.HTTPS_CERT2 = env.HTTPS_CERT2 || dcrt2;
  }

  // Latch domain from first incoming request Host header if still unknown
  let dlat = !!domain;
  function ldom(req) {
    if (dlat) return;
    const host = (req.headers.host || "").split(":")[0];
    if (host && host !== "localhost" && !/^\d+\.\d+\.\d+\.\d+$/.test(host)) {
      domain = host;
      dlat = true;
      try { xdg.ensure(cfgd); fs.writeFileSync(DOMF, domain + "\n"); } catch {}
      console.log("Domain latched from request:", domain);
      sscan();
      schd();
    }
  }

  // ── /status signed endpoint (CORS-enabled, consumed by AXE and agents) ────
  // Returns a pre-computed compact ZEN signed string (cached, refreshed on a
  // schedule). Signing (ECDSA) runs in the background — requests are served
  // synchronously from cachedStatus, no per-request crypto work.
  // Client: ZEN.recover(str) → pub, ZEN.verify(str, pub) → JSON payload.
  // Peers: only fully-qualified relay URLs ending in '/zen' (RTT-sorted).
  function rttOf(url) {
    const at = zen && zen._graph && zen._graph._;
    if (!at || !at.axe) return Infinity;
    for (const pid in at.axe.up) {
      const p = at.axe.up[pid];
      if (p && p.url === url && p.rtt > 0) return p.rtt;
    }
    return Infinity;
  }

  // Dedup peer list: domain > ip6 > ip4.
  // If any domain URL exists at port P, drop all [ipv6] and raw ipv4 URLs at
  // the same port (they are the same machines, just different address forms).
  function dedupeByDomain(urls) {
    const domainPorts = new Set();
    urls.forEach(u => {
      try {
        const h = new URL(u).hostname;
        // hostname is a domain if it's not a bracketed IPv6 and not a raw IPv4
        if (!/^\[/.test(h) && !/^\d+\.\d+\.\d+\.\d+$/.test(h)) {
          domainPorts.add(new URL(u).port);
        }
      } catch {}
    });
    if (!domainPorts.size) return urls;
    return urls.filter(u => {
      try {
        const parsed = new URL(u);
        // Drop bracketed IPv6 and raw IPv4 URLs on ports covered by domain URLs
        if (domainPorts.has(parsed.port)) {
          if (/^\[/.test(parsed.hostname)) return false; // [ipv6]
          if (/^\d+\.\d+\.\d+\.\d+$/.test(parsed.hostname)) return false; // ipv4
        }
      } catch {}
      return true;
    });
  }

  let cachedStatus = null;
  async function refreshStatus() {
    if (!identity) return;
    try {
      const payload = buildStatus({
        pub: identity.pair.pub,
        domain,
        ip4: discResult ? (discResult.ip || null) : null,
        ip6: discResult ? (discResult.ip6 || null) : null,
        port,
        peers: dedupeByDomain([...kprs.keys()].filter(u => /^wss:\/\//.test(u) && u.endsWith("/zen"))).sort((a, b) => rttOf(a) - rttOf(b)),
        mcp: false,
      });
      cachedStatus = await signStatus(payload, identity.pair);
    } catch {}
    kprsEvict(); // prune stale peers every refresh cycle
  }

  // Debounce rapid refreshStatus calls (e.g. 100 MCP nodes joining at once)
  let _rstTimer = null;
  function scheduleRefreshStatus() {
    clearTimeout(_rstTimer);
    _rstTimer = setTimeout(refreshStatus, 500);
    if (_rstTimer.unref) _rstTimer.unref();
  }

  let srv;
  function hndl(req, res) {
    ldom(req);
    if (req.method === "GET" && (req.url === "/status" || req.url === "/status/")) {
      res.writeHead(200, { "Content-Type": "text/plain; charset=utf-8", "Access-Control-Allow-Origin": "*" });
      res.end(cachedStatus || "");
      return;
    }
    srv(req, res);
  }

  // ── peer discovery ────────────────────────────────────────────────────────
  // kprs: Map<url, { seen: number, lastOk: number }>
  // - seen:   last time this URL was advertised to us via PEX/scan
  // - lastOk: last time a WebSocket connection to this URL succeeded
  // Entries with no confirmed connection for KPRS_TTL are evicted.
  // BOOT/configured peers and self URLs are protected from eviction.
  const KPRS_TTL = 30 * 60 * 1000;          // 30 min TTL for unconfirmed peers
  const kprsProtect = new Set(peers);        // BOOT + configured — never evict
  const kprs = new Map();
  // Maps normalised BOOT URL → remote pub key (learned on first successful connection).
  // Used by the watchdog to detect inbound-only connections (AXE can keep the inbound
  // while dropping the outbound, so opt.peers[url].wire may be null even though we ARE
  // connected to the BOOT peer via an inbound peer object keyed by random ID).
  const bootPubMap = {};
  // bootPidMap: BOOT url → peer.pid (AXE PID, used as axe.up key).
  // Fallback when peer.pub is not set (empty for relays without SEA user auth).
  const bootPidMap = {};
  peers.forEach(u => kprs.set(u, { seen: Date.now(), lastOk: Date.now() }));

  function kprsTouch(url, ok = false) {
    const entry = kprs.get(url);
    const now = Date.now();
    if (entry) {
      entry.seen = now;
      if (ok) entry.lastOk = now;
    } else {
      kprs.set(url, { seen: now, lastOk: ok ? now : 0 });
    }
  }

  function kprsEvict() {
    const now = Date.now();
    for (const [url, entry] of kprs) {
      if (kprsProtect.has(url)) continue;     // never evict BOOT peers
      if (url === surl || url === surl6) continue; // never evict self
      if (now - entry.seen > KPRS_TTL && now - entry.lastOk > KPRS_TTL) {
        kprs.delete(url);
      }
    }
  }

  const spat = new Set();       // patterns scanned this cycle
  let stmr     = null;
  let pmsh       = null;            // set after AXE attaches
  let fic  = false;           // tracks if adp fired this scan cycle
  const SIV     = 10 * 60 * 1000;   // 10 min base interval
  const MSIV = 2 * 60 * 60 * 1000; // 2 hr cap
  const MUPS     = 10;        // max outbound peer connections from scan
  let   siv      = SIV;

  function pkey(host) {
    const p = mkpat((host || "").split(":")[0]);
    return p ? (p.prefix + "*" + p.tail + p.suffix) : host;
  }

  function adp(url) {
    // Normalize protocol to prevent duplicate connections to the same host:
    // bootstrap uses https://, PEX gossip sends wss:// — they map to the same endpoint.
    var altUrl = url.startsWith('wss://') ? url.replace('wss://', 'https://')
               : url.startsWith('ws://')  ? url.replace('ws://', 'http://')
               : url.startsWith('https://') ? url.replace('https://', 'wss://')
               : url.startsWith('http://')  ? url.replace('http://', 'ws://')
               : null;
    if (kprs.has(url) || (altUrl && kprs.has(altUrl))) return;
    // Skip self-URLs (own hostname or IPv4/IPv6 address)
    if (url === surl || url === surl6 || (altUrl && (altUrl === surl || altUrl === surl6))) return;
    // Skip tombstoned peers (AXE-dropped; BOOT-WATCHDOG handles BOOT peers separately)
    const r = zen && zen._graph && zen._graph._;
    const tombs = r && r.opt && r.opt._tombUrls;
    const normUrl = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
    if (tombs && (tombs.has(url) || tombs.has(normUrl) || (altUrl && tombs.has(altUrl)))) return;
    kprsTouch(url);
    fic = true;
    scheduleRefreshStatus(); // debounced — safe even if 1000 nodes join rapidly
    console.log("Discovered peer:", url);
    // Connect only if under upstream limit (prevents full mesh / bandwidth waste)
    const ups = r && r.axe ? Object.keys(r.axe.up || {}).length : 0;
    if (pmsh && ups < MUPS) {
      try { pmsh.hi({ id: normUrl, url: normUrl, retry: 9 }); } catch {}
    } else if (!pmsh && r && r.opt) {
      // mesh not yet attached — queue in peer list for AXE to connect later
      if (!Array.isArray(r.opt.peers)) r.opt.peers = [];
      if (!r.opt.peers.includes(normUrl)) r.opt.peers.push(normUrl);
    }
    // Broadcast immediately to all currently connected peers
    if (pmsh) {
      try { pmsh.say({ dam: "pex", peers: [url] }, r && r.opt && r.opt.peers); } catch {}
    }
    // Expand scan to this peer's domain pat
    try { scnd(new URL(url).hostname); } catch {}
  }

  function scnd(host) {
    if (!host) return;
    const key = pkey(host);
    if (spat.has(key)) return;
    spat.add(key);
    console.log("Scanning pattern:", key);
    scanbg(host, { port, onFound: adp });
  }

  function sscan() {
    if (domain) scnd(domain);
  }

  function schd() {
    clearTimeout(stmr);
    stmr = setTimeout(() => {
      fic = false;
      spat.clear(); // new cycle — re-probe all known patterns
      sscan();
      // After 2 min (scan finishes well within that), check if we found anything
      const check = setTimeout(() => {
        if (!fic) {
          siv = Math.min(siv * 2, MSIV);
          console.log(`Scan: no new peers — next scan in ${Math.round(siv / 60000)}m`);
        } else {
          siv = SIV; // reset backoff on any discovery
        }
        schd();
      }, 2 * 60 * 1000);
      if (check.unref) check.unref();
    }, siv);
    stmr.unref();
  }

  if (
    env.HTTPS_KEY &&
    fs.existsSync(env.HTTPS_KEY) &&
    fs.existsSync(env.HTTPS_CERT)
  ) {
    const ahp = hport || opt.port || 443;
    const hprt = env.HTTP_PORT ? vport(env.HTTP_PORT) : 80;

    console.log("SSL certificates found, enabling HTTPS...");

    let kd;
    let cd;
    try {
      kd = fs.readFileSync(env.HTTPS_KEY, "utf8");
      cd = fs.readFileSync(env.HTTPS_CERT, "utf8");

      if (!kd.includes("BEGIN") || !kd.includes("PRIVATE KEY")) {
        throw new Error("Invalid private key format");
      }
      if (!cd.includes("BEGIN CERTIFICATE")) {
        throw new Error("Invalid certificate format");
      }

      opt.key = kd;
      opt.cert = cd;
    } catch (err) {
      console.error("SSL Certificate Error:", err.message);
      process.exit(1);
    }

    // ── SNI: load second cert (e.g. raw IPv6) and route by servername ────────
    // Set HTTPS_KEY2 + HTTPS_CERT2 env vars pointing to the second key/cert.
    // SNICallback is invoked for every TLS handshake — picks the matching cert
    // by servername, falls back to the primary cert for unknown/empty names
    // (raw-IP clients send no SNI extension, so fallback covers them only when
    // the primary cert is also valid for that IP, which is not the case here).
    // The correct long-term approach is a dedicated subdomain for the IPv6 addr.
    if (
      env.HTTPS_KEY2 &&
      env.HTTPS_CERT2 &&
      fs.existsSync(env.HTTPS_KEY2) &&
      fs.existsSync(env.HTTPS_CERT2)
    ) {
      let kd2, cd2;
      try {
        kd2 = fs.readFileSync(env.HTTPS_KEY2, "utf8");
        cd2 = fs.readFileSync(env.HTTPS_CERT2, "utf8");
        if (!kd2.includes("BEGIN") || !kd2.includes("PRIVATE KEY")) throw new Error("Invalid private key format (KEY2)");
        if (!cd2.includes("BEGIN CERTIFICATE")) throw new Error("Invalid certificate format (CERT2)");
      } catch (err) {
        console.error("SSL Certificate2 Error:", err.message);
        process.exit(1);
      }

      const ctx1 = tls.createSecureContext({ key: kd, cert: cd });
      const ctx2 = tls.createSecureContext({ key: kd2, cert: cd2 });

      // Raw-IP connections send no SNI extension (servername is empty/null).
      // Named-domain connections send their hostname as servername.
      // Route accordingly: empty → IP cert (ctx2), named → domain cert (ctx1).
      opt.SNICallback = (servername, cb) => {
        cb(null, servername ? ctx1 : ctx2);
      };

      console.log("SNI enabled: primary cert (domain) + secondary cert (IP/KEY2/CERT2)");
    }

    srv = ZEN.serve(__dirname);
    opt.server = https.createServer(opt, hndl);

    if (hport == 443 || env.HTTP_REDIRECT === "true") {
      try {
        http
          .createServer((req, res) => {
            ldom(req);
            const redirectUrl =
              "https://" +
              req.headers.host.replace(":" + hprt, ":" + hport) +
              req.url;
            res.writeHead(301, { Location: redirectUrl });
            res.end();
          })
          .listen(hprt);
        console.log(
          "HTTP redirect server started on port " +
            hprt +
            " -> HTTPS " +
            hport,
        );
      } catch (e) {
        console.log(
          "Warning: Could not start HTTP redirect server on port " +
            hprt +
            ": " +
            e.message,
        );
      }
    }

    opt.port = ahp;
    console.log("HTTPS server will start on port " + ahp);
  } else {
    srv = ZEN.serve(__dirname);
    opt.server = http.createServer(hndl);
    console.log("HTTP server will start on port " + opt.port);
  }

  // ── deterministic peer ID from hardware entropy ───────────────────────────
  let ppid = null;
  let identity = null;
  try {
    identity = await getOrCreateIdentity();
    if (identity) {
      ppid = identity.pair.pub;
      console.log("Peer ID (stable):", ppid.slice(0, 9) + "...");
      console.log("Hardware identity loaded from:", identity.hwid.slice(0, 30) + "...");
    }
  } catch (e) {
    console.log("Warning: identity derivation failed:", e.message);
  }

  // On Linux '::' is dual-stack (IPV6_V6ONLY=0 by default).
  // On Windows IPV6_V6ONLY=1 by default, so '::' only serves IPv6 — fall back to '0.0.0.0'.
  const bindHost = process.platform === "win32" ? "0.0.0.0" : "::"
  // Use the hardware identity pub directly for XOR routing.
  // Since MCP is now embedded in the relay (same process), there is no separate MCP peer
  // to cause a self-connection, so the old /relay-routing derivation is no longer needed.
  const relayPub = identity && identity.pair && identity.pair.pub ? identity.pair.pub : null;
  zen = new ZEN({
    web: opt.server.listen(opt.port, bindHost),
    peers: opt.peers,
    ...(domain && { domain }),
    ...(ppid && { pid: ppid }),
    ...(relayPub && { pub: relayPub }),
    // Storage resilience — configurable via env vars set at install time
    ...(process.env.FMB   !== undefined && { fmb:   parseInt(process.env.FMB) }),
    ...(process.env.FRAT  !== undefined && { frat:  parseFloat(process.env.FRAT) }),
    ...(process.env.EVICT !== undefined && { evict: process.env.EVICT !== '0' }),
  });
  console.log("Relay peer started on port " + opt.port + " with /zen (" + bindHost + ")");

  // Embed MCP server on this ZEN instance — exposes IPC socket for local IDE/agent connections.
  // This eliminates the need for a second ZEN peer process when MCP is used on the same machine.
  attachMcp(zen, { hwIdentity: identity, ipc: true });
  // ── PEX: peer exchange via direct DAM message (not public graph) ──────────
  // mesh.hear["pex"] + root.on("hi") — only shared with already-connected peers
  const surl = domain
    ? ((opt.key ? "wss" : "ws") + "://" + domain + ":" + port + "/zen")
    : null;

  if (surl) kprs.set(surl, { seen: Date.now(), lastOk: Date.now() }); // include self

  // Pre-populate kprs with all configured boot peers so adp() skips them
  // and avoids creating duplicate outbound connections on top of what the
  // ZEN constructor already establishes.
  for (const pu of peers) {
    const hn = pu.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
    const hw = pu.replace(/^https?:\/\//, 'wss://').replace(/^http:\/\//, 'ws://');
    kprs.set(hn, { seen: Date.now(), lastOk: 0 });
    kprs.set(hw, { seen: Date.now(), lastOk: 0 });
  }

  // ── IP discovery (IPv4 + IPv6) ────────────────────────────────────────────
  // Single source of truth for IP info: cached in discResult, refreshed every
  // 10 min (relays often have dynamic IPs). Used by /status and kprs.
  let surl6 = null;
  let discResult = null;

  function refreshDisc() {
    disc({ noSave: true }).then((di) => {
      discResult = di;
      if (di.ip6) {
        const scheme = opt.key ? "wss" : "ws";
        const newSurl6 = scheme + "://[" + di.ip6 + "]:" + port + "/zen";
        if (newSurl6 !== surl6) {
          if (surl6) kprs.delete(surl6); // remove stale IPv6 entry
          surl6 = newSurl6;
          // Only advertise IPv6 URL if no domain URL — avoids duplicate entries
          if (!surl) kprs.set(surl6, { seen: Date.now(), lastOk: Date.now() });
          console.log("IPv6 self-URL:", surl6);
        }
      }
      refreshStatus(); // rebuild status after IPs are updated
    }).catch((err) => {
      console.log("IP discovery failed:", err && err.message || err);
    });
  }

  refreshStatus();                                    // initial cache (no IP yet)
  refreshDisc();                                       // async: updates IPs then re-caches
  setInterval(refreshDisc, 10 * 60 * 1000);            // refresh IPs every 10 min
  setInterval(refreshStatus, 30 * 1000);               // keep timestamp + peers fresh

  const root = zen._graph._;

  // ── In-memory graph GC ────────────────────────────────────────────────────
  // root.graph is an unbounded in-memory cache of all graph nodes ever seen.
  // All data is persisted to disk (RAD), so evicting a soul just causes a
  // cache miss → storage read on next access.  We evict when heap is high,
  // skipping souls that have active on() listeners (root.next[soul]).
  const GRAPH_GC_HEAP_MB   = parseInt(process.env.GRAPH_GC_MB   || '400'); // evict above this
  const GRAPH_GC_INTERVAL  = parseInt(process.env.GRAPH_GC_SEC  || '60') * 1000;
  const GRAPH_GC_KEEP_SECS = parseInt(process.env.GRAPH_GC_KEEP || '120'); // keep recently-written souls
  const graphAt = new Map(); // soul → last-write timestamp (ms)

  // Hook into the existing root "put" stream to track write times.
  root.on('put', function graphGcTrack(msg) {
    const soul = (msg.put || '')['#'];
    if (soul) graphAt.set(soul, Date.now());
    this.to.next(msg);
  });

  setInterval(() => {
    const mem = process.memoryUsage();
    const heapMB = mem.heapUsed / 1048576;
    if (heapMB < GRAPH_GC_HEAP_MB) return;
    const graph = root.graph;
    const next  = root.next || {};
    const cutoff = Date.now() - GRAPH_GC_KEEP_SECS * 1000;
    let evicted = 0;
    for (const soul of Object.keys(graph)) {
      if (next[soul]) continue;                  // has active on() listener — skip
      if ((graphAt.get(soul) || 0) > cutoff) continue; // written recently — skip
      delete graph[soul];
      graphAt.delete(soul);
      evicted++;
    }
    const after = process.memoryUsage().heapUsed / 1048576;
    if (evicted) console.log(`[GC] Evicted ${evicted} souls (heap ${heapMB.toFixed(0)}→${after.toFixed(0)} MB)`);
  }, GRAPH_GC_INTERVAL).unref();

  // ── UDP unicast socket for inter-relay relay message fast path ────────────
  // VPS relay servers have public IPs — no NAT traversal needed.
  // Both sides advertise their UDP port in dam:"?" handshake (udp: <port>).
  // When forwarding relay messages between peers that support UDP, the relay
  // handler sends the JSON-serialised fwd object via UDP instead of WebSocket.
  // Falls back to WebSocket on any UDP error or if the peer has no UDP endpoint.
  const UDP_PORT = parseInt(process.env.UDP_PORT || '8421');
  // Random token for this relay session — exchanged over TLS WS in dam:"?" handshake.
  // Peers must prefix every UDP packet with this token. Prevents injection from unknown sources.
  const UDP_TOKEN = crypto.randomBytes(16).toString('hex'); // 32 hex chars
  const udpPeerMap = {}; // remote IP (v4 or v6) → peer object

  // Dual-stack UDP: one udp4 socket for IPv4 peers, one udp6 socket (ipv6Only) for
  // native IPv6 peers.  Both listen on UDP_PORT.  The OS delivers IPv4 packets to
  // udpSock4 and native IPv6 packets to udpSock6 without overlap.
  const udpSock4 = dgram.createSocket({ type: 'udp4', reuseAddr: true });
  let udpSock6 = null;
  try {
    udpSock6 = dgram.createSocket({ type: 'udp6', reuseAddr: true, ipv6Only: true });
  } catch(e) {
    console.log(`[UDP] IPv6 socket unavailable: ${e.message}`);
    try { udpSock6.close(); } catch(_) {}
    udpSock6 = null;
  }

  function onUdpMessage(buf, rinfo) {
    if (!pmsh) return;
    const raw = buf.toString('utf8');
    // Validate token: packet must start with UDP_TOKEN + '|'
    if (!raw.startsWith(UDP_TOKEN + '|')) return;
    // Normalize ::ffff:-mapped IPv4 addresses (dual-stack OS delivers them this way)
    let addr = rinfo.address;
    if (addr && addr.startsWith('::ffff:')) addr = addr.slice(7);
    const peer = udpPeerMap[addr];
    if (peer && peer.wire) { try { pmsh.hear(raw.slice(UDP_TOKEN.length + 1), peer); } catch(e) {} }
  }

  udpSock4.on('message', onUdpMessage);
  udpSock4.on('error', (e) => console.error('[UDP] v4 error:', e.message));
  udpSock4.bind(UDP_PORT, () => console.log(`[UDP] Listening on :${UDP_PORT} (v4)`));

  if (udpSock6) {
    udpSock6.on('message', onUdpMessage);
    udpSock6.on('error', (e) => { console.error('[UDP] v6 error:', e.message); udpSock6 = null; });
    udpSock6.bind(UDP_PORT, () => console.log(`[UDP] Listening on :${UDP_PORT} (v6)`));
  }

  // Resolve remote IP and register peer.udpSay once both sides have exchanged
  // UDP ports.  Called from the mesh.hear["?"] wrapper and the bye handler.
  // Declared in outer scope so the bye handler (outside setImmediate) can call it.
  function setupUdpForPeer(peer) {
    if (!peer || !peer.udpPort || !peer.udpToken || peer.udpSay) return;
    // Skip self-connections (AXE drops them, but guard early)
    if (peer.pid === root.opt.pid) return;

    let udpAddr = null;
    let sock = udpSock4; // default to IPv4

    if (peer.wire && peer.wire._socket) {
      const ra = peer.wire._socket.remoteAddress;
      if (ra && ra.startsWith('::ffff:')) {
        udpAddr = ra.slice(7); // IPv4-mapped → strip to plain IPv4, use udpSock4
      } else if (ra && ra.includes(':')) {
        // Native IPv6: use udpSock6 if available, otherwise fall through to URL/IPv4
        if (udpSock6) { udpAddr = ra; sock = udpSock6; }
      } else if (ra) {
        udpAddr = ra; // plain IPv4
      }
    }
    if (!udpAddr && peer.url) {
      // Hostname fallback: dgram resolves to IPv4 A record via udpSock4
      const m = peer.url.match(/(?:wss?:\/\/|https?:\/\/)([^:/[\]]+)/);
      if (m) { udpAddr = m[1]; sock = udpSock4; }
    }
    if (!udpAddr) return;

    // Don't overwrite an existing live entry for the same address.
    if (udpPeerMap[udpAddr] && udpPeerMap[udpAddr] !== peer) return;
    udpPeerMap[udpAddr] = peer;
    peer.udpAddr = udpAddr;
    const remoteToken = peer.udpToken; // token the remote relay expects at the start of our packets
    const sendSock = sock; // capture in closure
    peer.udpSay = (fwd) => {
      try {
        const raw = JSON.stringify(fwd);
        const buf = Buffer.from(remoteToken + '|' + raw, 'utf8');
        if (buf.length > 60000) return; // size guard: stay well within UDP MTU limits
        sendSock.send(buf, 0, buf.length, peer.udpPort, udpAddr,
          (err) => { if (err) console.error('[UDP] send err →', udpAddr, err.message); });
      } catch(e) {}
    };
    const family = sock === udpSock6 ? 'v6' : 'v4';
    console.log(`[UDP] Fast path for ${(peer.pub||'?').slice(0,8)} → ${udpAddr}:${peer.udpPort} (${family})`);
  }

  // Wait for AXE to attach opt.mesh (it runs synchronously but after ZEN init)
  setImmediate(() => {
    const mesh = root.opt && root.opt.mesh;
    if (!mesh) return;
    pmsh = mesh;
    root.opt.udpPort = UDP_PORT;   // mesh includes this in dam:"?" handshakes
    root.opt.udpToken = UDP_TOKEN; // mesh includes this in dam:"?" handshakes

    // Wrap mesh.hear["?"] to call setupUdpForPeer after the original handler.
    // At that point peer.udpPort is already stored by the original handler.
    // Also update bootPubMap/bootPidMap here: peer.pub/pid are set (by _origHearQ) for
    // outbound connections receiving the "@" reply — not yet set when on("hi") fired.
    const _origHearQ = mesh.hear["?"];
    mesh.hear["?"] = function(msg, peer) {
      _origHearQ.call(this, msg, peer);
      if (peer.url) {
        const nu = peer.url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
        const isBootPeer = kprsProtect.has(nu) || kprsProtect.has(peer.url);
        if (isBootPeer) {
          if (peer.pub) bootPubMap[nu] = peer.pub;
          if (peer.pid) bootPidMap[nu] = peer.pid;
        }
      }
      setupUdpForPeer(peer);
    };

    // UDP-aware message send: prefer UDP fast path for relay peers, fall back to WS.
    // Adds a random '#' so the dedup layer doesn't reject the packet.
    function saySmart(msg, peer) {
      if (peer && peer.udpSay) {
        try { peer.udpSay({ ...msg, '#': Math.random().toString(36).slice(2) }); return; } catch(e) {}
      }
      try { pmsh.say(msg, peer); } catch(e) {}
    }

    // Override mesh.ping: send ping via UDP fast path when available so peer.rtt
    // reflects the actual UDP RTT rather than the WebSocket RTT.
    const _origMeshPing = mesh.ping;
    mesh.ping = function(peer) {
      if (peer && peer.udpSay) {
        try {
          peer.udpSay({ dam: 'ping', t: +new Date(), '#': Math.random().toString(36).slice(2) });
          return;
        } catch(e) {}
      }
      _origMeshPing.call(this, peer);
    };

    // Override ping handler: reply with pong via UDP when fast path is available.
    const _origHearPing = mesh.hear['ping'];
    mesh.hear['ping'] = function(msg, peer) {
      if (peer && peer.udpSay) {
        try {
          peer.udpSay({ dam: 'pong', t: msg.t, '#': Math.random().toString(36).slice(2) });
          return;
        } catch(e) {}
      }
      _origHearPing.call(this, msg, peer);
    };

    // Connect to BOOT peers immediately — adp() returns early for pre-seeded kprs entries
    // so we must call pmsh.hi directly here to establish the initial outbound connections.
    // IMPORTANT: reuse the peer object already in opt.peers (created by ZEN constructor)
    // rather than creating a new object. This ensures opt.peers[url].wire is set
    // synchronously, preventing mesh.say from calling open() on the wireless ctor peer
    // (which would create a second outbound connection to the same URL).
    const _initOpt = zen._graph && zen._graph._ && zen._graph._.opt;
    peers.forEach(url => {
      const normUrl = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
      const existing = _initOpt && _initOpt.peers && _initOpt.peers[normUrl];
      if (existing && existing.wire) return; // already wired
      const peerObj = existing || { id: normUrl, url: normUrl, retry: 9 };
      if (existing) existing.retry = 9;
      try { pmsh.hi(peerObj); } catch {}
    });

    // Handle incoming peer lists from other nodes
    mesh.hear["pex"] = function (msg, _peer) {
      if (!Array.isArray(msg.peers)) return;
      msg.peers.forEach((url) => {
        if (typeof url === "string" && /^wss?:\/\//.test(url) &&
            url !== surl && url !== surl6) {
          adp(url);
        }
      });
    };

    // On new peer connection: mark URL as confirmed + send capped peer list
    // PEX_MAX: cap prevents flooding when 1000+ MCP nodes are in kprs.
    // Sort: wss:// first (browser-usable), then by RTT ascending.
    const PEX_MAX = 50;
    root.on("hi", function (peer) {
      this.to.next(peer);
      if (peer.url) kprsTouch(peer.url, true); // mark connection confirmed
      // Track remote pub/pid so the BOOT watchdog can detect inbound-only connections.
      if (peer.url && (peer.pub || peer.pid)) {
        const nu = peer.url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
        const isBootPeer = kprsProtect.has(nu) || kprsProtect.has(peer.url);
        if (isBootPeer) {
          if (peer.pub) bootPubMap[nu] = peer.pub;
          if (peer.pid) bootPidMap[nu] = peer.pid;
        }
      }
      const list = Array.from(kprs.keys())
        .filter(u => u !== surl && /^wss?:\/\//.test(u))
        .sort((a, b) => {
          const as = a.startsWith("wss://") ? 0 : 1;
          const bs = b.startsWith("wss://") ? 0 : 1;
          if (as !== bs) return as - bs;
          return rttOf(a) - rttOf(b);
        })
        .slice(0, PEX_MAX);
      setTimeout(() => {
        try {
          // browser pids: connected peers that have a pid but no URL (pure browser WS clients)
          const bpids = Object.values(root.opt.peers || {})
            .filter(p => p && p.pid && !p.url && p.pid !== peer.pid)
            .map(p => p.pid);
          const pexMsg = { dam: "pex", peers: list };
          if (bpids.length) pexMsg.bpids = bpids;
          saySmart(pexMsg, peer); // prefer UDP for relay peers
          // announce this peer's pid to all existing peers so they can WebRTC to it
          if (peer.pid && !peer.url) {
            Object.values(root.opt.peers || {}).forEach(p => {
              if (p && p.wire && p !== peer) {
                saySmart({ dam: "pex", peers: [], bpids: [peer.pid] }, p);
              }
            });
          }
        } catch {}
      }, 600);
      if (surl) {
        setTimeout(() => {
          try { mesh.say({ dam: "pex", peers: [surl] }, peer); } catch {}
        }, 700);
      }
      // Only advertise IPv6 URL when no domain URL — avoids duplicate peer entries
      if (surl6 && !surl) {
        setTimeout(() => {
          try { mesh.say({ dam: "pex", peers: [surl6] }, peer); } catch {}
        }, 750);
      }
    });
  });

  // Start scan immediately if domain already known, else wait for first request
  if (domain) { sscan(); schd(); }
  else console.log("Domain not configured — will scan after first request");

  // ── BOOT peer watchdog ────────────────────────────────────────────────────
  // AXE's PID-sort can drop one direction of a dual-connection (outbound or
  // inbound) between two relays and tombstone the URL, preventing reconnect.
  // If the "kept" connection later drops too (peer restart, network blip),
  // the relay is stranded with no path to the BOOT peer.
  //
  // This watchdog runs every 30s and reconnects any BOOT peer with no live
  // connection:
  //   - If we've learned the remote pub (bootPubMap, updated after "?" exchange),
  //     we scan ALL opt.peers AND axe.up by pub or pid — this catches inbound-only
  //     connections (AXE keeps inbound after conflict: axe.up[pid] = inbound).
  //   - If pub+pid are unknown yet, we check opt.peers[normUrl].wire (outbound only).
  //   - On reconnect, tombstone + backoff counters are cleared so the WebSocket
  //     open() guard doesn't block the attempt.
  setInterval(() => {
    const opt = root.opt;
    if (!pmsh || !opt) return;
    const at = zen && zen._graph && zen._graph._;
    const axeUp = (at && at.axe && at.axe.up) || {};
    for (const url of peers) {
      const norm = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
      const knownPub = bootPubMap[norm];
      const knownPid = bootPidMap[norm];
      if (knownPub || knownPid) {
        // Check opt.peers (outbound keyed by URL) and axe.up (includes inbounds
        // kept by AXE after conflict — stored without URL, keyed by pid).
        // Relays typically have empty pub (no SEA user), so knownPub may be "".
        // Use knownPid (AXE PID = axe.up key) as the primary fallback.
        const alive =
          (knownPub && (
            Object.values(opt.peers || {}).some(p => p && p.wire && p.pub === knownPub) ||
            Object.values(axeUp).some(p => p && p.wire && p.pub === knownPub)
          )) ||
          (knownPid && axeUp[knownPid] && axeUp[knownPid].wire);
        if (alive) continue;
      } else {
        // Pub+pid not yet learned — fall back to outbound URL check.
        const p = opt.peers[norm];
        if (p && p.wire) continue;
      }
      // No live connection — clear tombstone + backoff counters and reconnect.
      if (opt._tombUrls) {
        opt._tombUrls.delete(norm);
        opt._tombUrls.delete(norm.replace(/^https?:\/\//, 'wss://'));
        opt._tombUrls.delete(norm.replace(/^https?:\/\//, 'ws://'));
      }
      const p = opt.peers[norm];
      if (p) { delete p._noReconnect; delete p._hiGuess; delete p._axeGuess; }
      console.log('[BOOT-WATCHDOG] Reconnecting lost BOOT peer:', norm);
      try { pmsh.hi({ id: norm, url: norm, retry: 9 }); } catch {}
    }
  }, 30 * 1000).unref();

  // Reactive rescan: when a peer disconnects, rescan after a 30s debounce
  let btmr = null;
  root.on("bye", function (peer) {
    this.to.next.apply(this.to, arguments);
    if (peer && peer.udpAddr) {
      delete udpPeerMap[peer.udpAddr];
      peer.udpSay = null;
      peer.udpAddr = null;
      // Re-activate UDP fast path for any surviving peer at the same remote IP.
      // This handles the AXE conflict case: when outbound is AXE-dropped, the
      // surviving inbound (in axe.up, keyed by PID) should inherit the fast path.
      const axeUp = root.axe && root.axe.up;
      if (axeUp) {
        for (const pid in axeUp) { setupUdpForPeer(axeUp[pid]); }
      }
    }
    clearTimeout(btmr);
    btmr = setTimeout(() => {
      siv = SIV; // reset backoff — need to find replacements
      spat.clear();
      console.log("Peer disconnected — rescanning...");
      sscan();
      schd();
    }, 30000);
    btmr.unref();
  });
  })().catch(err => { console.error("Fatal:", err); process.exit(1); });
}

export default zen;
