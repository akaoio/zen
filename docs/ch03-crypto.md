# Chapter 3 — Cryptography

> **Goal:** Master ZEN's built-in crypto runtime — key generation, signing, encryption, shared secrets, hashing, and write-access certificates.

All crypto methods are available as **static methods on `ZEN`** and also as **instance methods on any ZEN chain**.

WebCrypto is required. In browsers, HTTPS is mandatory (WebCrypto is disabled on HTTP origins).

---

## 3.1 Key pairs

### `ZEN.pair(seed, opt)`

Generate a key pair.

```js
const pair = await ZEN.pair();
```

**Returns an object:**

| Field | Type | Description |
|-------|------|-------------|
| `pub` | string (45 chars) | Public key, base62 compressed (44-char x + 1 parity) |
| `priv` | string (44 chars) | Private key — **never share or store in the graph** |
| `address` | string (42 chars) | Ethereum-style keccak160 address (hex, checksummed) |
| `curve` | string | `"secp256k1"` by default |

```js
console.log(pair.curve);      // "secp256k1"
console.log(pair.pub.length); // 45
```

**Options:**

```js
// P-256 (secp256r1)
const p256pair = await ZEN.pair(null, { curve: "p256" });

// EVM-format (0x checksummed address)
const evmpair = await ZEN.pair(null, { format: "evm" });

// Bitcoin P2PKH mainnet
const btcpair = await ZEN.pair(null, { format: "btc" });
```

**Seed-based (deterministic):**

```js
const pair = await ZEN.pair("my deterministic seed");
// Same seed → same key pair, every time
```

**Format conversion — same key, different output format:**

Pass an existing `priv` back into `ZEN.pair()` with a different `format`. The underlying scalar is identical; only the encoding changes.

```js
// Generate once — use for graph writes, signing, ECDH
const zenPair = await ZEN.pair();

// Get the same key in EVM format — use for on-chain transactions (ETH, BSC, …)
const evmPair = await ZEN.pair(null, {
  priv:   zenPair.priv,
  format: "evm",
});
// evmPair.pub   = "0xAbCdEf…"  (EIP-55 checksummed address)
// evmPair.priv  = "0x1234…"    (32-byte hex private key)

// Or Bitcoin P2PKH
const btcPair = await ZEN.pair(null, {
  priv:   zenPair.priv,
  format: "btc",
});
// btcPair.pub  = "1AbcDef…"  (P2PKH mainnet address)
// btcPair.priv = "KwDiBf…"   (WIF compressed)
```

`zenPair`, `evmPair`, and `btcPair` all represent the **same cryptographic key** — just serialized differently.

---

## 3.2 Signing and verification

### `ZEN.sign(data, pair)`

Signs `data` with a private key. Returns a **compact string** in the format `<86-char base62 signature><v>:<message>` where `v` is the ECDSA recovery bit (`0` or `1`).

```js
const signed = await ZEN.sign("hello", pair);
// '<86 base62 chars>0:"hello"'
// signed[86] → '0' or '1'  (recovery bit)
// signed[87] → ':'          (secp256k1 separator)
// signed.slice(88) → '"hello"'  (JSON-encoded message)
```

For non-secp256k1 curves (e.g. P-256), the curve name is embedded after a `/`:

```js
const p256pair = await ZEN.pair(null, { curve: "p256" });
const signed   = await ZEN.sign("hello", p256pair);
// '<86 base62 chars>0/p256:"hello"'
// signed[87] → '/'  — marks a curve tag
// signed.slice(88) → 'p256:"hello"'
```

The recovery bit enables public-key recovery from the signature — any party can determine who signed the data without being told the public key upfront (see `ZEN.recover`).

**`data` can be any valid JS value:**

```js
await ZEN.sign(null, pair);
await ZEN.sign(true, pair);
await ZEN.sign(42, pair);
await ZEN.sign("hello", pair);
await ZEN.sign([1, 2, 3], pair);
await ZEN.sign({ key: "value" }, pair);
await ZEN.sign(JSON.stringify({ a: 1 }), pair);
```

### `ZEN.verify(sig, pub)`

Verifies a signed value against a public key. Returns the original data on success, `undefined` on failure.

