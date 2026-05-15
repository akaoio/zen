import ZEN from "../zen.js";
var Zen = ZEN;

// WebRTC browser-to-browser transport for ZEN mesh.
// Signaling travels via dam:"rtc" messages through the relay WS connection.
// Once the DataChannel opens the relay is bypassed for that peer pair.
Zen.on("opt", function (root) {
  this.to.next(root);
  var opt = root.opt;
  if (root.once) return;
  if (!Zen.window) return; // browser only
  if (false === opt.RTCPeerConnection) return;

  var env = (typeof globalThis !== "undefined" ? globalThis : {});
  var RTCPC = opt.RTCPeerConnection || env.RTCPeerConnection || env.webkitRTCPeerConnection;
  var RTCIC = opt.RTCIceCandidate  || env.RTCIceCandidate  || env.webkitRTCIceCandidate;
  if (!RTCPC || !RTCIC) return;

  opt.RTCPeerConnection = RTCPC;
  opt.rtc = opt.rtc || {};

  // ICE: 3 STUN + 3 TURN tiers (free Open Relay Project as default)
  opt.rtc.ice = opt.rtc.ice || [
    { urls: "stun:stun.l.google.com:19302" },
    { urls: "stun:stun1.l.google.com:19302" },
    { urls: "stun:stun.cloudflare.com:3478" },
    { urls: "turn:openrelay.metered.ca:80",         username: "openrelayproject", credential: "openrelayproject" },
    { urls: "turn:openrelay.metered.ca:443",         username: "openrelayproject", credential: "openrelayproject" },
    { urls: "turns:openrelay.metered.ca:443",        username: "openrelayproject", credential: "openrelayproject" },
  ];
  opt.rtc.dc  = opt.rtc.dc  || { ordered: false, maxRetransmits: 3 };
  opt.rtc.max = opt.rtc.max || 55; // Chrome ~256 hard limit, 55 is webtorrent convention

  var mesh = (opt.mesh = opt.mesh || Zen.Mesh(root));
  var pcs  = {}; // pid → RTCPeerConnection (pending, before DataChannel opens)

  // ── send a rtc DAM signal message, targeted to toPid ─────────────────────
  function sig(payload, toPid) {
    var msg = { "#": String.random(9), dam: "rtc", ok: { rtc: payload } };
    if (toPid) msg.ok.rtc.to = toPid;
    // send directly if already wired, else broadcast via relay (relay routes by .to)
    var peer = opt.peers[toPid];
    if (peer && peer.wire) {
      mesh.say(msg, peer);
    } else {
      root.on("out", msg);
    }
  }

  // ── create an RTCPeerConnection for a remote pid ──────────────────────────
  function mkpc(pid, answering) {
    if (pcs[pid]) return pcs[pid];
    var pc = pcs[pid] = new RTCPC({ iceServers: opt.rtc.ice });
    var dc;

    if (!answering) {
      dc = pc.createDataChannel("zen", opt.rtc.dc);
      wiredc(dc, pc, pid);
    }
    pc.ondatachannel = function (e) { wiredc(e.channel, pc, pid); };

    pc.onicecandidate = function (e) {
      if (e.candidate) sig({ candidate: e.candidate, id: opt.pid }, pid);
    };
    pc.onconnectionstatechange = function () {
      var s = pc.connectionState;
      if (s === "failed" || s === "closed" || s === "disconnected") drop(pid);
    };
    pc.oniceconnectionstatechange = function () {
      if (pc.iceConnectionState === "failed" && pc.restartIce) pc.restartIce();
    };

    if (!answering) {
      pc.createOffer()
        .then(function (o) { return pc.setLocalDescription(o).then(function () { return o; }); })
        .then(function (o) { sig({ offer: o, id: opt.pid }, pid); })
        .catch(function (e) { Zen.log("RTC offer err", e); drop(pid); });
    }
    return pc;
  }

  // ── wire a DataChannel as a ZEN mesh peer ─────────────────────────────────
  function wiredc(dc, pc, pid) {
    dc.onopen = function () {
      delete pcs[pid];
      pc.id  = pid;
      // Check readyState before each send; call drop() on failure so the dead
      // peer is removed from opt.peers and future sends fall back to relay.
      pc.say = function (raw) {
        if (dc.readyState !== "open") { drop(pid); return; }
        try { dc.send(raw); } catch (e) { drop(pid); }
      };
      pc.wire = dc;
      mesh.hi(pc);
      Zen.log("RTC ↔ " + pid.slice(0, 7) + " (direct)");
    };
    dc.onclose  = function () { drop(pid); };
    dc.onerror  = function () { drop(pid); };
    dc.onmessage = function (e) { mesh.hear(e.data || e, pc); };
  }

  // Clean up both pending (pcs) and wired (opt.peers) entries for a pid.
  // Called from connection-state changes, dc.onclose, dc.onerror, and pc.say
  // failures — ensures dead DataChannels are removed so traffic falls back
  // to the relay WebSocket automatically.
  function drop(pid) {
    var pending = pcs[pid];
    if (pending) { delete pcs[pid]; try { pending.close(); } catch (e) {} }
    var wired = opt.peers[pid];
    if (wired) mesh.bye(wired);
  }

  // ── incoming rtc DAM messages ─────────────────────────────────────────────
  mesh.hear["rtc"] = function (msg) {
    if (!msg.ok || !msg.ok.rtc) return;
    var rtc = msg.ok.rtc;
    if (!rtc.id || rtc.id === opt.pid) return;
    if (rtc.to && rtc.to !== opt.pid) return; // not addressed to us

    var pid = rtc.id, pc, sdp;

    if (rtc.candidate) {
      pc = pcs[pid] || opt.peers[pid];
      if (pc && pc.addIceCandidate) pc.addIceCandidate(new RTCIC(rtc.candidate)).catch(function () {});
      return;
    }
    if (rtc.answer) {
      pc = pcs[pid]; if (!pc) return;
      sdp = rtc.answer.sdp.replace(/\\r\\n/g, "\r\n");
      pc.setRemoteDescription({ type: "answer", sdp: sdp }).catch(function (e) { Zen.log("RTC ans err", e); });
      return;
    }
    if (rtc.offer) {
      pc = mkpc(pid, true); // responder — no createOffer, just answer
      sdp = rtc.offer.sdp.replace(/\\r\\n/g, "\r\n");
      pc.setRemoteDescription({ type: "offer", sdp: sdp })
        .then(function () { return pc.createAnswer(); })
        .then(function (a) { return pc.setLocalDescription(a).then(function () { return a; }); })
        .then(function (a) { sig({ answer: a, id: opt.pid }, pid); })
        .catch(function (e) { Zen.log("RTC ans err", e); drop(pid); });
    }
  };

  // ── initiate WebRTC to a known browser pid ────────────────────────────────
  // Lower pid always initiates to prevent simultaneous double-offer
  opt.rtc.connect = function (pid) {
    if (!pid || pid === opt.pid) return;
    if (pcs[pid] || opt.peers[pid]) return; // already pending or wired
    if (opt.pid >= pid) return;             // other side will initiate
    mkpc(pid, false);
  };

  // ── extend pex handler: trigger WebRTC when relay sends browser pids ──────
  var _pex = mesh.hear["pex"];
  mesh.hear["pex"] = function (msg, from) {
    if (_pex) _pex(msg, from);
    if (Array.isArray(msg.bpids)) {
      msg.bpids.forEach(function (pid) { opt.rtc.connect(pid); });
    }
  };

  Zen.log.once("RTC", "WebRTC P2P enabled (STUN+TURN)");
});
