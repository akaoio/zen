// pen.zig — PEN Core VM
// ISA v1, environment-agnostic, no stdlib dependency.
// Compiles to freestanding wasm32 with zero imports.
//
// Opcodes:
//   0x00 NULL    0x01 TRUE   0x02 FALSE
//   0x03 STR     0x04 UINT(uleb128) 0x07 INT(sleb128) 0x08 F64
//   0x10 REG(n)
//   0x20 AND(n)  0x21 OR(n)  0x22 NOT  0x23 PASS  0x24 FAIL
//   0x30 EQ  0x31 NE  0x32 LT  0x33 GT  0x34 LTE  0x35 GTE
//   0x40 ADD 0x41 SUB 0x42 MUL 0x43 DIVU 0x44 MOD 0x45 DIVF 0x46 ABS 0x47 NEG
//   0x50 LEN 0x51 SLICE 0x52 SEG 0x53 TONUM 0x54 TOSTR
//   0x55 CONCAT 0x56 PRE 0x57 SUF 0x58 INCLUDES 0x59 REGEX 0x5A UPPER 0x5B LOWER
//   0x60 ISS 0x61 ISN 0x62 ISX 0x63 ISB 0x64 LNG
//   0x70 LET(n, def, body)  0x71 IF(cond, then, else)
//   0x80 SEGR(reg, sep, idx)  0x81 SEGRN(reg, sep, idx)
//   0xE0..0xEF = inline uint 0..15
//   0xF0..0xF5 = REG(0)..REG(5) shorthands
//   0xF8..0xFB = local[0]..local[3] shorthands

const MAX_DEPTH: u32 = 32;
const MAX_LOCALS: usize = 32;
const MAX_REGS: usize = 64;

// String heap — freestanding bump allocator in WASM linear memory.
// Strings are stored here; Value holds (sptr, slen) instead of an inline buffer.
// resetStrHeap() must be called before each evaluation (in wasm.zig run()).
const MAX_STR_HEAP: usize = 4 * 1024 * 1024; // 4 MiB
var str_heap: [MAX_STR_HEAP]u8 = undefined;
var str_bump: u32 = 0;

pub fn resetStrHeap() void {
    str_bump = 0;
}

// Return the bytes of a TAG_STR Value.
fn strBytes(v: Value) []const u8 {
    if (v.slen == 0) return str_heap[0..0];
    return str_heap[v.sptr .. v.sptr + v.slen];
}

// Value tag
const TAG_NULL: u8 = 0;
const TAG_BOOL: u8 = 1;
const TAG_INT: u8 = 2;
const TAG_FLOAT: u8 = 3;
const TAG_STR: u8 = 4;
const TAG_ERR: u8 = 5;

pub const Value = struct {
    tag: u8,
    i: i64, // TAG_INT or TAG_BOOL (0/1) or TAG_ERR code
    f: f64, // TAG_FLOAT
    sptr: u32, // TAG_STR: byte offset into str_heap
    slen: u32, // TAG_STR: byte length (unlimited)
};

pub fn vNull() Value {
    var v: Value = undefined;
    v.tag = TAG_NULL;
    v.i = 0;
    v.f = 0;
    v.sptr = 0;
    v.slen = 0;
    return v;
}
pub fn vBool(b: bool) Value {
    var v: Value = undefined;
    v.tag = TAG_BOOL;
    v.i = if (b) 1 else 0;
    v.f = 0;
    v.sptr = 0;
    v.slen = 0;
    return v;
}
pub fn vInt(n: i64) Value {
    var v: Value = undefined;
    v.tag = TAG_INT;
    v.i = n;
    v.f = 0;
    v.sptr = 0;
    v.slen = 0;
    return v;
}
pub fn vFloat(f: f64) Value {
    var v: Value = undefined;
    v.tag = TAG_FLOAT;
    v.i = 0;
    v.f = f;
    v.sptr = 0;
    v.slen = 0;
    return v;
}
pub fn vStr(ptr: [*]const u8, len: usize) Value {
    var v: Value = undefined;
    v.tag = TAG_STR;
    v.i = 0;
    v.f = 0;
    const available = MAX_STR_HEAP - @as(usize, str_bump);
    const l = if (len > available) available else len;
    const off = str_bump;
    var i: usize = 0;
    while (i < l) : (i += 1) str_heap[off + i] = ptr[i];
    str_bump += @intCast(l);
    v.sptr = off;
    v.slen = @intCast(l);
    return v;
}
fn vStrSlice(sl: []const u8) Value {
    return vStr(sl.ptr, sl.len);
}
pub fn vErr(code: i64) Value {
    var v: Value = undefined;
    v.tag = TAG_ERR;
    v.i = code;
    v.f = 0;
    v.sptr = 0;
    v.slen = 0;
    return v;
}

