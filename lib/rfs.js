import __path from "path";
import __fs from "fs";
import ZEN from "../zen.js";
import tpath from "./tpath.js";
import * as xdg from "./xdg.js";
function Store(opt) {
  opt = opt || {};
  opt.log = opt.log || console.log;
  var testing = process.env.GUN_TEST_TMP === "1";
  var defaultFile = testing ? "radata" : __path.join(xdg.data(), "radata");
  opt.file = tpath(opt.file, defaultFile, testing);
  var fs = __fs,
    u;

  var store = function Store() {};
  if (Store[opt.file]) {
    console.log("Warning: reusing same fs store and options as 1st.");
    return Store[opt.file];
  }
  Store[opt.file] = store;
  var puts = {};
  function ensureDir() {
    if (!fs.existsSync(opt.file)) {
      fs.mkdirSync(opt.file, { recursive: true });
    }
  }

  // TODO!!! ADD ZLIB INFLATE / DEFLATE COMPRESSION!
  store.put = function (file, data, cb) {
    // Reject immediately when the store is in degraded (disk-full) mode.
    if (store.degraded) {
      return cb(new Error("ZEN storage is full (ENOSPC) — write rejected"));
    }
    ensureDir();
    var random = Math.random().toString(36).slice(-3);
    puts[file] = { id: random, data: data };
    var dir = __path.dirname(opt.file),
      base = __path.basename(opt.file);
    var tmp = __path.join(
      "." === dir ? "" : dir,
      base + "-" + file + "-" + random + ".tmp",
    );
    fs.writeFile(tmp, data, function (err, ok) {
      if (err) {
        if (random === (puts[file] || "").id) {
          delete puts[file];
        }
        if (err.code === "ENOSPC") {
          store._markFull(err);
        }
        return cb(err);
      }
      move(tmp, opt.file + "/" + file, function (err, ok) {
        if (random === (puts[file] || "").id) {
          delete puts[file];
        }
        if (err && err.code === "ENOSPC") {
          store._markFull(err);
        }
        cb(err, ok || !err);
      });
    });
  };

  store._markFull = function (err) {
    if (store.degraded) { return; }
    store.degraded = true;
    opt.log("ZEN STORAGE FULL (ENOSPC): writes are suspended until space is freed.", opt.file);
    if (Store._onFull) { Store._onFull(err, store); }
  };

  /** Recover from degraded mode after freeing disk space. */
  store.recover = function () {
    store.degraded = false;
    opt.log("ZEN storage recovered — writes resumed.", opt.file);
  };

  /** Returns disk usage statistics for the storage directory.
   *  cb(err, { used, free, total }) — all values in bytes.
   *  Requires Node 19+; calls cb(err) on older runtimes. */
  store.quota = function (cb) {
    if (!fs.statfs) {
      return cb(new Error("fs.statfs not available (requires Node.js 19+)"));
    }
    ensureDir();
    fs.statfs(opt.file, function (err, stats) {
      if (err) { return cb(err); }
      var total = stats.blocks * stats.bsize;
      var free  = stats.bfree  * stats.bsize;
      cb(null, { used: total - free, free: free, total: total });
    });
  };

  // Periodic low-disk-space warning (Node 19+ only; no-op on older runtimes).
  // opt.fmb sets the free-MB threshold (default 200 MB); opt.evict=false disables.
  if (fs.statfs && opt.evict !== false) {
    var minFreeBytes = (opt.fmb !== undefined ? opt.fmb : 200) * 1024 * 1024;
    var spaceTimer = setInterval(function () {
      fs.statfs(opt.file, function (err, stats) {
        if (err) { return; }
        var freeBytes = stats.bfree * stats.bsize;
        if (freeBytes < minFreeBytes) {
          opt.log(
            "ZEN WARNING: only",
            Math.round(freeBytes / 1024 / 1024) + "MB",
            "free in", opt.file,
            "— consider freeing disk space."
          );
          if (Store._onLow) { Store._onLow({ freeBytes: freeBytes, path: opt.file }); }
        }
      });
    }, 60 * 1000);
    spaceTimer.unref && spaceTimer.unref();
  }
  store.get = function (file, cb) {
    var tmp; // this took 3s+?
    if ((tmp = puts[file])) {
      cb(u, tmp.data);
      return;
    }
    if (!fs.existsSync(opt.file)) {
      cb();
      return;
    }
    fs.readFile(opt.file + "/" + file, function (err, data) {
      if (err) {
        if ("ENOENT" === (err.code || "").toUpperCase()) {
          return cb();
        }
        opt.log("ERROR:", err);
      }
      cb(err, data);
    });
  };

  ensureDir();

  function move(oldPath, newPath, cb) {
    var retry = 0,
      max = 12;
    function again() {
      fs.rename(oldPath, newPath, function (err) {
        if (!err) {
          return cb();
        }
        if (err.code === "EXDEV") {
          var readStream = fs.createReadStream(oldPath);
          var writeStream = fs.createWriteStream(newPath);

          readStream.on("error", cb);
          writeStream.on("error", cb);

          readStream.on("close", function () {
            fs.unlink(oldPath, cb);
          });

          readStream.pipe(writeStream);
          return;
        }
        if (
          "EEXIST" === err.code ||
          "EPERM" === err.code ||
          "EACCES" === err.code ||
          "EBUSY" === err.code
        ) {
          return fs.unlink(newPath, function (unlinkErr) {
            if (unlinkErr && "ENOENT" !== unlinkErr.code) {
              if (++retry > max) {
                return cb(err);
              }
              return setTimeout(again, 10 * retry);
            }
            if (++retry > max) {
              return cb(err);
            }
            setTimeout(again, 5 * retry);
          });
        }
        cb(err);
      });
    }
    again();
  }

  store.list = function (cb, match, params, cbs) {
    ensureDir();
    var dir;
    try {
      dir = fs.readdirSync(opt.file);
    } catch (err) {
      if ("ENOENT" === (err.code || "").toUpperCase()) {
        cb();
        return;
      }
      throw err;
    }
    dir.forEach(function (file) {
      cb(file);
    });
    cb();
  };

  return store;
}

var Zen = ZEN;
Zen.on("create", function (root) {
  var opt = root.opt;
  if (opt.rfs === false) {
    this.to.next(root);
    return;
  }
  opt.store = opt.store || ((!Zen.window || opt.rfs === true) && Store(opt));
  this.to.next(root);
});

export default Store;
