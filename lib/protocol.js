// lib/protocol.js — ZACP business logic
// Soul computation and channel key management.
// Runs inside MCP process — owner_pair.priv accessed from pairStore, never exposed to agents.
import ZEN from "../zen.js";

// ─── Constants ────────────────────────────────────────────────────────────────

// Standard candle window: hourly segments, accept ±2 hours.
// sep MUST be ":" — ZEN.candle() default sep is "_".
export const INBOX_CANDLE = { seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 };

// ─── Soul helpers ─────────────────────────────────────────────────────────────

/**
 * inboxSoul(pub, opts?) → soul string
 *
 * PEN soul for a public-key inbox. Deterministic: same pub → same soul.
 * Write path: zen.get(soul + "/" + pub).get(key).put(enc, null, { authenticator: sender })
 * R[6] = pub (validated in bytecode)
 *
 * opts.pow  — add PoW policy (anti-spam level 2+). Default: none (sign only).
 * opts.candle — override candle params.
 */
export function inboxSoul(pub, opts = {}) {
  const candle = { ...INBOX_CANDLE, ...(opts.candle || {}) };
  const spec = { path: pub, key: ZEN.candle(candle), sign: true };
  if (opts.pow) spec.pow = opts.pow;
  return ZEN.pen(spec);
}

/**
 * chanSoul(proj_id, chan_id, owner_pub, opts?) → soul string
 *
 * PEN soul for a project channel. Cert-gated: only members with a cert from owner_pub can write.
 * Path in bytecode: "proj/<proj_id>/chan/<chan_id>" — unique per channel.
 * Write path: zen.get(soul + "/proj/" + proj_id + "/chan/" + chan_id).get(key).put(...)
 * R[6] = "proj/<proj_id>/chan/<chan_id>" (validated in bytecode)
 *
 * opts.pow — add PoW policy (e.g. for public channels).
 */
export function chanSoul(proj_id, chan_id, owner_pub, opts = {}) {
  const spec = {
    path: "proj/" + proj_id + "/chan/" + chan_id,
    key:  ZEN.candle({ seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 }),
    sign: true,
    cert: owner_pub,
  };
  if (opts.pow) spec.pow = opts.pow;
  return ZEN.pen(spec);
}

/**
 * dmSoul(recipient_pub, opts?) → soul string
 *
 * PEN soul for 1:1 DM. No cert → PoW required to prevent flood.
 * Write path: zen.get(soul + "/dm/" + recipient_pub).get(key).put(...)
 * R[6] = "dm/<recipient_pub>" (validated in bytecode)
 *
 * opts.pow  — override PoW policy. Pass false to disable (not recommended).
 */
export function dmSoul(recipient_pub, opts = {}) {
  const spec = {
    path: "dm/" + recipient_pub,
    key:  ZEN.candle({ seg: 0, sep: ":", size: 3600000, back: 2, fwd: 0 }),
    sign: true,
  };
  if (opts.pow !== false) spec.pow = opts.pow || { unit: "0", difficulty: 1 };
  return ZEN.pen(spec);
}

// ─── Channel key management ───────────────────────────────────────────────────

/**
 * chanSeed(owner_priv, proj_id, chan_id, version) → Promise<base62 string>
 *
 * Derive a channel seed from the owner's private key.
 * SECURITY: includes owner_priv so chan_pair.priv cannot be computed from public data alone.
 * Must be called inside MCP process where owner_pair.priv is available from pairStore.
 */
export async function chanSeed(owner_priv, proj_id, chan_id, version) {
  return ZEN.hash(
    owner_priv + proj_id + chan_id + "v" + version, null, null,
    { name: "SHA-256", encode: "base62" }
  );
}

/**
 * wrapChanKey(chan_pair, members) → Promise<{ [member_pub]: encrypted_priv }>
 *
 * Encrypt chan_pair.priv for each member using ECDH shared secret.
 * members: array of public key strings.
 * Result: store each entry at ~<owner_pub>/proj/<pid>/chan/<cid>/keys/<member_pub>.
 */