fn isTruthy(v: Value) bool {
    return switch (v.tag) {
        TAG_NULL => false,
        TAG_BOOL => v.i != 0,
        TAG_INT => v.i != 0,
        TAG_FLOAT => v.f != 0.0,
        TAG_STR => v.slen > 0,
        else => false,
    };
}

fn toNumber(v: Value) f64 {
    return switch (v.tag) {
        TAG_INT => @floatFromInt(v.i),
        TAG_FLOAT => v.f,
        TAG_BOOL => @floatFromInt(v.i),
        TAG_STR => parseF64(strBytes(v)),
        else => 0.0,
    };
}

fn valEqual(a: Value, b: Value) bool {
    if (a.tag == TAG_NULL and b.tag == TAG_NULL) return true;
    if (a.tag == TAG_NULL or b.tag == TAG_NULL) return false;
    if (a.tag == TAG_STR and b.tag == TAG_STR) {
        if (a.slen != b.slen) return false;
        const ba = strBytes(a);
        const bb = strBytes(b);
        var i: usize = 0;
        while (i < a.slen) : (i += 1) {
            if (ba[i] != bb[i]) return false;
        }
        return true;
    }
    // numeric compare
    return toNumber(a) == toNumber(b);
}

fn strCmp(a: Value, b: Value) i32 {
    // lexicographic for strings, numeric for others
    if (a.tag == TAG_STR and b.tag == TAG_STR) {
        const ba = strBytes(a);
        const bb = strBytes(b);
        const la = a.slen;
        const lb = b.slen;
        const min = if (la < lb) la else lb;
        var i: usize = 0;
        while (i < min) : (i += 1) {
            if (ba[i] < bb[i]) return -1;
            if (ba[i] > bb[i]) return 1;
        }
        if (la < lb) return -1;
        if (la > lb) return 1;
        return 0;
    }
    const an = toNumber(a);
    const bn = toNumber(b);
    if (an < bn) return -1;
    if (an > bn) return 1;
    return 0;
}

// Simple float parser (no libc)
fn parseF64(s: []const u8) f64 {
    var r: f64 = 0;
    var dec: f64 = 0;
    var neg = false;
    var i: usize = 0;
    if (i < s.len and s[i] == '-') {
        neg = true;
        i += 1;
    }
    while (i < s.len and s[i] >= '0' and s[i] <= '9') : (i += 1) {
        r = r * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
    }
    if (i < s.len and s[i] == '.') {
        i += 1;
        dec = 0.1;
        while (i < s.len and s[i] >= '0' and s[i] <= '9') : (i += 1) {
            r += @as(f64, @floatFromInt(s[i] - '0')) * dec;
            dec *= 0.1;
        }
    }
    return if (neg) -r else r;
}

// f64 to string (simplified, enough for test output)
fn f64ToStr(f: f64, buf: []u8) usize {
    // Handle special cases
    if (f != f) { // NaN
        buf[0] = 'N';
        buf[1] = 'a';
        buf[2] = 'N';
        return 3;
    }
    // Handle Infinity before @intFromFloat (which is UB on Inf)
    if (f > 1.7976931348623157e+308 or f < -1.7976931348623157e+308) {
        if (f > 0) {
            buf[0] = 'I';
            buf[1] = 'n';
            buf[2] = 'f';
            return 3;
        } else {
            buf[0] = '-';
            buf[1] = 'I';
            buf[2] = 'n';
            buf[3] = 'f';
            return 4;
        }
    }
    var val = f;
    var pos: usize = 0;
    if (val < 0) {
        buf[pos] = '-';
        pos += 1;
        val = -val;
    }
    // Safety threshold: i64 can only hold values up to ~9.22e18
    // For larger floats use f64 digit extraction to avoid @intFromFloat UB
    const I64_MAX_F64: f64 = 9.22337203685477e18;
    if (val >= I64_MAX_F64) {
        var n = @trunc(val);
        var tmp2: [32]u8 = undefined;
        var tlen2: usize = 0;
        while (n >= 1.0 and tlen2 < 30) {
            var digit = @mod(n, 10.0);
            if (digit < 0.0) digit = 0.0;
            if (digit > 9.0) digit = 9.0;
            tmp2[tlen2] = @as(u8, @intFromFloat(digit)) + '0';
            tlen2 += 1;
            n = @trunc(n / 10.0);
        }
        if (tlen2 == 0) {
            tmp2[0] = '0';
            tlen2 = 1;
        }
        var j2: usize = tlen2;
        while (j2 > 0) : (j2 -= 1) {
            buf[pos] = tmp2[j2 - 1];
            pos += 1;
        }
        return pos; // large floats have no meaningful fractional part
    }
    // Integer part (safe: val < I64_MAX_F64 ≤ i64.MAX)
    const ipart: i64 = @intFromFloat(val);
    const fpart: f64 = val - @as(f64, @floatFromInt(ipart));
    // Write integer part
    if (ipart == 0) {
        buf[pos] = '0';
        pos += 1;
    } else {
        var tmp: [20]u8 = undefined;
        var tlen: usize = 0;
        var n = ipart;
        while (n > 0) : (n = @divTrunc(n, 10)) {
            tmp[tlen] = @intCast(@mod(n, 10) + '0');
            tlen += 1;
        }
        var j: usize = tlen;
        while (j > 0) : (j -= 1) {
            buf[pos] = tmp[j - 1];
            pos += 1;
        }
    }
    // Decimal part (up to 6 places, trim trailing zeros)
    if (fpart > 0.000001) {
        buf[pos] = '.';
        pos += 1;
        var fp = fpart;
        var places: usize = 0;
        var decbuf: [6]u8 = undefined;
        while (places < 6) : (places += 1) {
            fp *= 10.0;
            const d: u8 = @intCast(@as(i64, @intFromFloat(fp)));
            decbuf[places] = d + '0';
            fp -= @as(f64, @floatFromInt(d));
        }
        // trim trailing zeros
        var dlen: usize = 6;
        while (dlen > 1 and decbuf[dlen - 1] == '0') dlen -= 1;
        var k: usize = 0;
        while (k < dlen) : (k += 1) {
            buf[pos] = decbuf[k];
            pos += 1;
        }
    }
    return pos;
}

