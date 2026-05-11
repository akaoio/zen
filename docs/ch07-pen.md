# Chapter 7 — PEN Policy VM

> **Goal:** Understand what PEN is, how to write and run policies, and how the VM integrates with ZEN's write pipeline.

---

## 7.1 What PEN is

**PEN** (Predicate-Embedded Namespace) is a standalone bytecode virtual machine compiled from Zig to WebAssembly. It evaluates write-access policies as predicates: given a set of register values (key, value, soul, state, time, pub…), it returns `true` (allow) or `false` (deny).

PEN is layered:

```
┌─────────────────────────────────────────────────────┐
│  Layer 2: Application                               │
│  Define policies using ZEN.pen() API                │
│  Use candle numbers, windows, PoW hashes            │
├─────────────────────────────────────────────────────┤
│  Layer 1: ZEN-PEN Bridge (src/pen.js)               │
│  Knows register conventions (R0=key, R1=val…)       │
│  Injects R4=Date.now() before calling PEN core      │
│  Handles policy opcodes: SGN, CRT, NOA, POW         │
│  Compiles ZEN.pen(spec) → bytecode                  │
├─────────────────────────────────────────────────────┤
│  Layer 0: PEN Core (src/pen.wasm) — STANDALONE      │
│  Source: src/pen.zig (Zig), compiled to WASM        │
│  Input: (bytecode, registers[])                     │
│  Output: boolean                                    │
│  No knowledge of ZEN, time, or network              │
└─────────────────────────────────────────────────────┘
```

The WASM binary is 26 KB with zero imports. It is the smallest possible freestanding policy engine.

Build it yourself with Zig:

```bash
npm run build:pen
```

---

## 7.2 `ZEN.pen(spec)` — compile a policy

`ZEN.pen(spec)` compiles a policy spec into a base62 bytecode string.

```js
const bytecode = ZEN.pen({ key: "fixed" });
// "!..." — a short base62 string starting with "!"
```

The spec is a plain object describing what the policy should enforce. The compiled string can be stored in the graph and evaluated later.

---

## 7.3 `pen.run(bytecode, regs)` — evaluate a policy core

The low-level evaluator is `pen.run(bytecode, regs)` from `src/pen.js`. It evaluates a compiled bytecode string against a set of register values and returns `true` or `false`.

Application code normally compiles souls with `ZEN.pen(spec)` and lets the security bridge invoke `pen.run(...)` during writes.

```js
import pen from "../src/pen.js";

const allow = pen.run(bytecode, ["key", "value"]);
// regs[0] = key, regs[1] = value
```

---

## 7.4 Register conventions

When PEN is called from the ZEN bridge, registers are populated as follows:

| Register | Field | Description |
|---------|-------|-------------|
| `R[0]` | `key` | The graph key being written |
| `R[1]` | `val` | The value being written (verified plaintext when `sign:true`) |
| `R[2]` | `soul` | The soul of the node |
| `R[3]` | `state` | The HAM state timestamp |
| `R[4]` | `now` | `Date.now()` — injected by the ZEN bridge |
| `R[5]` | `pub` | The writer's public key (recovered from signature when `sign:true`) |
| `R[6]` | `path` | Path after `pencode/` in soul (e.g. soul `!abc/foo/bar` → path `foo/bar`) |
| `R[7]` | `nonce` | PoW nonce from `msg.put["^"]` — always reserved, never set by user |
| `R[128–255]` | `local[n]` | Local slots, set by `LET` opcode |

Host registers (`R[0..127]`) are provided by the ZEN bridge. Local registers (`R[128..255]`) are scratch space inside the policy.

---

## 7.5 Base62 encoding

PEN bytecode is encoded as a **base62 string** using only `[A-Za-z0-9]` characters. This makes it safe to store in a JSON graph node without escaping.

The encoding uses the theoretical minimum length: treat the full byte array (with a sentinel prefix byte `0x01`) as a big-endian integer and convert to base62.

```
10 bytes → 15 base62 chars
20 bytes → 29 base62 chars
32 bytes → ~44 base62 chars
```

`pen.pack(bytes)` and `pen.unpack(str)` handle the conversion:

```js
const packed   = pen.pack(uint8Array);  // → base62 string
const unpacked = pen.unpack(str);       // → Uint8Array
```

---

## 7.6 Core ISA

PEN Core v1 is an expression tree. Every instruction is a node in a tree; arguments follow immediately after the opcode byte. Evaluation is depth-first, left-to-right. The output is always a boolean.

### Constants

| Opcode | Encoding | Value |
|--------|----------|-------|
| `0x00` | `0x00` | `null` |
| `0x01` | `0x01` | `true` |
| `0x02` | `0x02` | `false` |
| `0x03` | `0x03 [u8 len][utf8…]` | string (≤ 255 bytes) |
| `0x04` | `0x04 [ULEB128]` | unsigned integer |
| `0x07` | `0x07 [SLEB128]` | signed integer |
| `0x08` | `0x08 [f64be]` | float64 |

