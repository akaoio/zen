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
import { discoverPeers } from "../lib/bootstrap.js";
import * as xdg from "../lib/xdg.js";
import { hwid, DOMF, PORTF } from "../lib/discover.js";
import { scanbg, mkpat, scanip6 } from "../lib/scan.js";
import { getOrCreateIdentity } from "../lib/identity.js";
import { buildStatus, signStatus } from "../lib/status.js";
import { attach as attachMcp } from "../lib/mcp/server.js";
import PeerRegistry from "../lib/preg.js";
import { setupRelayPex } from "../lib/pex.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const PEERSF = path.join(xdg.config(), "peers.json");
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

function screen(peers) {
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
    let publicPort;
    let peers;
    let domain;

  try {
    port = vport(env.PORT || process.argv[2] || 8420);
    hport = env.HTTPS_PORT ? vport(env.HTTPS_PORT) : null;
    publicPort = env.PUBLIC_PORT ? vport(env.PUBLIC_PORT) : null;
    peers = resolveBootstrapPeers(screen(env.PEERS), {
      includeBootstrap: !bootstrapDisabled(env),
    });
    // Domain: env var > XDG config file
    domain = env.DOMAIN || null;
    if (!domain) {
      try { domain = fs.readFileSync(DOMF, "utf8").trim() || null; } catch {}
    }
    // Build self-URL set once (used both for filtering and DNS dedup).
    const selfUrls = new Set();
    if (domain) {
      for (const scheme of ["https", "http", "wss", "ws"]) {
        selfUrls.add(`${scheme}://${domain}:${port}/zen`);
        selfUrls.add(`${scheme}://${domain}/zen`);
      }
    }
    peers = peers.filter(p => !selfUrls.has(p));

    // If no explicit peers configured: discover via DNS TXT "peers.akao.io".
    // This lets any new relay join the network without hardcoded addresses.
    // Skipped when NO_BOOTSTRAP=1 (tests, isolated deployments).
    if (!bootstrapDisabled(env) && peers.length === 0) {
      const discovered = await discoverPeers();
      peers = discovered.filter(p => !selfUrls.has(p));
      if (peers.length) console.log(`Bootstrap: discovered ${peers.length} peer(s) via DNS`);
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

  const cfgdir = xdg.config();
  const dkey = path.join(cfgdir, "key.pem");
  const cert = path.join(cfgdir, "cert.pem");

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

  if (fs.existsSync(cert)) {
    env.HTTPS_KEY = env.HTTPS_KEY || dkey;
    env.HTTPS_CERT = env.HTTPS_CERT || cert;
  }

  // Auto-load IPv6 cert if present alongside the primary cert (no env vars needed).
  // ssl.sh writes the IPv6 cert here automatically when --auto-ip6 fires.
  const dkey2 = path.join(cfgdir, "ip6-key.pem");
  const dcrt2 = path.join(cfgdir, "ip6-cert.pem");
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
      try { xdg.ensure(cfgdir); fs.writeFileSync(DOMF, domain + "\n"); } catch {}
      console.log("Domain latched from request:", domain);
      sweep();
      cycle();
    }
  }

  // ── ZEN internals helpers ─────────────────────────────────────────────────
  // Single access point for axe.up (inbound relay connections keyed by PID).
  // Avoids scattered zen._graph._.axe.up accesses throughout server.js.
  function getAxeUp() {
    const at = zen && zen._graph && zen._graph._;
    return (at && at.axe && at.axe.up) || {};
  }

  // Returns true when a registry entry has at least one live wire.
  // Checks both opt.peers (outbound, keyed by URL) and axe.up (inbound, keyed
  // by PID) so that inbound-only connections (after AXE conflict resolution)
  // are correctly detected as alive — replacing 3 duplicate inline checks.
  function isPeerAlive(entry, opt) {
    const { pub: knownPub, pid: knownPid, url } = entry;
    if (knownPub || knownPid) {
      const axeUp = getAxeUp();
      const byPubInPeers = knownPub && Object.values(opt && opt.peers || {}).some(p => p && p.wire && p.pub === knownPub);
      const byPubInAxe = knownPub && Object.values(axeUp).some(p => p && p.wire && p.pub === knownPub);
      const byPidInAxe = !!(knownPid && axeUp[knownPid] && axeUp[knownPid].wire);
      const alive = byPubInPeers || byPubInAxe || byPidInAxe;
      if (!alive) {
        const shortUrl = url && url.replace('https://','').replace(':8420/zen','');
        console.log(`[BOOT-WATCHDOG] DEAD: ${shortUrl} pub=${knownPub&&knownPub.slice(0,8)} axeKeys=${Object.keys(axeUp).map(k=>k.slice(0,8)).join(',')}`);
      }
      return alive;
    }
    const p = opt && opt.peers && opt.peers[url];
    const alive2 = !!(p && p.wire);
    if (!alive2) {
      // Fallback: inbound peer may have announced this URL as their self-URL (_bootUrl)
      // before registry.confirm() stored the pub (e.g. during outbound retry exhaustion)
      const axeUp = getAxeUp();
      const byInboundUrl = Object.values(axeUp).some(ap => ap && ap.wire && ap._bootUrl === url);
      if (byInboundUrl) return true;
      const shortUrl = url && url.replace('https://','').replace(':8420/zen','');
      console.log(`[BOOT-WATCHDOG] DEAD(nopub): ${shortUrl}`);
    }
    return alive2;
  }

  // ── /status signed endpoint (CORS-enabled, consumed by AXE and agents) ────
  // Returns a pre-computed compact ZEN signed string (cached, refreshed on a
  // schedule). Signing (ECDSA) runs in the background — requests are served
  // synchronously from cachedStatus, no per-request crypto work.
  // Client: ZEN.recover(str) → pub, ZEN.verify(str, pub) → JSON payload.
  // Peers: only fully-qualified relay URLs ending in '/zen' (RTT-sorted).
  function rttOf(url) {
    const n = PeerRegistry.norm(url);
    for (const [, p] of Object.entries(getAxeUp())) {
      if (p && PeerRegistry.norm(p.url) === n && p.rtt > 0) return p.rtt;
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
        peers: dedupeByDomain(registry.pexList(50, rttOf).filter(u => u.endsWith("/zen"))).sort((a, b) => rttOf(a) - rttOf(b)),
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
    if (req.method === "GET" && req.url === "/.well-known/peers.json") {
      // Return known peers (BOOT + confirmed) as host:port for bootstrap discovery.
      // Any ZEN relay exposes this so new relays/browsers can seed their peer list.
      const entries = [
        ...registry.bootEntries(),
        ...registry.confirmedNonBoot(),
      ];
      const peers = entries
        .map(e => {
          try {
            const u = new URL(e.url);
            const port = u.port || (u.protocol === "https:" ? "443" : "8420");
            return u.hostname + ":" + port;
          } catch { return null; }
        })
        .filter((v, i, a) => v && a.indexOf(v) === i); // unique, non-null
      res.writeHead(200, {
        "Content-Type": "application/json; charset=utf-8",
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "max-age=60",
      });
      res.end(JSON.stringify({ peers }));
      return;
    }
    srv(req, res);
  }

  // ── peer discovery ────────────────────────────────────────────────────────
  // Single source of truth: PeerRegistry handles discovery, quality tracking,
  // identity (pub/pid), eviction, and persistence — replacing the former
  // ad-hoc combination of kprs/bootPubMap/bootPidMap/kprsProtect.
  const registry = new PeerRegistry().bindSave(PEERSF);
  // BOOT peers are protected below via registry.protect(); origin is resolved inside setupRelayPex.

  const probed = new Set();       // patterns scanned this cycle
  let tscan   = null;
  let route   = null;            // set after AXE attaches
  let found    = false;           // tracks if adopt fired this scan cycle
  const SIV  = 10 * 60 * 1000;   // 10 min base interval
  const MSIV = 2 * 60 * 60 * 1000; // 2 hr cap
  const MUPS = 10;              // max outbound peer connections from scan
  let   siv  = SIV;

  function pkey(host) {
    const p = mkpat((host || "").split(":")[0]);
    return p ? (p.prefix + "*" + p.tail + p.suffix) : host;
  }

  function kprsEvict() { registry.evict(); } // kept for call-site compat; delegate to registry

  function probe(host) {
    if (!host) return;
    const key = pkey(host);
    if (probed.has(key)) return;
    probed.add(key);
    console.log("Scanning pattern:", key);
    scanbg(host, { port, onFound: adopt });
  }

  function sweep() {
    if (domain) probe(domain);
  }

  function cycle() {
    clearTimeout(tscan);
    tscan = setTimeout(() => {
      found = false;
      probed.clear(); // new cycle — re-probe all known patterns
      sweep();
      // After 2 min (scan finishes well within that), check if we found anything
      const check = setTimeout(() => {
        if (!found) {
          siv = Math.min(siv * 2, MSIV);
          console.log(`Scan: no new peers — next scan in ${Math.round(siv / 60000)}m`);
        } else {
          siv = SIV; // reset backoff on any discovery
        }
        cycle();
      }, 2 * 60 * 1000);
      if (check.unref) check.unref();
    }, siv);
    tscan.unref();
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
  // setupRelayPex wires mesh.hear["pex"] + root.on("hi") self-announce inside
  // setImmediate (after AXE attaches). server.js adds the saySmart/bpids send
  // via sendPeers, and drives scan-cycle bookkeeping via onAdopt/onDisc.
  registry.protect(peers); // BOOT peers: never evict, watchdog reconnects them

  let discResult = null;

  const { adopt } = setupRelayPex(zen, {
    domain,
    port,
    publicPort,
    key:      opt.key,
    registry,
    pexMax:   50,
    rttOf,
    sendPeers: (list, peer, pubmap) => {
      const r = zen._graph._;
      const bpids = Object.values((r && r.opt && r.opt.peers) || {})
        .filter(p => p && p.pid && !p.url && p.pid !== peer.pid)
        .map(p => p.pid);
      const msg = { dam: "pex", peers: list };
      if (pubmap && Object.keys(pubmap).length) msg.pubmap = pubmap;
      if (bpids.length) msg.bpids = bpids;
      saySmart(msg, peer);
    },
    onDisc: (di) => {
      discResult = di;
      refreshStatus();
    },
    onAdopt: (url) => {
      found = true;
      scheduleRefreshStatus();
      try { probe(new URL(PeerRegistry.norm(url)).hostname); } catch {}
    },
  });

  // setupRelayPex registers origin/self URLs and wires disc() refresh.
  // registry.protect() was already called above for BOOT peers.

  refreshStatus();                        // initial cache (no IP yet)
  setInterval(refreshStatus, 30 * 1000); // keep timestamp + peers fresh

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
    if (!route) return;
    const raw = buf.toString('utf8');
    // Validate token: packet must start with UDP_TOKEN + '|'
    if (!raw.startsWith(UDP_TOKEN + '|')) return;
    // Normalize ::ffff:-mapped IPv4 addresses (dual-stack OS delivers them this way)
    let addr = rinfo.address;
    if (addr && addr.startsWith('::ffff:')) addr = addr.slice(7);
    const peer = udpPeerMap[addr];
    if (peer && peer.wire) { try { route.hear(raw.slice(UDP_TOKEN.length + 1), peer); } catch(e) {} }
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
    route = mesh;
    root.opt.udpPort = UDP_PORT;   // mesh includes this in dam:"?" handshakes
    root.opt.udpToken = UDP_TOKEN; // mesh includes this in dam:"?" handshakes

    // Wrap mesh.hear["?"] to call setupUdpForPeer after the original handler.
    // At that point peer.udpPort is already stored by the original handler.
    // Also update pub/pid here: peer.pub/pid are set (by _origHearQ) for
    // outbound connections receiving the "@" reply — not yet set when on("hi") fired.
    //
    // For INBOUND connections (!peer.url): match source IP to a BOOT entry URL
    // using pre-resolved DNS addresses (bootIpToUrl). When matched, set peer._bootUrl
    // and call registry.confirm so isPeerAlive() can find alive inbound-only BOOT peers.
    const bootIpToUrl = new Map(); // ip → normalized BOOT entry URL
    (async () => {
      const { promises: dns } = await import("dns");
      for (const entry of registry.bootEntries()) {
        try {
          const hostname = new URL(entry.url).hostname.replace(/^\[|\]$/g, "");
          // If already an IP literal, register directly
          if (/^[\d.]+$/.test(hostname) || /^[0-9a-f:]+$/i.test(hostname)) {
            bootIpToUrl.set(hostname, entry.url);
            continue;
          }
          const addrs = await dns.lookup(hostname, { all: true, family: 0 });
          for (const { address } of addrs) {
            bootIpToUrl.set(address, entry.url);
          }
        } catch {}
      }
    })();

    const _origHearQ = mesh.hear["?"];
    mesh.hear["?"] = function(msg, peer) {
      _origHearQ.call(this, msg, peer);
      if (peer.url) {
        // Outbound: confirm() stores pub/pid for WATCHDOG even if on("hi") already ran
        const nu = PeerRegistry.norm(peer.url);
        registry.confirm(nu, { pub: peer.pub || "", pid: peer.pid || "" });
      } else if (peer.pub && peer.pid) {
        // Inbound: match source IP to a BOOT URL so isPeerAlive() can see the inbound
        const sock = peer.wire && peer.wire._socket;
        const srcIp = sock && (sock.remoteAddress || "").replace(/^::ffff:/, "");
        const bootUrl = srcIp && (bootIpToUrl.get(srcIp) || bootIpToUrl.get(srcIp.replace(/^\[|\]$/g, "")));
        if (bootUrl) {
          peer._bootUrl = bootUrl;
          registry.confirm(bootUrl, { pub: peer.pub, pid: peer.pid });
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
      try { route.say(msg, peer); } catch(e) {}
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

    // Connect to BOOT peers immediately — adopt() returns early for pre-seeded registry entries
    // so we must call route.hi directly here to establish the initial outbound connections.
    // IMPORTANT: reuse the peer object already in opt.peers (created by ZEN constructor)
    // rather than creating a new object. This ensures opt.peers[url].wire is set
    // synchronously, preventing mesh.say from calling open() on the wireless ctor peer
    // (which would create a second outbound connection to the same URL).
    const _initOpt = zen._graph && zen._graph._ && zen._graph._.opt;
    peers.forEach(url => {
      const normUrl = PeerRegistry.norm(url);
      const existing = _initOpt && _initOpt.peers && _initOpt.peers[normUrl];
      if (existing && existing.wire) return; // already wired
      const peerObj = existing || { id: normUrl, url: normUrl, retry: 9 };
      peerObj._isBoot = true; // prevent AXE hiGuess/axeGuess tombstoning for BOOT peers
      if (existing) existing.retry = 9;
      try { route.hi(peerObj); } catch {}
      // Ensure the stored peer object is also marked (open() may reuse opt.peers entry)
      const stored = _initOpt && _initOpt.peers && _initOpt.peers[normUrl];
      if (stored) stored._isBoot = true;
    });

    // Confirm inbound BOOT peer connections when they announce their URL via dam:"opt".
    // Without this, isPeerAlive() can't find the inbound connection in axe.up when
    // AXE dedup drops our outbound (lower-PID peer keeps its outbound = our inbound).
    const _origHearOpt = mesh.hear["opt"];
    mesh.hear["opt"] = function(msg, peer) {
      _origHearOpt.call(this, msg, peer);
      if (!msg.ok && msg.opt && typeof msg.opt.peers === "string" && peer && !peer.url && peer.pid) {
        const ann = PeerRegistry.norm(msg.opt.peers);
        if (ann && !registry.isSelf(ann)) {
          // Confirm ALL inbound connections (not just BOOT) so every peer
          // that announces its URL becomes visible in confirmedNonBoot().
          registry.confirm(ann, { pub: peer.pub || "", pid: peer.pid });
        }
      }
    };

    // Load previously discovered peers from disk and reconnect them.
    // This runs after BOOT peers are wired so adopt() MUPS check is accurate.
    try {
      const count = registry.load(adopt);
      console.log(`Loaded ${count} persisted peers from disk`);
    } catch {}

    // Confirm inbound peers that self-announce their URL via dam:"pex".
    // adopt() returns early (no confirm) when the peer is already known, leaving lastOk=0.
    // Inbound connections have peer.url=undefined — gossip PEX from relay outbounds has peer.url set.
    //
    // IMPORTANT: Do NOT use peer.pub here for any URL in msg.peers.
    // The pex gossip message can contain OTHER peers' URLs (re-gossip), not just the sender's
    // own URL. Using peer.pub for a different URL corrupts registry._pidx and triggers
    // spurious pub-dedup eviction of the BOOT entry. IP-based matching in dam:"?" is the
    // reliable way to map inbound pub → BOOT URL. Here we only update lastOk timestamps.
    const _origHearPex = mesh.hear["pex"];
    mesh.hear["pex"] = function(msg, peer) {
      _origHearPex.call(this, msg, peer);
      if (peer && peer.pid && !peer.url && Array.isArray(msg.peers)) {
        for (const u of msg.peers) {
          if (typeof u !== "string") continue;
          const ann = PeerRegistry.norm(u);
          if (!ann || registry.isSelf(ann)) continue;
          // Touch only (update seen without setting lastOk) — no pub available from
          // re-gossip messages so confirm() would create a nopub confirmed entry that
          // triggers spurious DEAD(nopub) logs in the WATCHDOG every 30s.
          registry.touch(ann);
        }
      }
    };

    // Handle incoming peer lists from other nodes
    // (mesh.hear["pex"] is wired by setupRelayPex — see lib/pex.js)

    // On new peer connection: mark URL as confirmed + announce new browser PIDs for WebRTC.
    // Peer list + self-URL are sent by setupRelayPex via the sendPeers/root.on("hi") hooks.
    root.on("hi", function (peer) {
      this.to.next(peer);
      if (peer.url) {
        const nu = PeerRegistry.norm(peer.url);
        // confirm() records lastOk and stores pub/pid for WATCHDOG to use
        registry.confirm(nu, { pub: peer.pub || "", pid: peer.pid || "" });
      }
      // Announce new browser peer's PID to all existing peers so they can WebRTC to it
      if (peer.pid && !peer.url) {
        setTimeout(() => {
          try {
            Object.values(root.opt.peers || {}).forEach(p => {
              if (p && p.wire && p !== peer) {
                saySmart({ dam: "pex", peers: [], bpids: [peer.pid] }, p);
              }
            });
          } catch {}
        }, 600);
      }
    });
  });

  // Start scan immediately if domain already known, else wait for first request
  if (domain) { sweep(); cycle(); }
  else console.log("Domain not configured — will scan after first request");

  // ── BOOT peer watchdog ────────────────────────────────────────────────────
  // AXE's PID-sort can drop one direction of a dual-connection (outbound or
  // inbound) between two relays and tombstone the URL, preventing reconnect.
  // If the "kept" connection later drops too (peer restart, network blip),
  // the relay is stranded with no path to the BOOT peer.
  //
  // This watchdog runs every 30s and reconnects any BOOT peer with no live
  // connection. PeerRegistry.confirm() stores pub/pid for each URL — this
  // allows detecting inbound-only connections (AXE keeps inbound after conflict).
  setInterval(() => {
    const opt = root.opt;
    if (!route || !opt) return;
    const at = zen && zen._graph && zen._graph._;
    if (!at) return;

    for (const entry of registry.bootEntries()) {
      const norm = entry.url;
      if (isPeerAlive(entry, opt)) continue;
      // No live connection — clear tombstone + backoff counters and reconnect.
      // Clear BOTH tombstone systems: opt._tombUrls (PEX) and opt.tomb (WebSocket).
      // After cascade restarts, _axeGuess/_hiGuess can reach threshold and add the
      // BOOT peer URL to opt.tomb — BOOT-WATCHDOG must clear it or reconnects are
      // permanently blocked even though _noReconnect is false.
      if (opt._tombUrls) {
        opt._tombUrls.delete(norm);
        opt._tombUrls.delete(PeerRegistry.alt(norm));
      }
      if (opt.tomb) {
        opt.tomb.delete(norm);
        opt.tomb.delete(PeerRegistry.alt(norm));
      }
      // Reuse the existing peer object so _isBoot=true is set on the SAME object
      // that open() will store back into opt.peers[norm] via wire.onopen → mesh.hi.
      // Creating a new object causes a race: _isBoot is set on the old entry while
      // the new entry (stored async on wire.onopen) has _isBoot=false, so onclose
      // fires with !peer._isBoot=true and increments _axeGuess/_hiGuess → tombstone.
      const watchPeer = (opt.peers && opt.peers[norm]) || { id: norm, url: norm };
      if (!watchPeer.retry || watchPeer.retry < 9) watchPeer.retry = 9;
      watchPeer._isBoot = true;
      delete watchPeer._noReconnect;
      delete watchPeer._hiGuess;
      delete watchPeer._axeGuess;
      console.log('[BOOT-WATCHDOG] Reconnecting lost BOOT peer:', norm);
      try { route.hi(watchPeer); } catch {}
      // Also mark whatever is currently in opt.peers (in case open() replaced it)
      const rp = opt.peers && opt.peers[norm];
      if (rp) rp._isBoot = true;
    }

    // Also reconnect confirmed PEX-discovered peers that dropped, if under capacity.
    // Like BOOT peers: clear tombstones so AXE dedup doesn't block reconnect.
    // Less aggressive than BOOT: retry: 3 (not 9).
    // Always iterate (not just when under capacity) so alive peers get touch()
    // to refresh their seen timestamp — otherwise evict() removes them after TTL.
    const ups = Object.keys(getAxeUp()).length;
    for (const entry of registry.confirmedNonBoot()) {
      const url = entry.url;
      // Skip entries with no pub/pid — liveness can't be verified and reconnect
      // attempts would produce DEAD(nopub) log spam with no useful recovery path.
      if (!entry.pub && !entry.pid) continue;
      if (isPeerAlive(entry, opt)) {
        registry.touch(url); // keep seen fresh → prevent TTL eviction while connected
        continue;
      }
      if (ups >= MUPS) continue;               // at capacity — skip reconnect
      const tombs = opt._tombUrls;
      const p = opt.peers && opt.peers[url];
      // Clear tombstone so AXE allows reconnect (same as BOOT watchdog does)
      if (tombs) {
        tombs.delete(url);
        tombs.delete(PeerRegistry.alt(url));
      }
      if (opt.tomb) {
        opt.tomb.delete(url);
        opt.tomb.delete(PeerRegistry.alt(url));
      }
      if (p) { delete p._noReconnect; delete p._hiGuess; delete p._axeGuess; }
      try { route.hi({ id: url, url: url, retry: 3 }); } catch {}
    }
  }, 30 * 1000).unref();

  // Reactive rescan: when a peer disconnects, rescan after a 30s debounce
  let tbye = null;
  root.on("bye", function (peer) {
    this.to.next.apply(this.to, arguments);
    if (peer && peer.udpAddr) {
      delete udpPeerMap[peer.udpAddr];
      peer.udpSay = null;
      peer.udpAddr = null;
      // Re-activate UDP fast path for any surviving peer at the same remote IP.
      // This handles the AXE conflict case: when outbound is AXE-dropped, the
      // surviving inbound (in axe.up, keyed by PID) should inherit the fast path.
      const axeUp = getAxeUp();
      for (const pid in axeUp) { setupUdpForPeer(axeUp[pid]); }
    }
    clearTimeout(tbye);
    tbye = setTimeout(() => {
      siv = SIV; // reset backoff — need to find replacements
      probed.clear();
      console.log("Peer disconnected — rescanning...");
      sweep();
      cycle();
    }, 30000);
    tbye.unref();
  });

  // Save peer list on graceful shutdown so the next restart can reconnect quickly.
  const _gracefulExit = (sig) => {
    registry.save();
    process.exit(sig === 'SIGINT' ? 130 : 0);
  };
  process.on('SIGTERM', () => _gracefulExit('SIGTERM'));
  process.on('SIGINT',  () => _gracefulExit('SIGINT'));

  })().catch(err => { console.error("Fatal:", err); process.exit(1); });
}

export default zen;
