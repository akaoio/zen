#!/usr/bin/env node

import cluster from "cluster";
import fs from "fs";
import path from "path";
import http from "http";
import https from "https";
import { fileURLToPath } from "node:url";
import { dirname } from "node:path";
import ZEN from "../index.js";
import * as xdg from "../lib/xdg.js";
import { disc, hwid, DOMF, PORTF } from "../lib/discover.js";
import { scanbg, mkpat, scanip6 } from "../lib/scan.js";
import { getOrCreateIdentity } from "../lib/identity.js";

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
    peers = vprs(env.PEERS);
    // Domain: env var > XDG config file
    domain = env.DOMAIN || null;
    if (!domain) {
      try { domain = fs.readFileSync(DOMF, "utf8").trim() || null; } catch {}
    }
  } catch (err) {
    console.error("Configuration Error:", err.message);
    process.exit(1);
  }

  const opt = {
    port,
    peers,
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

  if (fs.existsSync(dcrt)) {
    env.HTTPS_KEY = env.HTTPS_KEY || dkey;
    env.HTTPS_CERT = env.HTTPS_CERT || dcrt;
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

  // ── /peers JSON endpoint (CORS-enabled, consumed by browser AXE) ─────────
  let srv;
  function hndl(req, res) {
    ldom(req);
    if (req.method === "GET" && (req.url === "/peers" || req.url === "/peers/")) {
      res.writeHead(200, { "Content-Type": "application/json", "Access-Control-Allow-Origin": "*" });
      res.end(JSON.stringify([...kprs]));
      return;
    }
    srv(req, res);
  }

  // ── peer discovery ────────────────────────────────────────────────────────
  const kprs  = new Set(peers);  // all URLs ever seen
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
    if (kprs.has(url)) return;
    kprs.add(url);
    fic = true;
    console.log("Discovered peer:", url);
    const r = zen && zen._graph && zen._graph._;
    // Connect only if under upstream limit (prevents full mesh / bandwidth waste)
    const ups = r && r.axe ? Object.keys(r.axe.up || {}).length : 0;
    if (pmsh && ups < MUPS) {
      try { pmsh.hi({ id: url, url, retry: 9 }); } catch {}
    } else if (!pmsh && r && r.opt) {
      // mesh not yet attached — queue in peer list for AXE to connect later
      if (!Array.isArray(r.opt.peers)) r.opt.peers = [];
      if (!r.opt.peers.includes(url)) r.opt.peers.push(url);
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
  zen = new ZEN({ web: opt.server.listen(opt.port, bindHost), peers: opt.peers, ...(ppid && { pid: ppid }) });
  console.log("Relay peer started on port " + opt.port + " with /zen (" + bindHost + ")");

  // ── PEX: peer exchange via direct DAM message (not public graph) ──────────
  // mesh.hear["pex"] + root.on("hi") — only shared with already-connected peers
  const surl = domain
    ? ((opt.key ? "wss" : "ws") + "://" + domain + ":" + port + "/zen")
    : null;

  if (surl) kprs.add(surl); // include self in /peers responses

  // ── IPv6 self-URL discovery ───────────────────────────────────────────────
  // Advertise IPv6 self-URL only when running plain HTTP (no TLS).
  // Raw IPs cannot get a certificate from any public CA — advertising a raw
  // IPv6 address under wss:// causes browser cert errors for all clients that
  // try to connect to it.  When HTTPS is active the domain-based surl above
  // already covers IPv6 (dual-stack listener on "::" handles both families).
  let surl6 = null;
  if (!opt.key) {
    disc({ noSave: true }).then((di) => {
      if (di.ip6) {
        surl6 = "ws://[" + di.ip6 + "]:" + port + "/zen";
        kprs.add(surl6);
        console.log("IPv6 self-URL (plain ws, no TLS):", surl6);
      }
    }).catch((err) => {
      console.log("IPv6 discovery failed:", err && err.message || err);
    });
  }

  const root = zen._graph._;

  // Wait for AXE to attach opt.mesh (it runs synchronously but after ZEN init)
  setImmediate(() => {
    const mesh = root.opt && root.opt.mesh;
    if (!mesh) return;
    pmsh = mesh;

    // Handle incoming peer lists from other nodes
    mesh.hear["pex"] = function (msg, _peer) {
      if (!Array.isArray(msg.peers)) return;
      msg.peers.forEach((url) => {
        if (typeof url === "string" && /^wss?:\/\//.test(url) && url !== surl) {
          adp(url);
        }
      });
    };

    // On new peer connection: send our full known peer list + browser peer pids
    root.on("hi", function (peer) {
      this.to.next(peer);
      const list = Array.from(kprs).filter((u) => u !== surl);
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
      if (surl6) {
        setTimeout(() => {
          try { mesh.say({ dam: "pex", peers: [surl6] }, peer); } catch {}
        }, 750);
      }
    });
  });

  // Start scan immediately if domain already known, else wait for first request
  if (domain) { sscan(); schd(); }
  else console.log("Domain not configured — will scan after first request");

  // Reactive rescan: when a peer disconnects, rescan after a 30s debounce
  let btmr = null;
  root.on("bye", function () {
    this.to.next.apply(this.to, arguments);
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
