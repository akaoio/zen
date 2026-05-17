/**
 * PeerRegistry — single source of truth for relay peer discovery and quality tracking.
 *
 * Replaces the ad-hoc combination of:
 *   kprs Map       — discovery cache with TTL
 *   bootPubMap     — BOOT URL → remote pub key
 *   bootPidMap     — BOOT URL → remote AXE pid
 *   kprsProtect    — Set of never-evict BOOT URLs
 *   inline wss↔https normalization (was repeated 16+ times in server.js)
 *
 * Each entry is keyed by the canonical https:// (or http://) URL form.
 * The wss:// form is used for PEX wire messages and can be obtained via PeerRegistry.alt().
 */

import fs from "fs";
import * as xdg from "./xdg.js";
import path from "path";

// ── URL normalisation helpers ─────────────────────────────────────────────────

/**
 * Canonical (https://) form used as opt.peers key and registry key.
 * wss → https, ws → http.  Other schemes are returned unchanged.
 */
function norm(url) {
  if (typeof url !== "string") return url;
  return url
    .replace(/^wss:\/\//, "https://")
    .replace(/^ws:\/\//, "http://");
}

/**
 * Wire (wss://) form used in PEX DAM messages.
 * https → wss, http → ws.
 */
function alt(url) {
  if (typeof url !== "string") return url;
  return url
    .replace(/^https:\/\//, "wss://")
    .replace(/^http:\/\//, "ws://");
}

/**
 * Returns [canonicalUrl, wireUrl] for any input scheme.
 */
function both(url) {
  const n = norm(url);
  return [n, alt(n)];
}

// ── PeerRegistry ──────────────────────────────────────────────────────────────

class PeerRegistry {
  /**
   * @param {object} opts
   * @param {number} [opts.TTL=1800000]  ms before an unconfirmed entry is evicted (default 30 min)
   */
  constructor(opts = {}) {
    // Map<canonicalUrl, PeerEntry>
    // PeerEntry: { url, seen, lastOk, pub, pid, isBoot, isSelf }
    this._map  = new Map();
    this._boot = new Set(); // canonical URLs of BOOT peers (never evicted)
    this._self = new Set(); // canonical URLs of self (excluded from PEX + save)
    this.TTL   = opts.TTL != null ? opts.TTL : 30 * 60 * 1000;
    this._saveTmr = null;
  }

  // ── Static URL helpers (importable without an instance) ─────────────────────

  static norm = norm;
  static alt  = alt;
  static both = both;

  // ── Setup ────────────────────────────────────────────────────────────────────

  /**
   * Mark URLs as BOOT peers (never evicted, always reconnected by watchdog).
   * Accepts a single URL string or an array.
   */
  protect(urls) {
    (Array.isArray(urls) ? urls : [urls]).forEach(u => {
      const [n] = both(u);
      this._boot.add(n);
      const entry = this._map.get(n);
      if (entry) { entry.isBoot = true; }
      else { this._map.set(n, { url: n, seen: Date.now(), lastOk: 0, isBoot: true, isSelf: false }); }
    });
  }

  /**
   * Mark URLs as self (excluded from PEX output and persistence).
   */
  setSelf(urls) {
    (Array.isArray(urls) ? urls : [urls]).forEach(u => {
      if (!u) return;
      const [n] = both(u);
      this._self.add(n);
      const entry = this._map.get(n);
      if (entry) { entry.isSelf = true; }
      else { this._map.set(n, { url: n, seen: Date.now(), lastOk: Date.now(), isBoot: false, isSelf: true }); }
    });
  }

  // ── Core operations ──────────────────────────────────────────────────────────

  /**
   * Add a discovered peer (via PEX or scan).
   * Returns true if the URL was new, false if already known.
   */
  add(url) {
    const [n] = both(url);
    if (!n || this._self.has(n)) return false;
    if (this._map.has(n)) {
      this._map.get(n).seen = Date.now();
      return false;
    }
    this._map.set(n, { url: n, seen: Date.now(), lastOk: 0, isBoot: this._boot.has(n), isSelf: false });
    return true;
  }

  /**
   * Mark a connection as confirmed (lastOk = now).
   * Optionally attach remote pub and pid for WATCHDOG identity checks.
   */
  confirm(url, opts = {}) {
    const [n] = both(url);
    const entry = this._map.get(n);
    const now = Date.now();
    if (entry) {
      entry.lastOk = now;
      entry.seen   = now;
      if (opts.pub) entry.pub = opts.pub;
      if (opts.pid) entry.pid = opts.pid;
    } else {
      this._map.set(n, {
        url: n, seen: now, lastOk: now,
        isBoot: this._boot.has(n), isSelf: this._self.has(n),
        ...(opts.pub && { pub: opts.pub }),
        ...(opts.pid && { pid: opts.pid }),
      });
    }
    this._scheduleSave();
  }

  /** Update seen timestamp without marking as confirmed. */
  touch(url) {
    const [n] = both(url);
    const entry = this._map.get(n);
    if (entry) entry.seen = Date.now();
  }

  /** Returns true if url (either scheme) is in the registry. */
  has(url) {
    const [n, a] = both(url);
    return this._map.has(n) || this._map.has(a);
  }

  /** Returns true if url is a protected BOOT peer. */
  isBoot(url) {
    const [n] = both(url);
    return this._boot.has(n);
  }

  /** Returns true if url is a self URL. */
  isSelf(url) {
    const [n] = both(url);
    return this._self.has(n);
  }

  /** Returns the registry entry for url, or undefined. */
  entry(url) {
    const [n] = both(url);
    return this._map.get(n);
  }

  // ── Queries for WATCHDOG ─────────────────────────────────────────────────────

  /** Returns all BOOT peer entries (for WATCHDOG reconnect loop). */
  bootEntries() {
    return Array.from(this._boot)
      .map(n => this._map.get(n))
      .filter(Boolean);
  }

  /**
   * Returns confirmed non-BOOT, non-self entries (for WATCHDOG extension).
   * "Confirmed" means lastOk > 0 (at least one successful connection ever).
   */
  confirmedNonBoot() {
    return Array.from(this._map.values()).filter(
      e => e.lastOk > 0 && !e.isBoot && !e.isSelf
    );
  }

  // ── PEX helpers ──────────────────────────────────────────────────────────────

  /**
   * Returns a sorted wss:// URL list for PEX messages.
   * @param {number}   [max=50]    Maximum number of entries.
   * @param {Function} [rttOf]     Optional fn(url) → number for sorting.
   */
  pexList(max = 50, rttOf = null) {
    return Array.from(this._map.values())
      .filter(e => !e.isSelf && e.url)
      .sort((a, b) => {
        const as = a.url.startsWith("wss://") ? 0 : 1;
        const bs = b.url.startsWith("wss://") ? 0 : 1;
        if (as !== bs) return as - bs;
        const ar = rttOf ? rttOf(alt(a.url)) : Infinity;
        const br = rttOf ? rttOf(alt(b.url)) : Infinity;
        return ar - br;
      })
      .map(e => alt(e.url)) // PEX wire format uses wss://
      .slice(0, max);
  }

  // ── Eviction ─────────────────────────────────────────────────────────────────

  /**
   * Remove entries that have not been seen or confirmed within TTL.
   * BOOT and self entries are never evicted.
   */
  evict() {
    const now = Date.now();
    for (const [url, entry] of this._map) {
      if (entry.isBoot || entry.isSelf) continue;
      if (now - entry.seen > this.TTL && now - entry.lastOk > this.TTL) {
        this._map.delete(url);
      }
    }
  }

  // ── Persistence ──────────────────────────────────────────────────────────────

  /**
   * Persist confirmed non-BOOT, non-self entries to disk (atomic write).
   * @param {string} [filePath]  Destination file path (defaults to bound path from bindSave).
   */
  save(filePath) {
    const fpath = filePath || this._saveFile;
    if (!fpath) return;
    try {
      xdg.ensure(path.dirname(fpath));
      const list = Array.from(this._map.values())
        .filter(e => e.lastOk > 0 && !e.isBoot && !e.isSelf)
        .map(e => ({ url: e.url, lastOk: e.lastOk }))
        .sort((a, b) => b.lastOk - a.lastOk)
        .slice(0, 200);
      const tmp = fpath + ".tmp";
      fs.writeFileSync(tmp, JSON.stringify({ peers: list, savedAt: Date.now() }, null, 2));
      fs.renameSync(tmp, fpath);
    } catch {}
  }

  /**
   * Load persisted entries and call adpFn(url) for each eligible one.
   * @param {string|Function} filePathOrAdpFn  File path OR adpFn (if bindSave was used).
   * @param {Function}        [adpFn]          Called with each url string to trigger connect.
   * @param {number}          [maxAge]         Discard entries older than this (default 7 days).
   * @returns {number} count of loaded entries
   */
  load(filePathOrAdpFn, adpFn, maxAge = 7 * 24 * 60 * 60 * 1000) {
    let filePath, fn;
    if (typeof filePathOrAdpFn === "function") {
      filePath = this._saveFile;
      fn = filePathOrAdpFn;
    } else {
      filePath = filePathOrAdpFn;
      fn = adpFn;
    }
    if (!filePath || !fn) return 0;
    try {
      const data = JSON.parse(fs.readFileSync(filePath, "utf8"));
      const now  = Date.now();
      let count  = 0;
      for (const p of (data.peers || [])) {
        if (typeof p.url !== "string") continue;
        if (!p.lastOk || now - p.lastOk >= maxAge) continue;
        // Restore lastOk into the entry that adpFn will add
        const [n] = both(p.url);
        if (!this._map.has(n)) {
          this._map.set(n, { url: n, seen: now, lastOk: p.lastOk, isBoot: false, isSelf: false });
        }
        fn(n);
        count++;
      }
      return count;
    } catch {
      return 0;
    }
  }

  /**
   * Schedule a debounced save (5s).  Called automatically by confirm().
   * @param {string} filePath
   */
  scheduleSave(filePath) {
    if (this._saveTmr) return;
    this._saveTmr = setTimeout(() => {
      this._saveTmr = null;
      this.save(filePath);
    }, 5000);
  }

  // Internal: called by confirm() — needs filePath at call-time.
  // The public API binds the file path on construction for convenience.
  _scheduleSave() {
    if (this._saveFile) this.scheduleSave(this._saveFile);
  }

  /**
   * Bind a file path so confirm() auto-schedules saves without passing it each time.
   * @param {string} filePath
   */
  bindSave(filePath) {
    this._saveFile = filePath;
    return this;
  }
}

export { PeerRegistry, norm, alt, both };
export default PeerRegistry;
