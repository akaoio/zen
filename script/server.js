#!/usr/bin/env node

import cluster from "cluster";
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
    kprsTouch(url);
    fic = true;
    scheduleRefreshStatus(); // debounced — safe even if 1000 nodes join rapidly
    console.log("Discovered peer:", url);
    const r = zen && zen._graph && zen._graph._;
    // Connect only if under upstream limit (prevents full mesh / bandwidth waste)
    const ups = r && r.axe ? Object.keys(r.axe.up || {}).length : 0;
    // Normalize wss:// → https:// so DNS-discovered peers share the same key as BOOT peers
    const normUrl = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
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
  zen = new ZEN({ web: opt.server.listen(opt.port, bindHost), peers: opt.peers, ...(ppid && { pid: ppid }), ...(relayPub && { pub: relayPub }) });
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

  // ── UDP unicast socket for inter-relay relay message fast path ────────────
  // VPS relay servers have public IPs — no NAT traversal needed.
  // Both sides advertise their UDP port in dam:"?" handshake (udp: <port>).
  // When forwarding relay messages between peers that support UDP, the relay
  // handler sends the JSON-serialised fwd object via UDP instead of WebSocket.
  // Falls back to WebSocket on any UDP error or if the peer has no UDP endpoint.
  const UDP_PORT = parseInt(process.env.UDP_PORT || '8421');
  const udpPeerMap = {}; // normalised remote IPv4 → peer object
  const udpSock = dgram.createSocket({ type: 'udp4', reuseAddr: true });
  udpSock.bind(UDP_PORT, () => console.log(`[UDP] Listening on :${UDP_PORT}`));
  udpSock.on('message', (buf, rinfo) => {
    if (!pmsh) return;
    const peer = udpPeerMap[rinfo.address];
    if (peer) { try { pmsh.hear(buf.toString('utf8'), peer); } catch(e) {} }
  });
  udpSock.on('error', (e) => console.error('[UDP] Socket error:', e.message));

  // Wait for AXE to attach opt.mesh (it runs synchronously but after ZEN init)
  setImmediate(() => {
    const mesh = root.opt && root.opt.mesh;
    if (!mesh) return;
    pmsh = mesh;
    root.opt.udpPort = UDP_PORT; // mesh includes this in dam:"?" handshakes

    // Resolve remote IP and register peer.udpSay once both sides have exchanged
    // UDP ports.  Called from the mesh.hear["?"] wrapper below.
    function setupUdpForPeer(peer) {
      if (!peer || !peer.udpPort || peer.udpSay) return;
      let udpAddr = null;
      if (peer.wire && peer.wire._socket) {
        udpAddr = peer.wire._socket.remoteAddress;
        if (udpAddr && udpAddr.startsWith('::ffff:')) udpAddr = udpAddr.slice(7);
      }
      if (!udpAddr && peer.url) {
        const m = peer.url.match(/(?:wss?:\/\/|https?:\/\/)([^:/[\]]+)/);
        if (m) udpAddr = m[1];
      }
      if (udpAddr) {
        udpPeerMap[udpAddr] = peer;
        peer.udpAddr = udpAddr;
        peer.udpSay = (fwd) => {
          try {
            const raw = JSON.stringify(fwd);
            const buf = Buffer.from(raw, 'utf8');
            udpSock.send(buf, 0, buf.length, peer.udpPort, udpAddr, () => {});
          } catch(e) {}
        };
        console.log(`[UDP] Fast path for ${(peer.pub||'?').slice(0,8)} → ${udpAddr}:${peer.udpPort}`);
      }
    }

    // Wrap mesh.hear["?"] to call setupUdpForPeer after the original handler.
    // At that point peer.udpPort is already stored by the original handler.
    const _origHearQ = mesh.hear["?"];
    mesh.hear["?"] = function(msg, peer) {
      _origHearQ.call(this, msg, peer);
      setupUdpForPeer(peer);
    };

    // Connect to BOOT peers immediately — adp() returns early for pre-seeded kprs entries
    // so we must call pmsh.hi directly here to establish the initial outbound connections.
    peers.forEach(url => {
      const normUrl = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
      try { pmsh.hi({ id: normUrl, url: normUrl, retry: 9 }); } catch {}
    });

    // Handle incoming peer lists from other nodes
    mesh.hear["pex"] = function (msg, _peer) {
      if (!Array.isArray(msg.peers)) return;
      msg.peers.forEach((url) => {
        if (typeof url === "string" && /^wss?:\/\//.test(url) && url !== surl) {
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
      // Track remote pub so the BOOT watchdog can detect inbound-only connections.
      if (peer.url && peer.pub) {
        const nu = peer.url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
        if (kprsProtect.has(nu) || kprsProtect.has(peer.url)) bootPubMap[nu] = peer.pub;
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
          mesh.say(pexMsg, peer);
          // announce this peer's pid to all existing peers so they can WebRTC to it
          if (peer.pid && !peer.url) {
            Object.values(root.opt.peers || {}).forEach(p => {
              if (p && p.wire && p !== peer) {
                try { mesh.say({ dam: "pex", peers: [], bpids: [peer.pid] }, p); } catch {}
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
  //   - If we've learned the remote pub (from a prior successful hi), we scan
  //     ALL opt.peers by pub — this catches inbound-only connections where the
  //     peer object is keyed by random ID (no URL).
  //   - If pub is unknown yet, we check opt.peers[normUrl].wire (outbound only).
  //   - On reconnect, tombstone + backoff counters are cleared so the WebSocket
  //     open() guard doesn't block the attempt.
  //
  // Worst case when AXE keeps an inbound: watchdog reconnects outbound every
  // 30s → AXE drops outbound again (keeps inbound) → 1 brief connect+bye per
  // 30s interval.  Acceptable overhead for a relay with a handful of BOOT peers.
  setInterval(() => {
    const opt = root.opt;
    if (!pmsh || !opt) return;
    for (const url of peers) {
      const norm = url.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');
      const knownPub = bootPubMap[norm];
      if (knownPub) {
        // Check any peer (outbound or inbound) connected with this pub.
        const alive = Object.values(opt.peers || {}).some(p => p && p.wire && p.pub === knownPub);
        if (alive) continue;
      } else {
        // Pub not yet learned — fall back to outbound URL check.
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
    if (peer && peer.udpAddr) { delete udpPeerMap[peer.udpAddr]; peer.udpSay = null; }
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
