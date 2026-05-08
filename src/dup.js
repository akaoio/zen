import "./shim.js";

function Dup(opt) {
  var dup = { s: new Map() },
    s = dup.s;
  opt = opt || { max: 999, age: 1000 * 9 }; //*/ 1000 * 9 * 3};
  dup.check = function (id) {
    if (!s.has(id)) {
      return false;
    }
    return dt(id);
  };
  var dt = (dup.track = function (id) {
    var it = s.get(id);
    if (!it) {
      // Evict oldest entry when at capacity (Map is insertion-ordered → first key = oldest).
      if (s.size >= opt.max) { s.delete(s.keys().next().value); }
      it = {}; s.set(id, it);
    }
    it.was = dup.now = +new Date();
    if (!dup.to) {
      dup.to = setTimeout(dup.drop, opt.age + 9);
    }
    if (dt.ed) {
      dt.ed(id);
    }
    return it;
  });
  dup.drop = function (age) {
    dup.to = null;
    dup.now = +new Date();
    var l = [...s.keys()];
    console.STAT &&
      console.STAT(dup.now, +new Date() - dup.now, "dup drop keys"); // prev ~20% CPU 7% RAM 300MB // now ~25% CPU 7% RAM 500MB
    setTimeout.each(
      l,
      function (id) {
        var it = s.get(id);
        if (it && (age || opt.age) > dup.now - it.was) {
          return;
        }
        s.delete(id);
      },
      0,
      99,
    );
  };
  return dup;
}

export default Dup;
