let __defaultExport;
{
  function Radix() {
    var radix = function (key, val, t) {
      radix.unit = 0;
      if (!t && u !== val) {
        radix.last = "" + key < radix.last ? radix.last : "" + key;
        delete (radix.$ || {})[_];
      }
      t = t || radix.$ || (radix.$ = {});
      if (!key && Object.keys(t).length) {
        return t;
      }
      key = "" + key;
      var i = 0,
        l = key.length - 1,
        k = key[i],
        at,
        tmp;
      while (!(at = t[k]) && i < l) {
        k += key[++i];
      }
      if (!at) {
        if (
          !each(t, function (r, s) {
            var ii = 0,
              kk = "";
            if ((s || "").length) {
              while (s[ii] == key[ii]) {
                kk += s[ii++];
              }
            }
            if (kk) {
              if (u === val) {
                if (ii <= l) {
                  return;
                }
                (tmp || (tmp = {}))[s.slice(ii)] = r;
                //(tmp[_] = function $(){ $.sort = Object.keys(tmp).sort(); return $ }()); // get rid of this one, cause it is on read?
                return r;
              }
              var __ = {};
              __[s.slice(ii)] = r;
              ii = key.slice(ii);
              "" === ii ? (__[""] = val) : ((__[ii] = {})[""] = val);
              //(__[_] = function $(){ $.sort = Object.keys(__).sort(); return $ }());
              t[kk] = __;
              if (Radix.debug && "undefined" === "" + kk) {
                console.log(0, kk);
                debugger;
              }
              delete t[s];
              //(t[_] = function $(){ $.sort = Object.keys(t).sort(); return $ }());
              return true;
            }
          })
        ) {
          if (u === val) {
            return;
          }
          (t[k] || (t[k] = {}))[""] = val;
          if (Radix.debug && "undefined" === "" + k) {
            console.log(1, k);
            debugger;
          }
          //(t[_] = function $(){ $.sort = Object.keys(t).sort(); return $ }());
        }
        if (u === val) {
          return tmp;
        }
      } else if (i == l) {
        //if(u === val){ return (u === (tmp = at['']))? at : tmp } // THIS CODE IS CORRECT, below is
        if (u === val) {
          return u === (tmp = at[""]) ? at : (radix.unit = 1) && tmp;
        } // temporary help??
        at[""] = val;
        //(at[_] = function $(){ $.sort = Object.keys(at).sort(); return $ }());
      } else {
        if (u !== val) {
          delete at[_];
        }
        //at && (at[_] = function $(){ $.sort = Object.keys(at).sort(); return $ }());
        var down = radix(key.slice(++i), val, at || (at = {}));
        if (u === val && u === down) {
          // A node can hold two children where one is a prefix of the other --
          // a soul, and the same soul carrying its field separator. The walk
          // above grows the candidate a character at a time, so it always
          // takes the shorter one, and everything under the longer sibling is
          // invisible from here: on disk, acknowledged, and unreadable. Such a
          // node should never be built, but one that already exists must still
          // answer, so try the other children that prefix this key before
          // reporting it missing.
          var other = each(t, function (r, s) {
            if (!s || s === k || _ === s || 0 !== key.indexOf(s)) {
              return;
            }
            var deeper = radix(key.slice(s.length), val, r);
            if (u !== deeper) {
              return deeper;
            }
          });
          if (u !== other) {
            return other;
          }
        }
        return down;
      }
    };
    return radix;
  }

  Radix.map = function rap(radix, cb, opt, pre) {
    try {
      pre = pre || []; // TODO: BUG: most out-of-memory crashes come from here.
      var t = "function" == typeof radix ? radix.$ || {} : radix;
      //!opt && console.log("WHAT IS T?", JSON.stringify(t).length);
      if (!t) {
        return;
      }
      if ("string" == typeof t) {
        if (Radix.debug) {
          throw ["BUG:", radix, cb, opt, pre];
        }
        return;
      }
      if ("number" == typeof t || "boolean" == typeof t) {
        return;
      }
      var keys =
          (t[_] || no).sort ||
          (t[_] = (function $() {
            $.sort = Object.keys(t).sort();
            return $;
          })()).sort,
        rev; // ONLY 17% of ops are pre-sorted!
      //var keys = Object.keys(t).sort();
      opt = true === opt ? { branch: true } : opt || {};
      if ((rev = opt.reverse)) {
        keys = keys.slice(0).reverse();
      }
      var start = opt.start,
        end = opt.end,
        END = "\uffff";
      var i = 0,
        l = keys.length;
      for (; i < l; i++) {
        var key = keys[i],
          tree = t[key],
          tmp,
          p,
          pt;
        if (!tree || "" === key || _ === key || "undefined" === key) {
          continue;
        }
        p = pre.slice(0);
        p.push(key);
        pt = p.join("");
        if (u !== start && pt < (start || "").slice(0, pt.length)) {
          continue;
        }
        if (u !== end && (end || END) < pt) {
          continue;
        }
        if (rev) {
          // children must be checked first when going in reverse.
          tmp = rap(tree, cb, opt, p);
          if (u !== tmp) {
            return tmp;
          }
        }
        if (u !== (tmp = tree[""])) {
          var yes = 1;
          if (u !== start && pt < (start || "")) {
            yes = 0;
          }
          if (u !== end && pt > (end || END)) {
            yes = 0;
          }
          if (yes) {
            tmp = cb(tmp, pt, key, pre);
            if (u !== tmp) {
              return tmp;
            }
          }
        } else if (opt.branch) {
          tmp = cb(u, pt, key, pre);
          if (u !== tmp) {
            return tmp;
          }
        }
        pre = p;
        if (!rev) {
          tmp = rap(tree, cb, opt, pre);
          if (u !== tmp) {
            return tmp;
          }
        }
        pre.pop();
      }
    } catch (e) {
      console.error(e);
    }
  };

  try {
    __defaultExport = Radix;
  } catch (e) {}
  var each = (Radix.object = function (o, f, r) {
      for (var k in o) {
        if (!o.hasOwnProperty(k)) {
          continue;
        }
        if ((r = f(o[k], k)) !== u) {
          return r;
        }
      }
    }),
    no = {},
    u;
  var _ = String.fromCharCode(24);
}
export default __defaultExport;