export async function wrapChanKey(chan_pair, members) {
  const result = {};
  for (const pub of members) {
    const shared = await ZEN.secret(pub, chan_pair);
    result[pub] = await ZEN.encrypt(chan_pair.priv, { priv: shared });
  }
  return result;
}

/**
 * unwrapChanKey(chan_pair_pub, owner_pub, proj_id, chan_id, member_pair, zen)
 *   → Promise<chan_priv string | null>
 *
 * Read the wrapped key from graph and decrypt it using the member's keypair.
 * Reads from: ~<owner_pub>/proj/<proj_id>/chan/<chan_id>/keys/<member_pair.pub>
 */
export async function unwrapChanKey(chan_pair_pub, owner_pub, proj_id, chan_id, member_pair, zen) {
  const soul    = "zacp/" + owner_pub + "/" + proj_id + "/" + chan_id + "/keys";
  const wrapped = await new Promise(r =>
    zen.get(soul).get(member_pair.pub).once(v => r(v ?? null))
  );
  if (!wrapped) return null;
  const shared = await ZEN.secret(chan_pair_pub, member_pair);
  return ZEN.decrypt(wrapped, { priv: shared });
}

function projectBase(owner_pub, proj_id) {
  return "zacp/" + owner_pub + "/" + proj_id;
}

export async function setProjectMeta(proj_id, owner_pair, patch, zen) {
  const metaSoul = projectBase(owner_pair.pub, proj_id) + "/meta";
  const authOpt = { authenticator: owner_pair };
  for (const [key, value] of Object.entries(patch || {})) {
    await zen.get(metaSoul).get(key).put(value, null, authOpt);
  }
  return { ok: true };
}

export async function getProjectMeta(proj_id, owner_pub, zen) {
  return new Promise((resolve) => {
    zen.get(projectBase(owner_pub, proj_id) + "/meta").once((value) => resolve(value ?? null));
  });
}

export async function setProjectRole(proj_id, owner_pair, member_pub, role, zen) {
  const rolesSoul = projectBase(owner_pair.pub, proj_id) + "/roles";
  await zen.get(rolesSoul).get(member_pub).put(role, null, { authenticator: owner_pair });
  return { ok: true };
}

export async function getProjectRoles(proj_id, owner_pub, zen) {
  return new Promise((resolve) => {
    zen.get(projectBase(owner_pub, proj_id) + "/roles").once((value) => resolve(value ?? null));
  });
}

// ─── Internal helpers ─────────────────────────────────────────────────────────

async function decodeSignedMessage(val) {
  if (!val) return null;

  if (typeof val === "string") {
    try {
      const sender_pub = await ZEN.recover(val);
      const inner = await ZEN.verify(val, sender_pub);
      if (!inner) return null;
      const parsed = typeof inner === "string" ? JSON.parse(inner) : inner;
      return { sender_pub, parsed };
    } catch (_) {}

    try {
      val = JSON.parse(val);
    } catch (_) {
      return null;
    }
  }

  if (val && typeof val === "object" && val.a && val.m) {
    return { sender_pub: val.a, parsed: val };
  }

  if (!val || typeof val !== "object" || !val["~"] || val[":"] == null) {
    return null;
  }

  // rfs-persisted node: {":": inner, "~": graphSig}
  // graphSig was signed over the canonical {#, ., :, >} node — not usable for app-level verify.
  // Instead, try to decode inner as a compact app-signed string, then fall back to plain JSON.
  const inner = val[":"];

  if (typeof inner === "string") {
    // inner may be a compact signed string (app-level signing via ZEN.sign)
    try {
      const sender_pub = await ZEN.recover(inner);
      const decoded    = await ZEN.verify(inner, sender_pub);
      if (!decoded) return null;
      const parsed = typeof decoded === "string" ? JSON.parse(decoded) : decoded;
      return { sender_pub, parsed };
    } catch (_) {}
  }

  // Fallback: plain JSON inner (e.g. channel messages that must carry the "+" cert field).
  // Graph-level auth was already enforced by ZEN's check.$vfy on write; trust it here.
  try {
    const parsed     = typeof inner === "string" ? JSON.parse(inner) : inner;
    const sender_pub = parsed && parsed.a;
    if (!sender_pub) return null;
    return { sender_pub, parsed };
  } catch (_) {
    return null;
  }
}

