# Chapter 4 — Authenticated Data

> **Goal:** Use ZEN's identity model to own data, enforce write rules, and delegate access to others using certificates.

This chapter bridges raw cryptography (Ch 3) and storage (Ch 5). You will learn how ZEN decides who can write where, and how to use that system in your application.

---

## 4.1 Identity in ZEN

ZEN has no account server, no usernames, no sessions. **Identity is a key pair.** Whoever holds `priv` can write to the matching `pub` namespace. Anyone with `pub` can verify that data was written by the correct key holder.

```js
const me = await ZEN.pair();
// me.pub  — 45-char base62 public key  (share freely)
// me.priv — 44-char base62 private key (never share)
```

You can store this pair however you like — in the browser `localStorage`, an encrypted vault, or a hardware key. ZEN does not manage it for you.

---

## 4.2 Open vs. owned namespaces

Every write in ZEN goes through the security pipeline. The pipeline selects a rule based on the **soul** of the node being written to:

| Soul pattern | Rule |
|-------------|------|
| No `~` prefix | **Open** — any write is accepted (unless `opt.secure` is set) |
| `~<pub>` or `~<pub>/path` | **Owned** — writer must hold the matching `priv` |
| `~/...` | **Shard index** — tree-sharded pub routing table |

**Open namespace example:**

```js
// Anyone can write here — no auth required
zen.get("chat").get("room1").put({ text: "hello" });
```

**Owned namespace example:**

```js
const me = await ZEN.pair();

// Only the holder of me.priv can write to ~<pub>
zen.get("~" + me.pub).get("profile").put(
  { name: "Alice" },
  null,
  { authenticator: me }
);
```

---

## 4.3 Writing authenticated data

Pass your key pair as `authenticator` in the `opt` parameter (param 3 of `put()`):

```js
const me = await ZEN.pair();

// Write a single value
zen.get("~" + me.pub).get("username").put(
  "alice",
  function(ack) { if (ack.err) console.error(ack.err); },
  { authenticator: me }
);

// Write nested object — all nodes get signed
zen.get("~" + me.pub).put(
  { name: "Alice", city: "Hanoi" },
  null,
  { authenticator: me }
);
```

**How it works internally:**

1. `put()` places `{ authenticator: me }` in the outgoing message `opt`
2. The security middleware receives the write on the `"in"` channel
3. `check.auth()` calls `ZEN.sign(payload, me)` — signs the data with `me.priv`
4. The signed value is stored as `{ ":": value, "~": "<86-char base62 signature>" }`
5. On reads, `ZEN.recover(storedValue)` derives the signer's pub; `ZEN.verify(storedValue, pub)` confirms it

---

## 4.4 Reading back authenticated data

Reading is the same as any other read — no special options needed:

```js
const pub = me.pub;

zen.get("~" + pub).get("username").once(function(data) {
  console.log(data);  // "alice"  (the raw value, not the signed wrapper)
});
```

ZEN automatically unpacks the signed wrapper and gives you the raw value.

---

## 4.5 Registering a public key (bootstrap)

The `"pub"` key on the root node `~<pub>` is special: it bootstraps the identity into the graph. Write your public key to register yourself:

```js
zen.get("~" + me.pub).get("pub").put(me.pub, null, { authenticator: me });
```

After this write, any peer that fetches `zen.get("~" + pub).get("pub")` can verify that the key pair actually controls this namespace.

---

## 4.6 Certificates — delegated write access

A **certificate** lets you authorize another key pair to write to your namespace, without giving them your `priv`.

```js
const alice = await ZEN.pair();
const bob   = await ZEN.pair();

// Alice grants Bob permission to write to her "inbox" key
const cert = await ZEN.certify(bob.pub, "inbox", alice);
```

Bob can now write to Alice's namespace using the cert:

```js
// Bob writes to Alice's inbox, certified by Alice
zen.get("~" + alice.pub).get("inbox").put(
  "hello from bob",
  null,
  { authenticator: bob, cert: cert }
);
```

Without the cert, Bob's write would be rejected — he doesn't hold `alice.priv`.

---

## 4.7 Certificate policy forms

The second argument to `ZEN.certify()` is the **policy** — it defines what the certificate holder is allowed to write.

### String policy — exact key match

