#!/bin/sh
# script/mcp.sh — Dev-mode MCP launcher
#
# Resolves the absolute path of lib/mcp.js relative to this script,
# so it works regardless of the caller's CWD. Used in .mcp.json for
# local development (instead of `npx @akaoio/zen mcp`).
#
# Usage (in .mcp.json):
#   "command": "/home/x/zen/script/mcp.sh"
#
# Env vars:
#   ZEN_NET=testnet   — use testnet network (port 8421, separate data dir)
#   ZEN_NET=mainnet   — use mainnet network (port 8420) — default

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec node "$SCRIPT_DIR/../lib/mcp.js" "$@"
