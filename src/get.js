import __root from "./root.js";

var Zen = __root;

Zen.chain.get = function (key, cb, as) {
  var zen, tmp;
  if (typeof key === "string") {
    if (key.length == 0) {
      (zen = this.chain())._.err = { err: Zen.log("0 length key!", key) };
      if (cb) {
        cb.call(zen, zen._.err);
      }
      return zen;
    }
    var back = this,
      cat = back._;
    var next = cat.next || empty;
    if (
      back === cat.root.$ &&
      key.indexOf("/") >= 0 &&
      !cat.root._sl &&
      !cat.root.graph[key]
    ) {
      var parts = key.split("/"),
        i = 0,
        cur = back._,
        ok = 1;
      while (i < parts.length) {
        if (!(cur.next || {})[parts[i]]) {
          ok = 0;
          break;
        }
        cur = cur.next[parts[i++]].$._;
      }
      if (ok) {
        var nav = back;
        i = 0;
        while (i < parts.length) {
          nav = nav.get(parts[i++]);
        }
        return nav;
      }
    }
    if (!(zen = next[key])) {
      zen = key && cache(key, back);
    }
    zen = zen && zen.$;
  } else if ("function" == typeof key) {
    if (true === cb) {
      return (soul(this, key, cb, as), this);
    }
    zen = this;
    var cat = zen._,
      opt = cb || {},
      root = cat.root,
      id;
    opt.at = cat;
    opt.ok = key;
    var wait = {}, // what each node has folded into the open batch
      told = {}; // and which of those values it has already been handed
    // can we assign this to the at instead, like in once?
    //var path = []; cat.$.back(at => { at.get && path.push(at.get.slice(0,9))}); path = path.reverse().join('.');
    function any(msg, eve, f) {
      if (any.stun) {
        return;
      }
      if ((tmp = root.pass) && !tmp[id]) {
        return;
      }
      var at = msg.$._,
        sat = (msg.$$ || "")._,
        data = (sat || at).put,
        odd = !at.has && !at.soul,
        test = {},
        link,
        tmp;
      if (odd || u === data) {
        // handles non-core
        data =
          u === ((tmp = msg.put) || "")["="]
            ? u === (tmp || "")[":"]
              ? tmp
              : tmp[":"]
            : tmp["="];
      }
      if ((link = "string" == typeof (tmp = Zen.valid(data)))) {
        data = u === (tmp = root.$.get(tmp)._.put) ? (opt.not ? u : data) : tmp;
      }
      if (opt.not && u === data) {
        return;
      }
      if (u === opt.stun) {
        if ((tmp = root.stun) && tmp.on) {
          cat.$.back(function (a) {
            // our chain stunned?
            tmp.on("" + a.id, (test = {}));
            if ((test.run || 0) < any.id) {
              return test;
            } // if there is an earlier stun on gapless parents/self.
          });
          !test.run && tmp.on("" + at.id, (test = {})); // this node stunned?
          !test.run && sat && tmp.on("" + sat.id, (test = {})); // linked node stunned?
          if (any.id > test.run) {
            if (!test.stun || test.stun.end) {
              test.stun = tmp.on("stun");
              test.stun = test.stun && test.stun.last;
            }
            if (test.stun && !test.stun.end) {
              //if(odd && u === data){ return }
              //if(u === msg.put){ return } // "not found" acks will be found if there is stun, so ignore these.
              (test.stun.add || (test.stun.add = {}))[id] = function () {
                any(msg, eve, 1);
              }; // add ourself to the stun callback list that is called at end of the write.
              return;
            }
          }
        }
        if (/*odd &&*/ u === data) {
          f = 0;
        } // if data not found, keep waiting/trying.
        /*if(f && u === data){
					cat.on('out', opt.out);
					return;
				}*/
        if ((tmp = root.hatch) && !tmp.end && u === opt.hatch && !f) {
          // quick hack! // Because data is streamed we get things one by one, but callers would rather be called once per batch than once per piece, so the first delivery for a node parks on the batch and later ones fold into it, to be handed out together when the batch ends.
          var node = at.$._.id,
            held = wait[node];
          if (held) {
            // Folding is right while these say the same thing, and they usually
            // do: one put is carried out in several passes and stamps every
            // field it writes with a single state. A different state is a
            // different write, though, and the delivery already queued cannot
            // speak for the older one -- it carries no value of its own, it
            // reads the node when the batch ends, and by then this newer write
            // has replaced what it would have said. That is how a node written
            // twice inside one batch used to lose its first value with nobody
            // noticing: every message carrying it was folded away as a repeat.
            // So hand the older value over now, from the copy taken when it was
            // queued, which is the only one left of it.
            if ((tmp = stamp(data) || stamp(msg.put))) {
              if (tmp !== held.when) {
                // The copy on its own is not enough to hand over: a node
                // arrives a field at a time, so it can be half-built, and a
                // caller that keeps the first thing it is given would take the
                // half for the whole. It does not have to be, though -- every
                // field this write did not touch is still standing in the node.
                // So put the older value back together: the node as it is now,
                // with the fields this write replaced taken from the copy.
                if (!opt.v2020 && told[node] !== held.when) {
                  // A field's value is a value, not a node: there is nothing to
                  // put back together, the copy is simply what it said before.
                  var back =
                    data && "object" == typeof data
                      ? asof(data, held.snap, held.when)
                      : held.snap;
                  if (u !== back && back !== data) {
                    told[node] = held.when;
                    opt.ok.call(at.$, back, at.get, msg, eve || any);
                  }
                }
                held.when = tmp;
              }
              // Keep the copy current within this write. A node arrives a field
              // at a time and every piece carries the same state, so the first
              // message for it is usually a half-built one -- holding on to that
              // would mean handing over a value with the interesting part still
              // missing.
              held.snap = snapshot(data);
            }
            return;
          }
          wait[node] = held = {
            when: stamp(data) || stamp(msg.put),
            snap: snapshot(data),
          };
          tmp.push(function () {
            // What this was queued to say may have been said already, if the
            // node moved on while the batch was still going.
            var now = (msg.$$ || msg.$ || "")._;
            now = stamp(now && now.put);
            if (now && told[node] === now) {
              return;
            }
            any(msg, eve, 1);
          });
          return;
        }
        wait = {}; // end quick hack.
      }
      // call:
      if (root.pass) {
        if (root.pass[id + at.id]) {
          return;
        }
        // A read through a link delivers twice in the same pass: the pointer
        // first, then the node it points at once that has loaded. Claiming the
        // pass slot for the pointer makes the second delivery look like a
        // repeat of the first, and it is dropped -- so the listener is left
        // holding `{"#": soul}` and the node it asked for never arrives at
        // all. Handing over a pointer is not an answer; do not spend the slot
        // on it.
        if ("string" != typeof Zen.valid(data)) {
          root.pass[id + at.id] = 1;
        }
      }
      if (opt.v2020) {
        opt.ok(msg, eve || any);
        return;
      }
      opt.ok.call(at.$, data, at.get, msg, eve || any);
    }
    any.at = cat;
    //(cat.any||(cat.any=function(msg){ setTimeout.each(Object.keys(cat.any||''), function(act){ (act = cat.any[act]) && act(msg) },0,99) }))[id = String.random(7)] = any; // maybe switch to this in future?
    (cat.any || (cat.any = {}))[(id = String.random(7))] = any;
    any.off = function () {
      any.stun = 1;
      if (!cat.any) {
        return;
      }
      delete cat.any[id];
    };
    any.rid = rid; // logic from old version, can we clean it up now?
    any.id = opt.run || ++root.once; // used in callback to check if we are earlier than a write. // will this ever cause an integer overflow?
    tmp = root.pass;
    (root.pass = {})[id] = 1; // Explanation: test trade-offs want to prevent recursion so we add/remove pass flag as it gets fulfilled to not repeat, however map map needs many pass flags - how do we reconcile?
    cat.repass = 1; // A new listener needs link() to re-link this chain to what
    // it points at. root.pass only says so for as long as the out below stays
    // synchronous, and it does not always, so say it on the chain instead --
    // otherwise the re-link is skipped and no second delivery ever arrives.
    opt.out = opt.out || { get: {} };
    cat.on("out", opt.out);
    root.pass = tmp;
    return zen;
  } else if ("number" == typeof key) {
    return this.get("" + key, cb, as);
  } else if ("string" == typeof (tmp = valid(key))) {
    return this.get(tmp, cb, as);
  } else if ((tmp = this.get.next)) {
    zen = tmp(this, key);
  }
  if (!zen) {
    (zen = this.chain())._.err = { err: Zen.log("Invalid get request!", key) }; // CLEAN UP
    if (cb) {
      cb.call(zen, zen._.err);
    }
    return zen;
  }
  if (cb && "function" == typeof cb) {
    zen.get(cb, as);
  }
  return zen;
};
function cache(key, back) {
  var cat = back._,
    next = cat.next,
    zen = back.chain(),
    at = zen._;
  if (!next) {
    next = cat.next = {};
  }
  next[(at.get = key)] = at;
  if (back === cat.root.$) {
    at.soul = key;
    //at.put = {};
  } else if (cat.soul || cat.has) {
    at.has = key;
    //if(obj_has(cat.put, key)){
    //at.put = cat.put[key];
    //}
  }
  return at;
}
function soul(zen, cb, opt, as) {
  var cat = zen._,
    acks = 0,
    tmp;
  if ((tmp = cat.soul || cat.link)) {
    return cb(tmp, as, cat);
  }
  if (cat.jam) {
    return cat.jam.push([cb, as]);
  }
  cat.jam = [[cb, as]];
  zen.get(
    function go(msg, eve) {
      if (
        u === msg.put &&
        !cat.root.opt.super &&
        (tmp = Object.keys(cat.root.opt.peers).length) &&
        ++acks <= tmp
      ) {
        // TODO: super should not be in core code, bring AXE up into core instead to fix? // TODO: .keys( is slow
        return;
      }
      eve.rid(msg);
      var at = ((at = msg.$) && at._) || {},
        i = 0,
        as;
      tmp = cat.jam;
      delete cat.jam; // tmp = cat.jam.splice(0, 100);
      //if(tmp.length){ process.nextTick(function(){ go(msg, eve) }) }
      while ((as = tmp[i++])) {
        //Zen.obj.map(tmp, function(as, cb){
        var cb = as[0],
          id;
        as = as[1];
        cb &&
          cb(
            (id =
              at.link ||
              at.soul ||
              Zen.valid(msg.put) ||
              ((msg.put || {})._ || {})["#"]),
            as,
            msg,
            eve,
          );
      } //);
    },
    { out: { get: { ".": true } }, v2020: true },
  );
  return zen;
}
function rid(at) {
  var cat = this.at || this.on;
  if (!at || cat.soul || cat.has) {
    return this.off();
  }
  if (!(at = (at = (at = at.$ || at)._ || at).id)) {
    return;
  }
  var map = cat.map,
    tmp,
    seen;
  //if(!map || !(tmp = map[at]) || !(tmp = tmp.at)){ return }
  if ((tmp = (seen = this.seen || (this.seen = {}))[at])) {
    return true;
  }
  seen[at] = true;
  //tmp.echo[cat.id] = {}; // TODO: Warning: This unsubscribes ALL of this chain's listeners from this link, not just the one callback event.
  //obj.del(map, at); // TODO: Warning: This unsubscribes ALL of this chain's listeners from this link, not just the one callback event.
  return;
}
// The node as it stood before this write, or nothing if that cannot be told.
//
// Fields this write did not touch are still in the node itself, so they are
// taken from there and are whole. Fields it replaced are gone, and the only
// record left of them is the copy kept while they were being folded away -- if
// the copy does not have one either, then nothing was lost that can be given
// back, and there is nothing to hand over.
function asof(now, was, when) {
  var ns = ((now || "")._ || "")[">"],
    ws = ((was || "")._ || "")[">"] || "",
    out = {},
    at = {},
    lost = 0,
    k;
  if (!ns) {
    return;
  }
  for (k in ns) {
    if (!(ns[k] > when)) {
      out[k] = now[k];
      at[k] = ns[k];
      continue;
    } // this write left it alone, so the node still has it
    if (u === ws[k]) {
      // This write touched a field the copy knows nothing about. Either it
      // added one that did not exist, or it replaced one whose value had not
      // reached this listener yet -- a node arrives a field at a time, so the
      // copy runs behind. From here those look the same, and guessing wrong
      // means handing over a node with a field missing, which a caller that
      // keeps the first thing it is given takes for the whole truth. Say
      // nothing rather than say half.
      return;
    }
    out[k] = was[k]; // replaced: give back what the copy kept
    at[k] = ws[k];
    lost = 1;
  }
  if (!lost) {
    return;
  } // nothing was taken away, so nothing to hand back
  out._ = { "#": ((now || "")._ || "")["#"], ">": at };
  return out;
}
// A copy of a value as it stands now. Cached nodes are mutated in place by
// later writes, so holding one by reference holds nothing -- by the time it is
// read it says whatever the newest write left behind.
function snapshot(d) {
  if (!d || "object" != typeof d) {
    return d;
  }
  var o = {},
    k,
    s = d._,
    g,
    gs;
  for (k in d) {
    if ("_" !== k) {
      o[k] = d[k];
    }
  }
  if (s) {
    g = {};
    gs = s[">"] || "";
    for (k in gs) {
      g[k] = gs[k];
    }
    o._ = { "#": s["#"], ">": g };
  }
  return o;
}
// The state a write stamped on a value. One put stamps every field it writes
// with the same state, so this stays put while a single write is still being
// assembled, and moves as soon as a later write touches the node.
function stamp(d) {
  if (!d || "object" != typeof d) {
    return 0;
  }
  var s = d[">"],
    m = 0,
    k;
  if ("number" == typeof s) {
    return s;
  }
  s = (d._ || "")[">"];
  if (!s) {
    return 0;
  }
  for (k in s) {
    if (s[k] > m) {
      m = s[k];
    }
  }
  return m;
}
var empty = {},
  valid = Zen.valid,
  u;