// ─── Channel lifecycle ────────────────────────────────────────────────────────

/**
 * createChannel(proj_id, chan_id, version, owner_pair, members, zen)
 *   → Promise<{ chan_pub, version, soul }>
 *
 * Derives channel keypair from owner_priv, wraps chan_priv for all members
 * (owner always included), writes wrapped keys + meta to owner's graph namespace.
 * Must run inside MCP process — owner_pair.priv from pairStore, never exposed.
 */
export async function createChannel(proj_id, chan_id, version = 1, owner_pair, members = [], zen) {
  const seed      = await chanSeed(owner_pair.priv, proj_id, chan_id, version);
  const chan_pair = await ZEN.pair(null, { seed });
  const all       = [owner_pair.pub, ...members];
  const wrapped   = await wrapChanKey(chan_pair, all);
  const authOpt   = { authenticator: owner_pair };
  // zacp/ prefix avoids ~<pub> user namespace: that namespace requires ZEN-signed values;
  // zacp/ uses CRDT-level auth (HAM) which accepts raw encrypted blobs directly.
  const base      = "zacp/" + owner_pair.pub + "/" + proj_id + "/" + chan_id;
  for (const [pub, enc] of Object.entries(wrapped)) {
    await zen.get(base + "/keys").get(pub).put(enc, null, authOpt);
  }
  // Store meta as individual keys (ZEN nodes cannot hold a primitive at root)
  const metaSoul = base + "/meta";
  await zen.get(metaSoul).get("pub").put(chan_pair.pub, null, authOpt);
  await zen.get(metaSoul).get("version").put(version, null, authOpt);
  await zen.get(metaSoul).get("since").put(Date.now(), null, authOpt);
  return { chan_pub: chan_pair.pub, version, soul: chanSoul(proj_id, chan_id, owner_pair.pub) };
}

/**
 * inviteMember(proj_id, chan_id, version, owner_pair, member_pub, zen)
 *   → Promise<{ cert, chan_pub }>
 *
 * Wraps current channel key for a new member and issues a 30-day cert
 * allowing member_pub to write to the channel soul.
 */
export async function inviteMember(proj_id, chan_id, version = 1, owner_pair, member_pub, zen) {
  const seed      = await chanSeed(owner_pair.priv, proj_id, chan_id, version);
  const chan_pair = await ZEN.pair(null, { seed });
  const wrapped   = await wrapChanKey(chan_pair, [member_pub]);
  const base      = "zacp/" + owner_pair.pub + "/" + proj_id + "/" + chan_id;
  await zen.get(base + "/keys").get(member_pub).put(
    wrapped[member_pub],
    null,
    { authenticator: owner_pair },
  );
  const policy = { write: { "#": "proj/" + proj_id + "/chan/" + chan_id } };
  const cert   = await ZEN.certify(member_pub, policy, owner_pair, null, {
    expiry: Date.now() + 30 * 24 * 3600000,
  });
  return { cert, chan_pub: chan_pair.pub };
}

/**
 * rotateChanKey(proj_id, chan_id, old_version, owner_pair, remaining_members, zen)
 *   → Promise<{ chan_pub, version }>
 *
 * Bumps version, derives new chan_pair, re-wraps for remaining members only.
 * Do NOT issue new cert for the kicked member — their old cert expires naturally.
 */
export async function rotateChanKey(proj_id, chan_id, old_version = 1, owner_pair, remaining_members = [], zen) {
  const new_version = old_version + 1;
  const seed        = await chanSeed(owner_pair.priv, proj_id, chan_id, new_version);
  const chan_pair   = await ZEN.pair(null, { seed });
  const all         = [owner_pair.pub, ...remaining_members];
  const wrapped     = await wrapChanKey(chan_pair, all);
  const authOpt     = { authenticator: owner_pair };
  const base        = "zacp/" + owner_pair.pub + "/" + proj_id + "/" + chan_id;
  for (const [pub, enc] of Object.entries(wrapped)) {
    await zen.get(base + "/keys").get(pub).put(enc, null, authOpt);
  }
  const metaSoul = base + "/meta";
  await zen.get(metaSoul).get("pub").put(chan_pair.pub, null, authOpt);
  await zen.get(metaSoul).get("version").put(new_version, null, authOpt);
  await zen.get(metaSoul).get("since").put(Date.now(), null, authOpt);
  return { chan_pub: chan_pair.pub, version: new_version };
}