fn intToStr(n: i64, buf: []u8) usize {
    if (n == 0) {
        buf[0] = '0';
        return 1;
    }
    // Special-case i64.MIN: -v would overflow (two's complement)
    if (n == -9223372036854775808) {
        // Write "-9223372036854775808" literally
        const digits = [_]u8{ '-', '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8', '5', '4', '7', '7', '5', '8', '0', '8' };
        var k: usize = 0;
        while (k < digits.len) : (k += 1) buf[k] = digits[k];
        return digits.len;
    }
    var tmp: [20]u8 = undefined;
    var tlen: usize = 0;
    var pos: usize = 0;
    var neg = false;
    var v = n;
    if (v < 0) {
        neg = true;
        v = -v;
    }
    while (v > 0) : (v = @divTrunc(v, 10)) {
        tmp[tlen] = @intCast(@mod(v, 10) + '0');
        tlen += 1;
    }
    if (neg) {
        buf[pos] = '-';
        pos += 1;
    }
    var j: usize = tlen;
    while (j > 0) : (j -= 1) {
        buf[pos] = tmp[j - 1];
        pos += 1;
    }
    return pos;
}

// startsWith
fn strStartsWith(s: Value, prefix: Value) bool {
    if (s.tag != TAG_STR or prefix.tag != TAG_STR) return false;
    if (prefix.slen > s.slen) return false;
    const bs = strBytes(s);
    const bp = strBytes(prefix);
    var i: usize = 0;
    while (i < prefix.slen) : (i += 1) {
        if (bs[i] != bp[i]) return false;
    }
    return true;
}

// endsWith
fn strEndsWith(s: Value, suffix: Value) bool {
    if (s.tag != TAG_STR or suffix.tag != TAG_STR) return false;
    if (suffix.slen > s.slen) return false;
    const bs = strBytes(s);
    const bsuf = strBytes(suffix);
    const offset = s.slen - suffix.slen;
    var i: usize = 0;
    while (i < suffix.slen) : (i += 1) {
        if (bs[offset + i] != bsuf[i]) return false;
    }
    return true;
}