### Register access

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x10` | `0x10 [u8 n]` | `REG(n)` — load register n |

### Logic

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x20` | `0x20 [u8 n] [expr × n]` | AND — all n sub-expressions must be true |
| `0x21` | `0x21 [u8 n] [expr × n]` | OR — at least one must be true |
| `0x22` | `0x22 [expr]` | NOT |
| `0x23` | `0x23` | PASS — always `true` |
| `0x24` | `0x24` | FAIL — always `false` |

### Comparison

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x30` | `0x30 [expr][expr]` | EQ |
| `0x31` | `0x31 [expr][expr]` | NE |
| `0x32` | `0x32 [expr][expr]` | LT |
| `0x33` | `0x33 [expr][expr]` | GT |
| `0x34` | `0x34 [expr][expr]` | LTE |
| `0x35` | `0x35 [expr][expr]` | GTE |

String comparisons are lexicographic. Number comparisons are arithmetic.

### Arithmetic

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x40` | `0x40 [expr][expr]` | ADD |
| `0x41` | `0x41 [expr][expr]` | SUB |
| `0x42` | `0x42 [expr][expr]` | MUL |
| `0x43` | `0x43 [expr][expr]` | DIVU — integer floor division |
| `0x44` | `0x44 [expr][expr]` | MOD — remainder |
| `0x45` | `0x45 [expr][expr]` | DIVF — float division |
| `0x46` | `0x46 [expr]` | ABS |
| `0x47` | `0x47 [expr]` | NEG |

### String operations

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x50` | `0x50 [expr]` | LEN — string length → number |
| `0x51` | `0x51 [expr][expr][expr]` | SLICE(str, start, end) |
| `0x52` | `0x52 [expr][u8 sep][expr]` | SEG(str, sep_char, idx) — split by separator, return segment |
| `0x53` | `0x53 [expr]` | TONUM — parse string to number |
| `0x54` | `0x54 [expr]` | TOSTR — number to string |
| `0x55` | `0x55 [expr][expr]` | CONCAT |
| `0x56` | `0x56 [expr][expr]` | PRE — startsWith |
| `0x57` | `0x57 [expr][expr]` | SUF — endsWith |
| `0x58` | `0x58 [expr][expr]` | INCLUDES — contains |
| `0x59` | `0x59 [expr][expr]` | REGEX — matches regex |
| `0x5A` | `0x5A [expr]` | UPPER |
| `0x5B` | `0x5B [expr]` | LOWER |

### Type checks

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x60` | `0x60 [expr]` | ISS — is string |
| `0x61` | `0x61 [expr]` | ISN — is number (finite) |
| `0x62` | `0x62 [expr]` | ISX — is null |
| `0x63` | `0x63 [expr]` | ISB — is boolean |
| `0x64` | `0x64 [expr][u8 min][u8 max]` | LNG — string length in [min, max] |

### Local binding

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x70` | `0x70 [u8 n][expr_def][expr_body]` | LET(n, def, body) — bind `def` to local R[128+n], evaluate body |

### Conditional

| Opcode | Encoding | Description |
|--------|----------|-------------|
| `0x71` | `0x71 [cond][then][else]` | IF(cond, then, else) |

---

## 7.7 Optimization opcodes

Three size-reduction optimizations are baked into ISA v1:

### ULEB128/SLEB128 integers

Integers use variable-length encoding instead of fixed-width. Values 0–127 cost 1 byte; larger values scale logarithmically. The fixed `uint8`/`uint16`/`uint32` opcodes no longer exist.

```
300000 as ULEB128 = [0xE0, 0xA7, 0x12]  (3 bytes vs 4 for uint32)
```

### Register shorthands (`0xF0–0xFF`)

The most common registers (`R[0..5]` and `R[128..131]`) have single-byte shorthands instead of the 2-byte `0x10 [n]` form:

| Opcode | Meaning |
|--------|---------|
| `0xF0` | R[0] = key |
| `0xF1` | R[1] = val |
| `0xF2` | R[2] = soul |
| `0xF3` | R[3] = state |
| `0xF4` | R[4] = now |
| `0xF5` | R[5] = pub |
| `0xF8` | R[128] = local[0] |
| `0xF9` | R[129] = local[1] |

### SEGR macros (`0x80–0x81`)

`SEG(REG[r], sep, idx)` and `TONUM(SEG(…))` are common patterns. They have 4-byte macro forms.

---

## 7.8 Bytecode stream format

```
[0x01 version byte] [root_expr]
```

The first byte is always `0x01` (version 1). The root expression follows immediately.

---

## 7.9 Policy tail opcodes

The range `0xC0–0xDF` is reserved for ZEN policy tail opcodes appended by the bridge after the expression bytecode. These bytes are interpreted by `src/pen.js`, not by the standalone WASM evaluator.

The ZEN bridge defines:

| Opcode | Name | Description |
|--------|------|-------------|
| `0xC0` | SGN | Require a valid ECDSA signature from the writer |
| `0xC1` | CRT | Require a certificate signed by a specific public key |
| `0xC3` | NOA | Open write — no authentication required |
| `0xC4` | POW | Require proof-of-work over the canonical block using nonce register `R[7]` |

These are emitted when `ZEN.pen()` compiles a policy with `sign`, `cert`, `open`, or `pow` fields. The bridge peels them off, enforces them in JavaScript, and only then calls `pen.run(...)` on the pure predicate bytecode.

---

## 7.10 PoW policy — `pow` in `ZEN.pen(spec)`

When a soul is compiled with a `pow` policy, every write to that soul must include a valid proof-of-work nonce in `msg.put["^"]`. The proof is verified against the **canonical block** — all four fields that uniquely identify the write:

```js
JSON.stringify({
  "#": soul,   // namespace owner
  ".": key,    // the specific key
  ":": val,    // the value being written
  ">": state,  // HAM timestamp
}) + ":" + nonce
```

SHA-256 of this string must start with `unit.repeat(difficulty)`.

```js
// Compile a soul that requires PoW
const soul = ZEN.pen({ pow: { unit: "0", difficulty: 3 } })
// hash must start with "000"