```js
// Bob can write to alice's "inbox" key only
const cert = await ZEN.certify(bob.pub, "inbox", alice);
```

### Array policy — multiple keys

```js
// Bob can write to "inbox" or "outbox"
const cert = await ZEN.certify(bob.pub, ["inbox", "outbox"], alice);
```

### LEX object policy — path + key pattern

```js
// Bob can write any key under a path matching "messages/*"
const cert = await ZEN.certify(bob.pub, { "#": "messages", ".": "*" }, alice);
```

### Read and write policy

```js
// Grant read access to "pub", write access to "inbox"
const cert = await ZEN.certify(bob.pub, { read: "pub", write: "inbox" }, alice);
```

---

## 4.8 Certificate options

```js
const cert = await ZEN.certify(
  bob.pub,
  "inbox",
  alice,
  null,           // callback (optional, or use await)
  {
    expiry: Date.now() + 1000 * 60 * 60 * 24,  // expires in 24 hours
    block: "blocklist",                          // write blocklist soul reference
  }
);
```

| Option | Type | Description |
|--------|------|-------------|
| `expiry` | number | Timestamp after which the cert is invalid |
| `block` | string or `{ write, read }` | Soul of a blocklist node; entries there revoke the cert |

---

## 4.9 Multi-certificant — array of pub keys

Grant access to several specific key pairs at once:

```js
const cert = await ZEN.certify([bob.pub, carol.pub], "inbox", alice);
```

The cert's `c` field will be an array of pub strings. Each holder can use it independently.

---

## 4.10 Certificate structure

A cert is a **compact signed string** — the same format as `ZEN.sign()` output. Use `ZEN.verify(cert, issuerPub)` to read the payload:

```js
const cert    = await ZEN.certify(bob.pub, "inbox", alice);
// cert = "<86 base62 chars>0:<payload json>"
// cert[86] → '0' or '1'  (recovery bit)
// cert[87] → ':'          (secp256k1 separator)

const payload = await ZEN.verify(cert, alice.pub);
// payload = { c: "<bob.pub>", w: "inbox" }
```

Cert payload fields:

| Field | Meaning |
|-------|---------|
| `c` | Certificant — the pub being granted access (string or array; wildcard `"*"` not supported) |
| `w` | Write policy — key, array of keys, or LEX object |
| `r` | Read policy (optional) |
| `e` | Expiry timestamp (optional) |
| `wb` | Write blocklist soul reference (optional) |
| `rb` | Read blocklist soul reference (optional) |

---

## 4.11 `opt.secure` — lock a ZEN instance

If you set `opt.secure = true` when creating a ZEN instance, all writes to open (non-tilde) souls are rejected:

```js
const zen = new ZEN({ secure: true, peers: [] });

// This will be rejected — no auth on an open soul
zen.get("chat").put("hello");  // ack.err = "Soul missing public key..."

// This works — writing to an owned namespace
zen.get("~" + pair.pub).get("msg").put("hello", null, { authenticator: pair });
```

Use `secure: true` for applications that must ensure all data is signed.

---

## 4.12 Verifying data manually

You can verify signatures directly without going through the graph:

```js
// Sign some data
const signed = await ZEN.sign("important message", alice);
// signed[86] → recovery bit, signed[87] → ':', signed.slice(88) → '"important message"'

// Verify it (returns original data, or undefined if invalid)
const data = await ZEN.verify(signed, alice.pub);
console.log(data);  // "important message"

// Tampered data: replace the message part (chars 88+) while keeping the original sig
const tampered = signed.slice(0, 88) + '"other message"';
const bad = await ZEN.verify(tampered, alice.pub);
console.log(bad);   // undefined
```

---

## 4.13 Security pipeline summary

When any write arrives, the security middleware routes it through one of these handlers:

| Soul | Handler | What it checks |
|------|---------|----------------|
| `chat`, `app/data`, ... | `check.any` | Passes unless `opt.secure` is set |
| `~<pub>`, `~<pub>/path` | `check.pub` | Signature matches `pub` in soul; cert OK if different pub |
| `~/...` | `check.shard` | Signed pub routing for the shard tree |
| `!<bytecode>/...` | PEN VM | WASM bytecode policy evaluated against the write |
| `<soul>#...` | `check.hash` | Content-addressed data |