// includes
fn strIncludes(s: Value, needle: Value) bool {
    if (s.tag != TAG_STR or needle.tag != TAG_STR) return false;
    if (needle.slen == 0) return true;
    if (needle.slen > s.slen) return false;
    const bs = strBytes(s);
    const bn = strBytes(needle);
    const limit = s.slen - needle.slen;
    var i: usize = 0;
    while (i <= limit) : (i += 1) {
        var match = true;
        var j: usize = 0;
        while (j < needle.slen) : (j += 1) {
            if (bs[i + j] != bn[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

// lower/upper — allocate new string in str_heap
fn strToLower(v: Value) Value {
    if (v.tag != TAG_STR) return v;
    const src = strBytes(v);
    const off = str_bump;
    const available = MAX_STR_HEAP - @as(usize, str_bump);
    const l: u32 = if (@as(usize, v.slen) > available) @intCast(available) else v.slen;
    var i: usize = 0;
    while (i < l) : (i += 1) {
        const c = src[i];
        str_heap[off + i] = if (c >= 'A' and c <= 'Z') c + 32 else c;
    }
    str_bump += l;
    return Value{ .tag = TAG_STR, .i = 0, .f = 0, .sptr = off, .slen = l };
}
fn strToUpper(v: Value) Value {
    if (v.tag != TAG_STR) return v;
    const src = strBytes(v);
    const off = str_bump;
    const available = MAX_STR_HEAP - @as(usize, str_bump);
    const l: u32 = if (@as(usize, v.slen) > available) @intCast(available) else v.slen;
    var i: usize = 0;
    while (i < l) : (i += 1) {
        const c = src[i];
        str_heap[off + i] = if (c >= 'a' and c <= 'z') c - 32 else c;
    }
    str_bump += l;
    return Value{ .tag = TAG_STR, .i = 0, .f = 0, .sptr = off, .slen = l };
}

// SEG(str, sep_byte, idx) → split str by sep, return segment at idx
fn strSeg(s: Value, sep: u8, idx: i64) Value {
    if (s.tag != TAG_STR) return vStr("", 0);
    const bs = strBytes(s);
    var seg_idx: i64 = 0;
    var seg_start: usize = 0;
    var i: usize = 0;
    while (i <= s.slen) : (i += 1) {
        const at_sep = (i == s.slen or bs[i] == sep);
        if (at_sep) {
            if (seg_idx == idx) {
                return vStrSlice(bs[seg_start..i]);
            }
            seg_idx += 1;
            seg_start = i + 1;
        }
    }
    return vStr("", 0);
}

// ULEB128 read
fn readUleb(bc: []const u8, pos: *usize) u64 {
    var result: u64 = 0;
    var shift: u7 = 0;
    while (pos.* < bc.len) {
        const b = bc[pos.*];
        pos.* += 1;
        if (shift < 64) {
            result |= @as(u64, b & 0x7F) << @intCast(shift);
        }
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

// SLEB128 read
fn readSleb(bc: []const u8, pos: *usize) i64 {
    var result: i64 = 0;
    var shift: u7 = 0;
    var b: u8 = 0;
    while (pos.* < bc.len) {
        b = bc[pos.*];
        pos.* += 1;
        if (shift < 64) {
            result |= @as(i64, b & 0x7F) << @intCast(shift);
        }
        shift += 7;
        if ((b & 0x80) == 0) break;
    }
    if (shift < 64 and (b & 0x40) != 0) {
        result |= -(@as(i64, 1) << @intCast(shift));
    }
    return result;
}

pub const EvalError = error{
    BadVersion,
    UnknownOpcode,
    OutOfBounds,
    MaxDepth,
    DivisionByZero,
    BadBytecode,
};

pub const Ctx = struct {
    bc: []const u8,
    regs: []const Value,
    locals: [MAX_LOCALS]Value,
    depth: u32,
};

// skipExpr — advance pos past one complete expression without evaluating it.
// Used for short-circuit evaluation in AND, OR, and lazy IF branches.
fn skipExpr(bc: []const u8, pos: *usize, depth: u32) EvalError!void {
    if (depth == 0) return EvalError.MaxDepth;
    if (pos.* >= bc.len) return EvalError.OutOfBounds;
    const op = bc[pos.*];
    pos.* += 1;

    // Inline int shortcuts 0xE0..0xEF — no children
    if (op >= 0xE0 and op <= 0xEF) return;
    // Register shorthands 0xF0..0xF5 — no children
    if (op >= 0xF0 and op <= 0xF5) return;
    // Local shorthands 0xF8..0xFB — no children
    if (op >= 0xF8 and op <= 0xFB) return;

    switch (op) {
        // Leaf opcodes — no children
        0x00, 0x01, 0x02 => {},
        0x23, 0x24 => {},

        // STR: [u8 len] + len bytes
        // STR: 0x03 [uleb128 len] [utf8...]
        0x03 => {
            const len: usize = @intCast(readUleb(bc, pos));
            if (pos.* + len > bc.len) return EvalError.BadBytecode;
            pos.* += len;
        },
        // UINT (ULEB128): variable length
        0x04 => {
            _ = readUleb(bc, pos);
        },
        // INT (SLEB128): variable length
        0x07 => {
            _ = readSleb(bc, pos);
        },
        // F64: 8 bytes
        0x08 => {
            if (pos.* + 8 > bc.len) return EvalError.BadBytecode;
            pos.* += 8;
        },
        // REG(n): 1 byte arg
        0x10 => {
            if (pos.* >= bc.len) return EvalError.BadBytecode;
            pos.* += 1;
        },
        // AND(n) / OR(n): n children
        0x20, 0x21 => {
            if (pos.* >= bc.len) return EvalError.BadBytecode;
            const n = bc[pos.*];
            pos.* += 1;
            var i: u8 = 0;
            while (i < n) : (i += 1) try skipExpr(bc, pos, depth - 1);
        },
        // NOT, unary ops with 1 child
        0x22, 0x46, 0x47, 0x50, 0x53, 0x54, 0x5A, 0x5B, 0x60, 0x61, 0x62, 0x63 => {
            try skipExpr(bc, pos, depth - 1);
        },
        // Binary ops: 2 children
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x55, 0x56, 0x57, 0x58, 0x59 => {
            try skipExpr(bc, pos, depth - 1);
            try skipExpr(bc, pos, depth - 1);
        },
        // SLICE: 3 children
        0x51 => {
            try skipExpr(bc, pos, depth - 1);
            try skipExpr(bc, pos, depth - 1);
            try skipExpr(bc, pos, depth - 1);
        },
        // SEG: 1 child + 1 sep byte + 1 child
        0x52 => {
            try skipExpr(bc, pos, depth - 1);
            if (pos.* >= bc.len) return EvalError.BadBytecode;
            pos.* += 1; // sep byte
            try skipExpr(bc, pos, depth - 1);
        },
        // LNG: 1 child + 2 literal bytes (min, max)
        0x64 => {
            try skipExpr(bc, pos, depth - 1);
            if (pos.* + 2 > bc.len) return EvalError.BadBytecode;
            pos.* += 2;
        },
        // LET(slot, def, body): 1 literal byte + 2 children
        0x70 => {
            if (pos.* >= bc.len) return EvalError.BadBytecode;
            pos.* += 1; // slot
            try skipExpr(bc, pos, depth - 1); // def
            try skipExpr(bc, pos, depth - 1); // body
        },
        // IF(cond, then, else): 3 children
        0x71 => {
            try skipExpr(bc, pos, depth - 1);
            try skipExpr(bc, pos, depth - 1);
            try skipExpr(bc, pos, depth - 1);
        },
        // SEGR / SEGRN: 3 literal bytes
        0x80, 0x81 => {
            if (pos.* + 3 > bc.len) return EvalError.BadBytecode;
            pos.* += 3;
        },
        else => return EvalError.UnknownOpcode,
    }
}

pub fn eval(ctx: *Ctx, pos: *usize) EvalError!Value {
    if (ctx.depth >= MAX_DEPTH) return EvalError.MaxDepth;
    if (pos.* >= ctx.bc.len) return EvalError.OutOfBounds;

    ctx.depth += 1;
    defer ctx.depth -= 1;

    const op = ctx.bc[pos.*];
    pos.* += 1;

    // Inline integer shortcuts 0xE0..0xEF = 0..15
    if (op >= 0xE0 and op <= 0xEF) return vInt(op - 0xE0);

    // Register shorthands
    if (op >= 0xF0 and op <= 0xF5) {
        const n = op - 0xF0;
        if (n >= ctx.regs.len) return vNull();
        return ctx.regs[n];
    }
    if (op >= 0xF8 and op <= 0xFB) {
        return ctx.locals[op - 0xF8];
    }

    switch (op) {
        0x00 => return vNull(),
        0x01 => return vBool(true),
        0x02 => return vBool(false),

        // STR: 0x03 [uleb128 len] [utf8...]
        0x03 => {
            const len: usize = @intCast(readUleb(ctx.bc, pos));
            if (pos.* + len > ctx.bc.len) return EvalError.BadBytecode;
            const start = pos.*;
            pos.* += len;
            return vStr(ctx.bc[start..].ptr, len);
        },

        // UINT (ULEB128) — clamp to i64.MAX to avoid @intCast undefined behavior
        0x04 => {
            const u = readUleb(ctx.bc, pos);
            const clamped: i64 = if (u > @as(u64, 9223372036854775807)) 9223372036854775807 else @intCast(u);
            return vInt(clamped);
        },

        // INT (SLEB128)
        0x07 => return vInt(readSleb(ctx.bc, pos)),

        // F64 big-endian
        0x08 => {
            if (pos.* + 8 > ctx.bc.len) return EvalError.BadBytecode;
            var bytes: [8]u8 = undefined;
            var k: usize = 0;
            while (k < 8) : (k += 1) {
                bytes[k] = ctx.bc[pos.*];
                pos.* += 1;
            }
            const bits = (@as(u64, bytes[0]) << 56) | (@as(u64, bytes[1]) << 48) |
                (@as(u64, bytes[2]) << 40) | (@as(u64, bytes[3]) << 32) |
                (@as(u64, bytes[4]) << 24) | (@as(u64, bytes[5]) << 16) |
                (@as(u64, bytes[6]) << 8) | @as(u64, bytes[7]);
            return vFloat(@bitCast(bits));
        },

        // REG(n)
        0x10 => {
            if (pos.* >= ctx.bc.len) return EvalError.BadBytecode;
            const n = ctx.bc[pos.*];
            pos.* += 1;
            if (n >= 128) {
                const li = @as(usize, n - 128);
                if (li >= MAX_LOCALS) return vNull();
                return ctx.locals[li];
            }
            if (n >= ctx.regs.len) return vNull();
            return ctx.regs[n];
        },

        // AND(n) — short-circuit: skip remaining args on first false
        0x20 => {
            if (pos.* >= ctx.bc.len) return EvalError.BadBytecode;
            const n = ctx.bc[pos.*];
            pos.* += 1;
            var ok = true;
            var i: u8 = 0;
            while (i < n) : (i += 1) {
                if (ok) {
                    const v = try eval(ctx, pos);
                    if (!isTruthy(v)) ok = false;
                } else {
                    try skipExpr(ctx.bc, pos, MAX_DEPTH - ctx.depth);
                }
            }
            return vBool(ok);
        },

        // OR(n) — short-circuit: skip remaining args on first true
        0x21 => {
            if (pos.* >= ctx.bc.len) return EvalError.BadBytecode;
            const n = ctx.bc[pos.*];
            pos.* += 1;
            var ok = false;
            var i: u8 = 0;
            while (i < n) : (i += 1) {
                if (!ok) {
                    const v = try eval(ctx, pos);
                    if (isTruthy(v)) ok = true;
                } else {
                    try skipExpr(ctx.bc, pos, MAX_DEPTH - ctx.depth);
                }
            }
            return vBool(ok);
        },

        0x22 => { // NOT
            const v = try eval(ctx, pos);
            return vBool(!isTruthy(v));
        },
        0x23 => return vBool(true), // PASS
        0x24 => return vBool(false), // FAIL

        // Comparison
        0x30 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(valEqual(a, b));
        },
        0x31 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(!valEqual(a, b));
        },
        0x32 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strCmp(a, b) < 0);
        },
        0x33 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strCmp(a, b) > 0);
        },
        0x34 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strCmp(a, b) <= 0);
        },
        0x35 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strCmp(a, b) >= 0);
        },

        // Arithmetic
        0x40 => { // ADD
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            if (a.tag == TAG_INT and b.tag == TAG_INT) return vInt(a.i + b.i);
            return vFloat(toNumber(a) + toNumber(b));
        },
        0x41 => { // SUB
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            if (a.tag == TAG_INT and b.tag == TAG_INT) return vInt(a.i - b.i);
            return vFloat(toNumber(a) - toNumber(b));
        },
        0x42 => { // MUL
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            if (a.tag == TAG_INT and b.tag == TAG_INT) return vInt(a.i * b.i);
            return vFloat(toNumber(a) * toNumber(b));
        },
        0x43 => { // DIVU — floor division
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            const bn = toNumber(b);
            if (bn == 0) return EvalError.DivisionByZero;
            if (a.tag == TAG_INT and b.tag == TAG_INT) {
                if (b.i == 0) return EvalError.DivisionByZero;
                return vInt(@divFloor(a.i, b.i));
            }
            return vFloat(@floor(toNumber(a) / bn));
        },
        0x44 => { // MOD
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            if (a.tag == TAG_INT and b.tag == TAG_INT) {
                if (b.i == 0) return EvalError.DivisionByZero;
                return vInt(@mod(a.i, b.i));
            }
            return vFloat(@mod(toNumber(a), toNumber(b)));
        },
        0x45 => { // DIVF
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vFloat(toNumber(a) / toNumber(b));
        },
        0x46 => { // ABS — special-case i64.MIN to avoid overflow
            const a = try eval(ctx, pos);
            if (a.tag == TAG_INT) {
                if (a.i == -9223372036854775808) return vFloat(9223372036854775808.0);
                return vInt(if (a.i < 0) -a.i else a.i);
            }
            return vFloat(@abs(toNumber(a)));
        },
        0x47 => { // NEG — special-case i64.MIN to avoid overflow
            const a = try eval(ctx, pos);
            if (a.tag == TAG_INT) {
                if (a.i == -9223372036854775808) return vFloat(9223372036854775808.0);
                return vInt(-a.i);
            }
            return vFloat(-toNumber(a));
        },

        // String ops
        0x50 => { // LEN
            const a = try eval(ctx, pos);
            const l: i64 = if (a.tag == TAG_STR) @intCast(a.slen) else 0;
            return vInt(l);
        },
        0x51 => { // SLICE(str, start, end)
            const s = try eval(ctx, pos);
            const st_v = try eval(ctx, pos);
            const en_v = try eval(ctx, pos);
            if (s.tag != TAG_STR) return vStr("", 0);
            const max_f: f64 = @floatFromInt(s.slen);
            var stf = toNumber(st_v);
            var enf = toNumber(en_v);
            if (stf != stf or stf < 0.0) stf = 0.0; // NaN or negative → 0
            if (enf != enf or enf < 0.0) enf = 0.0;
            if (stf > max_f) stf = max_f;
            if (enf > max_f) enf = max_f;
            var st: usize = @intFromFloat(stf);
            var en: usize = @intFromFloat(enf);
            if (st > s.slen) st = s.slen;
            if (en > s.slen) en = s.slen;
            if (st > en) st = en;
            const bs = strBytes(s);
            return vStr(bs[st..].ptr, en - st);
        },
        0x52 => { // SEG(str_expr, sep_byte, idx_expr)
            const s = try eval(ctx, pos);
            if (pos.* >= ctx.bc.len) return EvalError.BadBytecode;
            const sep = ctx.bc[pos.*];
            pos.* += 1;
            const idx_v = try eval(ctx, pos);
            const idx_f = toNumber(idx_v);
            if (idx_f != idx_f or idx_f < 0.0) return vStr("", 0);
            const idx: i64 = if (idx_f >= 4294967295.0) 4294967295 else @intFromFloat(idx_f);
            return strSeg(s, sep, idx);
        },
        0x53 => { // TONUM
            const a = try eval(ctx, pos);
            if (a.tag == TAG_INT) return a;
            if (a.tag == TAG_FLOAT) return a;
            if (a.tag == TAG_STR) return vFloat(parseF64(strBytes(a)));
            return vInt(0);
        },
        0x54 => { // TOSTR
            const a = try eval(ctx, pos);
            switch (a.tag) {
                TAG_STR => return a,
                TAG_INT => {
                    var buf: [32]u8 = undefined;
                    const l = intToStr(a.i, &buf);
                    return vStr(&buf, l);
                },
                TAG_FLOAT => {
                    var buf: [32]u8 = undefined;
                    const l = f64ToStr(a.f, &buf);
                    return vStr(&buf, l);
                },
                TAG_BOOL => return if (a.i != 0) vStr("true", 4) else vStr("false", 5),
                else => return vStr("null", 4),
            }
        },
        0x55 => { // CONCAT — coerce non-strings via TOSTR then concatenate
            const a_raw = try eval(ctx, pos);
            const b_raw = try eval(ctx, pos);
            // coerce each operand to string
            var as_buf: [32]u8 = undefined;
            var bs_buf: [32]u8 = undefined;
            const a: Value = switch (a_raw.tag) {
                TAG_STR => a_raw,
                TAG_INT => blk: {
                    const l = intToStr(a_raw.i, &as_buf);
                    break :blk vStr(&as_buf, l);
                },
                TAG_FLOAT => blk: {
                    const l = f64ToStr(a_raw.f, &as_buf);
                    break :blk vStr(&as_buf, l);
                },
                TAG_BOOL => if (a_raw.i != 0) vStr("true", 4) else vStr("false", 5),
                else => vStr("null", 4),
            };
            const b: Value = switch (b_raw.tag) {
                TAG_STR => b_raw,
                TAG_INT => blk: {
                    const l = intToStr(b_raw.i, &bs_buf);
                    break :blk vStr(&bs_buf, l);
                },
                TAG_FLOAT => blk: {
                    const l = f64ToStr(b_raw.f, &bs_buf);
                    break :blk vStr(&bs_buf, l);
                },
                TAG_BOOL => if (b_raw.i != 0) vStr("true", 4) else vStr("false", 5),
                else => vStr("null", 4),
            };
            // Allocate concatenated result in str_heap
            const off = str_bump;
            const avail1 = MAX_STR_HEAP - @as(usize, str_bump);
            const a_copy: u32 = if (@as(usize, a.slen) > avail1) @intCast(avail1) else a.slen;
            const ba = strBytes(a);
            var k: usize = 0;
            while (k < a_copy) : (k += 1) str_heap[off + k] = ba[k];
            str_bump += a_copy;
            const avail2 = MAX_STR_HEAP - @as(usize, str_bump);
            const b_copy: u32 = if (@as(usize, b.slen) > avail2) @intCast(avail2) else b.slen;
            const bb = strBytes(b);
            var j: usize = 0;
            while (j < b_copy) : (j += 1) str_heap[str_bump + j] = bb[j];
            str_bump += b_copy;
            return Value{ .tag = TAG_STR, .i = 0, .f = 0, .sptr = off, .slen = a_copy + b_copy };
        },
        0x56 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strStartsWith(a, b));
        }, // PRE
        0x57 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strEndsWith(a, b));
        }, // SUF
        0x58 => {
            const a = try eval(ctx, pos);
            const b = try eval(ctx, pos);
            return vBool(strIncludes(a, b));
        }, // INCLUDES
        0x59 => { // REGEX — not implemented in core (host extension), return false
            _ = try eval(ctx, pos);
            _ = try eval(ctx, pos);
            return vBool(false);
        },
        0x5A => {
            const a = try eval(ctx, pos);
            return strToUpper(a);
        }, // UPPER
        0x5B => {
            const a = try eval(ctx, pos);
            return strToLower(a);
        }, // LOWER

        // Type checks
        0x60 => {
            const v = try eval(ctx, pos);
            return vBool(v.tag == TAG_STR);
        }, // ISS
        0x61 => {
            const v = try eval(ctx, pos);
            return vBool(v.tag == TAG_INT or v.tag == TAG_FLOAT);
        }, // ISN
        0x62 => {
            const v = try eval(ctx, pos);
            return vBool(v.tag == TAG_NULL);
        }, // ISX
        0x63 => {
            const v = try eval(ctx, pos);
            return vBool(v.tag == TAG_BOOL);
        }, // ISB
        0x64 => { // LNG: string len in [min, max] — non-string always fails
            const s = try eval(ctx, pos);
            if (pos.* + 2 > ctx.bc.len) return EvalError.BadBytecode;
            const mn = ctx.bc[pos.*];
            pos.* += 1;
            const mx = ctx.bc[pos.*];
            pos.* += 1;
            if (s.tag != TAG_STR) return vBool(false);
            return vBool(s.slen >= mn and s.slen <= mx);
        },

        // LET(n, def, body)
        0x70 => {
            if (pos.* >= ctx.bc.len) return EvalError.BadBytecode;
            const slot = ctx.bc[pos.*];
            pos.* += 1;
            if (slot >= MAX_LOCALS) return EvalError.BadBytecode;
            const def = try eval(ctx, pos);
            ctx.locals[slot] = def;
            return eval(ctx, pos);
        },

        // IF(cond, then, else) — lazy: only evaluate the taken branch
        0x71 => {
            const cond = try eval(ctx, pos);
            if (isTruthy(cond)) {
                const then_v = try eval(ctx, pos);
                try skipExpr(ctx.bc, pos, MAX_DEPTH - ctx.depth);
                return then_v;
            } else {
                try skipExpr(ctx.bc, pos, MAX_DEPTH - ctx.depth);
                return eval(ctx, pos);
            }
        },

        // SEGR macro: 0x80 [u8 reg][u8 sep][u8 idx]
        0x80 => {
            if (pos.* + 3 > ctx.bc.len) return EvalError.BadBytecode;
            const reg = ctx.bc[pos.*];
            pos.* += 1;
            const sep = ctx.bc[pos.*];
            pos.* += 1;
            const idx: i64 = ctx.bc[pos.*];
            pos.* += 1;
            const s = if (reg < 128)
                (if (reg < ctx.regs.len) ctx.regs[reg] else vNull())
            else if (reg - 128 < MAX_LOCALS)
                ctx.locals[reg - 128]
            else
                vNull();
            return strSeg(s, sep, idx);
        },

        // SEGRN macro: 0x81 [u8 reg][u8 sep][u8 idx] → TONUM(SEG(...))
        0x81 => {
            if (pos.* + 3 > ctx.bc.len) return EvalError.BadBytecode;
            const reg = ctx.bc[pos.*];
            pos.* += 1;
            const sep = ctx.bc[pos.*];
            pos.* += 1;
            const idx: i64 = ctx.bc[pos.*];
            pos.* += 1;
            const s = if (reg < 128)
                (if (reg < ctx.regs.len) ctx.regs[reg] else vNull())
            else if (reg - 128 < MAX_LOCALS)
                ctx.locals[reg - 128]
            else
                vNull();
            const seg = strSeg(s, sep, idx);
            if (seg.tag != TAG_STR) return vFloat(0);
            return vFloat(parseF64(strBytes(seg)));
        },

        else => return EvalError.UnknownOpcode,
    }
}

// Static context — avoids putting the large Ctx (locals array) on the shadow stack
var static_ctx: Ctx = Ctx{
    .bc = &[_]u8{},
    .regs = &[_]Value{},
    .locals = [_]Value{Value{ .tag = TAG_NULL, .i = 0, .f = 0, .sptr = 0, .slen = 0 }} ** MAX_LOCALS,
    .depth = 0,
};

pub fn run(bc: []const u8, regs: []const Value) EvalError!bool {
    if (bc.len < 2) return EvalError.BadBytecode;
    if (bc[0] != 0x01) return EvalError.BadVersion;
    // Reset static context
    static_ctx.bc = bc;
    static_ctx.regs = regs;
    static_ctx.depth = 0;
    for (0..MAX_LOCALS) |i| static_ctx.locals[i] = vNull();
    var pos: usize = 1;
    const result = try eval(&static_ctx, &pos);
    return isTruthy(result);
}
