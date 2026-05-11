// wasm.zig — WASM boundary for pen.zig
//
// Memory layout (shared linear memory):
//
//   [0..3]     u32 = bytecode length
//   [4..N+3]   bytecode bytes
//   [N+4..N+7] u32 = register count
//   [N+8..]    register data (packed)
//
// Register wire format (for each register):
//   [u8 tag][data...]
//   tag 0 = null
//   tag 1 = bool (1 byte: 0/1)
//   tag 2 = i64  (8 bytes LE)
//   tag 3 = f64  (8 bytes LE)
//   tag 4 = str  (2 bytes LE length + utf8 bytes)
//
// Exports:
//   alloc(size: u32) → ptr: u32   — allocate from internal bump allocator
//   free()                        — reset bump allocator
//   mem() → ptr: u32             — get start of shared buffer
//   run() → i32                  — 1=true, 0=false, -1=err, -2=bad_version, -3=max_depth
//
// This design is zero-import — no JS callbacks needed.

const pen = @import("pen.zig");

// 4 MiB static buffer — supports bytecode + large string register values.
// Increasing from 64 KiB (old) to 4 MiB matches pen.zig's str_heap size.
var buf: [4 * 1024 * 1024]u8 = undefined;
var bump: u32 = 0;

export fn mem() u32 {
    return @intFromPtr(&buf);
}

export fn alloc(size: u32) u32 {
    const ptr = bump;
    bump += size;
    return @intFromPtr(&buf) + ptr;
}

export fn free() void {
    bump = 0;
    pen.resetStrHeap();
}

// Parse registers from wire format starting at offset
fn parseRegs(offset: u32, count: u32, regs: []pen.Value) u32 {
    var off = offset;
    var i: u32 = 0;
    while (i < count and i < regs.len) : (i += 1) {
        if (off >= buf.len) break;
        const tag = buf[off];
        off += 1;
        switch (tag) {
            0 => {
                regs[i] = pen.vNull();
            },
            1 => {
                regs[i] = pen.vBool(if (off < buf.len) buf[off] != 0 else false);
                off += 1;
            },
            2 => { // i64 LE
                if (off + 8 > buf.len) {
                    regs[i] = pen.vNull();
                    break;
                }
                var v: i64 = 0;
                var k: u6 = 0;
                while (k < 8) : (k += 1) {
                    v |= @as(i64, buf[off]) << @intCast(k * 8);
                    off += 1;
                }
                regs[i] = pen.vInt(v);
            },
            3 => { // f64 LE
                if (off + 8 > buf.len) {
                    regs[i] = pen.vNull();
                    break;
                }
                var bits: u64 = 0;
                var k: u6 = 0;
                while (k < 8) : (k += 1) {
                    bits |= @as(u64, buf[off]) << @intCast(k * 8);
                    off += 1;
                }
                regs[i] = pen.vFloat(@bitCast(bits));
            },
            4 => { // str: 4 bytes LE u32 length + utf8
                if (off + 4 > buf.len) {
                    regs[i] = pen.vNull();
                    break;
                }
                const slen: u32 = @as(u32, buf[off]) | (@as(u32, buf[off + 1]) << 8) |
                    (@as(u32, buf[off + 2]) << 16) | (@as(u32, buf[off + 3]) << 24);
                off += 4;
                if (@as(u64, off) + @as(u64, slen) > buf.len) {
                    regs[i] = pen.vNull();
                    break;
                }
                regs[i] = pen.vStr(buf[off..].ptr, slen);
                off += slen;
            },
            else => {
                regs[i] = pen.vNull();
            },
        }
    }
    return off;
}

// Static register array — avoids putting 18KB on the shadow stack
var static_regs: [64]pen.Value = [_]pen.Value{pen.vNull()} ** 64;

export fn run() i32 {
    // Reset string heap before parsing registers (strings are allocated there)
    pen.resetStrHeap();

    // Read bytecode length from buf[0..3]
    const bclen: u32 = @as(u32, buf[0]) | (@as(u32, buf[1]) << 8) |
        (@as(u32, buf[2]) << 16) | (@as(u32, buf[3]) << 24);
    if (bclen == 0 or 4 + bclen > buf.len) return -1;

    const bc = buf[4 .. 4 + bclen];

    // Read register count from buf[4+bclen..4+bclen+3]
    const reg_off: u32 = 4 + bclen;
    if (reg_off + 4 > buf.len) return -1;
    const nregs: u32 = @as(u32, buf[reg_off]) | (@as(u32, buf[reg_off + 1]) << 8) |
        (@as(u32, buf[reg_off + 2]) << 16) | (@as(u32, buf[reg_off + 3]) << 24);

    // Reset static registers to null before use
    const actual_regs: u32 = if (nregs > 64) @as(u32, 64) else nregs;
    for (0..64) |i| static_regs[i] = pen.vNull();
    _ = parseRegs(reg_off + 4, actual_regs, static_regs[0..actual_regs]);

    const result = pen.run(bc, static_regs[0..actual_regs]) catch |err| {
        return switch (err) {
            pen.EvalError.BadVersion => -2,
            pen.EvalError.MaxDepth => -3,
            else => -1,
        };
    };
    return if (result) 1 else 0;
}
