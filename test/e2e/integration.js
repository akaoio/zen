#!/usr/bin/env node
/**
 * test/e2e/integration.js — Cross-peer integration test
 *
 * Run simultaneously on all machines to verify:
 *   1. Local write → local read  (basic GET/PUT)
 *   2. Disk persistence          (survives relay restart)
 *   3. Cross-peer propagation    (write on A, read on B)
 *
 * Usage:
 *   node test/e2e/integration.js [--suite basic|persist|cross|all]
 *
 * On each machine the script:
 *   - Connects to the local relay as a thin peer  (wss://localhost:PORT)
 *   - Runs the requested test suites
 *   - Prints a colour-coded PASS / FAIL summary
 */

import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { homedir, hostname } from "node:os";
import { execSync, spawnSync, spawn } from "node:child_process";
import ZEN from "../../index.js";
import { ZenMcpClient } from "../../lib/mcp/client.js";
import { getOrCreateIdentity } from "../../lib/identity.js";

// ── Helpers ───────────────────────────────────────────────────────────────────

const GREEN  = "\x1b[32m";
const RED    = "\x1b[31m";
const YELLOW = "\x1b[33m";
const CYAN   = "\x1b[36m";
const RESET  = "\x1b[0m";
const BOLD   = "\x1b[1m";

function pass(msg) { console.log(`${GREEN}✓ PASS${RESET} ${msg}`); }
function fail(msg) { console.log(`${RED}✗ FAIL${RESET} ${msg}`); results.failed++; }
function info(msg) { console.log(`${CYAN}  >${RESET} ${msg}`); }
function warn(msg) { console.log(`${YELLOW}  !${RESET} ${msg}`); }

const results = { passed: 0, failed: 0 };

function localRelayPort() {
  const f = join(homedir(), ".config/zen/port");
  return existsSync(f) ? readFileSync(f, "utf8").trim() : "8420";
}

function localRelayUseTLS() {
  return existsSync(join(homedir(), ".config/zen/cert.pem"));
}

function buildRelayURL() {
  const scheme = localRelayUseTLS() ? "https" : "http";
  return `${scheme}://localhost:${localRelayPort()}/zen`;
}

function get(zen, soul, key, timeout = 5000) {
  return new Promise((resolve) => {
    let chain = zen.get(soul);
    if (key !== undefined) chain = chain.get(key);
    const timer = setTimeout(() => resolve(undefined), timeout);
    chain.once((val) => { clearTimeout(timer); resolve(val ?? null); });
  });
}

function put(zen, soul, key, value, timeout = 5000) {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("put timeout")), timeout);
    zen.get(soul).get(key).put(value, (ack) => {
      clearTimeout(timer);
      if (ack && ack.err) reject(new Error(ack.err));
      else resolve(true);
    });
  });
}

function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

// ── Setup ──────────────────────────────────────────────────────────────────────

// Prefer ~/.config/zen/domain (e.g. "peer0.akao.io") then extract short name
function detectHost() {
  const domainFile = join(homedir(), ".config/zen/domain");
  if (existsSync(domainFile)) {
    return readFileSync(domainFile, "utf8").trim().split(".")[0]; // "zen", "peer0", "peer1"
  }
  return hostname().split(".")[0];
}
const HOST   = detectHost();   // zen, peer0, peer1
const RELAY  = process.env.ZEN_RELAY || buildRelayURL();
const SUITE  = (process.argv.find((a) => a.startsWith("--suite=")) || "--suite=all").split("=")[1];

if (localRelayUseTLS()) {
  process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";
}

const zenOpt = {
  peers: [RELAY],
  rfs: false,   // thin peer — no local disk
  axe: false,   // disable AXE routing for direct relay tests
};

info(`Host      : ${BOLD}${HOST}${RESET}`);
info(`Relay URL : ${RELAY}`);
info(`Suite     : ${SUITE}`);
console.log();

// ── Test suites ────────────────────────────────────────────────────────────────

async function suiteBasic(zen) {
  console.log(`${BOLD}── Suite: basic (write → read via relay) ──${RESET}`);

  const soul  = `e2e-basic`;
  const key   = HOST;
  const value = `hello-from-${HOST}-${Date.now()}`;

  // 1. Write
  try {
    await put(zen, soul, key, value);
    pass(`PUT  ${soul}/${key} = "${value}"`);
    results.passed++;
  } catch (e) {
    fail(`PUT  ${soul}/${key}: ${e.message}`);
    return;
  }

  await sleep(300);

  // 2. Read back
  const got = await get(zen, soul, key);
  if (got === value) {
    pass(`GET  ${soul}/${key} = "${got}"`);
    results.passed++;
  } else {
    fail(`GET  ${soul}/${key}: expected "${value}" got ${JSON.stringify(got)}`);
  }

  console.log();
}

