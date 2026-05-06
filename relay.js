import "./lib/crashed.js";
import cluster from "cluster";
import fs from "fs";
import os from "os";
import http from "http";
import https from "https";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import ZEN from "./index.js";
import { resolveEnvPeers } from "./src/bootstrap.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const isMain = !!process.argv[1] && __filename === process.argv[1];

let zen;

if (isMain && cluster.isPrimary) {
  cluster.setupPrimary({ exec: __filename });
  cluster.fork();
  cluster.on("exit", (worker, code) => {
    if (code !== 0) setTimeout(() => cluster.fork(), 1000);
  });
} else if (isMain) {
  const env = process.env;
  const opt = {
    port: env.PORT || process.argv[2] || 8420,
    peers: resolveEnvPeers(env),
  };

  if (fs.existsSync((opt.home = os.homedir()) + "/cert.pem")) {
    env.HTTPS_KEY = env.HTTPS_KEY || opt.home + "/key.pem";
    env.HTTPS_CERT = env.HTTPS_CERT || opt.home + "/cert.pem";
  }

  const cdn = ZEN.serve(join(__dirname, "examples"));

  if (env.HTTPS_KEY) {
    opt.port = 443;
    opt.key = fs.readFileSync(env.HTTPS_KEY);
    opt.cert = fs.readFileSync(env.HTTPS_CERT);
    opt.server = https.createServer(opt, cdn);
    http.createServer((req, res) => {
      res.writeHead(301, { Location: "https://" + req.headers.host + req.url });
      res.end();
    }).listen(80);
  } else {
    opt.server = http.createServer(cdn);
  }

  opt.server.on("error", (err) => {
    console.error("Server error:", err.message);
    process.exit(1);
  });
  zen = new ZEN({ web: opt.server.listen(opt.port), peers: opt.peers });
  console.log("ZEN relay peer started on port " + opt.port + " with /zen");
}

export default zen;
