#!/usr/bin/env node
// ZEN MCP — CLI entry point
// If a relay is running locally with attach() (script/server.js), forward stdio over its
// IPC socket instead of spawning a second ZEN peer. Falls back to standalone start() otherwise.
import net from "node:net";
import { existsSync } from "node:fs";
import { ipcSocketPath, start } from "./mcp/server.js";

const ipcPath = ipcSocketPath();

const RECONNECT_DELAY = 2000; // ms between reconnect attempts after service restart
const MAX_RECONNECTS  = 15;   // ~30 s window — covers typical service restart time

function standalone() {
  start().catch((err) => { process.stderr.write(err.message + "\n"); process.exit(1); });
}

// Try to bridge via IPC socket; on disconnect, retry reconnecting before falling back.
// In test/CI mode (NO_BOOTSTRAP=1) skip bridging entirely — no relay is running.
function bridge(attempt = 0) {
  if (!existsSync(ipcPath)) {
    // On first attempt with no socket: give up immediately unless we previously
    // had a live connection (service restart scenario). In test/CI there is no
    // relay, so retrying would block the process for MAX_RECONNECTS * 2 s.
    if (attempt === 0 || attempt >= MAX_RECONNECTS) return standalone();
    return setTimeout(() => bridge(attempt + 1), RECONNECT_DELAY);
  }

  let connected = false;
  const socket = net.connect(ipcPath);

  socket.once("connect", () => {
    connected = true;
    process.stdin.pipe(socket, { end: false });
    socket.pipe(process.stdout, { end: false });
    // Exit when the AI client closes stdin so this bridge process doesn't linger.
    process.stdin.once("end", () => { socket.destroy(); process.exit(0); });
  });

  function reconnect() {
    try { process.stdin.unpipe(socket); } catch (_) {}
    try { socket.unpipe(process.stdout); } catch (_) {}
    socket.destroy();
    const next = connected ? 1 : attempt + 1; // after live session, allow reconnect (start at 1, not 0)
    if (next >= MAX_RECONNECTS) return standalone();
    setTimeout(() => bridge(next), RECONNECT_DELAY);
  }

  socket.once("error", reconnect);
  socket.once("close", reconnect);
}

bridge();
