/**
 * lib/pex.js — Relay-side Peer Exchange (PEX) for ZEN relays.
 *
 * Encapsulates self-URL announcement, peer gossip, peer adoption, and IP
 * discovery so any relay embedding ZEN as a library gets correct PEX for
 * free — including IPv4-only relays that have no domain name.
 *
 * Usage (minimal):
 *   import { setupRelayPex } from "zen/lib/pex.js";
 *   const { origin, registry, adopt } = setupRelayPex(zen, { domain, port });
 *
 * Usage (custom server with UDP fast-path):
 *   setupRelayPex(zen, {
 *     domain, port, key, registry,
 *     sendPeers: (list, peer) => saySmart({ dam:"pex", peers:list }, peer),
 *     onDisc:    (di)         => { discResult = di; refreshStatus(); },
 *   });
 */

import { disc } from "./discover.js";
import { PeerRegistry } from "./peer-registry.js";

const DISC_INTERVAL = 10 * 60 * 1000; // re-discover public IPs every 10 min
const MUPS = 10;                       // max outbound connections from PEX

/**
 * Probe whether a domain is reachable via HTTPS (port 443).
 * Used to auto-detect TLS termination by Cloudflare Tunnel, nginx, Caddy, etc.
 * Returns true if the /status endpoint responds with HTTP 200.
 */
async function _probeHttps(domain, timeoutMs = 6000) {
  try {
    const ctrl = new AbortController();
    const timer = setTimeout(() => ctrl.abort(), timeoutMs);
    const res = await fetch(`https://${domain}/status`, { signal: ctrl.signal });
    clearTimeout(timer);
    return res.status === 200;
  } catch { return false; }
}

/**
 * Wire up relay-side Peer Exchange on a ZEN instance.
 *
 * @param {object} zen - ZEN instance (result of Gun/ZEN constructor)
 * @param {object} [opts]
 * @param {string}       [opts.domain]      Relay domain, e.g. "zen.akao.io". Null for IP-only.
 * @param {number}       [opts.port=8420]   Local WebSocket port (bind port).
 * @param {number|null}  [opts.publicPort]  Public-facing port to announce. Defaults to opts.port.
 *                                          Set to 443 (or omit from URL) when behind a reverse
 *                                          proxy / Cloudflare Tunnel that exposes port 443.
 * @param {object|null}  [opts.key]         TLS key presence → wss:// vs ws://.
 * @param {PeerRegistry} [opts.registry]    Shared PeerRegistry; creates a new one if omitted.
 * @param {number}       [opts.pexMax=50]   Max peers included in PEX list sent to new connections.
 * @param {Function}     [opts.rttOf]       fn(url) → RTT ms, used to sort peer list by latency.
 * @param {Function}     [opts.sendPeers]   fn(list, peer) — replaces default mesh.say for sending
 *                                          the peer list to newly connected peers. Use this hook
 *                                          for UDP fast-path, bpids injection, etc.
 * @param {Function}     [opts.onDisc]      fn(discResult) — called after every IP discovery run.
 *                                          Use to update /status or refresh cached state.
 * @param {Function}     [opts.onAdopt]     fn(url) — called when a NEW peer is added to the
 *                                          registry. Use to trigger scan-cycle bookkeeping.
 * @returns {{ origin: string|null, registry: PeerRegistry, adopt: Function, getSelfUrl: Function }}
 */
