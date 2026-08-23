import { createHash } from "node:crypto";
import { EventEmitter } from "node:events";

const WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

function nativeClient() {
  if (typeof globalThis.WebSocket === "function") {
    return globalThis.WebSocket;
  }
  throw new Error(
    "Native WebSocket client is unavailable in this environment.",
  );
}

function normalizeUrl(url) {
  return ("" + url).replace(/^http/i, "ws");
}

function toBuffer(data) {
  if (Buffer.isBuffer(data)) {
    return data;
  }
  if (data instanceof Uint8Array) {
    return Buffer.from(data);
  }
  return Buffer.from("" + data);
}

function acceptKey(key) {
  return createHash("sha1")
    .update(key + WS_GUID)
    .digest("base64");
}

class NativeWebSocket extends EventEmitter {
  constructor(url, protocols, options) {
    super();
    const WS = nativeClient();
    this._wire = new WS(normalizeUrl(url), protocols);
    this.OPEN = WS.OPEN;
    this.CONNECTING = WS.CONNECTING;
    this.CLOSING = WS.CLOSING;
    this.CLOSED = WS.CLOSED;
    this._options = options;
    this._wire.addEventListener("open", (event) => this.emit("open", event));
    this._wire.addEventListener("message", (event) =>
      this.emit("message", event),
    );
    this._wire.addEventListener("close", (event) => this.emit("close", event));
    this._wire.addEventListener("error", (event) => {
      const error = event && event.error ? event.error : event;
      // Same reasoning as ServerWire: a client that never set onerror should
      // not lose its process because the relay went away. Nothing to retire
      // here -- readyState is the underlying wire's to report.
      if (this.listenerCount("error")) {
        this.emit("error", error);
      }
    });
  }

  get readyState() {
    return this._wire.readyState;
  }

  set onopen(fn) {
    if (this._onopen) this.off("open", this._onopen);
    this._onopen = fn;
    if (fn) this.on("open", fn);
  }

  set onclose(fn) {
    if (this._onclose) this.off("close", this._onclose);
    this._onclose = fn;
    if (fn) this.on("close", fn);
  }

  set onerror(fn) {
    if (this._onerror) this.off("error", this._onerror);
    this._onerror = fn;
    if (fn) this.on("error", fn);
  }

  set onmessage(fn) {
    if (this._onmessage) this.off("message", this._onmessage);
    this._onmessage = fn;
    if (fn) this.on("message", fn);
  }

  send(data) {
    return this._wire.send(data);
  }

  close(code, reason) {
    return this._wire.close(code, reason);
  }
}

NativeWebSocket.CONNECTING = 0;
NativeWebSocket.OPEN = 1;
NativeWebSocket.CLOSING = 2;
NativeWebSocket.CLOSED = 3;

class ServerWire extends EventEmitter {
  constructor(socket, req, head) {
    super();
    this._socket = socket;
    this.headers = (req || "").headers || {};
    this.readyState = NativeWebSocket.OPEN;
    this.OPEN = NativeWebSocket.OPEN;
    this.CONNECTING = NativeWebSocket.CONNECTING;
    this.CLOSING = NativeWebSocket.CLOSING;
    this.CLOSED = NativeWebSocket.CLOSED;
    this._buffer = Buffer.alloc(0);
    this._fragments = null;
    socket.on("data", (chunk) => this._read(chunk));
    socket.on("close", () => this._close());
    socket.on("end", () => this._close());
    // Two things, and both matter. Re-emitting unconditionally makes any
    // client's reset fatal: EventEmitter throws on an 'error' nobody is
    // listening for, and zen removes its own listener when the wire closes
    // (lib/wire.js:92), so a write completing with ECONNRESET afterwards had
    // nothing left to tell. And a socket that has errored is finished, so the
    // wire is retired here exactly as 'close' and 'end' retire it -- otherwise
    // it stays OPEN over a dead socket and send() keeps writing into it.
    socket.on("error", (error) => {
      if (this.listenerCount("error")) {
        this.emit("error", error);
      }
      this._close();
    });
    if (head && head.length) {
      this._read(head);
    }
  }

  send(data) {
    if (this.readyState !== NativeWebSocket.OPEN) {
      return;
    }
    const payload = toBuffer(data);
    const header = [];
    header.push(0x80 | 0x1);
    if (payload.length < 126) {
      header.push(payload.length);
    } else if (payload.length < 65536) {
      header.push(126, (payload.length >> 8) & 0xff, payload.length & 0xff);
    } else {
      const len = BigInt(payload.length);
      header.push(127);
      for (let i = 7; i >= 0; i -= 1) {
        header.push(Number((len >> BigInt(i * 8)) & 0xffn));
      }
    }
    this._socket.write(Buffer.concat([Buffer.from(header), payload]));
  }

