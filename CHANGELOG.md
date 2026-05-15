# CHANGELOG

## 1.0.25 — 2026-05-15

### CLI & Browser Bootstrap

- **Fixed `zen` CLI crash on empty args** (`script/zen.sh`): `shift || true` fails in POSIX `dash` under `set -eu` when `$# = 0`. Changed to `if [ "$#" -ge 1 ]; then shift; fi`. Commit `33f70fe`.
- **Eliminated browser console peer-scan spam** (`src/axe.js`): `bscan()` was probing `peer0..peer100` via blind WebSocket connections, flooding the browser console with `ERR_CONNECTION_REFUSED`. WebSocket errors cannot be suppressed via try/catch — the browser logs them at the network layer. Replaced the entire `bscan()` approach with a single `fetch({origin}/status)` call (already CORS-enabled), which fails silently and returns known relay peers. Commit `33f70fe`.
- **Removed `browser.js`** (`bde5b40`): outdated demo shim that imported `zen.min.js` + lib addons. `index.html` now imports all modules directly. `BROWSER_RUNTIME` object defined inline; `optAxe.disabled` set to `false` (AXE is now always built-in). FAQ and code snippets updated to reflect that AXE is part of the core bundle, not a separate add-on.

### Memory Leaks Fixed

- **`src/on.js`** (`bc17eef`): `trackSub()` now self-splices from `eas.subs[]` on teardown. `chain.off()` clears `once` timers, `any` listeners, orphaned `subs`, and the `jam` queue — preventing subscription accumulation on long-running relay processes.
- **`src/websocket.js`** (`bc17eef`): Extracted `canReconnect()` null-safe guard. `reconnect()` always nulls `peer.defer` after `clearTimeout`. Inner `setTimeout` re-checks `canReconnect()` and stores handle back into `peer.defer`. `onopen` clears `peer.defer` immediately — previously timers could accumulate and fire after the connection was already alive.
- **`src/axe.js` bye handler** (`bc17eef`): Added `clearTimeout(peer.to)` + `clearTimeout(peer.defer)` on peer disconnect. Nulls `peer.next`/`peer.put`. Iterates `peer.sub`, removes the dead peer from each soul's `ref.route` Map, then nulls `peer.sub` — previously dead peers remained in route maps forever.

### Dead Code & Drift Cleanup

- **Deleted `src/scan.js`** (`a5b5b30`): file had no importers after `axe.js` cleanup.
- **Deleted `lib/radisk2.js`, `lib/radix2.js`** (and `.min.js`) (`a5b5b30`): orphaned alternate implementations with no importers.
- **Removed from `src/axe.js`** (`a5b5b30`): dead `bscan()` function, `scpat` variable, `import { mkpat, candidateHosts } from "./scan.js"`, and orphaned `relayUp()` function.
- **Minor cleanup in `lib/radisk.js`** (`a5b5b30`): removed dead empty branch and orphaned debug log.
- **Fixed misleading comment `src/root.js:120`** (`5061fd1`): prior comment said `TODO BUG` on the early `return` for `put["#"] && put["."]`. This is the leaf format emitted by `ham()` and IS correct — `map.js` handles the graph write from the same event; the `return` prevents double-processing. Comment replaced with correct explanation.
- **Removed dead `&& false` block in `src/mesh.js`** (`5061fd1`): permanently-disabled hash-compute block using the old `Type.obj.hash` API (replaced long ago by `String.hash` in `shim.js`). Block deleted entirely.

### WebRTC Browser-to-Browser P2P — Full Implementation

This release completes the WebRTC DataChannel layer, enabling browsers to communicate directly without routing through the relay once connected. The relay is still used as signaling server and as fallback.

**Bug 1 — Relay never sent `bpids` in PEX** (`530bc0a`)
- `lib/webrtc.js` had a `mesh.hear["pex"]` hook consuming `msg.bpids` (array of browser peer IDs) to auto-initiate WebRTC connections — but the relay never populated this field.
- Fix in `src/axe.js`: in the relay-side `mesh.hear["?"]` override, after registering a new inbound browser client in `axe.up`, relay now:
  1. Sends `{ dam:"pex", peers:[relay URLs], bpids:[existing browser pids] }` to the newly connected browser.
  2. Broadcasts `{ dam:"pex", bpids:[new browser pid] }` to all other currently-connected browsers.
- Both directions now learn each other's pid. The "lower pid initiates" rule in `webrtc.js` (`if (opt.pid >= pid) return`) prevents simultaneous double-offer races.

