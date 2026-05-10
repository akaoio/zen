import ZEN from "../zen.js";
import __dgram from "dgram";
import __os from "os";
var Zen = ZEN;

Zen.on("create", function (root) {
  this.to.next(root);
  var opt = root.opt;
  if (false === opt.multicast) {
    return;
  }
  if (
    typeof process !== "undefined" &&
    "false" === "" + (process.env || {}).MULTICAST
  ) {
    return;
  }
  //if(true !== opt.multicast){ return } // disable multicast by default for now.

  var udp = (opt.multicast = opt.multicast || {});
  udp.address = udp.address || "233.255.255.255";
  udp.address6 = udp.address6 || "ff02::1";
  udp.address6site = udp.address6site || "ff05::1";
  udp.pack = udp.pack || 50000; // UDP messages limited to 65KB.
  udp.port = udp.port || 8420;

  var noop = function () {},
    u;
  var pid = "2" + Math.random().toString().slice(-8);
  var mesh = (opt.mesh = opt.mesh || Zen.Mesh(root));
  var dgram;

  try {
    dgram = __dgram;
  } catch (e) {
    return;
  }

  var ifaces = (__os || {}).networkInterfaces ? __os.networkInterfaces() : {};
  var iface4, iface6;
  Object.entries(ifaces).forEach(function (entry) {
    var name = entry[0], addrs = entry[1];
    addrs.forEach(function (addr) {
      if (addr.internal) { return; }
      if ((addr.family === "IPv4" || addr.family === 4) && !iface4) {
        iface4 = addr.address;
      }
      if ((addr.family === "IPv6" || addr.family === 6)
          && addr.address.indexOf("fe80") === 0 && !iface6) {
        iface6 = addr.address + "%" + name; // link-local + zone ID required by libuv
      }
    });
  });

  function setupSocket(type, address, joinIface, peerKey) {
    var ipv6 = type === "udp6";
    var sock = dgram.createSocket({ type: type, reuseAddr: true, ipv6Only: !!ipv6 });
    var peer;
    sock.bind({ port: udp.port, exclusive: true }, function () {
      if (!ipv6) {
        sock.setBroadcast(true);
        sock.setMulticastTTL(128);
      }
    });

    sock.on("listening", function () {
      try {
        sock.addMembership(address, joinIface);
      } catch (e) {
        console.log("Multicast " + type + " unavailable:", e.code || e.message);
        sock.close();
        return;
      }
      peer = { id: address + ":" + udp.port, wire: sock };
      var slot = peerKey || (ipv6 ? "peer6" : "peer");
      udp[slot] = peer;

      peer.say = function (raw) {
        var buf = Buffer.from(raw, "utf8");
        if (udp.pack <= buf.length) { return; }
        sock.send(buf, 0, buf.length, udp.port, address, noop);
      };

      Zen.log.once("multi-" + type + "-" + address, "Multicast " + type + " on " + peer.id);
      return; // below code only needed for when WebSocket connections desired!
      setInterval(function broadcast() {
        port = port || ((opt.web && opt.web.address()) || {}).port;
        if (!port) { return; }
        peer.say(
          JSON.stringify({
            id: opt.pid || (opt.pid = Math.random().toString(36).slice(2)),
            port: port,
          }),
        );
      }, 1000);
    });

    sock.on("message", function (raw, info) {
      try {
        if (!raw) { return; }
        raw = raw.toString("utf8");
        if ("2" === raw[0]) { return check(raw, info); }
        opt.mesh.hear(raw, peer);

        return; // below code only needed for when WebSocket connections desired!
        var message;
        message = JSON.parse(raw.toString("utf8"));

        if (opt.pid === message.id) { return; } // ignore self

        var url =
          "http://" +
          info.address +
          ":" +
          (port || ((opt.web && opt.web.address()) || {}).port) +
          "/zen";
        if (root.opt.peers[url]) { return; }

        //console.log('discovered', url, message, info);
        root.$.opt(url);
      } catch (e) {
        //console.log('multicast error', e, raw);
        return;
      }
    });
  }

  setupSocket("udp4", udp.address, iface4, "peer");
  setupSocket("udp6", udp.address6, iface6, "peer6");
  // Also join the site-local multicast group (ff05::1) which can cross internal routers
  setupSocket("udp6", udp.address6site, iface6, "peer6site");

  function say(msg) {
    this.to.next(msg);
    if (udp.peer) { mesh.say(msg, udp.peer); }
    if (udp.peer6) { mesh.say(msg, udp.peer6); }
    if (udp.peer6site) { mesh.say(msg, udp.peer6site); }
  }

  function check(id, info) {
    var tmp;
    if (!udp.peer && !udp.peer6 && !udp.peer6site) { return; }
    if (!id) {
      id = check.id = check.id || Buffer.from(pid, "utf8");
      if (udp.peer) { udp.peer.wire.send(id, 0, id.length, udp.port, udp.address, noop); }
      if (udp.peer6) { udp.peer6.wire.send(id, 0, id.length, udp.port, udp.address6, noop); }
      if (udp.peer6site) { udp.peer6site.wire.send(id, 0, id.length, udp.port, udp.address6site, noop); }
      return;
    }
    if ((tmp = root.stats) && (tmp = tmp.gap) && info) {
      (tmp.near || (tmp.near = {}))[info.address] = info.port || 1;
    } // STATS!
    if (check.on || id === pid) { return; }
    root.on("out", (check.on = say)); // TODO: MULTICAST NEEDS TO BE CHECKED FOR NEW CODE SYSTEM!!!!!!!!!! // TODO: This approach seems interferes with other relays, below does not but...
    //opt.mesh.hi(udp.peer); //  IS THIS CORRECT?
  }

  setInterval(check, 1000 * 1).unref();
});