// ─── Messaging ────────────────────────────────────────────────────────────────

/**
 * sendToChannel(proj_id, chan_id, owner_pub, chan_priv, message, sender_pair, cert, zen)
 *   → Promise<{ ok: true }>
 *
 * Encrypts message with chan_priv (AES-GCM) and writes to the channel PEN soul.
 * Requires cert from owner — the channel soul enforces CRT(owner_pub) policy.
 * val = JSON.stringify({ a: sender_pub, m: encrypted_blob })
 */
export async function sendToChannel(proj_id, chan_id, owner_pub, chan_priv, message, sender_pair, cert, zen) {
  const soul    = chanSoul(proj_id, chan_id, owner_pub);
  const pathstr = "proj/" + proj_id + "/chan/" + chan_id;
  const enc     = await ZEN.encrypt(message, { priv: chan_priv });
  const seg     = Math.floor(Date.now() / 3600000);
  const hash    = (await ZEN.hash(enc, null, null, { name: "SHA-256", encode: "base62" })).slice(0, 16);
  const key     = seg + ":" + hash;
  // SGN+CRT policy: value must be plain JSON with "+" field containing the cert.
  // The authenticator provides the ZEN-level write signature (satisfies SGN policy).
  // CRT's applypolicy reads ctx.val as JSON to find raw["+"] = cert.
  const val     = JSON.stringify({ a: sender_pair.pub, m: enc, "+": cert });
  await zen.get(soul + "/" + pathstr).get(key).put(
    val,
    null,
    { authenticator: sender_pair },
  );
  return { ok: true };
}

/**
 * sendInbox(recipient_pub, message, sender_pair, zen, opts?)
 *   → Promise<{ ok: true }>
 *
 * ECDH shared secret with recipient, AES-GCM encrypt, write to recipient's inbox soul.
 * Value is plain JSON so readers can recover sender_pub without relying on compact signed blobs.
 */
export async function sendInbox(recipient_pub, message, sender_pair, zen, opts = {}) {
  const soul = inboxSoul(recipient_pub, opts);
  const shared = await ZEN.secret(recipient_pub, sender_pair);
  const enc = await ZEN.encrypt(message, { priv: shared });
  const seg = Math.floor(Date.now() / 3600000);
  const hash = (await ZEN.hash(enc, null, null, { name: "SHA-256", encode: "base62" })).slice(0, 16);
  const key = seg + ":" + hash;
  const val = JSON.stringify({ a: sender_pair.pub, m: enc });
  const putOpt = { authenticator: sender_pair };
  if (opts.pow) {
    putOpt.pow = opts.pow;
  }
  await zen.get(soul + "/" + recipient_pub).get(key).put(val, null, putOpt);
  return { ok: true };
}

/**
 * sendDM(recipient_pub, message, sender_pair, zen)
 *   → Promise<{ ok: true }>
 *
 * ECDH shared secret with recipient, AES-GCM encrypt, write to recipient's DM soul.
 * PoW (difficulty:1) is required by the DM soul PEN policy — auto-mined on write.
 * val = JSON.stringify({ a: sender_pub, m: encrypted_blob })
 */
export async function sendDM(recipient_pub, message, sender_pair, zen) {
  const soul   = dmSoul(recipient_pub);
  const shared = await ZEN.secret(recipient_pub, sender_pair);
  const enc    = await ZEN.encrypt(message, { priv: shared });
  const seg    = Math.floor(Date.now() / 3600000);
  const hash   = (await ZEN.hash(enc, null, null, { name: "SHA-256", encode: "base62" })).slice(0, 16);
  const key    = seg + ":" + hash;
  // authenticator: sender_pair handles ZEN-signing (satisfies SGN policy)
  const val    = JSON.stringify({ a: sender_pair.pub, m: enc });
  await zen.get(soul + "/dm/" + recipient_pub).get(key).put(
    val,
    null,
    { authenticator: sender_pair, pow: { unit: "0", difficulty: 1 } },
  );
  return { ok: true };
}