**Bug 2 — `drop()` returned early for open DataChannels** (`4bdb38a`)
- `dc.onopen` deletes `pcs[pid]` (pending) and calls `mesh.hi(pc)` to add the DataChannel peer to `opt.peers`. But the old `drop()` only checked `pcs[pid]` — when it found nothing there, it returned without cleaning up the wired `opt.peers` entry. Dead DataChannels accumulated indefinitely.
- Fix: `drop()` now checks both `pcs[pid]` (pending) and `opt.peers[pid]` (wired), calling `mesh.bye()` on wired peers.
- `pc.say` now checks `dc.readyState === "open"` before sending; calls `drop(pid)` on failure so traffic falls back to the relay WebSocket automatically.
- `dc.onerror` now calls `drop(pid)` (was empty).

**Bug 3 — `mesh.hear["rtc"]` never registered** (`b5ec256`)
- `src/mesh.js` `hear.one()` dispatches all `dam:X` messages via `mesh.hear[X]` and **always returns early** — `mesh.way` is never called for `dam` messages.
- `axe.js` had wired `rtcway` into `mesh.way` instead of `mesh.hear["rtc"]`, so WebRTC signaling (offer/answer/ICE) was never routed by the relay.
- Fix: added `mesh.hear["rtc"] = rtcway` in `src/axe.js` immediately after the `rtcway` function definition. After this fix, offer and answer SDPs are exchanged correctly.

**Bug 4 — ICE candidates serialized as `[object Object]`** (`2d3c5f2`)
- After fixing routing, DataChannels still never opened. Playwright WebSocket spy revealed the wire payload literally contained `"candidate":[object Object]` — invalid JSON at the source.
- Root cause: `RTCIceCandidate` (Chrome Web API) stores `candidate`, `sdpMid`, `sdpMLineIndex`, `usernameFragment` as **prototype getters**, not own enumerable properties. `JSON.stringify(iceCandidate)` works when called directly (invokes `toJSON()`), but when the object is nested inside a larger GUN message and serialized via ZEN's JSON pipeline, the prototype getters are not accessed and the object coerces to the string `[object Object]`.
- Fix: `sig({ candidate: e.candidate.toJSON(), ... })` — call `.toJSON()` at the source to produce a plain serialisable object before embedding in the message.
- Result: ICE candidates arrive correctly; `addIceCandidate()` succeeds; DataChannels open.

**Verified end-to-end with Playwright:**
- 2-tab localhost test: both peers reach `RTC ↔ ... (direct)` in ~7 s ✅
- 3-tab localhost test: full mesh (6/6 DataChannels) in ~8 s ✅
- 3-machine cross-relay test (zen.akao.io, peer0.akao.io, peer1.akao.io): full mesh (6/6 DataChannels) in ~9 s ✅

### MAX_RTC_PEERS Cap (`a28e1dd`)