```js
const data = await ZEN.verify(signed, pair.pub);
// "hello"

const bad = await ZEN.verify(signed, otherPair.pub);
// undefined — wrong key

// Tamper by replacing message part (chars 88+):
const tampered = signed.slice(0, 88) + '"evil"';
const result   = await ZEN.verify(tampered, pair.pub);
// undefined — signature mismatch
```

If the original data was `JSON.stringify(...)`, the result is automatically parsed back:

```js
const signed = await ZEN.sign(JSON.stringify({ a: 1 }), pair);
const data   = await ZEN.verify(signed, pair.pub);
// { a: 1 }  ← parsed back to object
```

### `ZEN.recover(signed)`

Recovers the **signer's public key** from a signed value — without knowing the public key in advance. Uses the ECDSA recovery bit `v` embedded in the signature.

```js
const signed = await ZEN.sign("hello", pair);

const pub = await ZEN.recover(signed);
// pub === pair.pub  ← same key, derived from the signature alone
```

Works with any valid JS value as original data:

```js
const signed = await ZEN.sign({ action: "transfer", amount: 100 }, pair);
const pub = await ZEN.recover(signed);
// pub — 45-char base62 public key of the signer
```

**Returns** the 45-char base62 public key string, or `undefined` if recovery fails (missing `v` bit, tampered signature, wrong curve).

**Cross-curve**: the `v` field includes the curve identifier (`c` field in the sig). Recovery automatically uses the correct curve.

```js
const p256pair = await ZEN.pair(null, { curve: "p256" });
const signed   = await ZEN.sign("hello", p256pair);
const pub      = await ZEN.recover(signed);
// pub === p256pair.pub
```

**Use `verify` after `recover` for defense in depth:**

```js
const pub  = await ZEN.recover(signed);
const data = await ZEN.verify(signed, pub);
// data === "hello" — cryptographically confirmed
```

---

## 3.3 Encryption and decryption

### `ZEN.encrypt(data, key)`

Encrypts `data` using AES-GCM. Returns a compact ciphertext string `<ct_base62>.<iv_base62_21>.<salt_base62_13>`.

```js
const enc = await ZEN.encrypt("secret", pair.pub);
// "7Tj2...Q.03Bf7wQbP8bBYlL6V3P8B.0A9Vwyv9N6YvP"
```

**`data` can be any valid JS value** (same as sign). **`key`** can be:
- Your own `pub` for self-encryption
- Another user's `pub` for shared-key mode (see §3.4)

### `ZEN.decrypt(enc, key)`

Decrypts a ciphertext string. Returns the original data, or `undefined` if the key is wrong.

```js
const dec = await ZEN.decrypt(enc, pair.priv);
// "secret"

const wrong = await ZEN.decrypt(enc, otherPair.priv).catch(() => undefined);
// undefined
```

Round-trip with a stringified object:

```js
const original = JSON.stringify({ msg: "hello" });
const enc = await ZEN.encrypt(original, pair.pub);
const dec = await ZEN.decrypt(enc, pair.priv);
// { msg: "hello" }  ← automatically parsed back
```

---

## 3.4 Shared secrets (ECDH)

### `ZEN.secret(pub, pair)`

Derives a shared AES secret using ECDH. Two parties can derive the same secret without transmitting it.

```js
const alice = await ZEN.pair();
const bob   = await ZEN.pair();

// Alice derives the shared secret using Bob's pub and her own pair
const sharedByAlice = await ZEN.secret(bob.pub, alice);

// Bob derives the same secret using Alice's pub and his own pair
const sharedByBob   = await ZEN.secret(alice.pub, bob);

// sharedByAlice === sharedByBob  ← same bytes, different derivations
```

Use the shared secret as an encryption key:

```js
const enc = await ZEN.encrypt("private msg", sharedByAlice);
const dec = await ZEN.decrypt(enc, sharedByBob);
```

---

## 3.5 Hashing

### `ZEN.hash(data, pair, cb, opt)`

Produces a deterministic hash of `data`.

```js
// SHA-256 (default, PBKDF2-derived, base62 output)
const h = await ZEN.hash("hello");

// SHA-256 direct digest, hex output
const sha = await ZEN.hash("hello", null, null, { name: "SHA-256", encode: "hex" });

// Keccak-256
const keccak = await ZEN.hash("hello", null, null, { name: "keccak256", encode: "hex" });

// keccak256 of empty string
// "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"

// ArrayBuffer input
const buf = new TextEncoder().encode("hello").buffer;
const h = await ZEN.hash(buf);
```

