import "./shim.js";
import __valid from "./valid.js";
import __state from "./state.js";
import __onto from "./onto.js";
import __dup from "./dup.js";
import __ask from "./ask.js";


function Zen(o) {
  if (o instanceof Zen) {
    return (this._ = { $: this }).$;
  }
  if (!(this instanceof Zen)) {
    return new Zen(o);
  }
  return Zen.create((this._ = { $: this, opt: o }));
}

Zen.is = function ($) {
  return $ instanceof Zen || ($ && $._ && $ === $._.$) || false;
};

Zen.version = 0.202;

Zen.chain = Zen.prototype;
Zen.chain.toJSON = function () {};

Zen.valid = __valid;
Zen.state = __state;
Zen.on = __onto;
Zen.dup = __dup;
Zen.ask = __ask;

{
  Zen.create = function (at) {
    at.root = at.root || at;
    at.graph = at.graph || {};
    at.on = at.on || Zen.on;
    at.ask = at.ask || Zen.ask;
    at.dup = at.dup || Zen.dup();
    var zen = at.$.opt(at.opt);
    if (!at.once) {
      at.on("in", universe, at);
      at.on("out", universe, at);
      at.on("put", map, at);
      Zen.on("create", at);
      at.on("create", at);
    }
    at.once = 1;
    return zen;
  };
  function universe(msg) {
    // TODO: BUG! msg.out = null being set!
    //if(!F){ var eve = this; setTimeout(function(){ universe.call(eve, msg,1) },Math.random() * 100);return; } // ADD F TO PARAMS!
    if (!msg) {
      return;
    }
    if (msg.out === universe) {
      this.to.next(msg);
      return;
    }
    var eve = this,
      as = eve.as,
      at = as.at || as,
      zen = at.$,
      dup = at.dup,
      tmp,
      DBG = msg.DBG;
    (tmp = msg["#"]) || (tmp = msg["#"] = text_rand(9));
    if (dup.check(tmp)) {
      return;
    }
    dup.track(tmp);
    tmp = msg._;
    msg._ = "function" == typeof tmp ? tmp : function () {};
    (msg.$ && msg.$ === (msg.$._ || "").$) || (msg.$ = zen);
    if (msg["@"] && !msg.put) {
      ack(msg);
    }
    if (!at.ask(msg["@"], msg)) {
      // is this machine listening for an ack?
      DBG && (DBG.u = +new Date());
      if (msg.put) {
        put(msg);
        return;
      } else if (msg.get) {
        Zen.on.get(msg, zen);
      }
    }
    DBG && (DBG.uc = +new Date());
    eve.to.next(msg);
    DBG && (DBG.ua = +new Date());
    if (msg.nts || msg.NTS) {
      return;
    } // TODO: This shouldn't be in core, but fast way to prevent NTS spread. Delete this line after all peers have upgraded to newer versions.
    msg.out = universe;
    at.on("out", msg);
    DBG && (DBG.ue = +new Date());
  }
  function put(msg) {
    if (!msg) {
      return;
    }
    var ctx = msg._ || "",
      root = (ctx.root = ((ctx.$ = msg.$ || "")._ || "").root);
    if (msg["@"] && ctx.faith && !ctx.miss) {
      // TODO: AXE may split/route based on 'put' what should we do here? Detect @ in AXE? I think we don't have to worry, as DAM will route it on @.
      msg.out = universe;
      root.on("out", msg);
      return;
    }
    ctx.latch = root.hatch;
    ctx.match = root.hatch = [];
    var put = msg.put;
    var DBG = (ctx.DBG = msg.DBG),
      S = +new Date();
    courtesyTime = courtesyTime || S;
    if (put["#"] && put["."]) {
      /*root && root.on('put', msg);*/ return;
    } // TODO: BUG! This needs to call HAM instead.
    DBG && (DBG.p = S);
    ctx["#"] = msg["#"];
    ctx.msg = msg;
    ctx.all = 0;
    ctx.stun = 1;
    var nl = Object.keys(put); //.sort(); // TODO: This is unbounded operation, large graphs will be slower. Write our own CPU scheduled sort? Or somehow do it in below? Keys itself is not O(1) either, create ES5 shim over ?weak map? or custom which is constant.
    console.STAT &&
      console.STAT(S, ((DBG || ctx).pk = +new Date()) - S, "put sort");
    var ni = 0,
      nj,
      kl,
      soul,
      node,
      states,
      err,
      tmp;
    function pop(o) {
      if (nj != ni) {
        nj = ni;
        if (!(soul = nl[ni])) {
          console.STAT &&
            console.STAT(S, ((DBG || ctx).pd = +new Date()) - S, "put");
          fire(ctx);
          return;
        }
        if (!(node = put[soul])) {
          err = ERR + cut(soul) + "no node.";
        } else if (!(tmp = node._)) {
          err = ERR + cut(soul) + "no meta.";
        } else if (soul !== tmp["#"]) {
          err = ERR + cut(soul) + "soul not same.";
        } else if (!(states = tmp[">"])) {
          err = ERR + cut(soul) + "no state.";
        }
        kl = Object.keys(node || {}); // TODO: .keys( is slow
      }
      if (err) {
        msg.err = ctx.err = err; // invalid data should error and stun the message.
        fire(ctx);
        //console.log("handle error!", err) // handle!
        return;
      }
      var i = 0,
        key;
      o = o || 0;
      while (o++ < 9 && (key = kl[i++])) {
        if ("_" === key) {
          continue;
        }
        var val = node[key],
          state = states[key];
        if (u === state) {
          err = ERR + cut(key) + "on" + cut(soul) + "no state.";
          break;
        }
        if (!valid(val)) {
          err =
            ERR + cut(key) + "on" + cut(soul) + "bad " + typeof val + cut(val);
          break;
        }
        //ctx.all++; //ctx.ack[soul+key] = '';
        ham(val, key, soul, state, msg);
        ++courtesy; // courtesy count;
      }
      if ((kl = kl.slice(i)).length) {
        turn(pop);
        return;
      }
      ++ni;
      kl = null;
      pop(o);
    }
    pop();
  }
  Zen.on.put = put;
  // TODO: MARK!!! clock below, reconnect sync, ZEN certify wire merge, User.auth taking multiple times, // msg put, put, say ack, hear loop...
  // WASIS BUG! local peer not ack. .off other people: .open
  function ham(val, key, soul, state, msg) {
    var ctx = msg._ || "",
      root = ctx.root,
      graph = root.graph,
      lot,
      tmp;
    var vertex = graph[soul] || empty,
      was = state_is(vertex, key, 1),
      known = vertex[key];

    var DBG = ctx.DBG;
    if ((tmp = console.STAT)) {
      if (!graph[soul] || !known) {
        tmp.has = (tmp.has || 0) + 1;
      }
    }

    var now = State(),
      u;
    if (state > now) {
      if ((tmp = state - now) > Ham.max) {
        msg.err = ctx.err =
          ERR + cut(key) + "on" + cut(soul) + "state too far in future.";
        fire(ctx);
        back(ctx);
        return;
      }
      setTimeout(
        function () {
          ham(val, key, soul, state, msg);
        },
        tmp > MD ? MD : tmp,
      ); // Max Defer 32bit. :(
      console.STAT &&
        console.STAT(((DBG || ctx).Hf = +new Date()), tmp, "future");
      return;
    }
    if (state < was) {
      return;
    } // but some chains have a cache miss that need to re-fire. // TODO: Improve in future. // for AXE this would reduce rebroadcast, but ZEN does it on message forwarding. // TURNS OUT CACHE MISS WAS NOT NEEDED FOR NEW CHAINS ANYMORE!!! DANGER DANGER DANGER, ALWAYS RETURN! (or am I missing something?)
    if (!ctx.faith) {
      // TODO: BUG? Can this be used for cache miss as well? // Yes this was a bug, need to check cache miss for RAD tests, but should we care about the faith check now? Probably not.
      if (state === was && (val === known || L(val) <= L(known))) {
        /*console.log("same");*/ /*same;*/ if (!ctx.miss) {
          return;
        }
      } // same
    }
    ctx.stun++; // TODO: 'forget' feature in ZEN tied to this, bad approach, but hacked in for now. Any changes here must update there.
    var aid = msg["#"] + ctx.all++,
      id = {
        toString: function () {
          return aid;
        },
        _: ctx,
      };
    id.toJSON = id.toString; // this *trick* makes it compatible between old & new versions.
    root.dup.track(id)["#"] = msg["#"]; // fixes new OK acks for RPC like RTC.
    DBG && (DBG.ph = DBG.ph || +new Date());
    root.on("put", {
      "#": id,
      "@": msg["@"],
      put: { "#": soul, ".": key, ":": val, ">": state },
      ok: msg.ok,
      _: ctx,
    });
  }
  function map(msg) {
    var DBG;
    if ((DBG = (msg._ || "").DBG)) {
      DBG.pa = +new Date();
      DBG.pm = DBG.pm || +new Date();
    }
    var eve = this,
      root = eve.as,
      graph = root.graph,
      ctx = msg._,
      put = msg.put,
      soul = put["#"],
      key = put["."],
      val = put[":"],
      state = put[">"],
      id = msg["#"],
      tmp;
    if ((tmp = ctx.msg) && (tmp = tmp.put) && (tmp = tmp[soul])) {
      state_ify(tmp, key, state, val, soul);
    } // necessary! or else out messages do not get ZEN transforms.
    //var bytes = ((graph[soul]||'')[key]||'').length||1;
    graph[soul] = state_ify(graph[soul], key, state, val, soul);
    if ((tmp = (root.next || "")[soul])) {
      //tmp.bytes = (tmp.bytes||0) + ((val||'').length||1) - bytes;
      //if(tmp.bytes > 2**13){ Zen.log.once('byte-limit', "Note: In the future, ZEN peers will enforce a ~4KB query limit. Please see https://zen.eco/docs/Page") }
      tmp.on("in", msg);
    }
    fire(ctx);
    eve.to.next(msg);
  }
  function fire(ctx, msg) {
    var root;
    if (ctx.stop) {
      return;
    }
    if (0 < --ctx.stun && !ctx.err) {
      return;
    } // decrement always runs; early-return only if stun still positive AND no error.
    if (ctx.stun < 0) { ctx.stun = 0; } // defensive: ctx.stop below prevents double-fire, but guard underflow
    ctx.stop = 1;
    if (!(root = ctx.root)) {
      return;
    }
    var tmp = ctx.match;
    tmp.end = 1;
    if (tmp === root.hatch) {
      if (!(tmp = ctx.latch) || tmp.end) {
        delete root.hatch;
      } else {
        root.hatch = tmp;
      }
    }
    ctx.hatch && ctx.hatch(); // TODO: rename/rework how put & this interact.
    setTimeout.each(ctx.match, function (cb) {
      cb && cb();
    });
    if (!(msg = ctx.msg) || ctx.err || msg.err) {
      return;
    }
    msg.out = universe;
    ctx.root.on("out", msg);

    courtesyCheck(); // courtesy check;
  }
  function ack(msg) {
    // aggregate ACKs.
    var id = msg["@"] || "",
      ctx,
      ok,
      tmp;
    if (!(ctx = id._)) {
      var dup =
        (dup = msg.$) && (dup = dup._) && (dup = dup.root) && (dup = dup.dup);
      if (!(dup = dup.check(id))) {
        return;
      }
      msg["@"] = dup["#"] || msg["@"]; // This doesn't do anything anymore, backtrack it to something else?
      return;
    }
    ctx.acks = (ctx.acks || 0) + 1;
    if ((ctx.err = msg.err)) {
      msg["@"] = ctx["#"];
      fire(ctx); // TODO: BUG? How it skips/stops propagation of msg if any 1 item is error, this would assume a whole batch/resync has same malicious intent.
    }
    ctx.ok = msg.ok || ctx.ok;
    if (!ctx.stop && !ctx.crack) {
      ctx.crack =
        ctx.match &&
        ctx.match.push(function () {
          back(ctx);
        });
    } // handle synchronous acks. NOTE: If a storage peer ACKs synchronously then the PUT loop has not even counted up how many items need to be processed, so ctx.STOP flags this and adds only 1 callback to the end of the PUT loop.
    back(ctx);
  }
  function back(ctx) {
    if (!ctx || !ctx.root) {
      return;
    }
    if (ctx.stun || (ctx.acks || 0) !== ctx.all) {
      return;
    } // normalize acks: undefined treated as 0 before first storage ack arrives.
    ctx.root.on("in", {
      "@": ctx["#"],
      err: ctx.err,
      ok: ctx.err ? u : ctx.ok || { "": 1 },
    });
  }

  var ERR = "Error: Invalid graph!";
  var cut = function (s) {
    return " '" + ("" + s).slice(0, 9) + "...' ";
  };
  var L = JSON.stringify,
    MD = 2147483647,
    State = Zen.state;
  var Ham = ham;
  Ham.max = 1000 * 60 * 60 * 24 * 7; // 1 week: legit clock skew is seconds, not days.
  let courtesy = 0,
    courtesyTime,
    courtesyCheck = function () {
      if (
        courtesy > 999 &&
        courtesy / -(courtesyTime - (courtesyTime = +new Date())) > 1
      ) {
        Zen.window &&
          console.log(
            "Warning: You're syncing 1K+ records a second, faster than DOM can update - consider limiting query.",
          );
        courtesyCheck = function () {
          courtesy = 0;
        };
      }
    };
}

