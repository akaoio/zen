import assert from "assert";
import http from "node:http";
import net from "node:net";
import { EventEmitter } from "node:events";

import WebSocket from "../../lib/websocket.js";

// A relay's whole job is to outlive its clients. ServerWire re-emitted its
// socket's 'error' onto itself, and an EventEmitter with no 'error' listener
// throws -- so anyone who could open a socket and reset it could stop the
// process (#98). Closing a browser tab was enough.
//
// zen attaches an empty error listener to every server wire (lib/wire.js:98),
// which is why a reset arriving *before* close was survivable. But onClose
// removes that listener again (lib/wire.js:92), so a write error completing
// after the socket closed had nothing listening -- which is the stack the issue
// was reported from.

function fakeSocket() {
  const socket = new EventEmitter();
  socket.write = () => true;
  socket.end = () => {};
  socket.destroy = () => {};
  return socket;
}

function serverWire(socket) {
  const wss = new WebSocket.Server({});
  let wire;
  wss.handleUpgrade(
    { headers: { "sec-websocket-key": "dGhlIHNhbXBsZSBub25jZQ==", upgrade: "websocket" } },
    socket,
    null,
    (w) => {
      wire = w;
    },
  );
  return wire;
}

describe("a server wire whose socket fails", function () {
  this.timeout(10 * 1000);

  it("does not throw when nothing is listening for errors", function () {
    const socket = fakeSocket();
    const wire = serverWire(socket);
    assert.doesNotThrow(() => socket.emit("error", new Error("read ECONNRESET")));
    void wire;
  });

  it("does not throw when the error arrives after the wire closed", function () {
    // The production path: zen drops its own error listener on close, so a
    // write completing with ECONNRESET afterwards had nobody to tell.
    const socket = fakeSocket();
    const wire = serverWire(socket);
    const onError = () => {};
    wire.on("error", onError);
    wire.on("close", () => wire.removeListener("error", onError));

    socket.emit("close");
    assert.doesNotThrow(() => socket.emit("error", new Error("write ECONNRESET")));
  });

  it("still delivers the error to a listener that is there", function () {
    const socket = fakeSocket();
    const wire = serverWire(socket);
    let seen = null;
    wire.on("error", (e) => (seen = e));
    const boom = new Error("read ECONNRESET");
    socket.emit("error", boom);
    assert.strictEqual(seen, boom, "an error with a listener was swallowed");
  });

  it("retires the wire, the way close and end do", function () {
    // Without this the wire stays OPEN over a dead socket, and send() keeps
    // writing into it until some later 'close' arrives.
    const socket = fakeSocket();
    const wire = serverWire(socket);
    wire.on("error", () => {});
    socket.emit("error", new Error("read ECONNRESET"));
    assert.strictEqual(wire.readyState, 3, "wire left open over a dead socket");
  });

  it("tells listeners it closed", function () {
    const socket = fakeSocket();
    const wire = serverWire(socket);
    let closed = false;
    wire.on("error", () => {});
    wire.on("close", () => (closed = true));
    socket.emit("error", new Error("read ECONNRESET"));
    assert.ok(closed, "a wire died without saying so");
  });
});

describe("a relay whose client resets the connection", function () {
  this.timeout(15 * 1000);

  it("keeps serving everyone else", function (done) {
    // The reproduction from #98, whole: a real socket, a real RST, no error
    // listener anywhere -- which is what a consumer of WebSocket.Server has.
    const server = http.createServer();
    const wss = new WebSocket.Server({ server });
    let connected = false;
    wss.on("connection", () => (connected = true));

    server.listen(0, () => {
      const { port } = server.address();
      const client = net.connect(port, "127.0.0.1", () => {
        client.write(
          "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\n" +
            "Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" +
            "Sec-WebSocket-Version: 13\r\n\r\n",
        );
      });
      client.on("error", () => {});
      client.once("data", () => client.resetAndDestroy());
    });

    setTimeout(() => {
      assert.ok(connected, "the client never got as far as connecting");
      assert.ok(server.listening, "the relay stopped serving");
      server.close();
      done();
    }, 1200);
  });
});

describe("a client wire whose socket fails", function () {
  this.timeout(15 * 1000);

  it("does not throw when nothing is listening for errors", function (done) {
    // Same shape on the other side of the wire: a client that never set
    // onerror should not lose its process because the relay went away. The
    // constructor builds its own socket, so this has to be a real one.
    const server = http.createServer();
    const wss = new WebSocket.Server({ server });
    wss.on("connection", (wire) => {
      wire.on("error", () => {});
      wire._socket.resetAndDestroy();
    });

    server.listen(0, () => {
      const { port } = server.address();
      const client = new WebSocket("ws://127.0.0.1:" + port);
      // deliberately no client.on("error", ...) -- that is the case under test
      setTimeout(() => {
        assert.ok(true, "the client survived its relay resetting the connection");
        try { client.close(); } catch {}
        server.close();
        done();
      }, 1200);
    });
  });
});