Without a connection limit, every browser connects to every other browser — O(n²) DataChannels. At 100 browsers this is 4,950 simultaneous RTCPeerConnections; Chrome crashes well before that. `opt.rtc.max = 55` was already declared (WebTorrent convention, well under Chrome's ~256 hard limit) but was never enforced.

- Added `dcPeers = {}` to track confirmed-open DataChannels separately from `pcs` (pending connections).
- Added `rtcCount()` helper: `Object.keys(pcs).length + Object.keys(dcPeers).length` — counts all active WebRTC slots (pending + wired).
- `dc.onopen` now sets `dcPeers[pid] = true`; `drop()` clears it.
- **Initiator guard**: `opt.rtc.connect()` bails if `rtcCount() >= opt.rtc.max`.
- **Responder guard**: incoming offer handler bails if `!pcs[pid] && rtcCount() >= opt.rtc.max` — respects already-in-progress connections, rejects new ones when at capacity.
- **Random peer selection**: `msg.bpids` is Fisher-Yates shuffled in the PEX handler before iterating — when at capacity, the relay picks a random subset of available peers rather than always the same (alphabetically first) peers.

## 1.0.22

- **All shell scripts POSIX-compliant** (`script/install.sh`, `update.sh`, `uninstall.sh`, `zen.sh`, `ssl.sh`, `mcp.sh`): changed shebang to `#!/bin/sh`; replaced all bash-specific syntax (`[[ ]]` → `[ ]`, `echo -e` → `printf`, `$EUID` → `$(id -u)`, bash arrays → inline loops, `grep -oP` → `sed`, `${var//k/v}` → `sed`). Scripts now run on any POSIX-compatible shell including `dash`. `nvm.sh` is loaded via `bash -c ". ~/.nvm/nvm.sh && cmd"` since nvm itself requires bash.
- **`install.sh` auto-detects SSL certs**: if `--https-key`/`--https-cert` are not specified, `install.sh` automatically uses `$XDG_CONFIG_HOME/zen/key.pem` and `$XDG_CONFIG_HOME/zen/cert.pem` (the paths where `ssl.sh` saves certificates). Run `ssl.sh` once, then `install.sh` — no extra flags needed.
- **`install.sh` new flags**: `--yes`/`-y` skips all interactive prompts (safe for SSH/piped installs); `--skip-deps` skips the `apt-get install nodejs` step (useful when Node.js is installed via nvm); `--https-key`/`--https-cert` for explicit cert paths.
- **`install.sh` correctness fixes under `sudo`**: added `REAL_USER="${SUDO_USER:-$(id -un)}"` so the systemd `User=` field is set to the actual login user (not `root`) when running via `sudo`; all `command -v` lookups made safe under `set -e` with `|| true`; nvm node resolution uses `bash -c '. ~/.nvm/nvm.sh && command -v node'` to get the nvm default version (not newest installed).
- **Service renamed**: systemd unit is now `zen.service` (was `relay.service`); auto-update timer is `zen-update.timer`. The `zen` CLI default service name updated accordingly.
- **`zen` CLI extended**: added `zen start`, `zen stop`, `zen restart` (via sudoers NOPASSWD) and `zen logs` (tails `journalctl -u zen -f`).
- **MCP `relay` tool renamed to `status`**: the MCP tool that returns relay health is now called `status`, matching the HTTP `GET /status` endpoint.
- **Sudoers NOPASSWD rules**: `install.sh` creates `/etc/sudoers.d/zen` granting passwordless `systemctl start/stop/restart zen` and CLI install operations, so `zen start/stop/restart` and `zen update` work without a password prompt.

## 1.0.10

- **`root.graph` GC eviction** (`script/server.js`): relay now evicts in-memory graph nodes when heap exceeds `GRAPH_GC_MB` (default 400 MB). Eviction skips souls with active `on()` listeners (`root.next[soul]`) and souls written in the last `GRAPH_GC_KEEP` seconds (default 120 s). All data remains on disk (RAD); eviction only causes cache misses. Configurable via `GRAPH_GC_MB`, `GRAPH_GC_SEC`, `GRAPH_GC_KEEP` env vars.
- **Fix: cluster worker crash loop** (`lib/mcp/server.js`): the MCP stdio transport registered a `process.stdin.once("end", process.exit)` handler to clean up on stdin close. In cluster workers, `process.stdin` is `/dev/null` — not a TTY and not a real pipe — so the `end` event fired immediately, killing the worker with `code 0`. Fixed by adding `!cluster.isWorker` to the stdio activation guard. The IPC (Unix socket) transport continues to work in workers.
- **UDP throughput benchmark** (`script/bench.js`): new benchmark script for the relay mesh. Connects a sender and receiver to a relay, sends N messages via `mesh.relay()` (which uses the UDP fast path when available), and reports throughput and RTT percentiles. Usage: `node script/bench.js [relay_url] [n_messages] [batch_size]`. Baseline: 2109 msg/s, p50=18 ms, p99=101 ms, 0% loss over localhost.



**Breaking changes** — compact wire format replaces JSON-wrapper format for all signed/encrypted values.

- **Compact wire format for signed values**: `ZEN.sign()` now returns `<86-char base62 sig><v>:<message>` (secp256k1) or `<86-char base62 sig><v>/curve:<message>` (other curves) instead of the previous `{"m":...,"s":...,"v":...}` JSON object format. This is a **breaking change** — old signed strings from prior versions will not verify.
- **Compact wire format for encrypted values**: `ZEN.encrypt()` now returns `<ct_b64url>:<iv_b64url>:<s_b64url>` (three colon-separated URL-safe base64 parts) instead of the previous JSON/object format.
- **New pair schema**: pair objects now have four fields — `{curve, pub, priv, address}`. Removed `epub`/`epriv` fields. Encrypt/decrypt operations use `pub`/`priv` directly via ECDH.
- **Compact certificate format**: `ZEN.certify()` now returns a compact signed string directly (not wrapped in a JSON object). Use `ZEN.verify(cert, pub)` to read cert payload.
- **`base62.js` new exports**: `bufToB62Fixed(buf, len)` and `b62ToBuf(s, byteLen)` — encode/decode fixed-length base62 for signature transport.
- **`settings.parse()` updated**: detects compact signed strings (position-based) and compact encrypted strings (3-part base64url). Detection order: signed first, then encrypted, then JSON.parse fallback.
- **`settings.unpack(m)` note**: expects an object/raw value — not a JSON string. Callers must `JSON.parse(m)` when `m` comes from `settings.parse().m`.
- **Security middleware fix** (`src/security.js`): `check.auth` now correctly handles the compact-format `m` field (JSON string → parsed object before `unpack`); `check.$vfy` now accepts compact cert strings (was silently rejecting string certs due to `certificate.m` guard).

## 1.0.8

- **POSIX XDG Base Directory compliance**: ZEN now follows the [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/) for all runtime paths, eliminating conflicts with other projects that use `~/`.
  - SSL keys/certs: `$XDG_CONFIG_HOME/zen/` (default `~/.config/zen/`) — previously `~/`
  - Graph data (radata): `$XDG_DATA_HOME/zen/radata/` (default `~/.local/share/zen/radata/`) — previously `./radata` in CWD
  - Stats file: `$XDG_STATE_HOME/zen/stats.radata` (default `~/.local/state/zen/`) — previously at repo root
  - Password file: `$XDG_CONFIG_HOME/zen/pass` — previously `~/pass`
  - All directories are created automatically on first use.
- **New `lib/xdg.js`**: shared XDG path resolution module; exports `config()`, `data()`, `state()`, `ensure()` helpers used by `lib/rfs.js`, `lib/stats.js`, `lib/service.js`, and `script/server.js`.
- **Migration note**: existing `~/key.pem`/`~/cert.pem` should be moved to `~/.config/zen/`; existing `./radata/` should be moved to `~/.local/share/zen/radata/`.
- **Test isolation**: all test npm scripts now set `GUN_TEST_TMP=1` to keep test data in `tmp/` and prevent cross-contamination with production XDG dirs.

## 1.0.7

- **AXE consolidation**: merged browser peer discovery and Node.js relay into single `lib/axe.js`; deleted root `axe.js` which was dead code (browser block was unreachable due to `root.axe` guard in `lib/axe.js` running first).
- **Import fix**: `lib/server.js` now imports `./axe.js` (local `lib/`) instead of `../axe.js` (root), fixing a broken `../axe.min.js` reference in the minified build.
- **Stats fix**: removed stale `typeof require === "undefined"` guard in `lib/stats.js` that prevented `stats.radata` from ever being written in ES module context (Node.js ESM has no `require`).
- **Multicast IPv6**: added `udp6` socket alongside `udp4` using `ff02::1` link-local multicast; refactored into shared `setupSocket()` helper to eliminate duplication; fixed interface detection using `fe80::addr%ifaceName` zone-ID format required by libuv for IPv6 `addMembership`; `setBroadcast`/`setMulticastTTL` guarded to IPv4 only; `ipv6Only: true` on `udp6` to avoid dual-stack port conflicts.
- **Multicast IPv4**: fixed `addMembership` using explicit interface IP from `os.networkInterfaces()` instead of letting OS default to a DOWN interface (fixes ENODEV on Orange Pi / non-standard interface names).
- **Build**: renamed `lib/uglify.js` → `lib/minify.js`; npm script `uglify` → `minify`; `build:release` updated accordingly.

## 0.2020.x

`>0.2020.520` may break in-process `gun1` `gun2` message passing. Check `test/common.js` "Check multi instance message passing" for a hint and/or complain on community chat.

- No breaking changes to core API.
- Storage adapter `put` event breaking change (temporary?), RAD is official now and storage adapters should be RAD plugins instead of GUN adapters.
- GUN soul format changed from being a random UUID to being a more predictable graph path (of where initially created) to support even better offline behavior. This means `null`ing & replacing an object will not create a new but re-merge.
- Pretty much all internal GUN utility will be deleted, these are mostly undocumented but will affect some people - they will still be available as a separate file but deprecated.
- As the DHT gets implemented, your relay peers may automatically connect to it, so do not assume your peer is standalone. `Gun({axe: false` should help prevent this but loses you most scaling properties.
- The 2019 -> 2020 "changes" are happening gradually, based on experimental in-production tests.
- As always, **most important** is to ask in the [community chat](http://chat.gun.eco) if you have any issues, and to keep up to date with changes.

## 0.2019.x

Some RAD & SEA data format changes, but with as much backward compatibility as possible, tho ideally should be dropped.

## 0.9.x

No breaking changes, but the new Radix Storage Engine (RSE) has been finally integrated and works with S3 as a backup.

// Edit: commentary removed.

## 0.8.x

Adapter interfaces have changed from `Gun.on('event', cb)` to `gun.on('event', cb)`, this will force adapters to be instance specific.

`.path()` and `.not()` have been officially removed from the core bundle, you can bundle them yourself at `lib/path.js` and `lib/not.js` if you still need them.

## 0.7.x

Small breaking change to `.val(cb)`:

Previously `.val(cb)` would ONLY be called when data exists, like `.on(cb)`.

However, due to popular demand, people wanted `.val(cb)` to also get called for `.not(cb)` rather than (before) it would "wait" until data arrived.

NOTE: For dynamic paths, `.val(cb)` will still wait, like:

`gun.get('users').map().val(cb)` because the behavior of the `map()` is simply to not fire anything down the chain unless items are found.

## 0.6.x

Introduced experimental features, chaining `.val()` (no callback) and `.map(cb)` behaving as a map/reduce function.

It also upgraded the socket adapters and did end-to-end load testing and correctness testing.

## 0.5.9

GUN 0.3 -> 0.4 -> 0.5 Migration Guide:
`gun.back` -> `gun.back()`;
`gun.get(key, cb)` -> cb(err, data) -> cb(at) at.err, at.put;
`gun.map(cb)` -> `gun.map().on(cb)`;
`gun.init` -> deprecated;
`gun.put(data, cb)` -> cb(err, ok) -> cb(ack) ack.err, ack.ok;
`gun.get(key)` global/absolute -> `gun.back(-1).get(key)`;
`gun.key(key)` -> temporarily broken;

## 0.3.7

- Catch localStorage errors.

## 0.3.6

- Fixed S3 typo.

## 0.3.5

- Fixed server push.

## 0.3.4

- Breaking Change! `list.set(item)` returns the item's chain now, not the list chain.
- Client and Server GUN servers are now more up to spec, trimmed excess HTTP/REST header data.
- Gun.is.lex added.

## 0.3.3

- You can now link nodes natively, `gun.get('mark').path('owner').put(gun.get('cat'))`!
- Sets (or tables, collections, lists) are now easily done with `gun.get('users').set(gun.get('person/mark'))`.

## 0.3.2

Bug fixes.

## 0.3.1

Bug fixes.

## 0.3

Migration Guide! Migrate by changing `.attach(` to `.wsp(` on your server if you have one with gun. Remove `.set()` (delete it), and change `.set($DATA)` (where you call set with something) to `.path('I' + Date.now() + 'R' + Gun.text.random(5)).put($DATA)`. If you have NodeJS style callbacks in your `.get` (which documentation previously recommended that you shouldn't) they previous took `err, graph` and now they take `err, node` (which means now using callback style is fine to use). Inside of `.not()` no longer use `return` or `this`, instead (probably) use `gun` and no `return`. If you are a module developer, use `opt.wire` now instead of `opt.hooks` and message Mark since he needs to talk to you since the wire protocol has changed.

- Server side default `.wsp()` renamed from `.attach()`.
- `.set()` deprecated because it did a bunch of random inconsistent things. Its useful behavior has now become implicit (see below) or can be done explicitly.
- `.not()` it was previously common to `return` the chain inside of .not, beware that if you have code like `gun.get(key).not(function(){ return this.put({}).key(key) }).val()` cause `.val()` to be triggered twice (this is intentional, because it funnels two separate chains together) which previously didn't happen. To fix this, just don't return the chain.
- `.put()` and `.path()` do implicit `.init()` by default, turn on explicit behavior with `Gun({init: true})`.
- `.get(soul, cb)` cb is called back with `err, node` rather than `err, graph`.
- Options `opt.wire` renamed from `opt.hooks`.
- `.val()` when called empty automatically cleanly logs for convenience purposes.
- `.init()` added.
- `Gun.is.val` renamed from `Gun.is.value`.
- `Gun.is.rel` renamed from `Gun.is.soul`.
- `Gun.is.node.soul` renamed from `Gun.is.soul.on`.
- `Gun.union.ify` renamed from `Gun.union.pseudo`.
- `Gun.union.HAM` renamed from `Gun.HAM`.
- `Gun.HAM` is now the actual HAM function for conflict resolution.
- `Gun._.state` renamed from `Gun._.HAM`.
- Maximum Callstack Exceeded is less problematic now, unless you intentionally choke the thread. #95
- Putting a regex or Date or NaN is actually detected and causes an error now while before it was silent. #122 #123
- `.on()` gets called when a key is later newly made while before it did not. #116
- `.val()` should not ever get called with a relation alone (internals should resolve it), this is fixed. #132