{
  Zen.on.get = function (msg, zen) {
    var root = zen._,
      get = msg.get,
      soul = get["#"],
      node = root.graph[soul],
      has = get["."];
    var next = root.next || (root.next = {}),
      at = next[soul];

    // TODO: Azarattum bug, what is in graph is not same as what is in next. Fix!

    // queue concurrent GETs?
    // TODO: consider tagging original message into dup for DAM.
    // TODO: ^ above? In chat app, 12 messages resulted in same peer asking for `#user.pub` 12 times. (same with #user GET too, yipes!) // DAM note: This also resulted in 12 replies from 1 peer which all had same ##hash but none of them deduped because each get was different.
    // TODO: Moving quick hacks fixing these things to axe for now.
    // TODO: a lot of GET #foo then GET #foo."" happening, why?
    // TODO: DAM's ## hash check, on same get ACK, producing multiple replies still, maybe JSON vs YSON?
    // TMP note for now: viMZq1slG was chat LEX query #.
    /*if(zen !== (tmp = msg.$) && (tmp = (tmp||'')._)){
                if(tmp.Q){ tmp.Q[msg['#']] = ''; return } // chain does not need to ask for it again.
                tmp.Q = {};
            }*/
    /*if(u === has){
                if(at.Q){
                    //at.Q[msg['#']] = '';
                    //return;
                }
                at.Q = {};
            }*/
    var ctx = msg._ || {},
      DBG = (ctx.DBG = msg.DBG);
    DBG && (DBG.g = +new Date());
    //console.log("GET:", get, node, has, at);
    //if(!node && !at){ return root.on('get', msg) }
    //if(has && node){ // replace 2 below lines to continue dev?
    if (!node) {
      return root.on("get", msg);
    }
    if (has) {
      if ("string" != typeof has || u === node[has]) {
        if (!((at || "").next || "")[has]) {
          root.on("get", msg);
          return;
        }
      }
      node = state_ify({}, has, state_is(node, has), node[has], soul);
      // If we have a key in-memory, do we really need to fetch?
      // Maybe... in case the in-memory key we have is a local write
      // we still need to trigger a pull/merge from peers.
    }
    //Zen.window? Zen.obj.copy(node) : node; // HNPERF: If !browser bump Performance? Is this too dangerous to reference root graph? Copy / shallow copy too expensive for big nodes. Zen.obj.to(node); // 1 layer deep copy // Zen.obj.copy(node); // too slow on big nodes
    node && ack(msg, node);
    root.on("get", msg); // send GET to storage adapters.
  };
  function ack(msg, node) {
    var S = +new Date(),
      ctx = msg._ || {},
      DBG = (ctx.DBG = msg.DBG);
    var to = msg["#"],
      id = text_rand(9),
      keys = Object.keys(node || "").sort(),
      soul = ((node || "")._ || "")["#"],
      kl = keys.length,
      j = 0,
      root = msg.$._.root,
      F = node === root.graph[soul];
    console.STAT &&
      console.STAT(S, ((DBG || ctx).gk = +new Date()) - S, "got keys");
    // PERF: Consider commenting this out to force disk-only reads for perf testing? // TODO: .keys( is slow
    function go() {
      S = +new Date();
      var i = 0,
        k,
        put = {},
        tmp;
      while (i < 9 && (k = keys[i++])) {
        state_ify(put, k, state_is(node, k), node[k], soul);
      }
      keys = keys.slice(i);
      (tmp = {})[soul] = put;
      put = tmp;
      var faith;
      if (F) {
        faith = function () {};
        faith.ram = faith.faith = true;
      } // HNPERF: We're testing performance improvement by skipping going through security again, but this should be audited.
      tmp = keys.length;
      console.STAT &&
        console.STAT(S, -(S - (S = +new Date())), "got copied some");
      DBG && (DBG.ga = +new Date());
      root.on("in", {
        "@": to,
        "#": id,
        put: put,
        "%": tmp ? (id = text_rand(9)) : u,
        $: root.$,
        _: faith,
        DBG: DBG,
      });
      console.STAT && console.STAT(S, +new Date() - S, "got in");
      if (!tmp) {
        return;
      }
      setTimeout.turn(go);
    }
    node && go();
    if (!node) {
      root.on("in", { "@": msg["#"] });
    } // TODO: I don't think I like this, the default lS adapter uses this but "not found" is a sensitive issue, so should probably be handled more carefully/individually.
  }
  Zen.on.get.ack = ack;
}