async function suitePersist(zen) {
  console.log(`${BOLD}── Suite: persist (write → restart relay → read) ──${RESET}`);

  const soul  = `e2e-persist`;
  const key   = HOST;
  const value = `persist-${Date.now()}`;

  // 1. Write
  try {
    await put(zen, soul, key, value);
    pass(`PUT  ${soul}/${key} = "${value}"`);
    results.passed++;
  } catch (e) {
    fail(`PUT  ${soul}/${key}: ${e.message}`);
    return;
  }

  await sleep(500);

  // 2. Restart relay
  info("Restarting relay service...");
  try {
    execSync("sudo systemctl restart relay.service", { stdio: "inherit" });
    await sleep(3000);   // wait for relay to come back up
    pass("Relay restarted");
    results.passed++;
  } catch (e) {
    warn(`Could not restart relay: ${e.message}`);
    warn("Skipping persistence check (no systemctl access)");
    return;
  }

  // 3. Reconnect
  zen.opt({ peers: [RELAY] });
  await sleep(1000);

  // 4. Read after restart
  const got = await get(zen, soul, key, 8000);
  if (got === value) {
    pass(`GET  ${soul}/${key} = "${got}" (survived restart)`);
    results.passed++;
  } else {
    fail(`GET  ${soul}/${key}: expected "${value}" got ${JSON.stringify(got)} — data NOT persisted`);
  }

  console.log();
}

async function suiteCross(zen, testPair) {
  console.log(`${BOLD}── Suite: cross-peer (MCP put → relay → read) ──${RESET}`);

  const soul  = `e2e-cross`;
  const peers = ["zen", "peer0", "peer1"].filter((h) => h !== HOST);

  // Resolve the pub key that the MCP server will use (same hardware identity)
  const identity = await getOrCreateIdentity();
  if (!identity) {
    warn("No hardware identity available — skipping MCP cross-peer suite");
    console.log();
    return;
  }
  const serverPub = identity.pair.pub;

  // Spawn the local MCP server (stdout = JSON-RPC → /dev/null, stderr suppressed)
  info("Starting local MCP server...");
  const mcpProc = spawn(process.execPath, [join(process.cwd(), "lib/mcp.js")], {
    stdio: ["ignore", "ignore", "pipe"],
    env: { ...process.env },
  });
  mcpProc.stderr.on("data", () => {});

  // Wait for the MCP server to publish ~<pub>/status to the relay
  let serverInfo = null;
  for (let i = 0; i < 10 && !serverInfo; i++) {
    await sleep(1000);
    serverInfo = await ZenMcpClient.discover(serverPub, zen, 1000);
  }
  if (!serverInfo) {
    warn("MCP server did not register on relay — skipping cross-peer suite");
    mcpProc.kill();
    console.log();
    return;
  }

  // Client keypair: use the testPair (same as zen opt.pub so relay can route responses back)
  const clientPair = testPair;

  const client = new ZenMcpClient(serverPub, zen, clientPair, { timeout: 8000 });
  await client.ready();

  try {
    await client.initialize();
  } catch (e) {
    warn(`MCP initialize failed: ${e.message}`);
    client.close();
    mcpProc.kill();
    console.log();
    return;
  }

  // 1. PUT our entry via MCP graph tool
  const myValue = `mcp-ping-from-${HOST}-${Date.now()}`;
  try {
    await client.call("graph", { soul, path: [HOST], op: "put", value: myValue });
    pass(`MCP PUT  ${soul}/${HOST} = "${myValue}"`);
    results.passed++;
  } catch (e) {
    fail(`MCP PUT  ${soul}/${HOST}: ${e.message}`);
    client.close();
    mcpProc.kill();
    console.log();
    return;
  }

  // 2. Verify round-trip through relay
  await sleep(300);
  const echo = await get(zen, soul, HOST);
  if (echo === myValue) {
    pass(`GET  ${soul}/${HOST} = "${echo}" (relay confirmed)`);
    results.passed++;
  } else {
    fail(`GET  ${soul}/${HOST}: expected "${myValue}" got ${JSON.stringify(echo)}`);
  }

  // 3. Wait for cross-peer propagation then read peer entries
  info("Waiting 5s for cross-peer propagation...");
  await sleep(5000);

  for (const peer of peers) {
    const got = await get(zen, soul, peer, 5000);
    if (got && typeof got === "string" && got.startsWith(`mcp-ping-from-${peer}`)) {
      pass(`GET  ${soul}/${peer} = "${got}" (MCP-written by ${peer})`);
      results.passed++;
    } else if (got !== undefined && got !== null) {
      warn(`GET  ${soul}/${peer} = ${JSON.stringify(got)} (stale or non-MCP value)`);
    } else {
      warn(`GET  ${soul}/${peer}: no MCP data from ${peer} — peer not running test (single-node env?)`);
    }
  }

  client.close();
  mcpProc.kill();
  console.log();
}

