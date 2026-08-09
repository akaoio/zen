import Zen from "./root.js";
import jsonAsync from "./json.js";

var env = (typeof process !== "undefined" && process.env) || {};

var noop = function () {},
  store,
  u;
try {
  store = (Zen.window || noop).localStorage;
} catch (e) {}
if (!store) {
  var isTTY = typeof process !== "undefined" && process.stdout && process.stdout.isTTY;
  if (!env.ZEN_SILENCE_TEST_WARNINGS && (typeof process === "undefined" || isTTY)) {
    Zen.log("Warning: No localStorage exists to persist data to!");
  }
  store = {
    setItem: function (k, v) {
      this[k] = v;
    },
    removeItem: function (k) {
      delete this[k];
    },
    getItem: function (k) {
      return this[k];
    },
  };
}

var pair = jsonAsync.createJsonPair();
var parse = pair.parse;
var json = pair.json;

Zen.on("create", function lg(root) {
  this.to.next(root);
  var opt = root.opt,
    graph = root.graph,
    acks = [],
    disk,
    to,
    size,
    stop;
  if (false === opt.localStorage) {
    // Memory-only mode: no disk writes but still ack puts so callbacks fire.
    root.on("put", function (msg) {
      this.to.next(msg);
      if (!msg["@"]) {
        root.on("in", { "@": msg["#"], ok: 1 });
      }
    });
    return;
  }
  opt.prefix = opt.file || "zen/";
  try {
    disk = lg[opt.prefix] =
      lg[opt.prefix] || JSON.parse((size = store.getItem(opt.prefix))) || {}; // TODO: Perf! This will block, should we care, since limited to 5MB anyways?
  } catch (e) {
    disk = lg[opt.prefix] = {};
  }
  size = (size || "").length;

  root.on("get", function (msg) {
    this.to.next(msg);
    var lex = msg.get,
      soul,
      data,
      tmp,
      u;
    if (!lex || !(soul = lex["#"])) {
      return;
    }
    data = disk[soul] || u;
    if (data && (tmp = lex["."]) && !Object.plain(tmp)) {
      // pluck!
      data = Zen.state.ify({}, tmp, Zen.state.is(data, tmp), data[tmp], soul);
    }
    //if(data){ (tmp = {})[soul] = data } // back into a graph.
    //setTimeout(function(){
    Zen.on.get.ack(msg, data); //root.on('in', {'@': msg['#'], put: tmp, lS:1});// || root.$});
    //}, Math.random() * 10); // FOR TESTING PURPOSES!
  });

  root.on("put", function (msg) {
    this.to.next(msg); // remember to call next middleware adapter
    var put = msg.put,
      soul = put["#"],
      key = put["."],
      id = msg["#"],
      ok = msg.ok || "",
      tmp; // pull data off wire envelope
    // <?N souls are EPHEMERAL — never persisted, browser storage included.
    // Same ack discipline as the memory-only branch above.
    if (0 <= soul.indexOf("<?")) {
      if (!msg["@"]) {
        root.on("in", { "@": id, ok: 1 });
      }
      return;
    }
    disk[soul] = Zen.state.ify(disk[soul], key, put[">"], put[":"], soul); // merge into disk object
    if (stop && size > 4999880) {
      root.on("in", { "@": id, err: "localStorage max!" });
      return;
    }
    //if(!msg['@']){ acks.push(id) } // then ack any non-ack write. // TODO: use batch id.
    if (!msg["@"] && (!msg._.via || Math.random() < ok["@"] / ok["/"])) {
      acks.push(id);
    } // then ack any non-ack write. // TODO: use batch id.
    if (to) {
      return;
    }
    to = setTimeout(flush, 9 + size / 333); // 0.1MB = 0.3s, 5MB = 15s
  });
  function flush() {
    if (!acks.length && ((setTimeout.turn || "").s || "").length) {
      setTimeout(flush, 99);
      return;
    } // defer if "busy" && no saves.
    var err,
      ack = acks;
    clearTimeout(to);
    to = false;
    acks = [];
    json(disk, function (err, tmp) {
      try {
        !err && store.setItem(opt.prefix, tmp);
      } catch (e) {
        err = stop = e || "localStorage failure";
      }
      if (err) {
        Zen.log(
          err +
            " Consider using ZEN's IndexedDB plugin for RAD for more storage space, https://zen.eco/docs/RAD#install",
        );
        root.on("localStorage:error", { err: err, get: opt.prefix, put: disk });
      }
      size = (tmp || "").length;

      //if(!err && !Object.empty(opt.peers)){ return } // only ack if there are no peers. // Switch this to probabilistic mode
      setTimeout.each(
        ack,
        function (id) {
          root.on("in", { "@": id, err: err, ok: 0 }); // localStorage isn't reliable, so make its `ok` code be a low number.
        },
        0,
        99,
      );
    });
  }
});