{
  Zen.chain.opt = function (opt) {
    opt = opt || {};
    var zen = this,
      at = zen._,
      tmp = opt.peers || opt;
    if (!Object.plain(opt)) {
      opt = {};
    }
    if (!Object.plain(at.opt)) {
      at.opt = opt;
    }
    if ("string" == typeof tmp) {
      tmp = [tmp];
    }
    if (!Object.plain(at.opt.peers)) {
      at.opt.peers = {};
    }
    if (tmp instanceof Array) {
      opt.peers = {};
      tmp.forEach(function (url) {
        var p = {};
        p.id = p.url = url;
        opt.peers[url] = at.opt.peers[url] = at.opt.peers[url] || p;
      });
    }
    obj_each(opt, function each(k) {
      var v = this[k];
      if (
        (this && this.hasOwnProperty(k)) ||
        "string" == typeof v ||
        Object.empty(v)
      ) {
        this[k] = v;
        return;
      }
      if (v && v.constructor !== Object && !(v instanceof Array)) {
        return;
      }
      obj_each(v, each);
    });
    at.opt.from = opt;
    Zen.on("opt", at);
    at.opt.uuid =
      at.opt.uuid ||
      function uuid(l) {
        return (
          Zen.state().toString(36).replace(".", "") + String.random(l || 12)
        );
      };
    return zen;
  };
}

