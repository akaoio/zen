import ZEN from "../zen.js";
import __path from "path";
import __os from "os";
import __fs from "fs";
import __child_process from "child_process";
import "./yson.js";
import * as xdg from "./xdg.js";
import { fileURLToPath } from "node:url";
import { dirname as __dirnameOf } from "node:path";
const __filename = fileURLToPath(import.meta.url);
const __dirname = __dirnameOf(__filename);
var Zen = ZEN;

Zen.on("opt", function (root) {
  this.to.next(root);
  if (root.once) {
    return;
  }
  if (typeof process === "undefined") {
    return;
  }
  if (false === root.opt.stats) {
    return;
  }
  var path = __path || {};
  var file = root.opt.file
    ? path.resolve(root.opt.file).split(path.sep).slice(-1)[0]
    : "radata";
  var noop = function () {};
  var os = __os || {};
  var fs = __fs || {};
  var stateDir = process.env.GUN_TEST_TMP === "1"
    ? __path.resolve(__dirname, "..")
    : xdg.ensure(xdg.state());
  var stateFile = function (f) { return __path.join(stateDir, "stats." + f); };
  fs.existsSync = fs.existsSync || path.existsSync;
  if (!fs.existsSync) {
    return;
  }
  if (!process) {
    return;
  }
  process.uptime = process.uptime || noop;
  process.cpuUsage = process.cpuUsage || noop;
  process.memoryUsage = process.memoryUsage || noop;
  os.totalmem = os.totalmem || noop;
  os.freemem = os.freemem || noop;
  os.loadavg = os.loadavg || noop;
  os.cpus = os.cpus || noop;
  var S = +new Date(),
    W;
  var obj_ify = function (o) {
    try {
      o = JSON.parse(o);
    } catch (e) {
      o = {};
    }
    return o;
  };
  setTimeout(function () {
    root.stats =
      obj_ify(
        fs.existsSync(stateFile(file)) &&
          fs.readFileSync(stateFile(file)).toString(),
      ) || {};
    root.stats.up = root.stats.up || {};
    root.stats.up.start = root.stats.up.start || +new Date();
    root.stats.up.count = (root.stats.up.count || 0) + 1;
    root.stats.stay = root.stats.stay || {};
    root.stats.over = +new Date();
  }, 1);
  setInterval(function () {
    if (!root.stats) {
      root.stats = {};
    }
    if (W) {
      return;
    }

    var stats = root.stats,
      tmp;
    stats.over = -(S - (S = +new Date()));
    (stats.up || {}).time = process.uptime();
    stats.memory = process.memoryUsage() || {};
    stats.memory.totalmem = os.totalmem();
    stats.memory.freemem = os.freemem();
    stats.cpu = process.cpuUsage() || {};
    stats.cpu.loadavg = os.loadavg();
    stats.cpu.stack = (((setTimeout || "").turn || "").s || "").length;
    stats.peers = {};

    stats.peers.count =
      console.STAT.peers || Object.keys(root.opt.peers || {}).length; // TODO: .keys( is slow
    stats.node = {};
    stats.node.count = Object.keys(root.graph || {}).length; // TODO: .keys( is slow
    stats.all = all;
    stats.sites = console.STAT.sites;
    all = {}; // will this cause missing stats?
    var dam = root.opt.mesh;
    if (dam) {
      stats.dam = {
        in: { count: dam.hear.c, done: dam.hear.d },
        out: { count: dam.say.c, done: dam.say.d },
      };
      dam.hear.c = dam.hear.d = dam.say.c = dam.say.d = 0; // reset
      stats.peers.time = dam.bye.time || 0;
    }
    var rad = root.opt.store;
    rad = rad && rad.stats;
    if (rad) {
      stats.rad = rad;
      root.opt.store.stats = {
        get: { time: {}, count: 0 },
        put: { time: {}, count: 0 },
      }; // reset
    }
    JSON.stringifyAsync(stats, function (err, raw) {
      if (err) {
        return;
      }
      W = true;
      fs.writeFile(stateFile(file), raw, function (err) {
        W = false;
        err && console.log((console.STAT.err = err));
        console.STAT && console.STAT(S, +new Date() - S, "stats stash");
      });
    });

    //exec("top -b -n 1", function(err, out){ out && fs.writeFile(__dirname+'/../stats.top.'+file, out, noop) }); // was it really seriously actually this?
    //}, 1000 * 15);
  }, 1000 * 5).unref();
});

var exec = __child_process.exec,
  noop = function () {};
var log = Zen.log,
  all = {},
  max = 1000;
console.STAT = function (a, b, c, d) {
  if ("number" == typeof a && "number" == typeof b && "string" == typeof c) {
    var tmp = all[c] || (all[c] = []);
    if (max < tmp.push([a, b])) {
      all[c] = [];
    } // reset
    //return;
  }
  if (!console.LOG || log.off) {
    return a;
  }
  return log.apply(Zen, arguments);
};
