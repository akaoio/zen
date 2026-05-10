# Chapter 1 — Getting Started

> **Goal:** Install ZEN, write your first graph node, read it back, and make your first crypto key pair — all in under 5 minutes.

---

## 1.1 Install

```bash
npm install @akaoio/zen
```

ZEN has **zero runtime dependencies** in the browser. The bundled `zen.js` file is self-contained.

For Node.js you need Node ≥ 18. No native modules required.

---

## 1.2 Hello ZEN!

```js
import ZEN from "@akaoio/zen";

// Create an instance. "file" sets the storage directory.
const zen = new ZEN({ file: "data" });

// Write a value
zen.get("hello").put("world");

// Read it once, immediately
zen.get("hello").once(function(data) {
  console.log(data);  // "world"
});
```

That's it. ZEN is a graph. `get("hello")` navigates to a node. `put(...)` writes a value. `once(...)` reads it once.

---

## 1.3 Nested objects

ZEN understands plain objects as graph nodes. Each key becomes a child node.

```js
zen.get("profile").put({ name: "Alice", age: 30 });

zen.get("profile").get("name").once(function(data) {
  console.log(data);  // "Alice"
});
```

You can chain arbitrarily deep:

```js
zen.get("app").get("settings").get("theme").put("dark");

zen.get("app").get("settings").get("theme").once(function(data) {
  console.log(data);  // "dark"
});
```

---

## 1.4 Subscribe to changes with `on()`

`once()` fires once and stops. `on()` fires every time the value changes.

```js
zen.get("counter").put(0);

zen.get("counter").on(function(data) {
  console.log("counter:", data);
});

// Somewhere later:
zen.get("counter").put(1);  // triggers on() again
zen.get("counter").put(2);  // triggers on() again
```

---

## 1.5 Collections with `set()` and `map()`

Use `set()` to add items to an unordered collection. Use `map()` to iterate over all of them in realtime.

```js
const list = zen.get("todos");

list.set({ text: "buy milk" });
list.set({ text: "learn ZEN" });

list.map().on(function(item, id) {
  console.log(id, item);
  // id   = the unique soul of this item
  // item = { text: "buy milk" } etc.
});
```

`map()` keeps firing as new items arrive — useful for building realtime lists.

---

## 1.6 Constructor options

```js
const zen = new ZEN({
  file:         "data",       // storage directory (Node.js, Radisk)
  localStorage: false,        // disable browser localStorage (default: enabled)
  radisk:       false,        // disable file persistence entirely
  peers:        ["wss://relay.example.com/zen"],  // peer URLs to connect to
});
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `file` | string | `undefined` | Radisk storage directory (Node.js) |
| `localStorage` | boolean | `true` | Use `localStorage` in browser |
| `radisk` | boolean | `true` | Enable Radisk storage |
| `peers` | string or string[] | `[]` | Peer relay URLs |
| `evict` | boolean | `true` | Enable memory-pressure eviction (Node.js) |
| `frat` | number | `0.10` | Evict when `os.freemem()/os.totalmem()` < this ratio |
| `fmb` | number | `200` | Evict / warn when free memory or disk drops below this MB |
| `tombstoneTTL` | number (ms) | `604800000` | Omit tombstones older than this from disk (7 days; `0` = keep forever) |

See [§5.12 Storage resilience](ch05-storage.md#512-storage-resilience) for full details.

---

## 1.7 Your first key pair

ZEN ships a full cryptographic runtime. The static methods on `ZEN` work without creating an instance.

```js
const pair = await ZEN.pair();
// pair.pub   — 45-character secp256k1 public key (base62 compressed)
// pair.priv  — private key (keep this secret!)
// pair.address — optional address/view format for some conversions
// pair.curve — "secp256k1"

console.log(pair.pub.length);  // 45
```

Sign and verify:

```js
const signed = await ZEN.sign("hello world", pair);
const data   = await ZEN.verify(signed, pair.pub);
console.log(data);  // "hello world"
```

Encrypt and decrypt:

```js
const enc = await ZEN.encrypt("secret message", pair.pub);
const dec = await ZEN.decrypt(enc, pair.priv);
console.log(dec);  // "secret message"
```

See [Chapter 3](ch03-crypto.md) for the full crypto API.

---

## 1.8 Browser usage

Copy `zen.js` from the package root into your project and load it directly:

```html
<script src="zen.js"></script>
<script>
  const zen = new ZEN({ peers: ["https://relay.example.com/zen"] });
  zen.get("hello").put("world");
  zen.get("hello").once(console.log);
</script>
```

`ZEN` is exposed as a global when loaded via `<script>`. No bundler required.

---

## 1.9 What's next?

- **[Chapter 2](ch02-graph-model.md)** — understand the graph data model, CRDT semantics, and the full core API (`get`, `put`, `on`, `map`, `back`)
- **[Chapter 3](ch03-crypto.md)** — deep dive into the cryptographic runtime
- **[Chapter 8](ch08-contributing.md)** — if you want to contribute or build from source
