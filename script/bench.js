#!/usr/bin/env node
/**
 * bench.js — UDP vs WebSocket throughput benchmark for ZEN relay mesh
 *
 * Connects a sender and a receiver to the relay, sends N push() messages
 * via mesh.relay() (which uses UDP fast path when available), and reports
 * throughput and round-trip latency.
 *
 * Usage:
 *   node script/bench.js [relay_url] [n_messages] [batch_size]
 *
 * Examples:
 *   node script/bench.js https://zen.akao.io:8420/zen 1000 20
 *   node script/bench.js http://localhost:8420/zen 500 1
 *
 * Environment overrides:
 *   BENCH_RELAY=https://zen.akao.io:8420/zen
 *   BENCH_N=1000
 *   BENCH_BATCH=20
 */

// Disable TLS verification — relay uses Let's Encrypt but hostname may not
// match when connecting from localhost or within the same machine.
process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0';

const relay = process.argv[2] || process.env.BENCH_RELAY || 'https://localhost:8420/zen';
const N     = parseInt(process.argv[3] || process.env.BENCH_N     || '1000');
const BATCH = parseInt(process.argv[4] || process.env.BENCH_BATCH || '20');

const { default: ZEN } = await import('../index.js');

console.log(`\n=== ZEN push() benchmark ===`);
console.log(`  Relay       : ${relay}`);
console.log(`  Messages    : ${N}`);
console.log(`  Batch size  : ${BATCH}`);
console.log();

// ── keypairs ──────────────────────────────────────────────────────────────
const senderPair   = await ZEN.pair();
const receiverPair = await ZEN.pair();

// ── create ZEN instances with known pub keys ──────────────────────────────
// pub in opts is advertised in dam:"?" handshake — lets the relay route to us.
// axe: false — disable AXE so relay doesn't manage/close benchmark connections.
// rfs: false — thin peer with no local disk store.
const sender   = new ZEN({ peers: [relay], pub: senderPair.pub,   rfs: false, axe: false });
const receiver = new ZEN({ peers: [relay], pub: receiverPair.pub, rfs: false, axe: false });

// ── wait for mesh to attach, then force both peers to connect to relay ────
const [sMesh, rMesh] = await Promise.all([getMesh(sender), getMesh(receiver)]);
connectPeers(sender);
connectPeers(receiver);

// ── subscribe receiver ────────────────────────────────────────────────────
const received   = new Map(); // id → latency
const latencies  = [];

rMesh.onRelay(({ data }) => {
  try {
    const { id, t } = JSON.parse(data);
    const rtt = Date.now() - t;
    latencies.push(rtt);
    received.set(id, rtt);
  } catch (_) {}
});

// ── settle (WS + UDP handshake) ───────────────────────────────────────────
process.stdout.write('Waiting for peers to settle...');
await sleep(2000);
console.log(' ok');

// ── warm-up ───────────────────────────────────────────────────────────────
process.stdout.write('Warm-up (20 messages)...');
await send(sMesh, receiverPair.pub, 20, 5);
await sleep(300);
latencies.length = 0;
received.clear();
console.log(' ok');

// ── benchmark ─────────────────────────────────────────────────────────────
console.log(`Sending ${N} messages in batches of ${BATCH}...`);
const t0 = Date.now();
await send(sMesh, receiverPair.pub, N, BATCH);
// wait for in-flight to arrive (up to 2 s)
await waitFor(() => received.size >= N * 0.99, 2000);
const elapsed = Date.now() - t0;

// ── stats ─────────────────────────────────────────────────────────────────
const recv  = received.size;
const lost  = N - recv;
const tput  = (recv / (elapsed / 1000)).toFixed(1);

latencies.sort((a, b) => a - b);
const p50 = pct(latencies, 0.50);
const p95 = pct(latencies, 0.95);
const p99 = pct(latencies, 0.99);
const avg = latencies.length
  ? (latencies.reduce((s, v) => s + v, 0) / latencies.length).toFixed(1)
  : '-';

console.log();
console.log('=== Results ===');
console.log(`  Sent        : ${N}`);
console.log(`  Received    : ${recv}  (lost: ${lost}  ${(lost/N*100).toFixed(1)}%)`);
console.log(`  Elapsed     : ${elapsed} ms`);
console.log(`  Throughput  : ${tput} msg/s`);
console.log(`  RTT avg     : ${avg} ms`);
console.log(`  RTT p50/p95/p99: ${p50}/${p95}/${p99} ms`);
console.log();
process.exit(0);

// ── helpers ───────────────────────────────────────────────────────────────

async function send(mesh, targetPub, count, batchSz) {
  for (let i = 0; i < count; i += batchSz) {
    const end = Math.min(i + batchSz, count);
    for (let j = i; j < end; j++) {
      mesh.relay(targetPub, JSON.stringify({ id: j, t: Date.now() }));
    }
    await sleep(0); // yield to event loop between batches
  }
}

function getMesh(zenInst) {
  return new Promise((resolve) => {
    const check = () => {
      const m = zenInst._opt && zenInst._opt.mesh;
      if (m) return resolve(m);
      setTimeout(check, 50);
    };
    check();
  });
}

function connectPeers(zenInst) {
  const mesh = zenInst._opt.mesh;
  const peers = zenInst._opt.peers || {};
  for (const k in peers) {
    const p = peers[k];
    if (p && p.url && !p.wire) mesh.hi(p);
  }
}

function waitFor(pred, timeout) {
  return new Promise((resolve) => {
    const start = Date.now();
    const tick = () => {
      if (pred() || Date.now() - start >= timeout) return resolve();
      setTimeout(tick, 20);
    };
    tick();
  });
}

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function pct(sorted, p) {
  if (!sorted.length) return '-';
  return sorted[Math.min(Math.floor(sorted.length * p), sorted.length - 1)];
}
