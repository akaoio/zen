# Hardware Identity Module

## Overview

The `lib/identity.js` module provides **hardware-derived, deterministic identity generation** for ZEN servers and MCP instances running on the same hardware.

## Features

- **Deterministic**: Same keypair on every restart
- **Hardware-bound**: Derived from `/etc/machine-id`, MAC address, and hostname
- **Shared**: Server and MCP use the same identity on the same machine
- **Runtime-only**: Identity calculated on demand, never persisted to disk
- **Secure**: Private keys and seed exist only in process memory

## API

### `getOrCreateIdentity()`

Returns a hardware-derived identity by calculating it from hardware entropy.

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

**Security:** Identity is calculated fresh on every call. Nothing is persisted to disk.

### `getIdentity()`

Alias for `getOrCreateIdentity()` for backward compatibility.

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

## Security Model

**No disk persistence whatsoever.**

- Identity is calculated fresh from hardware entropy on every call
- Private keys exist only in process memory during execution
- Seed (base62-encoded hash) exists only in memory
- Public keys are never written to disk
- Hardware fingerprint (hwid) is never logged or persisted

This approach eliminates entire classes of attacks:
- No keyfiles to steal or misconfigure
- No accidental backup of sensitive material
- No permission issues with key storage
- Keys cannot be moved to another machine

The identity is hardware-bound and ephemeral by design.

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
