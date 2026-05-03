import fs from "fs";
import path from "path";
import zlib from "zlib";
import { fileURLToPath } from "node:url";
import { dirname as __dirnameOf } from "node:path";
const __filename = fileURLToPath(import.meta.url);
const __dirname = __dirnameOf(__filename);
var dot = /\.\.+/g;
var slash = /\/\/+/g;

function gz(req) {
  return (req.headers && req.headers["accept-encoding"] || "").includes("gzip");
}

function pipe(rs, req, res) {
  if (gz(req)) {
    res.setHeader("Content-Encoding", "gzip");
    rs.pipe(zlib.createGzip()).pipe(res);
  } else {
    rs.pipe(res);
  }
}

function route(req, res, next) {
  var tmp;
  if ((tmp = req.socket) && (tmp = tmp.server) && (tmp = tmp.route)) {
    var url;
    if (
      (tmp = tmp[((req.url || "").slice(1).split("/")[0] || "").split(".")[0]])
    ) {
      try {
        return tmp(req, res, next);
      } catch (e) {
        console.log(req.url + " crashed with " + e);
      }
    }
  }
}

function CDN(dir) {
  return function (req, res) {
    req.url = (req.url || "").replace(dot, "").replace(slash, "/");
    if (serve(req, res)) {
      return;
    } // filters ZEN requests!
    if (req.url.slice(-3) === ".js") {
      res.setHeader("Content-Type", "text/javascript");
    }
    rs.on("error", function (tmp) {
        // static files!
        fs.readFile(path.join(dir, "index.html"), function (err, tmp) {
          try {
            res.writeHead(200, { "Content-Type": "text/html" });
            res.end(tmp + "");
          } catch (e) {} // or default to index
        });
      });
    pipe(rs, req, res);
  };
}

function serve(req, res, next) {
  var tmp;
  if (typeof req === "string") {
    return CDN(req);
  }
  if (!req || !res) {
    return false;
  }
  next = next || serve;
  if (!req.url) {
    return next();
  }
  if (res.setHeader) {
    res.setHeader("Access-Control-Allow-Origin", "*");
  }
  if (0 <= req.url.indexOf("zen/")) {
    var relayPath = __dirname + "/../" + req.url.split("/").slice(2).join("/");
    if ("/" === relayPath.slice(-1)) {
      fs.readdir(relayPath, function (err, dir) {
        res.end((dir || (err && 404)) + "");
      });
      return true;
    }
    var S = +new Date();
    var rs = fs.createReadStream(relayPath);
    if (req.url.slice(-3) === ".js") {
      res.setHeader("Content-Type", "text/javascript");
    }
    rs.on("open", function () {
      console.STAT && console.STAT(S, +new Date() - S, "serve file open");
      pipe(rs, req, res);
    });
    rs.on("error", function (err) {
      res.end(404 + "");
    });
    rs.on("end", function () {
      console.STAT && console.STAT(S, +new Date() - S, "serve file end");
    });
    return true;
  }
  if (route(req, res, next)) {
    return true;
  }
  var mime = { js: "text/javascript", mjs: "text/javascript", wasm: "application/wasm", html: "text/html", css: "text/css", json: "application/json" };
  var zenRoot = path.resolve(__dirname, "..");
  var filePath = path.resolve(zenRoot, req.url.split("?")[0].slice(1));
  if (filePath === zenRoot || filePath.startsWith(zenRoot + path.sep)) {
    fs.stat(filePath, function (err, stat) {
      if (err) { res.writeHead(404); res.end(); return; }
      var target = stat.isDirectory() ? path.join(filePath, "index.html") : filePath;
      fs.access(target, fs.constants.R_OK, function (err) {
        if (err) { res.writeHead(404); res.end(); return; }
        var ext = path.extname(target).slice(1);
        if (mime[ext]) res.setHeader("Content-Type", mime[ext]);
        var rs = fs.createReadStream(target);
        rs.on("error", function () { try { res.writeHead(404); res.end(); } catch (e) {} });
        pipe(rs, req, res);
      });
    });
    return true;
  }
  return next();
}

export default serve;
