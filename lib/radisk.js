import "./yson.js";
import __radmigtmp from "./radmigtmp.js";
import ZEN from "../zen.js";
import __radix from "./radix.js";
import tpath from "./tpath.js";

let __defaultExport;

function Radisk(opt) {
  opt = opt || {};
  opt.log = opt.log || console.log;
  opt.file = tpath(opt.file, "radata", ZEN && ZEN.TESTING);
  var has = (Radisk.has || (Radisk.has = {}))[opt.file];
  if (has) {
    return has;
  }

  opt.max = opt.max || (opt.memory ? opt.memory * 999 * 999 : 300000000) * 0.3;
  opt.until = opt.until || opt.wait || 250;
  opt.batch = opt.batch || 10 * 1000;
  opt.chunk = opt.chunk || 1024 * 1024 * 1; // 1MB
  opt.tombstoneTTL = opt.tombstoneTTL !== undefined ? opt.tombstoneTTL : (1000 * 60 * 60 * 24 * 7); // 7 days
  opt.code = opt.code || {};
  opt.code.from = opt.code.from || "!";
  opt.jsonify = true;

  function ename(t) {
    return encodeURIComponent(t).replace(/\*/g, "%2A");
  } // TODO: Hash this also, but allow migration!
  function atomic(v) {
    return u !== v && (!v || "object" != typeof v);
  }
  var timediate = "" + u === typeof setImmediate ? setTimeout : setImmediate;
  var puff = setTimeout.turn || timediate,
    u;
  var map = Radix.object;
  var ST = 0;

  if (!opt.store) {
    return opt.log(
      "ERROR: Radisk needs `opt.store` interface with `{get: fn, put: fn (, list: fn)}`!",
    );
  }
  if (!opt.store.put) {
    return opt.log(
      "ERROR: Radisk needs `store.put` interface with `(file, data, cb)`!",
    );
  }
  if (!opt.store.get) {
    return opt.log(
      "ERROR: Radisk needs `store.get` interface with `(file, cb)`!",
    );
  }
  if (!opt.store.list) {
    //opt.log("WARNING: `store.list` interface might be needed!");
  }

  var parse =
    JSON.parseAsync ||
    function (t, cb, r) {
      var u;
      try {
        cb(u, JSON.parse(t, r));
      } catch (e) {
        cb(e);
      }
    };
  var json =
    JSON.stringifyAsync ||
    function (v, cb, r, s) {
      var u;
      try {
        cb(u, JSON.stringify(v, r, s));
      } catch (e) {
        cb(e);
      }
    };
  /*
			Any and all storage adapters should...
			1. Because writing to disk takes time, we should batch data to disk. This improves performance, and reduces potential disk corruption.
			2. If a batch exceeds a certain number of writes, we should immediately write to disk when physically possible. This caps total performance, but reduces potential loss.
		*/
  var r = function (key, data, cb, tag, DBG) {
    if ("function" === typeof data) {
      var o = cb || {};
      cb = data;
      r.read(key, cb, o, DBG || tag);
      return;
    }
    // If the storage adapter is in degraded (disk-full) mode, fail fast.
    if (opt.store && opt.store.degraded) {
      if (cb) cb("ZEN storage is full — write rejected");
      return;
    }
    r.save(key, data, cb, tag, DBG);
  };
  r.save = function (key, data, cb, tag, DBG) {
    var s = { key: key },
      tags,
      f,
      d,
      q;
    s.find = function (file) {
      var tmp;
      s.file = file || (file = opt.code.from);
      DBG && (DBG = DBG[file] = DBG[file] || {});
      DBG && (DBG.sf = DBG.sf || +new Date());
      //console.only.i && console.log('found', file);
      if ((tmp = r.disk[file])) {
        s.mix(u, tmp);
        return;
      }
      if ((tmp = r.writing[file])) {
        // A parked write is invisible. Its key and value live only in this
        // closure until the flush lands and the write is replayed -- not in
        // r.disk, not in the snapshot being written, not on disk. For that
        // whole stretch a read for this key finds nothing anywhere and says
        // so, and nothing asks a second time.
        //
        // So park the value where a read can see it. This is the state the
        // Windows samples kept landing in: acknowledged, and simultaneously
        // nowhere.
        (tmp.pend || (tmp.pend = Radix()))(key, data);
        tmp.push(function () {
          // Ask the directory again rather than reusing `file`. The flush we
          // waited on may have split it, moving this key's range into another
          // file and saying so in the directory -- writing to the old name
          // would put the key somewhere no read will ever look.
          r.find(key, s.find);
        }); // flush in flight: parsing now would read the pre-flush file and
        return; // mix into it, dropping every key still being written.
      }
      r.parse(file, s.mix, u, DBG);
    };
    s.mix = function (err, disk) {
      DBG && (DBG.sml = +new Date());
      DBG && (DBG.sm = DBG.sm || +new Date());
      if ((s.err = err || s.err)) {
        cb(err);
        return;
      } // TODO: HANDLE BATCH EMIT
      var file = (s.file = (disk || "").file || s.file),
        tmp;
      if (!disk && file !== opt.code.from) {
        // corrupt file?
        r.find.bad(file); // remove from dir list
        r.save(key, data, cb, tag); // try again
        return;
      }
      if (!disk && !r.disk[file] && (tmp = r.writing[file])) {
        // The parse came back with nothing -- but a flush for this file began
        // while it was running, so "nothing" means "not on disk yet", not
        // "no such file". Building a fresh radix here would hand back a file
        // that has forgotten everything it holds, and the next split of it
        // would publish a boundary in the middle of keys it does not know
        // about, stranding them. Wait for the flush and start again.
        // Park the value where a read can see it while it waits, or it is
        // invisible for the length of the flush -- the very hole #40 closed
        // for the other parking path.
        (tmp.pend || (tmp.pend = Radix()))(key, data);
        (r.held || (r.held = {}))[key] = data;
        tmp.push(function () {
          r.find(key, s.find);
        });
        return;
      }
      (disk = r.disk[file] || (r.disk[file] = disk || Radix())).file ||
        (disk.file = file);
      if (opt.compare) {
        data = opt.compare(disk(key), data, key, file);
        if (u === data) {
          cb(err, -1);
          return;
        } // TODO: HANDLE BATCH EMIT
      }
      (s.disk = disk)(key, data);
      // This write is acknowledged from memory, long before it is on disk.
      // Between the two it must never read as missing -- that is the one
      // promise storage makes -- so remember which radix is holding it.
      (r.held || (r.held = {}))[key] = data;
      if (tag) {
        (tmp =
          (tmp = disk.tags || (disk.tags = {}))[tag] ||
          (tmp[tag] = r.tags[tag] || (r.tags[tag] = {})))[file] ||
          (tmp[file] = r.one[tag] || (r.one[tag] = cb));
        cb = null;
      }
      DBG && (DBG.st = DBG.st || +new Date());
      //console.only.i && console.log('mix', disk.Q);
      if (disk.Q) {
        cb && disk.Q.push(cb);
        return;
      }
      disk.Q = cb ? [cb] : [];
      disk.to = setTimeout(s.write, opt.until);
    };
    s.write = function () {
      DBG && (DBG.sto = DBG.sto || +new Date());
      var file = (f = s.file),
        disk = (d = s.disk);
      q = s.q = disk.Q;
      tags = s.tags = disk.tags;
      delete disk.Q;
      // Between here and s.ack the data is in neither place a lookup goes: the
      // in-memory radix is gone and the file on disk is not rewritten yet, so
      // both reads and writes would fall through to the pre-flush file. Keep
      // the snapshot reachable, and park writes on the queue until it lands.
      (r.writing[file] = r.writing[file] || []).rad = disk;
      delete r.disk[file];
      delete disk.tags;
      // This radix has outlived the directory it was routed under: keys landed
      // here while this file still covered them, splits since then published
      // files that took over the upper end of that range, and nothing came
      // back to move them. Written as-is they are stranded -- a read routes to
      // the greatest file name <= the key and never looks in an earlier file.
      // So a file writes only what it still owns; the rest goes back through
      // the directory once this write lands, and is routed afresh.
      s.drift = null;
      var edge;
      r.list &&
        Radix.map(
          r.list,
          function boundary(val, name) {
            if (!val || name === file) {
              return;
            }
            edge = name;
            return 1;
          },
          { start: file },
        );
      if (edge) {
        Radix.map(disk, function stray(val, key) {
          if (key >= edge) {
            (s.drift || (s.drift = [])).push([key, val]);
          }
        });
        if (s.drift) {
          var owned = Radix();
          Radix.map(disk, function keep(val, key) {
            if (key < edge) {
              owned(key, val);
            }
          });
          disk.$ = owned.$;
          disk.last = owned.last;
        }
      }
      //console.only.i && console.log('write', file, disk, 'was saving:', key, data);
      r.write(file, disk, s.ack, u, DBG);
    };
    s.ack = function (err, ok) {
      if (r.held) {
        delete r.held[key];
      }
      DBG && (DBG.sa = DBG.sa || +new Date());
      DBG && (DBG.sal = q.length);
      var ack, tmp;
      // TODO!!!! CHANGE THIS INTO PUFF!!!!!!!!!!!!!!!!
      for (var id in r.tags) {
        if (!r.tags.hasOwnProperty(id)) {
          continue;
        }
        var tag = r.tags[id];
        if ((tmp = r.disk[f]) && (tmp = tmp.tags) && tmp[tag]) {
          continue;
        }
        ack = tag[f];
        delete tag[f];
        var ne;
        for (var k in tag) {
          if (tag.hasOwnProperty(k)) {
            ne = true;
            break;
          }
        } // is not empty?
        if (ne) {
          continue;
        } //if(!obj_empty(tag)){ continue }
        delete r.tags[tag];
        ack && ack(err, ok);
      }
      !q && (q = "");
      var l = q.length,
        i = 0;
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      // TODO: PERF: Why is acks so slow, what work do they do??? CHECK THIS!!
      var S = +new Date();
      for (; i < l; i++) {
        (ack = q[i]) && ack(err, ok);
      }
      console.STAT &&
        console.STAT(S, +new Date() - S, "rad acks", ename(s.file));
      console.STAT && console.STAT(S, q.length, "rad acks #", ename(s.file));
      var drift = s.drift;
      s.drift = null;
      if (drift) {
        for (i = 0; i < drift.length; i++) {
          r.save(drift[i][0], drift[i][1], noop);
        }
      }
      var waited = r.writing[f];
      delete r.writing[f];
      if (waited) {
        for (i = 0; i < waited.length; i++) {
          waited[i] && waited[i]();
        }
      }
    };
    cb ||
      (cb = function (err, ok) {
        // test delete!
        if (!err) {
          return;
        }
      });
    //console.only.i && console.log('save', key);
    r.find(key, s.find);
  };
  // Is a flush in flight? While one is, the files and the directory that routes
  // to them are both still moving, so an empty read is not yet an answer.
  r.busy = function () {
    for (var bf in r.writing) {
      if (Object.prototype.hasOwnProperty.call(r.writing, bf)) {
        return true;
      }
    }
    return false;
  };
  r.disk = {};
  r.writing = {}; // file -> queue of writes parked on an in-flight flush,
  // carrying `.rad`, the snapshot being written, so reads can still be served.
  r.one = {};
  r.tags = {};

  /*
			Any storage engine at some point will have to do a read in order to write.
			This is true of even systems that use an append only log, if they support updates.
			Therefore it is unavoidable that a read will have to happen,
			the question is just how long you delay it.
		*/
  var RWC = 0;
  r.write = function (file, rad, cb, o, DBG) {
    if (!rad) {
      cb("No radix!");
      return;
    }
    o = "object" == typeof o ? o : { force: o };
    var f = function Fractal() {},
      a,
      b;
    f.text = "";
    f.file = file = rad.file || (rad.file = file);
    if (!file) {
      cb("What file?");
      return;
    }
    f.write = function () {
      var text = (rad.raw = f.text);
      r.disk[(file = rad.file || f.file || file)] = rad;
      var S = +new Date();
      DBG && (DBG.wd = S);
      //console.only.i && console.log('add', file);
      r.find.add(file, function add(err) {
        DBG && (DBG.wa = +new Date());
        if (err) {
          cb(err);
          return;
        }
        //console.only.i && console.log('disk', file, text);
        opt.store.put(ename(file), text, function safe(err, ok) {
          DBG && (DBG.wp = +new Date());
          console.STAT &&
            console.STAT(
              S,
              (ST = +new Date() - S),
              "wrote disk",
              JSON.stringify(file),
              ++RWC,
              "total all writes.",
            );
          //console.only.i && console.log('done', err, ok || 1, cb);
          cb(err, ok || 1);
          if (!rad.Q) {
            delete r.disk[file];
          } // VERY IMPORTANT! Clean up memory, but not if there is already queued writes on it!
        });
      });
    };
    f.split = function () {
      var S = +new Date();
      DBG && (DBG.wf = S);
      f.text = "";
      if (!f.count) {
        f.count = 0;
        Radix.map(rad, function count() {
          f.count++;
        }); // TODO: Perf? Any faster way to get total length?
      }
      DBG && (DBG.wfc = f.count);
      f.limit = Math.ceil(f.count / 2);
      var SC = f.count;
      f.count = 0;
      DBG && (DBG.wf1 = +new Date());
      f.sub = Radix();
      Radix.map(rad, f.slice, { reverse: 1 }); // IMPORTANT: DO THIS IN REVERSE, SO LAST HALF OF DATA MOVED TO NEW FILE BEFORE DROPPING FROM CURRENT FILE.
      DBG && (DBG.wf2 = +new Date());
      // Before naming a new file, check that the name does not land inside a
      // range some other file already holds. A split takes the upper half of
      // ITS radix -- but that radix may carry keys that belong to files
      // published since, and naming a boundary in the middle of one of those
      // strands everything it already wrote there: the file is on disk, out of
      // memory, and nothing will come back for it. That is how a key written
      // once, correctly, becomes unreachable later.
      var inside;
      r.list &&
        Radix.map(r.list, function claimed(val, name) {
          if (val && name > (rad.file || "") && name <= f.end) {
            inside = name;
            return 1;
          }
        });
      if (inside) {
        // The range is spoken for. Do not publish; hand these keys back to the
        // directory and let them land in whatever owns them now.
        var back = [];
        Radix.map(f.sub, function taken(val, key) {
          back.push([key, val]);
        });
        f.hub = Radix();
        Radix.map(rad, function below(val, key) {
          if (key < f.end) {
            f.hub(key, val);
          }
        });
        r.write(
          rad.file,
          f.hub,
          function rehomed(err, ok) {
            for (var b = 0; b < back.length; b++) {
              r.save(back[b][0], back[b][1], noop);
            }
            cb(err, ok);
          },
          o,
        );
        return true;
      }
      r.write(
        f.end,
        f.sub,
        function (err, ok) {
          if (err) {
            return cb(err);
          }
          DBG && (DBG.wf3 = +new Date());
          f.hub = Radix();
          Radix.map(rad, f.stop);
          DBG && (DBG.wf4 = +new Date());
          r.write(rad.file, f.hub, cb, o);
        },
        o,
      );
      DBG && (DBG.wf5 = +new Date());
      console.STAT &&
        console.STAT(S, +new Date() - S, "rad split", ename(rad.file), SC);
      return true;
    };
    f.slice = function (val, key) {
      f.sub((f.end = key), val);
      if (f.limit <= ++f.count) {
        return true;
      }
    };
    f.stop = function (val, key) {
      if (key >= f.end) {
        return true;
      }
      f.hub(key, val);
    };
    f.both = function (err, ok) {
      DBG && (DBG.wfd = +new Date());
      if (b) {
        cb(err || b);
        return;
      }
      if (a) {
        cb(err, ok);
        return;
      }
      a = true;
      b = err;
    };
    f.each = function (val, key, k, pre) {
      if (u !== val) {
        f.count++;
      }
      if (opt.max <= (val || "").length) {
        return (cb("Data too big!"), true);
      }
      var enc =
        Radisk.encode(pre.length) +
        "#" +
        Radisk.encode(k) +
        (u === val ? "" : ":" + Radisk.encode(val)) +
        "\n";
      if (opt.chunk < f.text.length + enc.length && 1 < f.count && !o.force) {
        return f.split();
      }
      f.text += enc;
    };
    //console.only.i && console.log('writing');
    if (opt.jsonify) {
      r.write.jsonify(f, rad, cb, o, DBG);
      return;
    } // temporary testing idea
    if (!Radix.map(rad, f.each, true)) {
      f.write();
    }
  };

  r.write.jsonify = function (f, rad, cb, o, DBG) {
    var raw;
    var S = +new Date();
    DBG && (DBG.w = S);
    try {
      // A radix handed here can hold nothing, and both ways it happens are
      // normal: a split's lower half can come out empty, and so can the set of
      // keys a file still owns once splits published since have taken over its
      // range. `Radix()` leaves `$` undefined until something is put in it, and
      // `JSON.stringify(undefined)` is `undefined` -- reading `.length` off
      // that threw from a timer, outside this try, and took the process down
      // rather than failing the write. A file that owns nothing is an empty
      // file, and it still has to be written: the keys that left it are handed
      // back to the directory once this write lands.
      var src = rad.$ || {};
      var data = src;
      // Filter out stale tombstones (null values older than opt.tombstoneTTL).
      if (src && opt.tombstoneTTL > 0) {
        var now = Date.now(), ttl = opt.tombstoneTTL, hasStale = false, k;
        for (k in src) {
          var e = src[k];
          if (e && null === e[":"] && "number" === typeof e[">"] && e[">"] < now - ttl) {
            hasStale = true;
            break;
          }
        }
        if (hasStale) {
          data = {};
          for (k in src) {
            var ev = src[k];
            if (ev && null === ev[":"] && "number" === typeof ev[">"] && ev[">"] < now - ttl) {
              continue; // skip stale tombstone — omit from disk, keep alive in RAM
            }
            data[k] = ev;
          }
        }
      }
      raw = JSON.stringify(data);
    } catch (e) {
      cb("Cannot radisk!");
      return;
    }
    DBG && (DBG.ws = +new Date());
    console.STAT && console.STAT(S, +new Date() - S, "rad stringified JSON");
    if (opt.chunk < raw.length && !o.force) {
      var c = 0;
      Radix.map(rad, function () {
        if (c++) {
          return true;
        } // more than 1 item
      });
      if (c > 1) {
        return f.split();
      }
    }
    f.text = raw;
    f.write();
  };

  r.range = function (tree, o) {
    if (!tree || !o) {
      return;
    }
    if (u === o.start && u === o.end) {
      return tree;
    }
    if (atomic(tree)) {
      return tree;
    }
    var sub = Radix();
    Radix.map(
      tree,
      function (v, k) {
        sub(k, v);
      },
      o,
    ); // ONLY PLACE THAT TAKES TREE, maybe reduce API for better perf?
    return sub("");
  };

  {
    r.read = function (key, cb, o, DBG) {
      o = o || {};
      var g = { key: key };
      g.find = function (file) {
        var tmp;
        g.file = file || (file = opt.code.from);
        DBG && (DBG = DBG[file] = DBG[file] || {});
        DBG && (DBG.rf = DBG.rf || +new Date());
        if ((tmp = r.disk[(g.file = file)])) {
          g.check(u, tmp);
          return;
        }
        if ((tmp = r.writing[file])) {
          // Flush in flight: the file on disk is still the pre-flush version,
          // so serve the snapshot being written. `busy` keeps it out of r.disk
          // — republishing it would let later writes mutate a radix that is
          // mid-serialisation.
          //
          // Writes that arrived during this flush are parked on it, and they
          // are newer than the snapshot, so answer from them first when they
          // hold what was asked for.
          if (tmp.pend) {
            var pv = tmp.pend(key);
            if (pv && (!o.atom || tmp.pend.unit)) {
              g.get(u, tmp.pend, { file: file, busy: 1 });
              return;
            }
          }
          g.get(u, tmp.rad, { file: file, busy: 1 });
          return;
        }
        r.parse(file, g.check, u, DBG);
      };
      g.get = function (err, disk, info) {
        DBG && (DBG.rgl = +new Date());
        DBG && (DBG.rg = DBG.rg || +new Date());
        if ((g.err = err || g.err)) {
          cb(err);
          return;
        }
        var file = (g.file = (disk || "").file || g.file);
        if (!disk && file !== opt.code.from) {
          // corrupt file?
          r.find.bad(file); // remove from dir list
          r.read(key, cb, o); // try again
          return;
        }
        disk = r.disk[file] || ((info || "").busy ? disk : (r.disk[file] = disk));
        if (!disk) {
          cb(file === opt.code.from ? u : "No file!");
          return;
        }
        disk.file || (disk.file = file);
        var data = r.range(disk(key), o);
        DBG && (DBG.rr = +new Date());
        o.unit = disk.unit;
        o.chunks = (o.chunks || 0) + 1;
        o.parsed =
          (o.parsed || 0) + ((info || "").parsed || o.chunks * opt.chunk);
        o.more = 1;
        o.next = u;
        Radix.map(
          r.list,
          function next(v, f) {
            if (!v || file === f) {
              return;
            }
            o.next = f;
            return 1;
          },
          o.reverse ? { reverse: 1, end: file } : { start: file },
        );
        DBG && (DBG.rl = +new Date());
        if (!o.next) {
          o.more = 0;
        }
        if (o.next) {
          if (
            !o.reverse &&
            ((key < o.next && 0 != o.next.indexOf(key)) ||
              (u !== o.end && (o.end || "\uffff") < o.next))
          ) {
            o.more = 0;
          }
          if (
            o.reverse &&
            ((key > o.next && 0 != key.indexOf(o.next)) ||
              (u !== o.start && (o.start || "") > o.next && file <= o.start))
          ) {
            o.more = 0;
          }
        }
        //console.log(5, process.memoryUsage().heapUsed);
        if (!o.more) {
          // A branch is not an answer either. When one field was asked for and
          // this file holds children under the key but no value at it, the
          // entry is somewhere this read was never sent -- and the caller
          // discards a branch, so it is told nothing. Same remedy: if the
          // listing we routed by was empty, it was never a real route.
          var nothing = u === data || (o.atom && data && !o.unit);
          if (nothing && !o.atom && r.held) {
            // A soul is read by prefix, not by exact key -- and the index of
            // acknowledged-but-not-yet-durable writes is keyed exactly, so a
            // whole-node read had no safety net at all: it was told the node
            // does not exist while its fields sat in memory, waiting to be
            // flushed. That answer is final, and it is what a reader gets when
            // it asks for a node during the flush that is writing it.
            var pending;
            for (var k in r.held) {
              if (r.held.hasOwnProperty(k) && 0 === k.indexOf(key)) {
                (pending || (pending = Radix()))(k, r.held[k]);
              }
            }
            if (pending) {
              o.unit = 0;
              cb(g.err, r.range(pending(key), o) || pending, o);
              return;
            }
          }
          if (nothing && r.held && r.held[key]) {
            // Acknowledged and not yet durable. The radix that took it is
            // still here, so answer from it rather than call the key missing.
            // Hold the value itself, not the radix that took it: a split can
            // empty that radix out from under us, and then the one thing we
            // were sure of is gone too.
            var kept = r.held[key];
            if (kept && "object" == typeof kept && ":" in kept) {
              o.unit = 1;
              cb(g.err, kept, o);
              return;
            }
          }
          // A branch answer means this file covers the range but does not hold
          // the key -- the entry is in a file this read was not sent to, or in
          // a flush that has not landed. Either way the caller gets nothing and
          // never asks again, so ask again here. Bounded, and only for reads
          // that came back with no value at all.
          if (nothing && r.find.stale && r.find.stale()) {
            // Nothing here, and the directory this read routed by listed
            // nothing -- so it never had a chance: it looked in the one file
            // used when nothing is listed. Only such reads come here, so a
            // store with any files at all pays nothing. Ask again; the listing
            // has been dropped, so each attempt reads it afresh and the first
            // one that sees a file routes properly.
            o.relisted = (o.relisted || 0) + 1;
            if (5 < o.relisted) {
              cb(g.err, data, o);
              return;
            }
            setTimeout(function () {
              r.read(key, cb, o, DBG);
            }, 100 * o.relisted * o.relisted);
            return;
          }
          cb(g.err, data, o);
          return;
        }
        if (data) {
          cb(g.err, data, o);
        }
        if (o.parsed >= o.limit) {
          return;
        }
        var S = +new Date();
        DBG && (DBG.rm = S);
        var next = o.next;
        timediate(function () {
          console.STAT && console.STAT(S, +new Date() - S, "rad more");
          r.parse(next, g.check);
        }, 0);
      };
      g.check = function (err, disk, info) {
        //console.log(4, process.memoryUsage().heapUsed);
        g.get(err, disk, info);
        if (!disk || disk.check) {
          return;
        }
        disk.check = 1;
        var S = +new Date();
        (info || (info = {})).file || (info.file = g.file);
        Radix.map(disk, function (val, key) {
          // assume in memory for now, since both write/read already call r.find which will init it.
          r.find(key, function (file) {
            if ((file || (file = opt.code.from)) === info.file) {
              return;
            }
            var id = ("" + Math.random()).slice(-3);
            puff(function () {
              var maxRetries = 3;
              r.save(key, val, function ack(err, ok, retries) {
                if (err) {
                  retries = (retries || 0) + 1;
                  if (retries >= maxRetries) {
                    opt.log("Radisk: gave up relocating mislocated data after", retries, "attempts:", ename(key), err);
                    return;
                  }
                  r.save(key, val, function (e, o) { ack(e, o, retries); });
                  return;
                }
                console.STAT &&
                  console.STAT(
                    "MISLOCATED DATA CORRECTED",
                    id,
                    ename(key),
                    ename(info.file),
                    ename(file),
                  );
              });
            }, 0);
          });
        });
        console.STAT && console.STAT(S, +new Date() - S, "rad check");
      };
      r.find(key || (o.reverse ? o.end || "" : o.start || ""), g.find);
    };
  }

  {
    /*
				Let us start by assuming we are the only process that is
				changing the directory or bucket. Not because we do not want
				to be multi-process/machine, but because we want to experiment
				with how much performance and scale we can get out of only one.
				Then we can work on the harder problem of being multi-process.
			*/
    let RPC = 0;
    let Q = {};
    const s = String.fromCharCode(31);
    r.parse = function (file, cb, raw, DBG) {
      var q;
      if (!file) {
        return cb();
      }
      if ((q = Q[file])) {
        q.push(cb);
        return;
      }
      q = Q[file] = [cb];
      var p = function Parse() {},
        info = { file: file };
      (p.disk = Radix()).file = file;
      p.read = function (err, data) {
        var tmp;
        DBG && (DBG.rpg = +new Date());
        console.STAT &&
          console.STAT(
            S,
            +new Date() - S,
            "read disk",
            JSON.stringify(file),
            ++RPC,
            "total all parses.",
          );
        //console.log(2, process.memoryUsage().heapUsed);
        if ((p.err = err) || (p.not = !data)) {
          delete Q[file];
          p.map(q, p.ack);
          return;
        }
        if ("string" !== typeof data) {
          try {
            if (opt.max <= data.length) {
              p.err = "Chunk too big!";
            } else {
              data = data.toString(); // If it crashes, it crashes here. How!?? We check size first!
            }
          } catch (e) {
            p.err = e;
          }
          if (p.err) {
            delete Q[file];
            p.map(q, p.ack);
            return;
          }
        }
        info.parsed = data.length;
        DBG && (DBG.rpl = info.parsed);
        DBG && (DBG.rpa = q.length);
        S = +new Date();
        if (!(opt.jsonify || "{" === data[0])) {
          p.radec(err, data);
          return;
        }
        parse(data, function (err, tree) {
          //console.log(3, process.memoryUsage().heapUsed);
          if (!err) {
            delete Q[file];
            p.disk.$ = tree;
            console.STAT &&
              (ST = +new Date() - S) > 9 &&
              console.STAT(S, ST, "rad parsed JSON");
            DBG && (DBG.rpd = +new Date());
            p.map(q, p.ack); // hmmm, v8 profiler can't see into this cause of try/catch?
            return;
          }
          if ("{" === data[0]) {
            delete Q[file];
            p.err = tmp || "JSON error!";
            p.map(q, p.ack);
            return;
          }
          p.radec(err, data);
        });
      };
      p.map = function () {
        // switch to setTimeout.each now?
        if (!q || !q.length) {
          return;
        }
        //var i = 0, l = q.length, ack;
        var S = +new Date();
        var err = p.err,
          data = p.not ? u : p.disk;
        var i = 0,
          ack;
        while (i < 9 && (ack = q[i++])) {
          ack(err, data, info);
        } // too much?
        console.STAT &&
          console.STAT(S, +new Date() - S, "rad packs", ename(file));
        console.STAT && console.STAT(S, i, "rad packs #", ename(file));
        if (!(q = q.slice(i)).length) {
          return;
        }
        puff(p.map, 0);
      };
      p.ack = function (cb) {
        if (!cb) {
          return;
        }
        if (p.err || p.not) {
          cb(p.err, u, info);
          return;
        }
        cb(u, p.disk, info);
      };
      p.radec = function (err, data) {
        delete Q[file];
        S = +new Date();
        var tmp = p.split(data),
          pre = [],
          i,
          k,
          v;
        if (!tmp || 0 !== tmp[1]) {
          p.err = "File '" + file + "' does not have root radix! ";
          p.map(q, p.ack);
          return;
        }
        while (tmp) {
          k = v = u;
          i = tmp[1];
          tmp = p.split(tmp[2]) || "";
          if ("#" == tmp[0]) {
            k = tmp[1];
            pre = pre.slice(0, i);
            if (i <= pre.length) {
              pre.push(k);
            }
          }
          tmp = p.split(tmp[2]) || "";
          if ("\n" == tmp[0]) {
            continue;
          }
          if ("=" == tmp[0] || ":" == tmp[0]) {
            v = tmp[1];
          }
          if (u !== k && u !== v) {
            p.disk(pre.join(""), v);
          }
          tmp = p.split(tmp[2]);
        }
        console.STAT && console.STAT(S, +new Date() - S, "parsed RAD");
        p.map(q, p.ack);
      };
      p.split = function (t) {
        if (!t) {
          return;
        }
        var l = [],
          o = {},
          i = -1,
          a = "",
          b,
          c;
        i = t.indexOf(s);
        if (!t[i]) {
          return;
        }
        a = t.slice(0, i);
        l[0] = a;
        l[1] = b = Radisk.decode(t.slice(i), o);
        l[2] = t.slice(i + o.i);
        return l;
      };
      if (r.disk) {
        raw || (raw = (r.disk[file] || "").raw);
      }
      var S = +new Date(),
        SM,
        SL;
      DBG && (DBG.rp = S);
      if (raw) {
        return puff(function () {
          p.read(u, raw);
        }, 0);
      }
      opt.store.get(ename(file), p.read);
      // TODO: What if memory disk gets filled with updates, and we get an old one back?
    };
  }

  {
    let dir, Q;
    const f = String.fromCharCode(28);
    r.find = function (key, cb) {
      if (!dir) {
        if (Q) {
          Q.push([key, cb]);
          return;
        }
        Q = [[key, cb]];
        r.parse(f, init);
        return;
      }
      Radix.map(
        (r.list = dir),
        function (val, key) {
          if (!val) {
            return;
          }
          return cb(key) || true;
        },
        { reverse: 1, end: key },
      ) || cb(opt.code.from);
    };
    r.find.add = function (file, cb) {
      if (!dir) {
        dir = Radix();
        dir.file = f;
      }
      var has = dir(file);
      // `dir(file)` answers with the subtree when the name is a strict prefix
      // of other file names, and a subtree is truthy -- so a split that names
      // its new file `names<esc>a120` while `names<esc>a120a26` already exists
      // used to look like "already listed" and never got an entry. The file was
      // written, the directory never learned of it, and every read routed past
      // the only file holding those keys. `unit` is the radix saying it handed
      // back a value rather than a subtree.
      if ((has && dir.unit) || file === f) {
        cb(u, 1);
        return;
      }
      dir(file, 1);
      cb.found = (cb.found || 0) + 1;
      r.write(
        f,
        dir,
        function (err, ok) {
          if (err) {
            cb(err);
            return;
          }
          cb.found = (cb.found || 0) - 1;
          if (0 !== cb.found) {
            return;
          }
          cb(u, 1);
        },
        true,
      );
    };
    // The directory is loaded once. If that load lands while the store has
    // nothing to give -- a first flush still in the air, a slow disk, another
    // process mid-write -- the empty listing is kept, and from then on every
    // lookup falls back to the root file, which does not hold the key. That
    // reader finds nothing again for the life of the process, while one opened
    // a moment later finds everything. An empty listing is a fact about when we
    // asked, not about the store, so drop it and let the next lookup re-read.
    r.find.stale = function () {
      var any = 0;
      dir &&
        Radix.map(dir, function () {
          any = 1;
          return 1;
        });
      if (!any) {
        dir = null;
      }
      return !any;
    };
    r.find.bad = function (file, cb) {
      if (!dir) {
        dir = Radix();
        dir.file = f;
      }
      dir(file, 0);
      r.write(f, dir, cb || noop);
    };
    function init(err, disk) {
      if (err) {
        opt.log("list", err);
        setTimeout(function () {
          r.parse(f, init);
        }, 1000);
        return;
      }
      if (disk) {
        drain(disk);
        return;
      }
      dir = dir || disk || Radix();
      if (!opt.store.list) {
        drain(dir);
        return;
      }
      // import directory.
      opt.store.list(function (file) {
        if (!file) {
          drain(dir);
          return;
        }
        r.find.add(file, noop);
      });
    }
    function drain(rad, tmp) {
      dir = dir || rad;
      dir.file = f;
      tmp = Q;
      Q = null;
      map(tmp, function (arg) {
        r.find(arg[0], arg[1]);
      });
    }
  }

  try {
    !Zen.globalThis && __radmigtmp(r);
  } catch (e) {}

  var noop = function () {},
    RAD,
    u;
  Radisk.has[opt.file] = r;
  return r;
}

