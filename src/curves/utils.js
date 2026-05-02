function createCurveCore(config) {
  const curve = config.curve;
  const P = config.P;
  const N = config.N;
  const A = config.A;
  const B = config.B;
  const G = config.G;
  const HALF_N = N >> 1n;
  const shim = config.shim;
  const base62 = config.base62;
  const settings = config.settings;
  const sha256 = config.sha256;
  const extras = config.extras || {};

  function mod(a, m) {
    return ((a % m) + m) % m;
  }

  function modPow(base, exp, modn) {
    let result = 1n;
    let value = mod(base, modn);
    let power = exp;
    while (power > 0n) {
      if (power & 1n) {
        result = mod(result * value, modn);
      }
      value = mod(value * value, modn);
      power >>= 1n;
    }
    return result;
  }

  function modInv(a, modn) {
    if (!a) {
      throw new Error("Inverse does not exist");
    }
    return modPow(mod(a, modn), modn - 2n, modn);
  }

  function isPoint(point) {
    return (
      !!point && typeof point.x === "bigint" && typeof point.y === "bigint"
    );
  }

  function isOnCurve(point) {
    if (!isPoint(point)) {
      return false;
    }
    if (point.x < 0n || point.x >= P || point.y < 0n || point.y >= P) {
      return false;
    }
    return (
      mod(
        point.y * point.y - (point.x * point.x * point.x + A * point.x + B),
        P,
      ) === 0n
    );
  }

  function pointAdd(left, right) {
    if (!left) {
      return right;
    }
    if (!right) {
      return left;
    }
    if (left.x === right.x && mod(left.y + right.y, P) === 0n) {
      return null;
    }
    let slope;
    if (left.x === right.x && left.y === right.y) {
      slope = mod(
        (3n * mod(left.x * left.x, P) + A) * modInv(2n * left.y, P),
        P,
      );
    } else {
      slope = mod((right.y - left.y) * modInv(right.x - left.x, P), P);
    }
    const x = mod(slope * slope - left.x - right.x, P);
    const y = mod(slope * (left.x - x) - left.y, P);
    return { x, y };
  }

  // ── Jacobian coordinate helpers ────────────────────────────────────────────
  // Jacobian (X:Y:Z) represents affine (X/Z², Y/Z³). Infinity ↔ Z=0.
  // Avoids one modInv per point addition (only one at the end per scalar mult).

  function fadd(a, b) { return mod(a + b, P); }
  function fsub(a, b) { return mod(a - b, P); }
  function fmul(a, b) { return mod(a * b, P); }

  // Jacobian doubling. Uses dbl-2007-bl formulas from hyperelliptic.org/EFD.
  function jacobianDouble(X1, Y1, Z1) {
    if (!Z1) return [0n, 1n, 0n];
    const XX = fmul(X1, X1);
    const YY = fmul(Y1, Y1);
    const YYYY = fmul(YY, YY);
    const ZZ = fmul(Z1, Z1);
    const X1pYY = fadd(X1, YY);
    const S = fadd(
      fsub(fsub(fmul(X1pYY, X1pYY), XX), YYYY),
      fsub(fsub(fmul(X1pYY, X1pYY), XX), YYYY),
    );
    const M = A === 0n
      ? fmul(3n, XX)
      : fadd(fmul(3n, XX), fmul(A, fmul(ZZ, ZZ)));
    const T = fsub(fmul(M, M), fadd(S, S));
    const Y3 = fsub(fmul(M, fsub(S, T)), fmul(8n, YYYY));
    const Y1pZ1 = fadd(Y1, Z1);
    const Z3 = fsub(fsub(fmul(Y1pZ1, Y1pZ1), YY), ZZ);
    return [T, Y3, Z3];
  }

  // Jacobian + affine mixed addition. Uses madd-2007-bl formulas.
  function jacobianMixedAdd(X1, Y1, Z1, x2, y2) {
    if (!Z1) return [x2, y2, 1n];
    const Z1Z1 = fmul(Z1, Z1);
    const U2 = fmul(x2, Z1Z1);
    const S2 = fmul(y2, fmul(Z1, Z1Z1));
    const H = fsub(U2, X1);
    if (!H) {
      if (!fsub(S2, Y1)) return jacobianDouble(X1, Y1, Z1);
      return [0n, 1n, 0n];
    }
    const HH = fmul(H, H);
    const I = fadd(fadd(HH, HH), fadd(HH, HH));
    const J = fmul(H, I);
    const r = fadd(fsub(S2, Y1), fsub(S2, Y1));
    const V = fmul(X1, I);
    const X3 = fsub(fsub(fmul(r, r), J), fadd(V, V));
    const Y3 = fsub(fmul(r, fsub(V, X3)), fadd(fmul(Y1, J), fmul(Y1, J)));
    const Z3 = fmul(fadd(Z1, Z1), H);
    return [X3, Y3, Z3];
  }

  // Convert Jacobian (X:Y:Z) back to affine {x, y}. One modInv.
  function jacobianToAffine(X, Y, Z) {
    if (!Z) return null;
    const Zinv = modInv(Z, P);
    const Zinv2 = fmul(Zinv, Zinv);
    const Zinv3 = fmul(Zinv, Zinv2);
    return { x: fmul(X, Zinv2), y: fmul(Y, Zinv3) };
  }

  // Jacobian scalar × arbitrary affine point (MSB-first double-and-add).
  // ~39× faster than affine double-and-add: 255 doubles + ~128 mixed adds + 1 modInv.
  function pointMultiply(scalar, point) {
    const n = mod(scalar, N);
    if (!n || !point) return null;
    let [RX, RY, RZ] = [0n, 1n, 0n];
    const bits = n.toString(2);
    for (let i = 0; i < bits.length; i++) {
      if (RZ) [RX, RY, RZ] = jacobianDouble(RX, RY, RZ);
      if (bits[i] === "1") [RX, RY, RZ] = jacobianMixedAdd(RX, RY, RZ, point.x, point.y);
    }
    return jacobianToAffine(RX, RY, RZ);
  }

  // Precomputed power-of-2 multiples of G: gPowers[i] = 2^i * G (affine).
  // Built once per curve instance; ~82ms first call, free afterwards.
  let gPowers = null;
  function ensureGPowers() {
    if (gPowers) return;
    gPowers = [];
    let pt = G;
    for (let i = 0; i < 256; i++) {
      gPowers.push(pt);
      pt = pointAdd(pt, pt);
    }
  }

  // Fixed-base scalar × G using precomputed gPowers + Jacobian accumulator.
  // ~28× faster than affine: ~128 mixed adds + 1 modInv (no doublings needed).
  function pointMultiplyG(scalar) {
    ensureGPowers();
    let n = mod(scalar, N);
    if (!n) return null;
    let [RX, RY, RZ] = [0n, 1n, 0n];
    for (let i = 0; i < 256; i++) {
      if (n & 1n) [RX, RY, RZ] = jacobianMixedAdd(RX, RY, RZ, gPowers[i].x, gPowers[i].y);
      n >>= 1n;
      if (!n) break;
    }
    return jacobianToAffine(RX, RY, RZ);
  }

  function bytesToBigInt(bytes) {
    return BigInt(
      "0x" +
        (Array.from(bytes)
          .map(function (byte) {
            return byte.toString(16).padStart(2, "0");
          })
          .join("") || "0"),
    );
  }

  function bigIntToBytes(num, length) {
    let hex = num.toString(16);
    if (hex.length % 2) {
      hex = "0" + hex;
    }
    const raw = shim.Buffer.from(hex, "hex");
    if (!length) {
      return new Uint8Array(raw);
    }
    if (raw.length > length) {
      return new Uint8Array(raw.slice(raw.length - length));
    }
    const out = new Uint8Array(length);
    out.set(raw, length - raw.length);
    return out;
  }

  function concatBytes() {
    const chunks = Array.prototype.slice.call(arguments).map(function (chunk) {
      if (chunk instanceof Uint8Array) {
        return chunk;
      }
      if (Array.isArray(chunk)) {
        return Uint8Array.from(chunk);
      }
      if (typeof chunk === "number") {
        return Uint8Array.from([chunk]);
      }
      if (chunk && chunk.buffer instanceof ArrayBuffer) {
        return new Uint8Array(
          chunk.buffer,
          chunk.byteOffset || 0,
          chunk.byteLength,
        );
      }
      return new Uint8Array(0);
    });
    const length = chunks.reduce(function (total, chunk) {
      return total + chunk.length;
    }, 0);
    const out = new Uint8Array(length);
    let offset = 0;
    chunks.forEach(function (chunk) {
      out.set(chunk, offset);
      offset += chunk.length;
    });
    return out;
  }

  function assertScalar(value, name) {
    if (value <= 0n || value >= N) {
      throw new Error((name || "Scalar") + " out of range");
    }
    return value;
  }

  function parseScalar(value, name) {
    if (typeof value === "bigint") {
      return assertScalar(value, name);
    }
    if (typeof value !== "string" || !value) {
      throw new Error((name || "Scalar") + " must be a string");
    }
    let scalar;
    if (/^[A-Za-z0-9]{44}$/.test(value)) {
      scalar = base62.b62ToBI(value);                   // zen base62 (44 chars)
    } else if (/^0x[0-9a-fA-F]{64}$/.test(value)) {
      scalar = BigInt(value);                            // EVM/BTC 0x-prefixed hex (32 bytes)
    } else {
      throw new Error((name || "Scalar") + " format not recognised");
    }
    return assertScalar(scalar, name);
  }

  function scalarToString(value) {
    return base62.biToB62(assertScalar(value));
  }

  // Recover y from x and parity bit (works for any curve where p ≡ 3 mod 4)
  function liftX(x, parity) {
    const rhs = mod(x * x * x + A * x + B, P);
    let y = modPow(rhs, (P + 1n) >> 2n, P);
    if ((y & 1n) !== parity) {
      y = P - y;
    }
    return y;
  }

  // Recover public key point from ECDSA (v, r, s) + hash bytes
  // pub = u1*G + u2*R  where u1 = -z*r^-1, u2 = s*r^-1 mod N
  function recoverPub(v, r, s, hashBytes) {
    const z = mod(bytesToBigInt(hashBytes), N);
    const ry = liftX(r, BigInt(v) & 1n);
    const R = { x: r, y: ry };
    if (!isOnCurve(R)) throw new Error("Recovery: R not on curve");
    const rinv = modInv(r, N);
    const u1 = mod((N - mod(z, N)) * rinv, N);
    const u2 = mod(s * rinv, N);
    const point = pointAdd(pointMultiplyG(u1), pointMultiply(u2, R));
    if (!point || !isOnCurve(point)) throw new Error("Recovery failed");
    return point;
  }

  function pointToPub(point) {
    if (!isOnCurve(point)) {
      throw new Error("Invalid public point");
    }
    // Compressed: 44-char base62 x + "0"/"1" parity of y  →  45 chars total
    return base62.biToB62(point.x) + (point.y & 1n ? "1" : "0");
  }

  function parsePub(pub) {
    if (typeof pub !== "string") {
      throw new Error("Public key must be a string");
    }
    let point;
    if (pub.length === 45 && /^[A-Za-z0-9]{44}[01]$/.test(pub)) {
      // Current compressed format: 44-char base62 x + "0"/"1" parity
      const x = base62.b62ToBI(pub.slice(0, 44));
      const parity = BigInt(pub[44]);
      const y = liftX(x, parity);
      point = { x, y };
    } else if (pub.length === 132 && /^0x04[0-9a-fA-F]{128}$/.test(pub)) {
      // EVM uncompressed pubkey: 0x04 + 32-byte x + 32-byte y
      point = {
        x: BigInt("0x" + pub.slice(4, 68)),
        y: BigInt("0x" + pub.slice(68)),
      };
    } else if (pub.length === 68 && /^0x0[23][0-9a-fA-F]{64}$/.test(pub)) {
      // BTC/hex compressed pubkey: 0x02/03 + 32-byte x
      const x = BigInt("0x" + pub.slice(4));
      const parity = BigInt(parseInt(pub[3]) & 1);
      point = { x, y: liftX(x, parity) };
    } else {
      throw new Error("Unrecognized public key format");
    }
    if (!isOnCurve(point)) {
      throw new Error("Public key is not on " + curve);
    }
    return point;
  }

  function publicFromPrivate(priv) {
    const point = pointMultiplyG(assertScalar(priv, "Private key"));
    if (!point || !isOnCurve(point)) {
      throw new Error("Could not derive public key");
    }
    return point;
  }

  function compactPoint(point) {
    return concatBytes(
      [point.y & 1n ? 0x03 : 0x02],
      bigIntToBytes(point.x, 32),
    );
  }

  async function shaBytes(data) {
    return new Uint8Array(await sha256(data));
  }

  async function hmacSha256(keyBytes, dataBytes) {
    const key = await shim.subtle.importKey(
      "raw",
      keyBytes,
      { name: "HMAC", hash: { name: "SHA-256" } },
      false,
      ["sign"],
    );
    return new Uint8Array(await shim.subtle.sign("HMAC", key, dataBytes));
  }

  async function deterministicK(priv, hashBytes, attempt) {
    const x = bigIntToBytes(priv, 32);
    const h1 = bigIntToBytes(bytesToBigInt(hashBytes) % N, 32);
    const extra = attempt
      ? Uint8Array.from([attempt & 255])
      : new Uint8Array(0);
    let K = new Uint8Array(32);
    let V = new Uint8Array(32).fill(1);
    K = await hmacSha256(K, concatBytes(V, [0], x, h1, extra));
    V = await hmacSha256(K, V);
    K = await hmacSha256(K, concatBytes(V, [1], x, h1, extra));
    V = await hmacSha256(K, V);
    while (true) {
      V = await hmacSha256(K, V);
      const candidate = bytesToBigInt(V);
      if (candidate > 0n && candidate < N) {
        return candidate;
      }
      K = await hmacSha256(K, concatBytes(V, [0]));
      V = await hmacSha256(K, V);
    }
  }

  async function hashToScalar(seed, label) {
    const digest = await shaBytes(
      concatBytes(shim.toBytes(label), shim.toBytes(seed)),
    );
    return (bytesToBigInt(digest) % (N - 1n)) + 1n;
  }

  async function randomScalar() {
    return (bytesToBigInt(shim.random(32)) % (N - 1n)) + 1n;
  }

  async function normalizeMessage(data) {
    if (typeof data === "string") {
      return settings.check(data) ? data : await settings.parse(data);
    }
    return data;
  }

  function parseSignature(sigBytes) {
    if (sigBytes.length !== 64) {
      throw new Error("Invalid signature length");
    }
    const r = bytesToBigInt(sigBytes.slice(0, 32));
    const s = bytesToBigInt(sigBytes.slice(32));
    if (r <= 0n || r >= N || s <= 0n || s >= N) {
      throw new Error("Signature out of range");
    }
    return { r, s };
  }

  async function finalize(result, opt, cb) {
    const out = opt && opt.raw ? result : await shim.stringify(result);
    if (cb) {
      try {
        cb(out);
      } catch (e) {
        console.log(e);
      }
    }
    return out;
  }

  return Object.assign(
    {
      curve,
      P,
      N,
      A,
      B,
      G,
      HALF_N,
      shim,
      base62,
      settings,
      mod,
      modPow,
      modInv,
      isPoint,
      isOnCurve,
      pointAdd,
      pointMultiply,
      pointMultiplyG,
      bytesToBigInt,
      bigIntToBytes,
      concatBytes,
      parseScalar,
      assertScalar,
      scalarToString,
      liftX,
      recoverPub,
      pointToPub,
      parsePub,
      publicFromPrivate,
      compactPoint,
      shaBytes,
      hmacSha256,
      deterministicK,
      hashToScalar,
      randomScalar,
      normalizeMessage,
      parseSignature,
      finalize,
    },
    extras,
  );
}

export default createCurveCore;