**Callback form:**

```js
ZEN.hash("hello", null, function(result) {
  console.log(result);
});
```

---

### 3.5.1 Mining — `opt.pow`

When `opt.pow` is set, `hash()` enters **mining mode**: it loops with base62-encoded nonces until the hash output starts with the required prefix, then returns `{ hash, nonce, proof }`.

```js
const result = await ZEN.hash(
  nonce => `order:${candle}:${nonce}`,   // nonce is a base62 string
  null, null,
  { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 2 } }
);
// result.hash  — winning SHA-256 hex hash (starts with "00")
// result.nonce — base62-encoded nonce string, e.g. "G" or "1z"
// result.proof — the full data string that was hashed, e.g. "order:1712000000:G"
```

`opt.pow` fields mirror the pen soul `pow` policy:

| Field | Type | Description |
|-------|------|-------------|
| `unit` | `string` | Prefix character(s) each hash must start with (default `"0"`) |
| `difficulty` | `number` | How many times `unit` is repeated (default `1`) |

**Nonce encoding:** nonces are base62 (`0-9A-Za-z`) so they stay short as the search space grows.

**Separator:** when `data` is a plain string, the nonce is appended with `:` as separator:
```js
// data = "work", nonce = "G" → proof = "work:G"
const result = await ZEN.hash("work", null, null,
  { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 1 } });
```

**Function form** (full nonce control — nonce in any position):
```js
const result = await ZEN.hash(
  nonce => `${candle}:${nonce}`,  // nonce anywhere in the string
  null, null,
  { name: "SHA-256", encode: "hex", pow: { unit: "0", difficulty: 2 } }
);
```

---

### 3.5.2 Pen PoW compatibility

ZEN's pen policy engine verifies PoW by reconstructing a **canonical block** from all four write fields and hashing it with the nonce:

```js
// pen.js internals — how the verifier checks PoW:
// The canonical block binds soul + key + val + state together.
// R[7] = nonce (from msg.put["^"])
var proof = JSON.stringify({
  "#": soul,    // R[2] — soul of the node
  ".": key,     // R[0] — graph key being written
  ":": val,     // R[1] — value being written
  ">": state,   // R[3] — HAM timestamp
}) + ":" + nonce;
hash(proof, null, cb, { name: "SHA-256", encode: "hex" })
// passes if hash starts with pow.unit.repeat(pow.difficulty)
```

Binding the nonce to the full canonical block (not just the key) prevents replay attacks across different keys, values, or time periods.

**Auto-mining** happens automatically inside `penStage` when you pass `pow` as a policy object in `opt`:

```js
// ZEN auto-mines the nonce — just pass the policy object:
zen.get(penSoul).get(myKey).put(data, cb, {
  authenticator: pair,
  pow: { unit: "0", difficulty: 2 }  // ZEN mines and stores nonce in msg.put["^"]
});
```

The mined nonce is stored in `msg.put["^"]` and propagated to all peers. Peers re-verify by rebuilding the same canonical block.

> **Why not `opt.pow: nonce` (a string)?**
>
> `opt.pow` must be a **policy object** `{ unit, difficulty }` — never a nonce string.
> The nonce is computed internally by ZEN during `penStage` and stored in `msg.put["^"]` (R[7]).
> Passing a nonce string as `opt.pow` will not trigger mining and will be silently ignored.

---

## 3.6 Certificates — delegating write access

### `ZEN.certify(certificant, policy, authority)`

Creates a certificate that grants write access to another user (or all users) for a specific path.

```js
const alice = await ZEN.pair();  // the authority
const bob   = await ZEN.pair();  // the recipient

// Alice grants Bob write access to the "inbox" path
const cert = await ZEN.certify(bob.pub, "inbox", alice);
```

**`certificant` can be:**

| Value | Meaning |
|-------|---------|
| `bob.pub` (string) | Grant access to Bob only |
| `"*"` | Grant access to any user (wildcard) |
| `[bob.pub, carol.pub]` | Grant access to multiple users |
| `[alice, bob]` (pair objects) | Uses `.pub` from each pair |

**`policy`** is a string path, e.g. `"inbox"`, `"messages"`.

**Returns:** a compact signed string in the same wire format as `ZEN.sign()`. The certificate can be verified:

```js
const data = await ZEN.verify(cert, alice.pub);
// { c: bob.pub, w: "inbox" }
```