// With sign as well (independent policies)
const soul = ZEN.pen({ sign: true, pow: { unit: "0", difficulty: 3 } })
```

### `field` is NOT part of the API

R[7] is always reserved for the PoW nonce. **Do not pass `field`** in the `pow` spec — it is ignored:

```js
// WRONG — field is silently ignored
ZEN.pen({ pow: { field: 7, difficulty: 3 } })

// CORRECT
ZEN.pen({ pow: { difficulty: 3 } })           // unit defaults to "0"
ZEN.pen({ pow: { unit: "0", difficulty: 3 } }) // explicit
```

### Auto-mining in `put`

When writing to a PoW soul, pass `pow` as a **policy object** in `opt` — ZEN will mine automatically:

```js
zen.get(soul).get(key).put(data, cb, {
  authenticator: pair,              // needed if soul has sign:true
  pow: { unit: "0", difficulty: 3 } // triggers auto-mining
})
```

Mining happens inside `penStage` before the predicate runs. The computed nonce is stored in `msg.put["^"]` and propagated to all peers. Peers re-verify by rebuilding the same canonical block.

**Defaults**: `unit` defaults to `"0"`, `difficulty` defaults to `3`. If `pow` is not passed at all, no mining occurs.

---

## 7.11 Using PEN for write access control

A practical example: only the key owner can write to a specific path.

```js
// Compile a policy: require a valid signature from the writer
const policy = ZEN.pen({ sign: true });

// Store the policy in the graph at a specific path
zen.get("~" + pair.pub).get("protected").put(
  { policy: policy },
  null,
  { authenticator: pair }
);

// Writing with authenticator — runs the policy
zen.get("~" + pair.pub).get("protected").get("data").put(
  "hello",
  function(ack) { console.log(ack); },
  { authenticator: pair }
);
```

The ZEN security middleware calls `pen.run(policy, registers)` through the bridge in `src/pen.js` before accepting the write. If it returns `false`, the write is rejected with an error.

---

## 7.12 `ZEN.candle` — time-window policies

A **candle** is a unit of time: `floor(Date.now() / windowMs)`. Two writes within the same candle share the same candle number.

```js
const window = 5 * 60 * 1000;  // 5-minute candles
const candle = Math.floor(Date.now() / window);
```

Candle-based policies restrict writes to a rolling time window. A write with a key like `"123456_data"` (candle_suffix) is only accepted if the candle in the key is within a small delta of the current candle.

The PEN bytecode for this check:

```
LET(0, DIVU(R[4], window),            ← current candle (R[128])
  LET(1, TONUM(SEG(R[0], '_', 0)),   ← candle from key (R[129])
    AND(2,
      GTE(R[129], SUB(R[128], 100)), ← key candle ≥ current − 100
      LTE(R[129], ADD(R[128], 2))    ← key candle ≤ current + 2
    )
  )
)
```

---

## 7.13 Building PEN from source

PEN Core is written in Zig. The source lives at `src/pen.zig` and `src/wasm.zig`.

```bash
# Build pen.wasm from Zig source
npm run build:pen

# Build pen.js (JS bridge + compiler) from zen.js
npm run build:zen
```

Zig must be on your `PATH`. See [ziglang.org](https://ziglang.org/download/) to install.

---

## 7.14 Testing PEN

```bash
npm run test:pen:unit
```

This runs `test/pen.js` with a 10-second timeout. Tests cover:
- `pen.pack` / `pen.unpack` round-trips
- `pen.bc.*` bytecode builder
- `pen.run(bytecode, regs)` evaluation
- Integration tests with ZEN graph writes using `{ authenticator: pair }` options
