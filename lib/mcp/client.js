// lib/mcp/client.js — ZEN-native MCP client over DAM relay
//
// Usage:
//   import { ZenMcpClient } from "./mcp/client.js"
//   const client = new ZenMcpClient(serverPub, zen, myPair)
//   await client.ready()
//   const result = await client.call("tools/call", { name, arguments: args })
//   client.close()
//
// Relay transport: DAM relay messages (dam:"relay") carry encrypted JSON-RPC.
// No HTTP server, no open ports — both sides only need outbound WebSocket.

import ZEN from "../../zen.js";

export class ZenMcpClient {
  /**
   * @param {string}  serverPub  — target server's pub key
   * @param {object}  zen        — a live ZEN instance (new ZEN({...}))
   * @param {object}  pair       — caller's key pair { pub, priv }
   * @param {object}  [opt]
   * @param {number}  [opt.timeout=10000]  — default request timeout (ms)
   * @param {number}  [opt.ttl=5]          — DAM relay TTL
   */
  constructor(serverPub, zen, pair, opt = {}) {
    this._serverPub = serverPub;
    this._zen = zen;
    this._pair = pair;
    this._timeout = opt.timeout ?? 10000;
    this._ttl = opt.ttl ?? 5;
    this._seq = 0;
    this._pending = new Map(); // id → { resolve, reject, timer }
    this._off = null; // mesh.onRelay unsubscribe handle
    this._closed = false;
    this._initialized = false;
    this._onNotification = null; // callback for server-push events (relay subscribe)
  }

  /** Wire up the relay listener. Call once before making requests. */
  async ready() {
    if (this._off) return;
    const root = (this._zen._graph && this._zen._graph._) || this._zen._;
    const mesh = root && root.opt && root.opt.mesh;
    if (!mesh) throw new Error("ZEN instance has no mesh (WebSocket disabled?)");

    this._off = mesh.onRelay(async ({ from, data }) => {
      if (from !== this._serverPub) return;
      let msg;
      try {
        const shared = await ZEN.secret(from, this._pair);
        msg = await ZEN.decrypt(data, shared);
      } catch (_) { return; }
      if (!msg || msg.jsonrpc !== "2.0") return;
      // Server-push notification (e.g. subscribe events from relay)
      if (msg.id == null && msg.method === "notifications/message") {
        if (this._onNotification) this._onNotification(msg.params);
        return;
      }
      const entry = this._pending.get(msg.id);
      if (!entry) return;
      this._pending.delete(msg.id);
      clearTimeout(entry.timer);
      entry.resolve(msg);
    });
  }

  /** Stop the relay listener and clean up. */
  close() {
    this._closed = true;
    if (this._off) { this._off(); this._off = null; }
    for (const [, { reject, timer }] of this._pending) {
      clearTimeout(timer);
      reject(new Error("MCP client closed"));
    }
    this._pending.clear();
  }

  /**
   * Register a callback for server-push notifications (subscribe events via relay).
   * The callback receives the `params` object: { level, logger, data }.
   * `data` is a JSON string — parse it to get { sub_id, soul, key, val }.
   * @param {function} cb
   * @returns {this}
   */
  onNotification(cb) {
    this._onNotification = cb;
    return this;
  }

  /**
   * Send a raw JSON-RPC request and wait for the response.
   * @param {string} method
   * @param {object} params
   * @param {number} [timeout]
   * @returns {Promise<object>} raw JSON-RPC response
   */
  async request(method, params, timeout) {
    if (this._closed) { throw new Error("MCP client closed"); }
    const id = this._seq++;
    const req = JSON.stringify({ jsonrpc: "2.0", id, method, params });
    let encrypted;
    try {
      const shared = await ZEN.secret(this._serverPub, this._pair);
      encrypted = await ZEN.encrypt(req, shared);
    } catch (e) { throw new Error("encrypt failed: " + e.message); }

    return new Promise((resolve, reject) => {
      if (this._closed) { return reject(new Error("MCP client closed")); }
      const ms = timeout ?? this._timeout;
      const timer = setTimeout(() => {
        this._pending.delete(id);
        reject(new Error("Timeout waiting for response to " + method + " (id:" + id + ")"));
      }, ms);
      this._pending.set(id, { resolve, reject, timer });
      this._zen.push(this._serverPub, encrypted, { ttl: this._ttl });
    });
  }

  /** Initialize the MCP session. @returns {Promise<object>} serverInfo + capabilities */
  async initialize() {
    const res = await this.request("initialize", {});
    if (res.error) throw new Error(res.error.message);
    this._initialized = true;
    return res.result;
  }

  /** List available tools. @returns {Promise<Array>} */
  async listTools() {
    const res = await this.request("tools/list", {});
    if (res.error) throw new Error(res.error.message);
    return res.result.tools;
  }

  /**
   * Call a tool by name.
   * @param {string} name — tool name
   * @param {object} args — tool arguments
   * @param {number} [timeout] — override default timeout
   * @returns {Promise<*>} parsed tool result
   */
  async call(name, args, timeout) {
    const res = await this.request("tools/call", { name, arguments: args }, timeout);
    if (res.error) throw new Error(res.error.message);
    return JSON.parse(res.result.content[0].text);
  }

  /**
   * Discover an MCP server's info from its well-known graph soul ~<serverPub>/status.
   * @param {string} serverPub
   * @param {object} zen
   * @param {number} [timeout=2000]
   * @returns {Promise<object|null>}
   */
  static discover(serverPub, zen, timeout = 2000) {
    return new Promise((resolve) => {
      const t = setTimeout(() => resolve(null), timeout);
      zen.get("~" + serverPub + "/status").once((v) => {
        clearTimeout(t);
        resolve(v ?? null);
      });
    });
  }
}

export default ZenMcpClient;