**Using a certificate in `put()`:**

```js
// Bob writes to Alice's inbox using the certificate
zen.get("~" + alice.pub).get("inbox").put(
  { text: "hello" },
  function(ack) { console.log(ack); },
  { authenticator: bob, cert: cert }
);
```

---

## 3.7 Key IDs

### `ZEN.keyid(pub)`

Returns a short identifier for a public key (first N chars). Useful for display or lookup without revealing the full key.

```js
const id = ZEN.keyid(pair.pub);
```

---

## 3.8 Base62 encoding

ZEN uses base62 (`[A-Za-z0-9]`) for all key material and bytecode. The encoding utilities are internal and live in `src/base62.js`. They are also exposed on the `pen` object for PEN bytecode operations (see §7.5 in Ch 7):

```js
// PEN bytecode encoding (pen object — not ZEN static):
const packed   = pen.pack(uint8Array);  // → base62 string
const unpacked = pen.unpack(str);       // → Uint8Array
```

These are **not** exported as `ZEN.pack` / `ZEN.unpack` on the ZEN class.

---

## 3.9 Public key format

All public keys in ZEN use **base62 encoding** — only alphanumeric characters (`0-9A-Za-z`), no `+`, `/`, `=`, `_`, `-`, `.` separators. **ZEN does NOT use base64 or base64url.**

| Field | Length | Format | Notes |
|-------|--------|--------|-------|
| `pub` | **45 chars** | 44-char base62 x + `"0"`/`"1"` parity | Compressed EC public key |
| `priv` | 44 chars | base62 scalar | secp256k1 scalar |
| `address` | 42 chars | hex (checksummed) | Ethereum-style keccak160 address |

Public keys are **compressed**: only the x-coordinate (44 chars base62) plus 1 parity character (`"0"` = y even, `"1"` = y odd) are stored. The y-coordinate is recovered on-the-fly via modular square root. This is the same concept as Bitcoin compressed pubkey (33 bytes), expressed in base62 instead of bytes.

This is a deliberate change from the original GUN database which used base64url. Base62 enables keys to be used as URL path segments and graph souls without any escaping.

ZEN automatically accepts legacy formats for backward compatibility:
- Old ZEN uncompressed (88-char, `[A-Za-z0-9]{88}`) — base62 x + base62 y (legacy, read-only)
- Legacy GUN base64url (87-char, `base64url.base64url`) — original GUN format

---

## 3.10 `ZEN.opt.pub(soul)` — extract pub from soul

Extracts the public key from a `~/...` soul path:

```js
ZEN.opt.pub("~" + pair.pub + "/profile");  // → pair.pub
```

Returns `undefined` if the soul is not a valid tilde path.

---

## 3.11 Crypto on instance methods

All static crypto methods are also available as instance methods for convenience:

```js
const zen = new ZEN({ ... });

const pair   = await zen.pair();
const signed = await zen.sign("data", pair);
const ok     = await zen.verify(signed, pair.pub);
const enc    = await zen.encrypt("msg", pair.pub);
const dec    = await zen.decrypt(enc, pair.priv);
```

They behave identically to the static forms.

---

## 3.12 SECP256K1 namespace

The secp256k1 curve is exposed as a namespace:

```js
zen.SECP256K1.curve   // "secp256k1"
ZEN.SECP256K1.curve   // "secp256k1"
```

---

## 3.13 `ZEN.check`

`ZEN.check` is a function that validates whether a graph write is authorized. It is called automatically by the security middleware — you do not normally call it directly.

---

## 3.14 `ZEN.security`

```js
ZEN.security.opt === ZEN.opt  // true
```

`ZEN.security` is an object that holds the security configuration. `ZEN.security.opt` is the same function as `ZEN.opt` — they refer to the same option-parser.

---

## 3.15 Security summary

| Method | Direction | Key used |
|--------|-----------|---------|
| `sign(data, pair)` | Write | `pair.priv` |
| `verify(sig, pub)` | Read | `pub` string |
| `encrypt(data, pub)` | Write | recipient's `pub` |
| `decrypt(enc, priv)` | Read | `priv` (own private key) |
| `secret(pub, pair)` | Key exchange | `pair.priv` + `pub` |
| `certify(cert, policy, auth)` | Delegation | `auth.priv` |
| `hash(data)` | Fingerprint | none (deterministic) |
