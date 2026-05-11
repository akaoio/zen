# CHANGELOG

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