var obj_each = function (o, f) {
    Object.keys(o).forEach(f, o);
  },
  text_rand = String.random,
  turn = setTimeout.turn,
  valid = Zen.valid,
  state_is = Zen.state.is,
  state_ify = Zen.state.ify,
  u,
  empty = {},
  C;

Zen.log = function () {
  var log = C && (typeof process !== "undefined" && !Zen.window ? C.error : C.log);
  return (
    !Zen.log.off && "function" == typeof log && log.apply(C, arguments),
    [].slice.call(arguments).join(" ")
  );
};
Zen.log.once = function (w, s, o) {
  return (((o = Zen.log.once)[w] = o[w] || 0), o[w]++ || Zen.log(s));
};

typeof globalThis !== "undefined" &&
typeof window === "undefined" &&
typeof WorkerGlobalScope !== "undefined"
  ? ((globalThis.Zen = Zen).window = globalThis)
  : typeof window !== "undefined"
    ? ((window.Zen = Zen).window = window)
    : undefined;
(globalThis.Zen = Zen).globalThis = globalThis;
try {
  if (typeof MODULE !== "undefined") {
    MODULE.exports = Zen;
  }
} catch (e) {}

(Zen.window || {}).console = (Zen.window || {}).console || {
  log: function () {},
};
(C = typeof console !== "undefined" ? console : { log: function () {} }).only =
  function (i, s) {
    var log = C && C.log;
    return (
      C.only.i &&
      i === C.only.i &&
      C.only.i++ &&
      (("function" == typeof log && log.apply(C, arguments)) || s)
    );
  };

export default Zen;
