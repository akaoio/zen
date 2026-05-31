/**
 * test/mcp/relay.js — ZEN-native relay RPC: ZenMcpClient + in-process relay server
 *
 * Uses two in-process ZEN instances wired together via mock peer pairs (same
 * technique as test/mesh/dam.js) so no real WebSocket / subprocess is needed.
 * A minimal relay responder mimics what lib/mcp.js does in relay mode.
 */
import assert from "assert";
import { ZEN } from "../../zen.js";
import { ZenMcpClient } from "../../lib/mcp/client.js";

// ── helpers ──────────────────────────────────────────────────────────────────

function makeZen(pub) {
  const z = ZEN.graph.create({ localStorage: false, peers: {}, WebSocket: false });
  const root = z._;
  const mesh = root.opt.mesh || ZEN.Mesh(root);
  root.opt.mesh = mesh;
  root.opt.pub = pub;
  return { zen: z, mesh, root };
}

/**
 * Wire two meshes together so messages relay in-process.
 * Returns peer handles so tests can inspect them.
 */
function wirePair(meshA, pubA, meshB, pubB) {
  const peerBatA = {
    id: "b-at-a", pub: pubB,
    wire: { send: (r) => meshB.hear(r, peerAatB) },
  };
  const peerAatB = {
    id: "a-at-b", pub: pubA,
    wire: { send: (r) => meshA.hear(r, peerBatA) },
  };
  meshA.hi(peerBatA);
  meshB.hi(peerAatB);
  return { peerBatA, peerAatB };
}

/**
 * Minimal in-process relay MCP server.
 * Accepts encrypted JSON-RPC via mesh.on(), dispatches, replies.
 */
async function makeRelayServer(pair, mesh) {
  const TOOLS = [
    { name: "echo", description: "Echo tool", inputSchema: { type: "object", properties: { msg: { type: "string" } } } },
  ];

  const off = mesh.on(async ({ from, data }) => {
    if (!from || !data) return;
    let req;
    try {
      const shared = await ZEN.secret(from, pair);
      req = await ZEN.decrypt(data, shared);
    } catch (_) { return; }
    if (!req || req.jsonrpc !== "2.0") return;

    let res;
    const { id, method, params } = req;

    if (method === "initialize") {
      res = { jsonrpc: "2.0", id, result: {
        protocolVersion: "2024-11-05",
        capabilities: { tools: {} },
        serverInfo: { name: "relay-test-server", version: "1.0.0" },
      }};
    } else if (method === "tools/list") {
      res = { jsonrpc: "2.0", id, result: { tools: TOOLS } };
    } else if (method === "tools/call") {
      const { name, arguments: args } = params || {};
      if (name === "echo") {
        res = { jsonrpc: "2.0", id, result: { content: [{ type: "text", text: JSON.stringify({ echo: args.msg }) }] } };
      } else {
        res = { jsonrpc: "2.0", id, error: { code: -32000, message: "Unknown tool: " + name } };
      }
    } else {
      res = { jsonrpc: "2.0", id, error: { code: -32601, message: "Method not found" } };
    }

    if (res == null) return;
    let enc;
    try {
      const shared = await ZEN.secret(from, pair);
      enc = await ZEN.encrypt(JSON.stringify(res), shared);
    } catch (_) { return; }
    mesh.relay(from, enc);
  });

  return { off };
}

// ── fixtures ─────────────────────────────────────────────────────────────────

let serverPair, clientPair;
let serverZen, serverMesh;
let clientZen, clientMesh;
let relayServer;
let client;

before(async function () {
  this.timeout(15000);
  [serverPair, clientPair] = await Promise.all([ZEN.pair(), ZEN.pair()]);

  ({ zen: serverZen, mesh: serverMesh } = makeZen(serverPair.pub));
  ({ zen: clientZen, mesh: clientMesh } = makeZen(clientPair.pub));

  wirePair(clientMesh, clientPair.pub, serverMesh, serverPair.pub);

  relayServer = await makeRelayServer(serverPair, serverMesh);

  client = new ZenMcpClient(serverPair.pub, clientZen, clientPair, { timeout: 8000 });
  await client.ready();
});

after(function () {
  client.close();
  if (relayServer) relayServer.off();
});

// ── ZenMcpClient — discovery ─────────────────────────────────────────────────

describe("ZenMcpClient.discover()", function () {
  this.timeout(10000);

  it("returns null when no info soul exists for an unknown pub", async function () {
    const unknown = (await ZEN.pair()).pub;
    const info = await ZenMcpClient.discover(unknown, clientZen);
    assert.strictEqual(info, null);
  });

  it("reads info published to graph soul ~<serverPub>/status", async function () {
    // Server publishes discovery info to graph
    const soul = "~" + serverPair.pub + "/status";
    await new Promise((resolve, reject) => {
      serverZen.get(soul).put(
        { name: "zen", pub: serverPair.pub, mcp: true },
        (ack) => { if (ack && ack.err) reject(new Error(ack.err)); else resolve(); },
        { authenticator: serverPair },
      );
    });
    const info = await ZenMcpClient.discover(serverPair.pub, serverZen);
    assert.ok(info, "should find discovery info");
    assert.strictEqual(info.pub, serverPair.pub);
    assert.strictEqual(info.mcp, true);
  });
});

