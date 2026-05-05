import "./yson.js";
import "./store.js";
import "./rfs.js";
import "./rs3.js";
import "./wire.js";
import "./axe.js";
import "./dht.js";
import "./multicast.js";
import "./stats.js";
import ZEN from "../zen.js";
import serve from "./serve.js";

var Zen = ZEN;
var u;
Zen.serve = serve;
Zen.on("opt", function (root) {
  if (u === root.opt.super) {
    root.opt.super = true;
  }
  if (u === root.opt.faith) {
    root.opt.faith = true;
  }
  root.opt.log = root.opt.log || Zen.log;
  this.to.next(root);
});

export default ZEN;
