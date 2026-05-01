import ZEN from "../zen.js";

let __defaultExport;
{
  function Store(opt) {
    opt = opt || {};
    opt.file = String(opt.file || "radata");
    var store = Store[opt.file],
      dir,
      ready,
      u;

    if (store) {
      console.log("Warning: reusing same OPFS store and options as 1st.");
      return Store[opt.file];
    }
    if (!Store.supported(opt)) {
      console.log("Warning: No OPFS exists to persist data to!");
      return;
    }
    store = Store[opt.file] = function () {};

    store.start = function (cb) {
      ready =
        ready ||
        Store.root(opt).then(function (root) {
          return root.getDirectoryHandle(opt.file, { create: true });
        });
      ready.then(function (handle) {
        dir = handle;
        cb(null, dir);
      }, cb);
    };

    store.put = function (key, data, cb) {
      store.start(function (err, dir) {
        if (err) {
          cb(err);
          return;
        }
        dir
          .getFileHandle("" + key, { create: true })
          .then(function (file) {
            return file.createWritable();
          })
          .then(function (write) {
            return Promise.resolve(write.write(data)).then(function () {
              return write.close();
            });
          })
          .then(
            function () {
              cb(null, 1);
            },
            function (err) {
              cb(err || "put.opfs.error");
            },
          );
      });
    };

    store.get = function (key, cb) {
      store.start(function (err, dir) {
        if (err) {
          cb(err);
          return;
        }
        dir
          .getFileHandle("" + key)
          .then(function (file) {
            return file.getFile();
          })
          .then(function (file) {
            return file.text();
          })
          .then(
            function (data) {
              cb(null, data);
            },
            function (err) {
              if ("NotFoundError" === (err || "").name) {
                cb(null, u);
                return;
              }
              cb(err || "get.opfs.error");
            },
          );
      });
    };

    store.list = function (cb) {
      store.start(function (err, dir) {
        if (err) {
          (opt.log || console.log)("list", err);
          cb();
          return;
        }
        var keys = dir.keys();
        function next() {
          keys.next().then(
            function (step) {
              if (step.done) {
                cb();
                return;
              }
              if (cb(step.value)) {
                return;
              }
              next();
            },
            function (err) {
              (opt.log || console.log)("list", err);
              cb();
            },
          );
        }
        next();
      });
    };

    return store;
  }

  Store.globalThis = Store.globalThis || globalThis;

  Store.storage = function (opt) {
    return (
      (opt || "").opfs || ((Store.globalThis || "").navigator || "").storage
    );
  };

  Store.supported = function (opt) {
    var storage;
    try {
      storage = Store.storage(opt);
    } catch (e) {}
    return !!(storage && storage.getDirectory);
  };

  Store.root = function (opt) {
    var storage = Store.storage(opt);
    return storage.getDirectory.call(storage);
  };

  try {
    __defaultExport = Store;
  } catch (e) {}

  try {
    var Zen = ZEN;
    Zen.on("create", function (root) {
      var store;
      if (false === root.opt.opfs) {
        this.to.next(root);
        return;
      }
      store = root.opt.store || Store(root.opt);
      if (store) {
        root.opt.store = store;
      }
      this.to.next(root);
    });
  } catch (e) {}
}
export default __defaultExport;