// ── ZenMcpClient — request/response ──────────────────────────────────────────

describe("ZenMcpClient request / response", function () {
  this.timeout(10000);

  it("initialize() returns serverInfo from relay server", async function () {
    const result = await client.initialize();
    assert.strictEqual(result.serverInfo.name, "relay-test-server");
    assert.strictEqual(result.protocolVersion, "2024-11-05");
    assert.ok(result.capabilities.tools);
  });

  it("listTools() returns tools array", async function () {
    const tools = await client.listTools();
    assert.ok(Array.isArray(tools));
    assert.strictEqual(tools[0].name, "echo");
  });

  it("call('echo', { msg }) returns echoed value", async function () {
    const result = await client.call("echo", { msg: "hello relay" });
    assert.strictEqual(result.echo, "hello relay");
  });

  it("call() propagates server-side tool errors", async function () {
    await assert.rejects(
      () => client.call("nonexistent", {}),
      /Unknown tool/,
    );
  });

  it("request() propagates unknown method as -32601 error", async function () {
    const res = await client.request("no/such/method", {});
    assert.ok(res.error);
    assert.strictEqual(res.error.code, -32601);
  });

  it("multiple concurrent requests resolve independently", async function () {
    const [r1, r2, r3] = await Promise.all([
      client.call("echo", { msg: "one" }),
      client.call("echo", { msg: "two" }),
      client.call("echo", { msg: "three" }),
    ]);
    assert.strictEqual(r1.echo, "one");
    assert.strictEqual(r2.echo, "two");
    assert.strictEqual(r3.echo, "three");
  });

  it("request times out when server does not reply", async function () {
    // Use a fresh client pointing at a server pub with no responder
    const unknown = (await ZEN.pair()).pub;
    const c2 = new ZenMcpClient(unknown, clientZen, clientPair, { timeout: 400 });
    await c2.ready();
    await assert.rejects(
      () => c2.request("initialize", {}),
      /Timeout/,
    );
    c2.close();
  });
});

// ── ZenMcpClient — lifecycle ──────────────────────────────────────────────────

describe("ZenMcpClient lifecycle", function () {
  this.timeout(5000);

  it("close() rejects all pending requests", async function () {
    const unknown = (await ZEN.pair()).pub;
    const c = new ZenMcpClient(unknown, clientZen, clientPair, { timeout: 10000 });
    await c.ready();
    const pending = c.request("initialize", {}).catch(e => e.message);
    c.close();
    const msg = await pending;
    assert.ok(msg === "MCP client closed" || msg.includes("closed") || msg.includes("Timeout"),
      "expected closed error, got: " + msg);
  });

  it("ready() is idempotent — calling twice does not duplicate listeners", async function () {
    const c = new ZenMcpClient(serverPair.pub, clientZen, clientPair, { timeout: 5000 });
    await c.ready();
    await c.ready(); // second call should be a no-op
    const result = await c.initialize();
    assert.strictEqual(result.serverInfo.name, "relay-test-server");
    c.close();
  });
});

// ── Encryption / payload integrity ───────────────────────────────────────────

describe("relay RPC — payload security", function () {
  this.timeout(10000);

  it("relay messages are encrypted — plaintext not visible in transit", async function () {
    const captured = [];
    const origHi = clientMesh.hi.bind(clientMesh);
    // Intercept raw relay sends by wrapping mesh.relay
    const origRelay = clientMesh.relay.bind(clientMesh);
    clientMesh.relay = function (to, data, opt) {
      captured.push(data);
      return origRelay(to, data, opt);
    };
    await client.call("echo", { msg: "secret payload" });
    clientMesh.relay = origRelay;

    assert.ok(captured.length > 0, "should have captured at least one relay message");
    for (const data of captured) {
      assert.ok(typeof data === "string");
      // Encrypted format from ZEN.encrypt: three base62 parts separated by "."
      assert.match(data, /^[0-9A-Za-z]+\.[0-9A-Za-z]+\.[0-9A-Za-z]+$/,
        "relay data should be an encrypted blob, not plaintext");
      // Should not contain the plaintext
      assert.ok(!data.includes("secret payload"), "plaintext must not appear in relay data");
    }
  });

  it("relay messages from wrong sender pub are ignored", async function () {
    // Create a rogue client with a different pair
    const roguePair = await ZEN.pair();
    const rogueClient = new ZenMcpClient(serverPair.pub, clientZen, roguePair, { timeout: 400 });
    await rogueClient.ready();

    // Rogue's messages are encrypted with rogue key — server can't decrypt → no response
    await assert.rejects(
      () => rogueClient.initialize(),
      /Timeout/,
    );
    rogueClient.close();
  });
});