export function setupRelayPex(zen, opts = {}) {
  const {
    domain      = null,
    port        = 8420,
    publicPort  = null,   // public-facing port; defaults to port (local bind port)
    key         = null,
    registry  = new PeerRegistry(),
    pexMax    = 50,
    rttOf     = null,
    sendPeers = null,
    onDisc    = null,
    onAdopt   = null,
  } = opts;

  const annPort  = publicPort || port;  // port to advertise in self-URL
  const scheme   = key ? "wss" : "ws";

  // Domain-based self-URL (stable across restarts).
  // Uses annPort (public-facing) — may differ from local bind port when behind
  // a reverse proxy or Cloudflare Tunnel (e.g. bind=8420, public=443).
  const defaultWssPort = scheme === "wss" ? 443 : 80;
  const portSuffix     = annPort === defaultWssPort ? "" : `:${annPort}`;
  let origin = domain ? `${scheme}://${domain}${portSuffix}/zen` : null;
  if (origin) registry.setSelf(origin);

  // When a domain is configured but no local TLS key exists, probe for external
  // HTTPS availability (Cloudflare Tunnel, nginx, Caddy, etc.). If the domain
  // responds on port 443, upgrade self-URL to wss:// with no explicit port.
  // This is fully automatic — no env vars or manual configuration needed.
  if (domain && !key) {
    _probeHttps(domain).then(ok => {
      if (!ok) return;
      const upgraded = `wss://${domain}/zen`;
      if (upgraded === origin) return;
      if (origin) registry.setSelf(origin); // absorb old URL into self-set
      origin = upgraded;
      registry.setSelf(origin);
      console.log("[PEX] HTTPS detected — upgraded self-URL to:", origin);
      // Re-announce to any peers already connected
      if (route) {
        try { route.say({ dam: "pex", peers: [origin] }); } catch {}
      }
    }).catch(() => {});
  }

  let originv  = null; // IPv6 self-URL  (updated by _refreshIpSelf)
  let originip = null; // IPv4 self-URL  (updated by _refreshIpSelf; used when no domain)

  // Track which URLs have been gossiped this session so reconnecting peers
  // get re-gossiped exactly once instead of being silently skipped.
  const gossiped = new Set();

  // AXE mesh route — available after setImmediate fires
  let route = null;

  // ── Self-URL helpers ──────────────────────────────────────────────────────

  /** Best self-URL to announce: domain > IPv4 > IPv6 */
  function getSelfUrl() { return origin || originip || originv; }

  /** True when url is any of our known self-URLs (canonical form) */
  function isSelf(url) {
    const n = PeerRegistry.norm(url);
    return n === (origin && PeerRegistry.norm(origin)) ||
           n === (originv  && PeerRegistry.norm(originv))  ||
           n === (originip && PeerRegistry.norm(originip));
  }

  // ── IP discovery ──────────────────────────────────────────────────────────

  function _refreshIpSelf() {
    disc({ noSave: true }).then((di) => {
      if (di.ip6) {
        const newv = `${scheme}://[${di.ip6}]:${port}/zen`;
        if (newv !== originv) {
          if (originv) registry.setSelf(originv); // absorb stale IPv6 into self set
          originv = newv;
          if (!origin) registry.setSelf(originv); // only advertise when no domain
          console.log("[PEX] IPv6 self-URL:", originv);
        }
      }
      // IPv4 fallback: used when no domain is configured
      if (!origin && di.ip) {
        const newip = `${scheme}://${di.ip}:${port}/zen`;
        if (newip !== originip) {
          if (originip) registry.setSelf(originip); // absorb stale IPv4 into self set
          originip = newip;
          registry.setSelf(originip);
          console.log("[PEX] IPv4 self-URL:", originip);
        }
      }
      if (onDisc) onDisc(di);
    }).catch(() => {});
  }

  // ── Peer adoption + gossip ────────────────────────────────────────────────

  /**
   * Adopt a peer URL: normalise, guard against self/tombstones/duplicates,
   * add to registry, attempt outbound connection, and gossip to all peers.
   *
   * @param {string} url - wss:// or https:// peer URL
   */
  function adopt(url) {
    const n = PeerRegistry.norm(url); // canonical https:// form
    if (registry.isSelf(n)) return;

    // Skip tombstoned peers (AXE dropped them intentionally)
    const r = zen && zen._graph && zen._graph._;
    const tombs = r && r.opt && r.opt._tombUrls;
    if (tombs && (tombs.has(n) || tombs.has(PeerRegistry.alt(n)))) return;

    if (registry.has(n)) {
      // Already known — re-gossip once per session so peers that joined after
      // this relay connected can still discover it.
      if (!gossiped.has(n) && route) {
        gossiped.add(n);
        try {
          route.say({ dam: "pex", peers: [PeerRegistry.alt(n)] }, r && r.opt && r.opt.peers);
        } catch {}
      }
      return;
    }

    registry.add(n);
    gossiped.add(n);
    if (onAdopt) onAdopt(n);
    console.log("[PEX] Discovered peer:", n);

    // Connect if under upstream limit
    const ups = Object.keys((r && r.axe && r.axe.up) || {}).length;
    if (route && ups < MUPS) {
      try { route.hi({ id: n, url: n, retry: 9 }); } catch {}
    } else if (!route && r && r.opt) {
      // mesh not yet ready — queue for AXE to connect later
      if (!Array.isArray(r.opt.peers)) r.opt.peers = [];
      if (!r.opt.peers.includes(n)) r.opt.peers.push(n);
    }

    // Broadcast to all currently connected peers (wss:// wire format)
    if (route) {
      try {
        route.say({ dam: "pex", peers: [PeerRegistry.alt(n)] }, r && r.opt && r.opt.peers);
      } catch {}
    }
  }

  // ── Wire up after ZEN initialises ────────────────────────────────────────

  const root = zen._graph._;

  setImmediate(() => {
    const mesh = root.opt && root.opt.mesh;
    if (!mesh) {
      console.warn("[PEX] AXE mesh not available — PEX not wired");
      return;
    }
    route = mesh;

    // Receive peer lists from connected peers
    mesh.hear["pex"] = function(msg, _peer) {
      if (!Array.isArray(msg.peers)) return;
      msg.peers.forEach((url) => {
        if (typeof url === "string" && /^wss?:\/\//.test(url) && !isSelf(url)) {
          adopt(url);
        }
      });
    };

    // On new peer connection: send our peer list + announce our own URL
    root.on("hi", function(peer) {
      this.to.next(peer);

      const list = registry.pexList(pexMax, rttOf);
      setTimeout(() => {
        try {
          if (sendPeers) {
            sendPeers(list, peer);
          } else {
            mesh.say({ dam: "pex", peers: list }, peer);
          }
        } catch {}
      }, 600);

      // Announce our public URL so the newly connected peer can reach us back
      const selfUrl = getSelfUrl();
      if (selfUrl) {
        setTimeout(() => {
          try { mesh.say({ dam: "pex", peers: [selfUrl] }, peer); } catch {}
        }, 700);
      }
    });

    // Start IP discovery immediately, then refresh every 10 min
    _refreshIpSelf();
    const timer = setInterval(_refreshIpSelf, DISC_INTERVAL);
    if (timer.unref) timer.unref();
  });

  return { origin, registry, adopt, getSelfUrl };
}
