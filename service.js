import __child_process from "child_process";
import __fs from "fs";
import __path from "path";
import { fileURLToPath } from "node:url";
import { dirname as __dirnameOf } from "node:path";
import * as xdg from "./xdg.js";
const __filename = fileURLToPath(import.meta.url);
const __dirname = __dirnameOf(__filename);

export default function (root) {
  var mesh = root.opt.mesh,
    cmd = {},
    run = __child_process.exec,
    fs = __fs,
    cfgDir = xdg.config(),
    scriptDir = __path.resolve(__dirname, "../script");
  mesh.hear["service"] = function (msg, peer) {
    if (!fs.existsSync("/etc/systemd/system/zen.service")) {
      mesh.say({ dam: "!", err: "Not serviced." });
      return;
    }
    try {
      (cmd[msg.try] || cmd.any)(msg, peer);
    } catch (err) {
      mesh.say({ dam: "!", err: "service error: " + err });
    }
  };
  cmd.update = function (msg, peer) {
    var log, pass;
    try {
      pass = ("" + fs.readFileSync(__path.join(cfgDir, "pass"))).trim();
    } catch (e) {}
    if (!pass || (msg.pass || "").trim() != pass) {
      return;
    }
    root.stats = root.stats || {};
    root.stats.stay = root.stats.stay || {};
    root.stats.stay.updated = +new Date();
    run(
      "bash " + scriptDir + "/install.sh",
      { env: { VERSION: msg.version || "" } },
      function (e, out, err) {
        log = e + "|" + out + "|" + err;
        mesh.say({ dam: "!", log: "" + log }, peer);
        setTimeout(function () {
          process.exit();
        }, 999);
      },
    );
  };
  function update() {
    var last;
    if (!fs.existsSync(__path.join(cfgDir, "cert.pem"))) {
      return;
    }
    setTimeout(update, 1000 * 60 * 60 * 24);
    if (!root.stats || !root.stats.stay) {
      return;
    }
    last = root.stats.stay.updated || 0;
    if (+new Date() - last < 1000 * 60 * 60 * 24 * 15) {
      return;
    }
    root.stats.stay.updated = +new Date();
    run("bash " + scriptDir + "/install.sh", {}, function () {});
  }
  update();

  cmd.any = function () {};
}
