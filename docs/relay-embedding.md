# Embedding a ZEN Relay — Custom Server Guide

This guide is for projects that embed ZEN as a library and run their own HTTP/WebSocket server (like [`scobru/delay`](https://github.com/scobru/delay)). Using `setupRelayPex` gives you correct Peer Exchange out of the box — including IPv4-only relay support, re-gossip of reconnecting peers, and self-URL announcement.

## Quick Start

```js
import ZEN from "zen";
import { setupRelayPex } from "zen/lib/pex";
import { PeerRegistry } from "zen/lib/peer-registry";

// 1. Create ZEN with your bootstrap peers
const zen = ZEN({ peers: ["wss://zen.akao.io:8420/zen"] });

// 2. Wire up PEX (call before your HTTP server starts)
const registry = new PeerRegistry();

const { origin, adopt } = setupRelayPex(zen, {
  domain: process.env.DOMAIN || null,   // e.g. "shogun.scobru.io"; null = IP-only relay
  port:   8765,
  key:    null,                          // set to TLS key object for wss://
  registry,
  pexMax: 50,
});

console.log("Self URL:", origin);       // e.g. "ws://shogun.scobru.io:8765/zen"
```

That's it. `setupRelayPex` wires:

| What | How |
|------|-----|
| Receive peer lists from other relays | `mesh.hear["pex"]` handler |
| Send our peer list to new connections | `root.on("hi")` → `mesh.say` |
| Announce our own URL to new peers | `root.on("hi")` → `mesh.say` |
| Discover public IPv4/IPv6 self-URL | `disc()` every 10 min |
| Gossip reconnecting peers | once-per-session re-gossip via `gossiped` Set |

## IPv4-Only Relays (no domain)

Set `domain: null`. `setupRelayPex` will call `disc()` to discover your public IPv4 address and build `ws://<ip>:<port>/zen` as the self-URL automatically.

```js
setupRelayPex(zen, { domain: null, port: 8765 });
// → discovers e.g. "ws://164.132.57.192:8765/zen" automatically
```

## Serving `/.well-known/peers.json`

Expose the registry so other relays can bootstrap from you:

```js
import { PeerRegistry } from "zen/lib/peer-registry";

app.get("/.well-known/peers.json", (req, res) => {
  const peers = registry.bootEntries().map(e => e.url)
    .concat(registry.confirmedNonBoot().map(e => e.url));
  res.json({ peers });
});
```

## Custom Peer List Sending (UDP fast-path, bpids)

Pass a `sendPeers` function to override the default `mesh.say`:

```js
setupRelayPex(zen, {
  domain, port,
  registry,
  sendPeers: (list, peer) => {
    // example: include browser peer IDs for WebRTC discovery
    const root = zen._graph._;
    const bpids = Object.values(root.opt.peers || {})
      .filter(p => p && p.pid && !p.url && p.pid !== peer.pid)
      .map(p => p.pid);
    const msg = { dam: "pex", peers: list };
    if (bpids.length) msg.bpids = bpids;
    mesh.say(msg, peer); // or your UDP fast-path here
  },
});
```

## IP Discovery Updates (for `/status`)

Use `onDisc` to receive IP discovery results for your own status endpoint:

```js
let discResult = null;

setupRelayPex(zen, {
  domain, port,
  onDisc: (di) => {
    discResult = di;
    // di.ip   — public IPv4
    // di.ip6  — public IPv6 (if available)
    // di.domain — configured domain (if any)
  },
});

app.get("/status", (req, res) => {
  res.json({ ip4: discResult?.ip, ip6: discResult?.ip6 });
});
```

## Persist & Load Peers

```js
import path from "path";
import * as xdg from "zen/lib/xdg"; // or your own path

const PEERSF = path.join(xdg.data(), "peers.json");
const registry = new PeerRegistry().bindSave(PEERSF);

// After PEX is set up, load previously discovered peers:
// (inside setImmediate or after zen is ready)
setImmediate(() => {
  const count = registry.load(adopt);
  console.log(`Loaded ${count} persisted peers`);
});
```

## Full Example

See [`script/server.js`](../script/server.js) for the canonical relay implementation that uses all of these features.

## API Reference

### `setupRelayPex(zen, opts)`

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `domain` | `string\|null` | `null` | Relay domain. `null` → discover public IP. |
| `port` | `number` | `8420` | WebSocket port for self-URL construction. |
| `key` | `object\|null` | `null` | TLS key → `wss://` if truthy, `ws://` if falsy. |
| `registry` | `PeerRegistry` | new instance | Shared peer registry. |
| `pexMax` | `number` | `50` | Max peers sent to new connections. |
| `rttOf` | `fn(url)→ms` | `null` | RTT sorter for peer list quality ordering. |
| `sendPeers` | `fn(list, peer)` | `null` | Override default `mesh.say` for peer list send. |
| `onDisc` | `fn(discResult)` | `null` | Called after each IP discovery run. |
| `onAdopt` | `fn(url)` | `null` | Called when a new peer URL is added to registry. |

Returns: `{ origin, registry, adopt, getSelfUrl }`
