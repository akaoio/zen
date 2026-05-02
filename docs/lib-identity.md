# Hardware Identity Module

## Overview

The `lib/identity.js` module provides **hardware-derived, deterministic identity generation** for ZEN servers and MCP instances running on the same hardware.

## Features

- **Deterministic**: Same keypair on every restart
- **Hardware-bound**: Derived from `/etc/machine-id`, MAC address, and hostname
- **Shared**: Server and MCP use the same identity on the same machine
- **Secure**: Private keys never written to disk, only derived from hardware entropy on demand
- **Persistent metadata**: Public keys saved to `~/.local/state/zen/identity.json` for verification

## API

### `getOrCreateIdentity()`

Returns a hardware-derived identity, creating and persisting it if it doesn't exist.

```javascript
import { getOrCreateIdentity } from "./lib/identity.js";

const identity = await getOrCreateIdentity();
// → { pair: { pub, priv, epub, epriv }, seed, hwid }
```

**Returns:**
- `pair`: Full keypair (signing + encryption keys)
- `seed`: Base62-encoded SHA-256 hash of hardware ID
- `hwid`: Raw hardware ID string (machine-id|MAC|hostname)

**Returns `null` if hardware entropy is unavailable** (e.g., in containers without machine-id).

### `getIdentity()`

Returns the hardware-derived identity without persisting metadata. Useful for read-only contexts.

```javascript
import { getIdentity } from "./lib/identity.js";

const identity = await getIdentity();
// → { pair, seed, hwid } or null
```

## Usage

### In ZEN Server (`script/server.js`)

```javascript
import { getOrCreateIdentity } from "../lib/identity.js";

const identity = await getOrCreateIdentity();
if (identity) {
  zen = new ZEN({ pid: identity.pair.pub, ... });
  // Use identity.pair for signing/encryption
}
```

### In ZEN MCP (`lib/mcp.js`)

```javascript
import { getOrCreateIdentity } from "./identity.js";

const hwIdentity = await getOrCreateIdentity();
if (hwIdentity) {
  zenOpt.pid = hwIdentity.pair.pub;
  // Store in pairStore with special "hw" ID
  pairStore.set("hw", hwIdentity.pair);
}
```

MCP users can then use `pairId: "hw"` in crypto/graph operations to sign with the hardware identity.

## Hardware Entropy Sources

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

## Key Derivation

```
hwid() → "96e66a7085...|00:0d:3a:9a:da:5b|hostname"
   ↓
SHA-256 + base62 encode
   ↓
seed = "03EdUg7H7Y9bhcPj7XU2OWerQQ6hvdD7iFl04WEDuvsJ"
   ↓
ZEN.pair(null, { seed })
   ↓
{ pub, priv, epub, epriv }
```

The seed is hashed with curve-specific labels before scalar derivation:
- `"ZEN|secp256k1|sign|"` → signing private key
- `"ZEN|secp256k1|encrypt|"` → encryption private key

## Persistence

Metadata is saved to `~/.local/state/zen/identity.json`:

```json
{
  "hwid": "96e66a7085...",
  "seed": "03EdUg7H7Y...",
  "pub": "05B7ebtSYj...",
  "epub": "0YkHKxBrB3...",
  "created": 1735776000000
}
```

**Private keys are NEVER persisted**. They are re-derived from the hardware ID on every call to `getOrCreateIdentity()` or `getIdentity()`.

## Testing

```bash
# Test deterministic key generation
node test/identity-debug.js

# Full identity module test
node test/identity.js

# Clean up
rm ~/.local/state/zen/identity.json
```

## Limitations

- **Container environments**: May not have `/etc/machine-id`
- **VM clones**: VMs cloned from the same image may have identical machine IDs
- **MAC address changes**: Rare but possible with hardware replacement or virtualization

If hardware entropy is unavailable, both functions return `null`, and the caller should fall back to `String.random(9)` (as done in `mesh.js`).

## Security Considerations

### Why private keys aren't persisted

Storing private keys on disk increases attack surface:
- Filesystem permissions must be carefully managed
- Keys may be backed up inadvertently
- Accidental key exposure in logs/errors

By deriving keys from hardware entropy on demand:
- Keys exist only in memory during use
- No file to steal or misconfigure
- Hardware-bound: keys cannot be moved to another machine

### When NOT to use hardware identity

- **Multi-tenant environments**: Different users/apps should have separate identities
- **Per-application keys**: Use `ZEN.pair(null, { seed: "app-name" })` instead
- **User-specific operations**: Use user-provided keys or WebAuthn

The hardware identity is ideal for:
- Server-to-server authentication
- Persistent node identity in P2P networks
- System-level cryptographic operations