  close(code = 1000, reason = "") {
    if (this.readyState >= NativeWebSocket.CLOSING) {
      return;
    }
    this.readyState = NativeWebSocket.CLOSING;
    const payload = Buffer.alloc(2 + Buffer.byteLength(reason));
    payload.writeUInt16BE(code, 0);
    payload.write(reason, 2);
    this._sendFrame(0x8, payload);
    this._socket.end();
  }

  _sendFrame(opcode, payload) {
    if (this.readyState === NativeWebSocket.CLOSED) {
      return;
    }
    const frame = [0x80 | opcode];
    if (payload.length < 126) {
      frame.push(payload.length);
    } else if (payload.length < 65536) {
      frame.push(126, (payload.length >> 8) & 0xff, payload.length & 0xff);
    } else {
      const len = BigInt(payload.length);
      frame.push(127);
      for (let i = 7; i >= 0; i -= 1) {
        frame.push(Number((len >> BigInt(i * 8)) & 0xffn));
      }
    }
    this._socket.write(Buffer.concat([Buffer.from(frame), payload]));
  }

  _read(chunk) {
    this._buffer = Buffer.concat([this._buffer, chunk]);
    while (this._buffer.length >= 2) {
      const first = this._buffer[0];
      const second = this._buffer[1];
      const fin = !!(first & 0x80);
      const opcode = first & 0x0f;
      const masked = !!(second & 0x80);
      let length = second & 0x7f;
      let offset = 2;
      if (length === 126) {
        if (this._buffer.length < offset + 2) {
          return;
        }
        length = this._buffer.readUInt16BE(offset);
        offset += 2;
      } else if (length === 127) {
        if (this._buffer.length < offset + 8) {
          return;
        }
        length = Number(this._buffer.readBigUInt64BE(offset));
        offset += 8;
      }
      let mask;
      if (masked) {
        if (this._buffer.length < offset + 4) {
          return;
        }
        mask = this._buffer.subarray(offset, offset + 4);
        offset += 4;
      }
      if (this._buffer.length < offset + length) {
        return;
      }
      let payload = this._buffer.subarray(offset, offset + length);
      this._buffer = this._buffer.subarray(offset + length);
      if (masked) {
        const unmasked = Buffer.alloc(payload.length);
        for (let i = 0; i < payload.length; i += 1) {
          unmasked[i] = payload[i] ^ mask[i % 4];
        }
        payload = unmasked;
      }
      this._frame(opcode, fin, payload);
    }
  }

  _frame(opcode, fin, payload) {
    if (opcode === 0x8) {
      this.readyState = NativeWebSocket.CLOSING;
      this._sendFrame(0x8, payload);
      this._socket.end();
      return;
    }
    if (opcode === 0x9) {
      this._sendFrame(0xa, payload);
      return;
    }
    if (opcode === 0xa) {
      return;
    }
    if (opcode === 0x1 || opcode === 0x2) {
      if (fin) {
        this.emit(
          "message",
          opcode === 0x1 ? payload.toString("utf8") : payload,
        );
        return;
      }
      this._fragments = [payload];
      return;
    }
    if (opcode === 0x0 && this._fragments) {
      this._fragments.push(payload);
      if (fin) {
        const data = Buffer.concat(this._fragments);
        this._fragments = null;
        this.emit("message", data.toString("utf8"));
      }
    }
  }

  _close() {
    if (this.readyState === NativeWebSocket.CLOSED) {
      return;
    }
    this.readyState = NativeWebSocket.CLOSED;
    this.emit("close");
  }
}

class WebSocketServer extends EventEmitter {
  constructor(options = {}) {
    super();
    this.options = options;
    if (options.server && !options.noServer) {
      options.server.on("upgrade", (req, socket, head) => {
        if (options.path && req.url !== options.path) {
          return;
        }
        this.handleUpgrade(req, socket, head, (wire) =>
          this.emit("connection", wire, req),
        );
      });
    }
  }

  handleUpgrade(req, socket, head, done) {
    const headers = req.headers || {};
    const key = headers["sec-websocket-key"];
    const upgrade = (headers.upgrade || "").toLowerCase();
    if (!key || upgrade !== "websocket") {
      socket.destroy();
      return;
    }
    socket.write(
      [
        "HTTP/1.1 101 Switching Protocols",
        "Upgrade: websocket",
        "Connection: Upgrade",
        "Sec-WebSocket-Accept: " + acceptKey(key),
        "\r\n",
      ].join("\r\n"),
    );
    const wire = new ServerWire(socket, req, head);
    if (done) {
      done(wire, req);
      return;
    }
    this.emit("connection", wire, req);
  }
}

NativeWebSocket.Server = WebSocketServer;

export default NativeWebSocket;
