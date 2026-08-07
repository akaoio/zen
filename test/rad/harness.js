// A radisk you can single-step.
//
// Every flaky read-after-write in this repo has been chased with statistics:
// pin the suite to one core, run it twenty times, compare failure rates. That
// is how four separate diagnoses were nearly drawn from noise. The windows are
// tiny and they only open when the machine is busy, so the *scenario* is never
// the thing under control -- luck is.
//
// This puts the scenario under control. Radisk's asynchrony comes from three
// places, and it captures all three when the instance is built:
//
//   setTimeout / setImmediate   the flush timer and the read walk's yields
//   JSON.parseAsync / stringifyAsync   yson's chunked (de)serialisation
//   opt.store.get / put         the disk itself
//
// Replace all three before constructing, and nothing happens that a test did
// not ask for. A whole flush -- split, directory update, several file writes --
// becomes a list of steps you can stop between, which is what makes "read after
// exactly k steps of the flush" a thing you can assert on rather than hope for.

import __radisk from "../../lib/radisk.js";

// Timers the test drives. No wall clock: `at` is a virtual millisecond count,
// and nothing fires until asked.
export function makeClock() {
  var q = [],
    now = 0,
    seq = 0;
  function add(fn, ms) {
    var t = { id: ++seq, at: now + (ms || 0), fn: fn, seq: seq };
    q.push(t);
    return t.id;
  }
  function due() {
    // earliest first, insertion order breaking ties -- a fixed policy, so the
    // same scenario always replays the same way
    var best = null;
    for (var i = 0; i < q.length; i++) {
      if (
        !best ||
        q[i].at < best.at ||
        (q[i].at === best.at && q[i].seq < best.seq)
      ) {
        best = q[i];
      }
    }
    return best;
  }
  return {
    setTimeout: add,
    setImmediate: function (fn) {
      return add(fn, 0);
    },
    clearTimeout: function (id) {
      q = q.filter(function (t) {
        return t.id !== id;
      });
    },
    pending: function () {
      return q.length;
    },
    fire: function () {
      var t = due();
      if (!t) {
        return false;
      }
      q = q.filter(function (x) {
        return x !== t;
      });
      now = t.at > now ? t.at : now;
      t.fn();
      return true;
    },
  };
}

// A disk that only moves when told to.
export function makeStore() {
  var files = {},
    q = [];
  var s = {
    files: files,
    puts: 0,
    gets: 0,
    put: function (file, data, cb) {
      s.puts++;
      q.push({
        op: "put",
        file: file,
        run: function () {
          files[file] = data;
          cb(null, 1);
        },
      });
    },
    get: function (file, cb) {
      s.gets++;
      q.push({
        op: "get",
        file: file,
        run: function () {
          cb(null, files[file]);
        },
      });
    },
    // list is synchronous on purpose: radisk only uses it to enumerate, never
    // in the read or write path, so queueing it would add steps that mean
    // nothing.
    list: function (cb) {
      Object.keys(files).forEach(function (f) {
        cb(f);
      });
      cb();
    },
    pending: function () {
      return q.length;
    },
    fire: function () {
      var t = q.shift();
      if (!t) {
        return false;
      }
      t.run();
      return true;
    },
  };
  return s;
}

var ids = 0;

// Builds a radisk whose every asynchronous move is a step, hands it to `fn`,
// and puts the globals back afterwards.
//
// fn(r, ctx) where ctx is:
//   step()      one unit of pending work -- a queued disk op if there is one,
//               otherwise the earliest timer. Returns false when idle.
//   settle(n)   step until idle (or n steps), returns steps taken
//   pending()   units of work outstanding
//   store, clock
export function withRadisk(opts, fn) {
  var clock = makeClock(),
    store = makeStore();
  var real = {
    setTimeout: globalThis.setTimeout,
    clearTimeout: globalThis.clearTimeout,
    setImmediate: globalThis.setImmediate,
    parseAsync: JSON.parseAsync,
    stringifyAsync: JSON.stringifyAsync,
  };
  globalThis.setTimeout = clock.setTimeout;
  globalThis.clearTimeout = clock.clearTimeout;
  globalThis.setImmediate = clock.setImmediate;
  // Radisk falls back to plain JSON when these are absent, which is
  // synchronous -- one less scheduler in play.
  delete JSON.parseAsync;
  delete JSON.stringifyAsync;
  var ctx = {
    clock: clock,
    store: store,
    pending: function () {
      return store.pending() + clock.pending();
    },
    step: function () {
      // Disk first, then timers: a fixed order, so a scenario replays the same
      // way every run. This is the whole point of the harness.
      return store.fire() || clock.fire();
    },
    settle: function (max) {
      var n = 0;
      max = max || 100000;
      while (n < max && ctx.step()) {
        n++;
      }
      return n;
    },
  };
  try {
    var o = {
      store: store,
      file: "tmp/det-" + ++ids + "-" + (opts.file || ""),
    };
    for (var k in opts) {
      if ("file" !== k) {
        o[k] = opts[k];
      }
    }
    var r = __radisk(o);
    return fn(r, ctx);
  } finally {
    globalThis.setTimeout = real.setTimeout;
    globalThis.clearTimeout = real.clearTimeout;
    globalThis.setImmediate = real.setImmediate;
    if (real.parseAsync) {
      JSON.parseAsync = real.parseAsync;
    }
    if (real.stringifyAsync) {
      JSON.stringifyAsync = real.stringifyAsync;
    }
  }
}

// Reads one key and returns what came back, without leaving the step model:
// the read is issued, then the caller settles.
export function readNow(r, key, o) {
  var out = { called: 0, data: undefined, err: undefined, done: false };
  r(
    key,
    function (err, data) {
      out.called++;
      out.err = out.err || err;
      // radisk streams chunks; keep the first one that carried a value
      if (undefined === out.data) {
        out.data = data;
      }
      out.done = true;
    },
    o || {},
  );
  return out;
}
