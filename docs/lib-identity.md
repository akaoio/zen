# Identity Module

## Overview

The `lib/identity.js` module provides deterministic identity generation for ZEN servers and MCP instances.
It supports both portable seed-based identities and machine-locked hardware-derived identities.

## Features

- **Deterministic**: Same input entropy yields the same keypair
- **Portable mode**: Explicit seed or identity file works across machines
- **Hardware mode**: Derived from `/etc/machine-id`, MAC address, and hostname
- **Shared**: Server and MCP can derive the same identity on the same machine
- **Runtime-only**: Identity calculated on demand, never persisted by this module
- **Secure**: Private keys and seed exist only in process memory

## API

### `getOrCreateIdentity()`

Returns a deterministic identity using the following priority order:

1. `opt.seed`
2. `ZEN_IDENTITY_SEED`
3. `--identity-file <path>` from `process.argv`
4. `opt.identityFile`
5. hardware fingerprint fallback

```javascript
import { getOrCreateIdentity } from "./lib/identity.js";

const identity = await getOrCreateIdentity();
// → { pair: { pub, priv, address, curve }, seed, hwid, mode }
```

**Returns:**
- `pair`: `{ curve, pub, priv, address }`
- `seed`: Base62-encoded SHA-256 hash of the selected input entropy
- `hwid`: Raw hardware identity string in hardware mode, else `null`
- `mode`: `"seed"` or `"hardware"`

**Returns `null`** if no seed source is provided and hardware entropy is unavailable.

**Security:** Identity is calculated fresh on every call. Nothing is persisted to disk.

### `getIdentity()`

Alias for `getOrCreateIdentity()` for backward compatibility.

```javascript
import { getIdentity } from "./lib/identity.js";

const identity = await getIdentity();
// → { pair, seed, hwid } or null
```

## Usage

### In a ZEN server

```javascript
import { getOrCreateIdentity } from "../lib/identity.js";

const identity = await getOrCreateIdentity();
if (identity) {
  zen = new ZEN({ pid: identity.pair.pub, ... });
   // Use identity.pair for authenticated operations
}
```

### In ZEN MCP (`lib/mcp.js`)

```javascript
import { getOrCreateIdentity } from "./identity.js";

const localIdentity = await getOrCreateIdentity();
if (localIdentity) {
   zenOpt.pid = localIdentity.pair.pub;
   pairStore.set("self", localIdentity.pair);
}
```

MCP users can then use `pairId: "self"` in crypto/graph/protocol operations to sign with the local MCP identity.

## Seed sources and hardware fallback

Identity resolution order:

1. explicit `opt.seed`
2. `ZEN_IDENTITY_SEED`
3. `--identity-file`
4. `opt.identityFile`
5. hardware fingerprint

When the module falls back to hardware mode, the hardware ID is derived from:

The hardware ID is derived from (in order of priority):

1. **`/etc/machine-id`** or `/var/lib/dbus/machine-id`
   - 128-bit UUID set at OS install time
   - Never changes unless OS is reinstalled

2. **First non-loopback MAC address**
   - Read from `/sys/class/net/*/address`
   - Sorted alphabetically for determinism
   - Filters out `00:00:00:00:00:00`

3. **Hostname**
   - Fallback entropy source
   - Combined with above sources

Format: `machine-id|MAC|hostname` (joined with `|`)

## Key derivation

```
hwid() → "96e66a7085...|00:0d:3a:9a:da:5b|hostname"
   ↓
SHA-256 + base62 encode
   ↓
seed = "03EdUg7H7Y9bhcPj7XU2OWerQQ6hvdD7iFl04WEDuvsJ"
   ↓
ZEN.pair(null, { seed })
   ↓
{ pub, priv, address, curve }
```

The resulting seed is passed into `ZEN.pair(null, { seed })`, which derives the signing keypair according to the current curve implementation.

## Security Model

**This module does not persist identity state.**

- Identity is calculated fresh from the chosen seed source on every call
- Private keys exist only in process memory during execution
- Seed (base62-encoded hash) exists only in memory
- Public keys are never written to disk
- Hardware fingerprint (hwid) is never logged or persisted

This approach eliminates entire classes of attacks:
- No keyfiles to steal or misconfigure
- No accidental backup of sensitive material
- No permission issues with key storage
- Keys cannot be moved to another machine

In hardware mode, the identity is machine-bound by design.
In seed mode, the identity is portable by design.

## Testing

```bash
# Test deterministic key generation
node test/identity-debug.js

# Full identity module test
node test/identity.js
```

No cleanup needed — nothing is written to disk.

## Limitations

- **Container environments**: May not have `/etc/machine-id`
- **VM clones**: VMs cloned from the same image may have identical machine IDs
- **MAC address changes**: Rare but possible with hardware replacement or virtualization

If hardware entropy is unavailable, both functions return `null`, and the caller should fall back to `String.random(9)` (as done in `mesh.js`).

## Security Rationale

### Why nothing is persisted to disk

Even persisting public keys or metadata creates security risks:

1. **Seed exposure**: The seed could be used to re-derive private keys
2. **Identity fingerprinting**: Public keys reveal the identity permanently
3. **Hardware fingerprint leakage**: The hwid contains sensitive system information
4. **Attack surface**: Any file on disk is a potential target for theft or tampering
5. **Backup risks**: Files may be inadvertently backed up to insecure locations

By calculating identity from hardware on demand:
- **Zero attack surface**: No files to steal, misconfigure, or backup accidentally
- **Hardware-bound**: Identity cannot be moved to another machine
- **Ephemeral**: Keys exist only in memory during use
- **Deterministic**: Same keys on every calculation from same hardware

### When NOT to use hardware identity

- **Multi-tenant environments**: Different users/apps should have separate identities
- **Per-application keys**: Use `ZEN.pair(null, { seed: "app-name" })` instead
- **User-specific operations**: Use user-provided keys or WebAuthn

The hardware identity is ideal for:
- Server-to-server authentication
- Persistent node identity in P2P networks
- System-level cryptographic operations
