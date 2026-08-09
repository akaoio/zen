import zenbase from "../../index.js"; // full Node ZEN — includes lib/wire.js
import assert from "assert";
import http from "http";

// opt(url) is the documented way to add a peer at runtime — akao's zen hub
// boots with no peers and dials the site's relay once the site config has
// loaded. Until 1.0.46 that call only REGISTERED the peer: the websocket layer
// wired its dialer on the first opt() and returned early on every later one,
// so nothing ever opened the socket. In akao the symptom was silent and total
// — user state and alert documents never left the browser, so a user's alerts
// could never reach the engine and never fired.

function sleep(ms) {
  return new Promise(function (r) {
    setTimeout(r, ms);
  });
}

function graphOf(zen) {
  return zen._graph || zen;
}

function meshOf(zen) {
  var at = graphOf(zen)._;
  return at && at.opt && at.opt.mesh;
}

async function connected(zen, timeout) {
  var start = Date.now();
  while (Date.now() - start < timeout) {
    var mesh = meshOf(zen);
    if (mesh && mesh.near > 0) return true;
    await sleep(100);
  }
  return false;
}

// A live wire reconnects forever, so it would hold the relay open past the
// hook timeout. Hang up before closing the server.
function hangUp(zen) {
  var at = graphOf(zen)._;
  var peers = (at && at.opt && at.opt.peers) || {};
  Object.keys(peers).forEach(function (url) {
    var peer = peers[url];
    if (!peer) return;
    peer._noReconnect = true;
    peer.retry = 0;
    try {
      peer.wire && peer.wire.close && peer.wire.close();
    } catch (e) {}
  });
}

describe("peers added after construction", function () {
  this.timeout(60000);
  var server, url, client;

  before(async function () {
    server = http.createServer();
    new zenbase({ web: server, localStorage: false, axe: false, multicast: false });
    await new Promise(function (r) {
      server.listen(0, r);
    });
    url = "http://127.0.0.1:" + server.address().port + "/zen";
  });

  after(async function () {
    if (client) hangUp(client);
    await sleep(100);
    server.closeAllConnections && server.closeAllConnections();
    await Promise.race([new Promise(function (r) { server.close(r); }), sleep(3000)]);
  });

  it("opt(url) on a running instance actually dials it", async function () {
    const pair = await zenbase.pair();
    client = new zenbase({ peers: [], pub: pair.pub, localStorage: false, super: false, axe: false, multicast: false, radisk: false });

    assert.strictEqual(await connected(client, 1500), false, "no peers means no connection — the premise of the test");

    graphOf(client).opt(url);

    assert.strictEqual(await connected(client, 20000), true, "a peer added with opt() must be dialled, not merely recorded");
  });
});