{
  const _ = String.fromCharCode(31);
  Radisk.encode = function (d, o, s) {
    s = s || _;
    var t = s,
      tmp;
    if (typeof d == "string") {
      var i = d.indexOf(s);
      while (i != -1) {
        t += s;
        i = d.indexOf(s, i + 1);
      }
      return t + '"' + d + s;
    } else if (d && d["#"] && 1 == Object.keys(d).length) {
      return t + "#" + tmp + t;
    } else if ("number" == typeof d) {
      return t + "+" + (d || 0) + t;
    } else if (null === d) {
      return t + " " + t;
    } else if (true === d) {
      return t + "+" + t;
    } else if (false === d) {
      return t + "-" + t;
    } // else
    //if(binary){}
  };
  Radisk.decode = function (t, o, s) {
    s = s || _;
    var d = "",
      i = -1,
      n = 0,
      c,
      p;
    if (s !== t[0]) {
      return;
    }
    while (s === t[++i]) {
      ++n;
    }
    p = t[(c = n)] || true;
    while (--n >= 0) {
      i = t.indexOf(s, i + 1);
    }
    if (i == -1) {
      i = t.length;
    }
    d = t.slice(c + 1, i);
    if (o) {
      o.i = i + 1;
    }
    if ('"' === p) {
      return d;
    } else if ("#" === p) {
      return { "#": d };
    } else if ("+" === p) {
      if (0 === d.length) {
        return true;
      }
      return parseFloat(d);
    } else if (" " === p) {
      return null;
    } else if ("-" === p) {
      return false;
    }
  };
}

var Zen = ZEN;
var Radix = __radix;

Radisk.Radix = Radix;
try {
  __defaultExport = Radisk;
} catch (e) {}
export default __defaultExport;
