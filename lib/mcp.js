#!/usr/bin/env node
// ZEN MCP — CLI entry point
// Server logic lives in lib/mcp/server.js; client library in lib/mcp/client.js.
import { start } from "./mcp/server.js";
start().catch((err) => { console.error(err); process.exit(1); });