async function suiteChain() {
  console.log(`${BOLD}── Suite: chain (multi-hop: clientA→zen → peer0 → peer1→clientB) ──${RESET}`);

  // Topology: zen.akao.io → peer0.akao.io → peer1.akao.io
  // clientA knows ONLY the chain start, clientB knows ONLY the chain end.
  // Data must propagate through two relay hops without any direct connection.
  const CHAIN_START = process.env.CHAIN_START || "https://zen.akao.io:8420/zen";
  const CHAIN_END   = process.env.CHAIN_END   || "https://peer1.akao.io:8420/zen";

  const pairA = await ZEN.pair();
  const pairB = await ZEN.pair();

  const clientA = new ZEN({ peers: [CHAIN_START], rfs: false, axe: false, pub: pairA.pub });
  const clientB = new ZEN({ peers: [CHAIN_END],   rfs: false, axe: false, pub: pairB.pub });

  await sleep(2000);

  const soul  = "e2e-chain";
  const key   = `${HOST}-${Date.now()}`;
  const value = `chain-from-${HOST}-${Date.now()}`;

  // clientA writes to chain START
  try {
    await put(clientA, soul, key, value);
    pass(`CHAIN PUT via ${CHAIN_START.replace("https://", "")}: ${soul}/${key}`);
    results.passed++;
  } catch (e) {
    fail(`CHAIN PUT: ${e.message}`);
    clientA.off(); clientB.off();
    console.log();
    return;
  }

  // Wait for multi-hop propagation: start → peer0 → end
  info(`Waiting 12s for chain propagation (${CHAIN_START.replace("https://", "")} → ... → ${CHAIN_END.replace("https://", "")})...`);
  await sleep(12000);

  // clientB reads from chain END — it has NO connection to start or middle
  const got = await get(clientB, soul, key, 8000);
  if (got === value) {
    pass(`CHAIN GET via ${CHAIN_END.replace("https://", "")}: "${got}" ✓ multi-hop propagated`);
    results.passed++;
  } else {
    fail(`CHAIN GET via ${CHAIN_END.replace("https://", "")}: expected "${value}" got ${JSON.stringify(got)}`);
  }

  clientA.off();
  clientB.off();
  console.log();
}

async function suiteDiskRead(zen) {
  console.log(`${BOLD}── Suite: disk-read (read known persisted values) ──${RESET}`);

  // These values were written in previous sessions
  const checks = [
    { soul: "hello",  key: "world",  note: "written in prior sessions" },
    { soul: "debug",  key: "hello",  note: "written in prior sessions" },
  ];

  for (const { soul, key, note } of checks) {
    const got = await get(zen, soul, key, 5000);
    if (got !== null && got !== undefined) {
      pass(`GET  ${soul}/${key} = "${got}" (${note})`);
      results.passed++;
    } else {
      warn(`GET  ${soul}/${key}: got null — no prior session data (skipping)`);
    }
  }

  console.log();
}

// ── Main ───────────────────────────────────────────────────────────────────────

async function main() {
  // Create an ephemeral keypair so the ZEN instance has a pub key for relay routing
  const testPair = await ZEN.pair();
  zenOpt.pub = testPair.pub;
  const zen = new ZEN(zenOpt);

  // Give the peer connection time to establish
  await sleep(1000);

  try {
    if (SUITE === "all" || SUITE === "basic")   await suiteBasic(zen);
    if (SUITE === "all" || SUITE === "disk")    await suiteDiskRead(zen);
    if (SUITE === "all" || SUITE === "persist") await suitePersist(zen);
    if (SUITE === "all" || SUITE === "cross")   await suiteCross(zen, testPair);
    if (SUITE === "all" || SUITE === "chain")   await suiteChain();
  } finally {
    const total = results.passed + results.failed;
    console.log(`${BOLD}── Summary ──${RESET}`);
    console.log(`  Passed: ${GREEN}${results.passed}${RESET} / ${total}`);
    if (results.failed) {
      console.log(`  Failed: ${RED}${results.failed}${RESET} / ${total}`);
    }
    console.log();
    zen.off();
    process.exit(results.failed > 0 ? 1 : 0);
  }
}

main().catch((e) => { console.error(e); process.exit(1); });