/**
 * readInbox(owner_pair, zen, limit?) → Promise<[{ key, plaintext, sender_pub }]>
 *
 * Reads messages from the owner's inbox PEN soul (inboxSoul).
 * Decrypts each using ECDH shared secret with sender (val.a = sender_pub).
 * Resolves after 2s quiet period or when limit is reached.
 *
 * NOTE: This reads from inboxSoul, not dmSoul. To read DMs use readDMs().
 */
export async function readInbox(owner_pair, zen, limit = 50) {
  const soul = inboxSoul(owner_pair.pub);
  const msgs = [];
  return new Promise((resolve) => {
    let settled = false;
    const done = () => { if (!settled) { settled = true; resolve(msgs); } };
    zen.get(soul + "/" + owner_pair.pub).map().once(async (val, key) => {
      try {
        const decoded = await decodeSignedMessage(val);
        if (!decoded) return;
        const { sender_pub, parsed } = decoded;
        const shared     = await ZEN.secret(sender_pub, owner_pair);
        const plaintext  = await ZEN.decrypt(parsed.m, { priv: shared });
        msgs.push({ key, plaintext, sender_pub });
        if (msgs.length >= limit) done();
      } catch (e) { process.stderr && process.stderr.write && process.stderr.write("readInbox err: " + String(e) + "\n"); }
    });
    setTimeout(done, 2000);
  });
}

/**
 * readChannel(proj_id, chan_id, owner_pub, chan_priv, zen, limit?)
 *   → Promise<[{ key, plaintext, sender_pub }]>
 *
 * Reads channel messages from the channel PEN soul using the current channel private key.
 */
export async function readChannel(proj_id, chan_id, owner_pub, chan_priv, zen, limit = 50) {
  const soul = chanSoul(proj_id, chan_id, owner_pub);
  const pathstr = "proj/" + proj_id + "/chan/" + chan_id;
  const msgs = [];
  return new Promise((resolve) => {
    let settled = false;
    const done = () => { if (!settled) { settled = true; resolve(msgs); } };
    zen.get(soul + "/" + pathstr).map().once(async (val, key) => {
      try {
        const decoded = await decodeSignedMessage(val);
        if (!decoded) return;
        const { sender_pub, parsed } = decoded;
        const plaintext = await ZEN.decrypt(parsed.m, { priv: chan_priv });
        msgs.push({ key, plaintext, sender_pub });
        if (msgs.length >= limit) done();
      } catch (e) { process.stderr && process.stderr.write && process.stderr.write("readChannel err: " + String(e) + "\n"); }
    });
    setTimeout(done, 2000);
  });
}

/**
 * readDMs(owner_pair, zen, limit?) → Promise<[{ key, plaintext, sender_pub }]>
 *
 * Reads DMs from the owner's DM soul (dmSoul).
 * Decrypts each using ECDH shared secret with sender (val.a = sender_pub).
 */
export async function readDMs(owner_pair, zen, limit = 50) {
  const soul = dmSoul(owner_pair.pub);
  const msgs = [];
  return new Promise((resolve) => {
    let settled = false;
    const done = () => { if (!settled) { settled = true; resolve(msgs); } };
    zen.get(soul + "/dm/" + owner_pair.pub).map().once(async (val, key) => {
      try {
        const decoded = await decodeSignedMessage(val);
        if (!decoded) return;
        const { sender_pub, parsed } = decoded;
        const shared     = await ZEN.secret(sender_pub, owner_pair);
        const plaintext  = await ZEN.decrypt(parsed.m, { priv: shared });
        msgs.push({ key, plaintext, sender_pub });
        if (msgs.length >= limit) done();
      } catch (e) { process.stderr && process.stderr.write && process.stderr.write("readDMs err: " + String(e) + "\n"); }
    });
    setTimeout(done, 2000);
  });
}
