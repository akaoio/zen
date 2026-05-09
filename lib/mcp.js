#!/usr/bin/env node
// ZEN MCP — CLI entry point
// If a relay is running locally with attach() (script/server.js), forward stdio over its
// IPC socket instead of spawning a second ZEN peer. Falls back to standalone start() otherwise.
import net from "node:net";
import { existsSync } from "node:fs";
import { ipcSocketPath, start } from "./mcp/server.js";

const ipcPath = ipcSocketPath();

function standalone() {
  start().catch((err) => { process.stderr.write(err.message + "\n"); process.exit(1); });
}

if (existsSync(ipcPath)) {
  let bridged = false;
  const socket = net.connect(ipcPath);

  socket.once("connect", () => {
    bridged = true;
    process.stdin.pipe(socket, { end: false });
    socket.pipe(process.stdout, { end: false });
    // Exit when the AI client closes stdin so this bridge process doesn't linger.
    process.stdin.once("end", () => { socket.destroy(); process.exit(0); });
  });

  function recover() {
    try { process.stdin.unpipe(socket); } catch (_) {}
    try { socket.unpipe(process.stdout); } catch (_) {}
    socket.destroy();
    standalone();
  }

  // Never connected → fall back immediately
  socket.once("error", recover);
  // Relay restarted mid-session → clean up and switch to standalone
  socket.once("close", () => { if (bridged) recover(); });
} else {
  standalone();
}
