/**
 * lib/scan.js — ZEN smart peer scanner + hostname family logic
 *
 * Given a domain like "peer1.akao.io", derives sibling patterns and probes them.
 * Supports formats:
 *   peer1.akao.io    → peer{N}.akao.io and peer.akao.io
 *   zen.akao.io      → zen{N}.akao.io
 *   zen0.akao.io     → zen.akao.io + zen{N}.akao.io
 *   node-1.site.com  → node-{N}.site.com
 *   relay01.net      → relay{N}.net + relay.net (zero-padded)
 *
 * Probe: GET /zen.js (HTTP/HTTPS) on configured port + 8420.
 * Returns discovered peer WSS URLs usable by ZEN.
 */

const DPORT = 8420;
const MIDX  = 100;
const MFND  = 10;   // stop once this many peers found per cycle
const CONC  = 10;
const TOUT  = 3000;

var LABEL_RE = /^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?$/i;

function splitHost(d) {
  if (!d) {
    return null;
  }
  var s = "" + d;
  // Raw IPv6 address: contains two or more colons (e.g. "2a02:c207::1").
  // Also handle bracket notation "[2a02::1]" passed directly.
  // No sibling-domain scan is possible for raw IPs — bail out immediately.
  s = s.replace(/^\[|\]$/g, ""); // strip brackets if present
  if ((s.match(/:/g) || []).length >= 2) {
    return null;
  }
  var h = s.split(":")[0]; // strip port if any (single colon = host:port)
  var dot = h.indexOf(".");
  var lbl = dot >= 0 ? h.slice(0, dot) : h;
  var rst = dot >= 0 ? h.slice(dot) : "";
  if (!lbl) {
    return null;
  }
  return { host: h, label: lbl, suffix: rst };
}

function validLabel(label) {
  return !!label && LABEL_RE.test(label);
}

export function mkpat(d) {
  var parts = splitHost(d);
  var m;
  if (!parts) {
    return null;
  }
  m = parts.label.match(/^(.*?)(\d+)(.*)$/);
  if (!m) {
    return {
      prefix: parts.label,
      index: null,
      tail: "",
      suffix: parts.suffix,
      padLen: 0,
      label: parts.label,
      host: parts.host,
    };
  }
  return {
    prefix: m[1],
    index: parseInt(m[2], 10),
    tail: m[3],
    suffix: parts.suffix,
    padLen: m[2].length > 1 && m[2][0] === "0" ? m[2].length : 0,
    label: parts.label,
    host: parts.host,
  };
}

export function bdom(p, n) {
  var idx = String(n);
  if (p.padLen > 0) {
    idx = idx.padStart(p.padLen, "0");
  }
  return p.prefix + idx + p.tail + p.suffix;
}

export function candidateHosts(d, opt) {
  var pat = typeof d === "string" ? mkpat(d) : d;
  var maxIndex = (opt || {}).maxIndex;
  var out = [];
  var seen = {};
  var n;
  var base;
  if (!pat) {
    return out;
  }
  maxIndex = Number.isInteger(maxIndex) ? maxIndex : 100;

  function add(label) {
    var host;
    if (!validLabel(label) || seen[label] || label === pat.label) {
      return;
    }
    seen[label] = 1;
    host = label + pat.suffix;
    if (host !== pat.host) {
      out.push(host);
    }
  }

  if (pat.index === null) {
    for (n = 0; n <= maxIndex; n++) {
      add(pat.prefix + n + pat.tail);
    }
    return out;
  }

  base = pat.prefix + pat.tail;
  add(base);
  for (n = 0; n <= maxIndex; n++) {
    if (n === pat.index) {
      continue;
    }
    add(bdom({
      prefix: pat.prefix,
      tail: pat.tail,
      suffix: "",
      padLen: pat.padLen,
    }, n));
  }
  return out;
}

// ── probe ──────────────────────────────────────────────────────────────────

function hostInUrl(host) {
  if (!host || host.indexOf(":") === -1 || host[0] === "[") {
    return host;
  }
  return "[" + host + "]";
}

async function prb(host, port) {
  var https = await import("node:https");
  var http = await import("node:http");
  return new Promise((ok) => {
    const h = hostInUrl(host);
    const att = (tls) => {
      const mod = tls ? https.default : http.default;
      const url = (tls ? "https" : "http") + "://" + h + ":" + port + "/zen.js";
      const req = mod.get(url, { timeout: TOUT, rejectUnauthorized: false }, (res) => {
        if (res.statusCode === 200) {
          ok((tls ? "wss" : "ws") + "://" + h + ":" + port + "/zen");
        } else {
          if (tls) att(false); else ok(null);
        }
        res.resume();
      });
      req.on("error",   () => { if (tls) att(false); else ok(null); });
      req.on("timeout", () => { req.destroy(); if (tls) att(false); else ok(null); });
    };
    att(true);
  });
}

// ── concurrency pool ───────────────────────────────────────────────────────

async function pool(tasks, con) {
  const res = [];
  let i = 0;
  const run = async () => {
    while (i < tasks.length) {
      const idx = i++;
      res[idx] = await tasks[idx]();
    }
  };
  const wkr = Array.from({ length: Math.min(con, tasks.length) }, run);
  await Promise.all(wkr);
  return res.filter(Boolean);
}

// ── main exports ───────────────────────────────────────────────────────────

/**
 * scan(domain, opt) → Promise<string[]>
 *
 * opt.port     — configured port (probed in addition to DPORT)
 * opt.maxIndex — how far to scan (default: 100)
 * opt.maxFound — stop after N found per cycle (default: 10)
 * opt.onFound  — callback(url) called as each peer is discovered
 * opt.ip6      — array of IPv6 addresses to probe directly
 */
export async function scan(d, opt = {}) {
  const uport = Number(opt.port) || DPORT;
  const midx  = opt.maxIndex || MIDX;
  const mfnd  = opt.mfnd !== undefined ? opt.mfnd : MFND;
  const fnd   = [];
  let   stp   = false;

  const cands = [];

  // Domain-pattern candidates (IPv4 hostname scanning)
  for (const host of candidateHosts(d, { maxIndex: midx })) {
    const pts = [...new Set([uport, DPORT])];
    for (const port of pts) cands.push({ host, port });
  }

  // Direct IPv6 address probes
  if (Array.isArray(opt.ip6)) {
    const pts = [...new Set([uport, DPORT])];
    for (const addr of opt.ip6) {
      for (const port of pts) cands.push({ host: addr, port });
    }
  }

  if (!cands.length) return [];

  const tasks = cands.map(({ host, port }) => async () => {
    if (stp) return null;
    const url = await prb(host, port);
    if (url) {
      fnd.push(url);
      if (opt.onFound) opt.onFound(url);
      if (mfnd > 0 && fnd.length >= mfnd) stp = true;
    }
    return url;
  });

  await pool(tasks, CONC);
  return fnd;
}

/**
 * scanip6(addresses, opt) → Promise<string[]>
 *
 * Probe a list of raw IPv6 addresses directly (no pattern needed).
 * opt.port    — configured port (probed in addition to DPORT)
 * opt.onFound — callback(url) per peer found
 */
export function scanip6(addresses, opt = {}) {
  return scan(null, { ...opt, ip6: addresses });
}

/**
 * scanbg(domain, opt) — fire-and-forget, calls opt.onFound per peer found.
 */
export function scanbg(d, opt = {}) {
  if (!d) return;
  (typeof setImmediate === "function" ? setImmediate : setTimeout)(() => { scan(d, opt).catch(() => {}); }, 0);
}
