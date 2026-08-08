import ZEN from "../zen.js";
import __radisk from "./radisk.js";
import tpath from "./tpath.js";
var Zen = ZEN;

Zen.on("create", function (root) {
  if (Zen.TESTING) {
    root.opt.file = tpath(root.opt.file, "radatatest", true);
  }
  if (root.opt.rfs === false) {
    this.to.next(root);
    return;
  }
  this.to.next(root);
  var opt = root.opt,
    empty = {},
    u;
  if (false === opt.rad || false === opt.radisk) {
    return;
  }
  if (u + "" != typeof process && "false" === "" + (process.env || "").RAD) {
    return;
  }
  var Radisk = __radisk;
  var Radix = Radisk.Radix;
  var dare = Radisk(opt),
    esc = String.fromCharCode(27);
  var ST = 0;

  root.on("put", function (msg) {
    this.to.next(msg);
    if ((msg._ || "").rad) {
      return;
    } // don't save what just came from a read.
    //if(msg['@']){ return } // WHY DID I NOT ADD THIS?
    var id = msg["#"],
      put = msg.put,
      soul = put["#"],
      key = put["."],
      val = put[":"],
      state = put[">"],
      tmp;
    var DBG = (msg._ || "").DBG;
    DBG && (DBG.sp = DBG.sp || +new Date());
    //var lot = (msg._||'').lot||''; count[id] = (count[id] || 0) + 1;
    var S = (msg._ || "").RPS || ((msg._ || "").RPS = +new Date());
    //console.log("PUT ------->>>", soul,key, val, state);
    //dare(soul+esc+key, {':': val, '>': state}, dare.one[id] || function(err, ok){
    dare(
      soul + esc + key,
      { ":": val, ">": state },
      function (err, ok) {
        //console.log("<<<------- PAT", soul,key, val, state, 'in', +new Date - S);
        DBG && (DBG.spd = DBG.spd || +new Date());
        console.STAT && console.STAT(S, +new Date() - S, "put");
        //if(!err && count[id] !== lot.s){ console.log(err = "Disk count not same as ram count."); console.STAT && console.STAT(+new Date, lot.s - count[id], 'put ack != count') } delete count[id];
        if (err) {
          root.on("in", { "@": id, err: err, DBG: DBG });
          return;
        }
        root.on("in", { "@": id, ok: ok, DBG: DBG });
        //}, id, DBG && (DBG.r = DBG.r || {}));
      },
      false && id,
      DBG && (DBG.r = DBG.r || {}),
    );
    DBG && (DBG.sps = DBG.sps || +new Date());
  });
  var count = {},
    obj_empty = Object.empty;

  root.on("get", function (msg) {
    this.to.next(msg);
    var ctx = msg._ || "",
      DBG = (ctx.DBG = msg.DBG);
    DBG && (DBG.sg = +new Date());
    var id = msg["#"],
      get = msg.get,
      soul = msg.get["#"],
      has = msg.get["."] || "",
      o = {},
      graph,
      lex,
      key,
      tmp,
      force;
    if ("string" == typeof soul) {
      key = soul;
    } else if (soul) {
      if (u !== (tmp = soul["*"])) {
        o.limit = force = 1;
      }
      if (u !== soul[">"]) {
        o.start = soul[">"];
      }
      if (u !== soul["<"]) {
        o.end = soul["<"];
      }
      key = force ? "" + tmp : tmp || soul["="];
      force = null;
    }
    if (key && !o.limit) {
      // a soul.has must be on a soul, and not during soul*
      if ("string" == typeof has) {
        key = key + esc + (o.atom = has);
      } else if (has) {
        if (u !== has[">"]) {
          o.start = has[">"];
          o.limit = 1;
        }
        if (u !== has["<"]) {
          o.end = has["<"];
          o.limit = 1;
        }
        if (u !== (tmp = has["*"])) {
          o.limit = force = 1;
        }
        if (key) {
          key =
            key +
            esc +
            (force ? "" + (tmp || "") : tmp || (o.atom = has["="] || ""));
        }
      }
    }
    if ((tmp = get["%"]) || o.limit) {
      o.limit = tmp <= (o.pack || 1000 * 100) ? tmp : 1;
    }
    if (has["-"] || (soul || {})["-"] || get["-"]) {
      o.reverse = true;
    }
    if ((tmp = (root.next || "")[soul]) && tmp.put) {
      if (o.atom) {
        tmp = (tmp.next || "")[o.atom];
        if (
          tmp &&
          tmp.root &&
          tmp.root.graph &&
          tmp.root.graph[soul] &&
          tmp.root.graph[soul][o.atom]
        ) {
          return;
        }
      } else if (tmp && tmp.rad) {
        return;
      }
    }
    var now = Zen.state();
    var S = +new Date(),
      C = 0,
      SPT = 0; // STATS!
    DBG && (DBG.sgm = S);
    //var GID = String.random(3); console.log("GET ------->>>", GID, key, o, '?', get);
    dare(
      key || "",
      function (err, data, info) {
        //console.log("<<<------- GOT", GID, +new Date - S, err, data);
        DBG && (DBG.sgr = +new Date());
        DBG && (DBG.sgi = info);
        try {
          opt.store.stats.get.time[statg % 50] = +new Date() - S;
          ++statg;
          opt.store.stats.get.count++;
          if (err) {
            opt.store.stats.get.err = err;
          }
        } catch (e) {} // STATS!
        //if(u === data && info.chunks > 1){ return } // if we already sent a chunk, ignore ending empty responses. // this causes tests to fail.
        console.STAT &&
          console.STAT(S, +new Date() - S, "got", JSON.stringify(key));
        S = +new Date();
        info = info || "";
        var va, ve;
        if (
          info.unit &&
          data &&
          u !== (va = data[":"]) &&
          u !== (ve = data[">"])
        ) {
          // new format
          var tmp = key.split(esc),
            so = tmp[0],
            ha = tmp[1];
          (graph = graph || {})[so] = Zen.state.ify(graph[so], ha, ve, va, so);
          root.$.get(so).get(ha)._.rad = now;
          // REMEMBER TO ADD _rad TO NODE/SOUL QUERY!
        } else if (data) {
          // old code path
          if (typeof data !== "string") {
            if (o.atom) {
              data = u;
            } else {
              Radix.map(data, each, o); // IS A RADIX TREE, NOT FUNCTION!
            }
          }
          if (!graph && data) {
            each(data, "");
          }
          // TODO: !has what about soul lookups?
          if (
            !o.atom &&
            !has & ("string" == typeof soul) &&
            !o.limit &&
            !o.more
          ) {
            root.$.get(soul)._.rad = now;
          }
        }
        DBG && (DBG.sgp = +new Date());
        // TODO: PERF NOTES! This is like 0.2s, but for each ack, or all? Can you cache these preps?
        // TODO: PERF NOTES! This is like 0.2s, but for each ack, or all? Can you cache these preps?
        // TODO: PERF NOTES! This is like 0.2s, but for each ack, or all? Can you cache these preps?
        // TODO: PERF NOTES! This is like 0.2s, but for each ack, or all? Can you cache these preps?
        // TODO: PERF NOTES! This is like 0.2s, but for each ack, or all? Can you cache these preps?
        // Or benchmark by reusing first start date.
        if (console.STAT && (ST = +new Date() - S) > 9) {
          console.STAT(S, ST, "got prep time");
          console.STAT(S, C, "got prep #");
        }
        SPT += ST;
        C = 0;
        S = +new Date();
        var faith = function () {};
        faith.faith = true;
        faith.rad = get; // HNPERF: We're testing performance improvement by skipping going through security again, but this should be audited.
        root.on("in", {
          "@": id,
          put: graph,
          "%": info.more ? 1 : u,
          err: err ? err : u,
          _: faith,
          DBG: DBG,
        });
        console.STAT &&
          (ST = +new Date() - S) > 9 &&
          console.STAT(S, ST, "got emit", Object.keys(graph || {}).length);
        // A subscription means "tell me when you have it", but storage has no
        // way to tell anyone: a read that finds nothing is the last word. And
        // it can be wrong. While a flush is landing, the file covering a soul
        // may not be in the directory yet -- measured at 5 to 112ms behind on
        // Windows -- so the read is routed past data that is already durable,
        // comes back empty, and nobody ever asks again.
        //
        // So ask once more, after the batching window, and deliver what turns
        // up as ordinary data rather than as an answer to the original request:
        // the id is left off deliberately, since that request has already been
        // answered and must not be answered twice.
        if (!info.more && !graph && !err && "string" == typeof soul && !o.atom) {
          var askAgain = function (next) {
            var late;
            dare(
              key || "",
              function (e2, d2, i2) {
                if (e2 || !d2) {
                  if (!e2 && next) {
                    next();
                  }
                  return;
                }
                graph = late = late || {};
                if ((i2 || "").unit) {
                  var pair = key.split(esc);
                  late[pair[0]] = Zen.state.ify(
                    late[pair[0]],
                    pair[1],
                    d2[">"],
                    d2[":"],
                    pair[0],
                  );
                } else if ("string" != typeof d2) {
                  Radix.map(d2, each, o);
                }
                if (!(i2 || "").more) {
                  if (graph) {
                    root.on("in", { put: graph, _: { rad: get } });
                  } else if (next) {
                    next(); // still nothing -- try the next rung
                  }
                }
                graph = u;
              },
              { atom: o.atom },
            );
          };
          // A ladder, stopping the moment something turns up. How far the
          // directory lags the acknowledgement varies -- 5, 32, 36 and 112ms
          // across the samples, and longer still in others -- so one delay
          // cannot cover it: too early and the entry is not there yet, too late
          // and a caller may already have written over what it was waiting for.
          //
          // Only while a flush is in flight. With storage idle an empty answer
          // is trustworthy, and a key that genuinely does not exist should not
          // cost four reads.
          var rungs = [
            Math.max(19, (opt.until || 250) >> 2),
            Math.max(39, (opt.until || 250) >> 1),
            Math.max(79, ((opt.until || 250) * 3) >> 2),
            (opt.until || 250) + 99,
          ];
          var rung = 0;
          var climb = function () {
            if (rung >= rungs.length) {
              return;
            }
            var wait = rungs[rung++];
            setTimeout(function () {
              // Idle at this instant is not the same as settled for good: a
              // flush may not have started yet, or may have paused between
              // files. Skip the read, but keep climbing -- abandoning the
              // ladder here puts the original failure straight back.
              if (!dare.busy || !dare.busy()) {
                climb();
                return;
              }
              askAgain(climb);
            }, wait);
          };
          climb();
        }
        graph = u; // each is outside our scope, we have to reset graph to nothing!
      },
      o,
      DBG && (DBG.r = DBG.r || {}),
    );
    DBG && (DBG.sgd = +new Date());
    console.STAT &&
      (ST = +new Date() - S) > 9 &&
      console.STAT(S, ST, "get call"); // TODO: Perf: this was half a second??????
    function each(val, has, a, b) {
      // TODO: THIS CODE NEEDS TO BE FASTER!!!!
      C++;
      if (!val) {
        return;
      }
      has = (key + has).split(esc);
      var soul = has.slice(0, 1)[0];
      has = has.slice(-1)[0];
      if (o.limit && o.limit <= o.count) {
        return true;
      }
      var va,
        ve,
        so = soul,
        ha = has;
      //if(u !== (va = val[':']) && u !== (ve = val['>'])){ // THIS HANDLES NEW CODE!
      if ("string" != typeof val) {
        // THIS HANDLES NEW CODE!
        va = val[":"];
        ve = val[">"];
        (graph = graph || {})[so] = Zen.state.ify(graph[so], ha, ve, va, so);
        //root.$.get(so).get(ha)._.rad = now;
        o.count = (o.count || 0) + ((va || "").length || 9);
        return;
      }
      o.count = (o.count || 0) + val.length;
      var tmp = val.lastIndexOf(">");
      var state = Radisk.decode(val.slice(tmp + 1), null, esc);
      val = Radisk.decode(val.slice(0, tmp), null, esc);
      (graph = graph || {})[soul] = Zen.state.ify(
        graph[soul],
        has,
        state,
        val,
        soul,
      );
    }
  });
  var val_is = Zen.valid;
  (opt.store || {}).stats = {
    get: { time: {}, count: 0 },
    put: { time: {}, count: 0 },
  }; // STATS!
  var statg = 0,
    statp = 0; // STATS!

  // OOM eviction: periodically evict persisted souls from the in-memory graph
  // based on *current* system free memory (re-evaluated every 30 s so the
  // decision always reflects the real runtime state, not a one-time snapshot).
  // opt.evict = false disables the feature entirely.
  if (opt.evict !== false && typeof process !== "undefined" && process.memoryUsage) {
    var __os = (function () { try { return require("os"); } catch (e) { return null; } }());
    // Thresholds (either condition triggers eviction):
    //   opt.frat : evict when free/total < this ratio  (default 0.10 = 10%)
    //   opt.fmb  : evict when free < this many MB       (default 200 MB)
    var minFreeRatio = opt.frat !== undefined ? opt.frat : 0.10;
    var minFreeBytes = opt.fmb  !== undefined ? opt.fmb * 1024 * 1024 : (200 * 1024 * 1024);

    var evictTimer = setInterval(function () {
      if (!__os) { return; }
      var free  = __os.freemem();
      var total = __os.totalmem();
      var ratio = free / total;

      // Check if system memory is tight enough to warrant eviction.
      var tight = (ratio < minFreeRatio) || (free < minFreeBytes);
      if (!tight) { return; }

      var graph = root.graph;
      if (!graph) { return; }
      var souls = Object.keys(graph);
      if (!souls.length) { return; }

      // Build (soul, lastAccess) pairs — use _.rad timestamp as proxy for LRU.
      var pairs = [];
      for (var i = 0; i < souls.length; i++) {
        var soul = souls[i];
        var chain = (root.next || {})[soul];
        // Only evict souls confirmed persisted on disk (_.rad set after a
        // successful radisk read/write) and not currently being written.
        if (chain && chain._ && chain._.rad && !(dare.disk && dare.disk[soul])) {
          pairs.push([soul, chain._.rad]);
        }
      }
      if (!pairs.length) { return; }

      // Evict more aggressively the tighter memory is.
      // pressure 0→1 maps eviction fraction 5%→40%.
      var pressure = Math.min(1, Math.max(0,
        1 - (free / (total * minFreeRatio))   // how far below threshold
      ));
      var fraction = 0.05 + pressure * 0.35;
      pairs.sort(function (a, b) { return a[1] - b[1]; }); // coldest first
      var limit = Math.max(1, Math.ceil(pairs.length * fraction));
      for (var j = 0; j < limit; j++) { delete graph[pairs[j][0]]; }

      opt.log && opt.log(
        "ZEN: evicted", limit, "souls from memory.",
        "free:", Math.round(free / 1024 / 1024) + "MB",
        "(" + Math.round(ratio * 100) + "% of " + Math.round(total / 1024 / 1024) + "MB total)"
      );
      root.on("memory/evict", {
        count: limit,
        freeMemMB: Math.round(free / 1024 / 1024),
        totalMemMB: Math.round(total / 1024 / 1024),
        heapMB: Math.round(process.memoryUsage().heapUsed / 1024 / 1024),
      });
    }, 30 * 1000); // every 30 s
    evictTimer.unref && evictTimer.unref();
  }
});
