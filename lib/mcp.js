#!/usr/bin/env node
// ZEN MCP — CLI entry point
// If a relay is running locally with attach() (script/server.js), forward stdio over its
// IPC socket instead of spawning a second ZEN peer. Falls back to standalone start() otherwise.
import net from "node:net";
import { existsSync } from "node:fs";
import { ipcSocketPath, start } from "./mcp/server.js";

const ipcPath = ipcSocketPath();
if (existsSync(ipcPath)) {
  const socket = net.connect(ipcPath, () => {
    process.stdin.pipe(socket);
    socket.pipe(process.stdout);
  });
  socket.on("error", () => start().catch((err) => { console.error(err); process.exit(1); }));
} else {
  start().catch((err) => { console.error(err); process.exit(1); });
}
