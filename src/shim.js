import BufferApi from "./buffer.js";
import jsonAsync from "./json.js";

const globalScope =
  typeof globalThis !== "undefined"
    ? globalThis
    : typeof global !== "undefined"
      ? global
      : typeof window !== "undefined"
        ? window
        : {};
const api = { Buffer: globalScope.Buffer || BufferApi };
const empty = {};

jsonAsync.ensureJsonAsync();

api.parse = function (text, reviver) {
  return new Promise(function (resolve, reject) {
    jsonAsync.parseAsync(
      text,
      function (error, raw) {
        error ? reject(error) : resolve(raw);
      },
      reviver,
    );
  });
};

api.stringify = function (value, replacer, space) {
  return new Promise(function (resolve, reject) {
    jsonAsync.stringifyAsync(
      value,
      function (error, raw) {
        error ? reject(error) : resolve(raw);
      },
      replacer,
      space,
    );
  });
};

if (!api.TextEncoder) {
  api.TextEncoder = globalScope.TextEncoder;
}
if (!api.TextDecoder) {
  api.TextDecoder = globalScope.TextDecoder;
}

api.crypto = globalScope.crypto;
api.subtle =
  (globalScope.crypto || empty).subtle ||
  (globalScope.crypto || empty).webkitSubtle;
api.random = function (len) {
  var c = api.crypto || globalScope.crypto;
  return api.Buffer.from(
    c.getRandomValues(new Uint8Array(api.Buffer.alloc(len))),
  );
};

// JS utility shims (ported from zen/shim.js)
String.random =
  String.random ||
  function (l, c) {
    var s = "";
    l = l || 24;
    c = c || "0123456789ABCDEFGHIJKLMNOPQRSTUVWXZabcdefghijklmnopqrstuvwxyz";
    while (l-- > 0) {
      s += c.charAt(Math.floor(Math.random() * c.length));
    }
    return s;
  };
String.match =
  String.match ||
  function (t, o) {
    var tmp, u;
    if ("string" !== typeof t) {
      return false;
    }
    if ("string" == typeof o) {
      o = { "=": o };
    }
    o = o || {};
    tmp = o["="] || o["*"] || o[">"] || o["<"];
    if (t === tmp) {
      return true;
    }
    if (u !== o["="]) {
      return false;
    }
    tmp = o["*"] || o[">"];
    if (t.slice(0, (tmp || "").length) === tmp) {
      return true;
    }
    if (u !== o["*"]) {
      return false;
    }
    if (u !== o[">"] && u !== o["<"]) {
      return t >= o[">"] && t <= o["<"] ? true : false;
    }
    if (u !== o[">"] && t >= o[">"]) {
      return true;
    }
    if (u !== o["<"] && t <= o["<"]) {
      return true;
    }
    return false;
  };
String.hash =
  String.hash ||
  function (s, c) {
    if (typeof s !== "string") {
      return;
    }
    c = c || 0;
    if (!s.length) {
      return c;
    }
    for (var i = 0, l = s.length, n; i < l; ++i) {
      n = s.charCodeAt(i);
      c = (c << 5) - c + n;
      c |= 0;
    }
    return c;
  };
var has = Object.prototype.hasOwnProperty;
Object.plain =
  Object.plain ||
  function (o) {
    return o
      ? (o instanceof Object && o.constructor === Object) ||
          Object.prototype.toString.call(o).match(/^\[object (\w+)\]$/)[1] ===
            "Object"
      : false;
  };
Object.empty =
  Object.empty ||
  function (o, n) {
    for (var k in o) {
      if (has.call(o, k) && (!n || -1 == n.indexOf(k))) {
        return false;
      }
    }
    return true;
  };

function createImmediateFallback(sT, undefinedValue) {
  if (typeof MessageChannel == "" + undefinedValue) {
    return sT;
  }
  var channel = new MessageChannel();
  var fn;
  channel.port1.onmessage = function (e) {
    "" == e.data && fn();
  };
  return function (q) {
    fn = q;
    channel.port2.postMessage("");
  };
}

let undefinedValue;
const timeoutApi = setTimeout;
let pollLast = 0;
let pollSpin = 0;
let pollActive = 0;
const immediateApi =
  (typeof setImmediate !== "" + undefinedValue && setImmediate) ||
  createImmediateFallback(timeoutApi, undefinedValue);
const clockApi = (timeoutApi.check = timeoutApi.check ||
  (typeof performance !== "" + undefinedValue && performance) || {
    now: function () {
      return +new Date();
    },
  });
timeoutApi.hold = timeoutApi.hold || 9;
timeoutApi.poll =
  timeoutApi.poll ||
  function (task) {
    if (pollActive) {
      immediateApi(
        function () {
          pollLast = clockApi.now();
          pollActive = 1;
          try {
            task();
          } finally {
            pollActive = 0;
          }
        },
        (pollSpin = 0),
      );
      return;
    }
    if (timeoutApi.hold >= clockApi.now() - pollLast && pollSpin++ < 3333) {
      pollActive = 1;
      try {
        task();
      } finally {
        pollActive = 0;
      }
      return;
    }
    immediateApi(
      function () {
        pollLast = clockApi.now();
        pollActive = 1;
        try {
          task();
        } finally {
          pollActive = 0;
        }
      },
      (pollSpin = 0),
    );
  };

const pollApi = timeoutApi.poll;
let turnIndex = 0;
let turnTask;
let turnQueue = [];
const drainTurnQueue = function () {
  if ((turnTask = turnQueue[turnIndex++])) {
    turnTask();
  }
  if (turnIndex == turnQueue.length || 99 == turnIndex) {
    turnQueue = turn.s = turnQueue.slice(turnIndex);
    turnIndex = 0;
  }
  if (turnQueue.length) {
    // Bypass pollActive check for continuation: use direct call with spin-based yielding
    if (timeoutApi.hold >= clockApi.now() - pollLast && pollSpin++ < 3333) {
      drainTurnQueue();
    } else {
      immediateApi(function () {
        pollLast = clockApi.now();
        pollSpin = 0;
        drainTurnQueue();
      }, 0);
    }
  }
};
const turn = (timeoutApi.turn =
  timeoutApi.turn ||
  function (task) {
    1 == turnQueue.push(task) && pollApi(drainTurnQueue);
  });
turn.s = turnQueue;

const turnApi = timeoutApi.turn;
timeoutApi.each =
  timeoutApi.each ||
  function (list, fn, done, size) {
    size = size || 9;
    function next(batch, batchLength, result) {
      if ((batchLength = (batch = (list || []).splice(0, size)).length)) {
        for (var i = 0; i < batchLength; i++) {
          if (undefinedValue !== (result = fn(batch[i]))) {
            break;
          }
        }
        if (undefinedValue === result) {
          turnApi(next);
          return;
        }
      }
      done && done(result);
    }
    next();
  };

api.toBytes = function toBytes(data) {
  if (data instanceof Uint8Array) {
    return data;
  }
  if (data instanceof ArrayBuffer) {
    return new Uint8Array(data);
  }
  if (data && data.buffer instanceof ArrayBuffer) {
    return new Uint8Array(data.buffer, data.byteOffset || 0, data.byteLength);
  }
  return new api.TextEncoder().encode(typeof data === "string" ? data : String(data));
};

export default api;
