import "../lib/yson.js";
import zenbase from "../zen.js";
import "../lib/store.js";
import "../lib/rfs.js";
import "./rad/rad.js";
import fs from "fs";
import fsrm from "../lib/fsrm.js";
var ZEN;
{
  var W = function (o) {
    o = o || {};
    if (o.peers === undefined) {
      o.peers = [];
    }
    return new zenbase(o);
  };
  Object.setPrototypeOf(W, zenbase);
  W.prototype = zenbase.prototype;
  Object.defineProperty(W.prototype, "_", {
    get: function () {
      return this._graph._;
    },
    configurable: true,
  });
  W.is = function ($) {
    return $ instanceof zenbase;
  };
  ZEN = W;
}
describe("ZEN", function () {
  var root;
  {
    var env;
    if (typeof global !== "undefined") {
      env = global;
    }
    if (typeof window !== "undefined") {
      env = window;
    }
    root = env.window ? env.window : global;
    try {
      env.window && root.localStorage && root.localStorage.clear();
    } catch (e) {}
    try {
      localStorage.clear();
    } catch (e) {}
    try {
      fs.unlinkSync("tmp/data.json");
    } catch (e) {}
    try {
      fsrm("tmp/radatatest");
    } catch (e) {}
    //root.Zen = root.Zen || load('../zen');
    root.Zen = ZEN;
    root.Zen.TESTING = true;
  }
  var Zen = root.Zen;
  Zen.is = function ($) {
    return $ instanceof ZEN;
  };
  //Zen.log.squelch = true;
  var gleak = {
    globals: {},
    check: function () {
      // via tobyho
      var leaked = [];
      for (var key in gleak.globe) {
        if (!(key in gleak.globals)) {
          leaked.push(key);
        }
      }
      if (leaked.length > 0) {
        console.log("GLOBAL LEAK!", leaked);
        return leaked;
      }
    },
  };
  {
    var gleakEnv = this;
    for (var key in (gleak.globe = gleakEnv)) {
      gleak.globals[key] = true;
    }
  }

  var t = {};

  describe("Utility", function () {
    it("deleting old ZEN tests (may take long time)", function (done) {
      done(); // Mocha doesn't print test until after its done, so show this first.
    });
    it("deleted", function (done) {
      this.timeout(60 * 1000);
      if (!Zen.window) {
        return done();
      }
      indexedDB.deleteDatabase("radatatest").onsuccess = function (e) {
        done();
      };
    });
    var u;
    /* // causes logger to no longer log.
		it('verbose console.log debugging', function(done) {

			var zen = Zen();
			var log = root.console.log, counter = 1;
			root.console.log = function(a,b,c){
				--counter;
				//log(a,b,c);
			}
			Zen.log.verbose = true;
			zen.put('bar', function(err, yay){ // intentionally trigger an error that will get logged.
				expect(counter).to.be(0);

				Zen.log.verbose = false;
				zen.put('bar', function(err, yay){ // intentionally trigger an error that will get logged.
					expect(counter).to.be(0);

					root.console.log = log;
					done();
				});
			});
		} );
		*/

    describe("YSON", function () {
      it("parse", function () {
        //var json = load('fs').readFileSync('./radix.json').toString();
        //var json = load('fs').readFileSync('./data.json').toString();
        //var json = load('fs').readFileSync('./big.json').toString();
        //var json = load('fs').readFileSync('./stats.json').toString();
        //var json = load('fs').readFileSync('./video.json').toString();
      });
      it("backslash", function (done) {
        var o = { z: 'test"wow\\' };
        JSON.stringifyAsync(o, function (err, t) {
          JSON.parseAsync(t, function (err, data) {
            expect(data).to.be.eql(o);
            next();
          });
        });
        function next() {
          JSON.parseAsync('{"webRTCsdp":"v=0\r\no=-"}', function (err, data) {
            var o = { webRTCsdp: "v=0\r\no=-" };
            expect(data).to.be.eql(o);
            JSON.stringifyAsync(o, function (err, t) {
              expect(JSON.parse(t)).to.be.eql(o);
              expect(t).to.be(JSON.stringify(o));
              expect(t).to.be('{"webRTCsdp":"v=0\\r\\no=-"}');
              JSON.parseAsync(t, function (err, d) {
                expect(d).to.be.eql(o);
                done();
              });
            });
          });
        }
      });
      it("stringify", function (done) {
        function Foo() {}
        Foo.prototype.toJSON = function () {};
        //var obj = {"what\"lol": {"a": 1, "b": true, "c": false, "d": null, "wow": [{"z": 9}, true, "hi", 3.3]}};
        var obj = {
          what: {
            a: 1,
            b: true,
            c: false,
            d: null,
            wow: [{ z: 9 }, true, "hi", 3.3],
          },
        };
        var obj = [
          {
            x: "test 😎\\😄🔥",
            z: "test\\",
            'what"lol': { 0: 1.01 },
            a: true,
            b: new Foo(),
            c: 3,
            y: "yes",
            get: { "#": "chat" },
            wow: undefined,
            foo: [1, function () {}, function () {}, "go"],
            blah: {
              a: 5,
              toJSON: function () {
                return 9;
              },
            },
          },
          { webRTCsdp: "v=0\r\no=-" },
          [[]],
          10e9,
          NaN,
        ];
        JSON.stringifyAsync(obj, function (err, text) {
          JSON.parseAsync(text, function (err, data) {
            expect(data).to.be.eql([
              {
                x: "test 😎\\😄🔥",
                z: "test\\",
                'what"lol': { 0: 1.01 },
                a: true,
                c: 3,
                y: "yes",
                get: { "#": "chat" },
                foo: [1, null, null, "go"],
                blah: 9,
              },
              { webRTCsdp: "v=0\r\no=-" },
              [[]],
              10e9,
              null,
            ]);
            var obj = {
              a: [],
              b: [""],
              c: ["", 1],
              d: [1, ""],
              e: { "": [] },
              'a"b': { 0: 1 },
              wow: { "": { cool: 1 } },
            };
            obj.lol = { 0: { sweet: 9 } };
            obj.wat = { "": "cool" };
            obj.oh = { phew: {}, "": {} };
            JSON.stringifyAsync(obj, function (err, text2) {
              JSON.parseAsync(text2, function (err, data) {
                expect(data).to.be.eql(obj);
                done();
              });
            });
          });
        });
      });
    });

    describe("Type Check", function () {
      it("random text", function () {
        expect(String.random().length).to.be(24);
        expect(String.random(11).length).to.be(11);
        expect(String.random(4).length).to.be(4);
        t.tr = String.random(2, "as");
        expect(
          t.tr == "as" || t.tr == "aa" || t.tr == "sa" || t.tr == "ss",
        ).to.be.ok();
      });
      it("match text", function () {
        expect(String.match("user/mark", "user/mark")).to.be.ok();
        expect(
          String.match("user/mark/nadal", { "=": "user/mark" }),
        ).to.not.be.ok();
        expect(String.match("user/mark/nadal", { "*": "user/" })).to.be.ok();
        expect(
          String.match("email/mark@zen.eco", { "*": "user/" }),
        ).to.not.be.ok();
        expect(
          String.match("user/mark/nadal", { ">": "user/j", "<": "user/o" }),
        ).to.be.ok();
        expect(
          String.match("user/timber/nadal", { ">": "user/c", "<": "user/j" }),
        ).to.not.be.ok();
        expect(
          String.match("user/timber/nadal", { ">": "user/m", "<": "user/u" }),
        ).to.be.ok();
        expect(
          String.match("user/mark/nadal", { ">": "user/a", "<": "user/c" }),
        ).to.not.be.ok();
        expect(String.match("mary", { "<": "m" })).to.not.be.ok();
        expect(String.match("mary", { ">": "m" })).to.be.ok();
        expect(String.match("m", { ">": "m" })).to.be.ok(); // lex is inclusive because it evaluates stricter (=) to looser (>) comparisons, see docs.
        expect(String.match("m", { "<": "m" })).to.be.ok(); // lex is inclusive because it evaluates stricter (=) to looser (<) comparisons, see docs.
        return; // below is OLD bloat, still available in lib/match.js
      });
      it("plain object", function () {
        expect(Object.plain({})).to.be(true);
        expect(Object.plain({ a: 1 })).to.be(true);
        expect(Object.plain(u)).to.be(false);
        expect(Object.plain()).to.be(false);
        expect(Object.plain(undefined)).to.be(false);
        expect(Object.plain(null)).to.be(false);
        expect(Object.plain(NaN)).to.be(false);
        expect(Object.plain(0)).to.be(false);
        expect(Object.plain(1)).to.be(false);
        expect(Object.plain("")).to.be(false);
        expect(Object.plain("a")).to.be(false);
        expect(Object.plain([])).to.be(false);
        expect(Object.plain([1])).to.be(false);
        expect(Object.plain(false)).to.be(false);
        expect(Object.plain(true)).to.be(false);
        expect(Object.plain(function () {})).to.be(false);
        expect(Object.plain(new Date())).to.be(false);
        expect(Object.plain(/regex/)).to.be(false);
        this.document &&
          expect(Object.plain(document.createElement("div"))).to.be(false);
        expect(
          Object.plain(
            new (function Class() {
              this.x = 1;
              this.y = 2;
            })(),
          ),
        ).to.be(true);
      });
      it("empty", function () {
        expect(Object.empty()).to.be(true);
        expect(Object.empty({ a: false })).to.be(false);
        expect(Object.empty({ a: false }, ["a"])).to.be(true);
        expect(Object.empty({ a: false }, ["a"])).to.be(true);
        expect(Object.empty({ a: false, b: 1 }, ["a"])).to.be(false);
        expect(Object.empty({ a: false, b: 1 }, ["a"])).to.be(false);
        expect(Object.empty({ a: false, b: 1 }, ["a", "b"])).to.be(true);
        expect(Object.empty({ a: false, b: 1, c: 3 }, ["a", "b"])).to.be(false);
        expect(Object.empty({ 1: 1 }, "danger")).to.be(false);
      });
    });
    describe("Functions", function () {
      /*
			it.skip('sum',function(done){ // deprecate?
				var obj = {a:2, b:2, c:3, d: 9};
				Zen.obj.map(obj, function(num, key){
					setTimeout(this.add(function(){
						this.done(null, num * num);
					}, key), parseInt((""+Math.random()).substring(2,5)));
				}, Zen.fn.sum(function(err, val){
					expect(val.a).to.eql(4);
					expect(val.b).to.eql(4);
					expect(val.c).to.eql(9);
					expect(val.d).to.eql(81);
					done();
				}));
			});
			*/
    });
    describe("Scheduler", function () {
      it("nested poll yields instead of recursing synchronously", function (done) {
        var now = setTimeout.check.now,
          order = [];
        setTimeout.check.now = function () {
          return 0;
        };
        setTimeout.poll(function () {
          order.push("outer-start");
          setTimeout.poll(function () {
            order.push("inner");
          });
          order.push("outer-end");
        });
        expect(order).to.eql(["outer-start", "outer-end"]);
        setTimeout(function () {
          setTimeout.check.now = now;
          expect(order).to.eql(["outer-start", "outer-end", "inner"]);
          done();
        }, 20);
      });
    });
    describe("On", function () {
      it("subscribe", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a) {
          done.first = true;
          expect(a).to.be(1);
          this.to.next(a);
        });
        e.on("foo", function (a) {
          expect(a).to.be(1);
          expect(done.first).to.be.ok();
          done();
        });
        e.on("foo", 1);
      });
      it("unsubscribe", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a) {
          this.off();
          done.first = a;
          expect(a).to.be(1);
          this.to.next(a);
        });
        e.on("foo", function (a) {
          var to = this;
          expect(a).to.be(done.second ? 2 : 1);
          expect(done.first).to.be(1);
          done.second = true;
          if (a === 2) {
            setTimeout(function () {
              expect(e.tag.foo.to === to).to.be.ok();
              done();
            }, 10);
          }
        });
        e.on("foo", 1);
        e.on("foo", 2);
      });
      it("stun", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          if (2 === a) {
            done.first2 = true;
            this.to.next(a);
            return;
          }
          setTimeout(function () {
            expect(done.second).to.not.be.ok();
            expect(done.second2).to.be.ok();
            expect(done.first2).to.be.ok();
            done();
          }, 10);
        });
        e.on("foo", function (a, ev) {
          if (2 === a) {
            done.second2 = true;
          } else {
            done.second = true;
          }
        });
        e.on("foo", 1);
        e.on("foo", 2);
      });
      it("resume", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            expect(done.second).to.not.be.ok();
            to.next(a);
          }, 10);
        });
        e.on("foo", function (a) {
          done.second = true;
          expect(a).to.be(1);
          done();
        });
        e.on("foo", 1);
      });
      it("double resume", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            if (1 === a) {
              done.first1 = true;
              expect(done.second).to.not.be.ok();
            }
            if (2 === a) {
              done.first2 = true;
            }
            to.next(a);
          }, 10);
        });
        e.on("foo", function (a, ev) {
          done.second = true;
          if (1 === a) {
            expect(done.first2).to.not.be.ok();
            done.second1 = true;
          }
          if (2 === a) {
            expect(done.first2).to.be.ok();
            if (done.second1) {
              done();
            }
          }
        });
        e.on("foo", 1);
        e.on("foo", 2);
      });
      it("double resume different event", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            done.first1 = true;
            to.next(a);
          }, 10);
        });
        e.on("foo", function (a) {
          if (1 === a) {
            expect(done.first1).to.be.ok();
            done();
          }
        });
        e.on("foo", 1);
        e.on("bar", 2);
      });
      it("resume params", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            expect(done.second).to.not.be.ok();
            to.next(0);
          }, 10);
        });
        e.on("foo", function (a) {
          done.second = true;
          expect(a).to.be(0);
          done();
        });
        e.on("foo", 1);
      });
      it("map", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          Object.keys(a.it).forEach(function (f) {
            var v = a.it[f];
            setTimeout(function () {
              var emit = { field: "where", soul: f };
              to.next(emit);
            }, 10);
          });
        });
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            to.next({ node: a.soul });
          }, 100);
        });
        e.on("foo", function (a) {
          if ("a" == a.node) {
            done.a = true;
          } else {
            expect(done.a).to.be.ok();
            done();
          }
        });
        e.on("foo", { field: "where", it: { a: 1, b: 2 } });
      });
      it("map synchronous", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a, ev) {
          var to = this.to;
          Object.keys(a.node).forEach(function (f) {
            var v = a.node[f];
            //setTimeout(function(){
            var emit = { field: "where", soul: f };
            to.next(emit);
            //},10);
          });
        });
        e.on("foo", function (a, ev) {
          var to = this.to;
          setTimeout(function () {
            to.next({ node: a.soul });
          }, 100);
        });
        e.on(
          "foo",
          function (a) {
            expect(this.as.hi).to.be(1);
            if ("a" == a.node) {
              done.a = true;
            } else {
              expect(done.a).to.be.ok();
              done();
            }
          },
          { hi: 1 },
        ).on.on("foo", { field: "where", node: { a: 1, b: 2 } });
      });
      it("synchronous async", function (done) {
        var e = { on: Zen.on };
        e.on("foo", function (a) {
          expect(a.b).to.be(5);
          done.first = true;
          this.to.next(a);
        });
        e.on("foo", function (a, ev) {
          expect(a.b).to.be(5);
          done.second = true;
          var to = this.to;
          setTimeout(function () {
            to.next({ c: 9, again: a.again });
          }, 100);
        });
        e.on("foo", function (a) {
          this.off();
          expect(a.again).to.not.be.ok();
          expect(a.c).to.be(9);
          expect(done.first).to.be.ok();
          expect(done.second).to.be.ok();
          done();
        })
          .on.on("foo", { b: 5 })
          .on.on("foo", { b: 5, again: true });
      });
    });
    describe("flow", function () {
      var i = 0;
      function flow() {
        var f = function (arg) {
            var cb = f.cb ? f.cb.fn : f.fn;
            if (cb) {
              f.cb = cb;
              var ff = flow();
              ff.f = f;
              cb(ff);
              return;
            }
            if (f.f) {
              f.f(arg);
              f.cb = 0;
              return;
            }
          },
          cb;
        f.flow = function (fn) {
          cb = (cb || f).fn = fn;
          return f;
        };
        return f;
      }
      it("intermittent interruption", function (done) {
        var f = flow();
        //var f = {flow: flow}
        f.flow(function (f) {
          //console.log(1);
          f.flow(function (f) {
            //console.log(2);
            f({ yes: "please" });
          });
          setTimeout(function () {
            f.flow(function (f) {
              //console.log(2.1);
              f({ forever: "there" });
            });
            f({ strange: "places" });
            //console.log("-----");
            f({ earlier: "location" });
          }, 100);
        });
        f.flow(function (f) {
          //console.log(3);
          f({ ok: "now" });
        });
        f.flow(function (f) {
          //console.log(4);
          done();
        });
        setTimeout(function () {
          f({ hello: "world" });
        }, 100);
      });
      var i = 0;
      (function (exports) {
        function next(arg) {
          var n = this;
          if (arg instanceof Function) {
            if (!n.fn) {
              return ((n.fn = arg), n);
            }
            var f = { next: next, fn: arg, first: n.first || n };
            n.last = (n.last || n).to = f;
            return n;
          }
          if (n.fn) {
            var sub = { next: next, from: n.to || (n.first || {}).from };
            n.fn(sub);
            return;
          }
          if (n.from) {
            n.from.next(arg);
            return;
          }
        }
        Zen.next = next;
      })(Zen);
      it("intermittent interruptions", function (done) {
        //var f = flow();
        var f = { next: Zen.next }; // for now
        f.next(function (f) {
          //console.log(1, f);
          f.next(function (f) {
            //console.log(2, f);
            f.next({ yes: "please" });
          });
          setTimeout(function () {
            f.next(function (f) {
              //console.log(2.1, f);
              f.next({ forever: "there" });
            });
            f.next({ strange: "places" });
            //console.log("-----");
            f.next({ earlier: "location" });
          }, 100);
        });
        f.next(function (f) {
          //console.log(3);
          f.next({ ok: "now" });
        });
        f.next(function (f) {
          //console.log(4);
          if (!done.a) {
            return (done.a = true);
          }
          done();
        });
        setTimeout(function () {
          f.next({ hello: "world" });
        }, 100);
      });
    });
    describe("Zen Safety", function () {
      /* WARNING NOTE: Internal API has significant breaking changes! */

      var zen = Zen();
      it("is", function () {
        expect(Zen.is(zen)).to.be(true);
        expect(Zen.is(true)).to.be(false);
        expect(Zen.is(false)).to.be(false);
        expect(Zen.is(0)).to.be(false);
        expect(Zen.is(1)).to.be(false);
        expect(Zen.is("")).to.be(false);
        expect(Zen.is("a")).to.be(false);
        expect(Zen.is(Infinity)).to.be(false);
        expect(Zen.is(-Infinity)).to.be(false);
        expect(Zen.is(NaN)).to.be(false);
        expect(Zen.is([])).to.be(false);
        expect(Zen.is([1])).to.be(false);
        expect(Zen.is({})).to.be(false);
        expect(Zen.is({ a: 1 })).to.be(false);
        expect(Zen.is(function () {})).to.be(false);
      });
      it("valid", function () {
        expect(Zen.valid(false)).to.be(true);
        expect(Zen.valid(true)).to.be(true);
        expect(Zen.valid(0)).to.be(true);
        expect(Zen.valid(1)).to.be(true);
        expect(Zen.valid("")).to.be(true);
        expect(Zen.valid("a")).to.be(true);
        expect(Zen.valid({ "#": "somesoulidhere" })).to.be("somesoulidhere");
        expect(Zen.valid({ "#": "somesoulidhere", and: "nope" })).to.be(false);
        expect(Zen.valid(Infinity)).to.be(false); // boohoo :(
        expect(Zen.valid(-Infinity)).to.be(false); // boohoo :(
        expect(Zen.valid(NaN)).to.be(false);
        expect(Zen.valid([])).to.be(false);
        expect(Zen.valid([1])).to.be(false);
        expect(Zen.valid({})).to.be(false);
        expect(Zen.valid({ a: 1 })).to.be(false);
        expect(Zen.valid(function () {})).to.be(false);
      });
      it("is link", function () {
        expect(Zen.valid({ "#": "somesoulidhere" })).to.be("somesoulidhere");
        expect(Zen.valid({ "#": "somethingelsehere" })).to.be(
          "somethingelsehere",
        );
        expect(
          "string" == typeof Zen.valid({ "#": "somesoulidhere", and: "nope" }),
        ).to.be(false);
        expect(
          "string" == typeof Zen.valid({ or: "nope", "#": "somesoulidhere" }),
        ).to.be(false);
        expect("string" == typeof Zen.valid(false)).to.be(false);
        expect("string" == typeof Zen.valid(true)).to.be(false);
        expect("string" == typeof Zen.valid("")).to.be(false);
        expect("string" == typeof Zen.valid("a")).to.be(false);
        expect("string" == typeof Zen.valid(0)).to.be(false);
        expect("string" == typeof Zen.valid(1)).to.be(false);
        expect("string" == typeof Zen.valid(Infinity)).to.be(false); // boohoo :(
        expect("string" == typeof Zen.valid(-Infinity)).to.be(false); // boohoo :(
        expect("string" == typeof Zen.valid(NaN)).to.be(false);
        expect("string" == typeof Zen.valid([])).to.be(false);
        expect("string" == typeof Zen.valid([1])).to.be(false);
        expect("string" == typeof Zen.valid({})).to.be(false);
        expect("string" == typeof Zen.valid({ a: 1 })).to.be(false);
        expect("string" == typeof Zen.valid(function () {})).to.be(false);
      });
    });
  });

  describe("API", function () {
    var gopt = {
      wire: {
        put: function (n, cb) {
          cb();
        },
        get: function (k, cb) {
          cb();
        },
      },
    };
    if (((Zen.globalThis || {}).location || {}).search) {
      /*console.log("LOCALHOST PEER MUST BE ON!");
			var peer = {url: 'http://localhost:8420/zen'};
			Zen.on('opt', function(root){
				if(root.opt.test_no_peer){ return this.to.next(root) }
				root.opt.peers = root.opt.peers || {};
				root.opt.peers['http://localhost:8420/zen'] = peer;
				this.to.next(root);
			});*/
    }
    var goff = Zen();
    Zen.statedisk = function (o, s, cb) {
      goff.get(s).put(o, cb, null, {
        turn: function (fn) {
          fn();
        },
      });
    };
    var zen = Zen();

    var nopasstun = function (done, g) {
      g = (g || zen)._.root;
      var start = +new Date();
      (function wait() {
        if ((g.stun || g.pass) && +new Date() - start < 250) {
          setTimeout(wait, 9);
          return;
        }
        done && done();
      })();
    };

    describe("predictable souls", function () {
      it("public", function (done) {
        zen
          .get("z")
          .get("y")
          .get("x")
          .put({ c: { b: { a: 1 } } }, function () {
            if (done.c) {
              return;
            }
            done.c = 1;
            var g = zen._.graph;
            expect(g["z"]).to.be.ok();
            expect(g["z/y"]).to.be.ok();
            expect(g["z/y/x"]).to.be.ok();
            expect(g["z/y/x/c"]).to.be.ok();
            expect(g["z/y/x/c/b"]).to.be.ok();
            nopasstun(done, zen);
          });
      });
      it("no not found on incremental write", function (done) {
        zen
          .get("nnfoiw")
          .get("y")
          .put({ a: 1 }, function (ack) {
            if (ack.err) {
              return;
            }
            nopasstun(done, zen);
          });
      });
      it("public mix", function (done) {
        var ref = zen.get("zasdf").put({ a: 9 });
        var at = zen.get("zfdsa").get("y").get("x").get("c").put(ref);
        at.get("foo")
          .get("bar")
          .put("yay", function (ack) {
            done.a = 1;
            end();
          });
        ref
          .get("foo")
          .get("ah")
          .put(1, function (ack) {
            done.b = 1;
            end();
          });
        function end(ack) {
          if (!done.a || !done.b) {
            return;
          }
          if (done.c) {
            return;
          }
          done.c = 1;
          var g = zen._.graph;
          expect(Object.keys(g["zasdf"] || "").sort()).to.be.eql(
            ["_", "a", "foo"].sort(),
          );
          expect(Object.keys(g["zasdf/foo"] || "").sort()).to.be.eql(
            ["_", "bar", "ah"].sort(),
          );
          nopasstun(done, zen);
        }
        //setTimeout(function(){ console.log('???', zen._.stun); }, 1700);
      });
    });

    describe("plural chains", function () {
      this.timeout(9000);
      it("uncached synchronous map on", function (done) {
        /*
					Biggest challenges so far:
					 - Unsubscribe individual mapped next. !
					 - Performance deduplication on asking relation's next. !
					 - Replying immediately to parent cached contexts.
					 - Performant read lock on write contexts.
					 - Proxying event across maps.
				*/
        Zen.statedisk(
          {
            alice: { age: 26, name: "Alice", pet: { a: 1, name: "Fluffy" } },
            bob: { age: 29, name: "Bob!", pet: { b: 2, name: "Frisky" } },
          },
          "u/m",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m")
              .map()
              .on(function (v, f) {
                check[f] = v;
                count[f] = (count[f] || 0) + 1;
                //console.log("***********", f, v);
                if (check.alice && check.bob) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(check.alice.age).to.be(26);
                    expect(check.alice.name).to.be("Alice");
                    expect(
                      "string" == typeof Zen.valid(check.alice.pet),
                    ).to.be.ok();
                    //expect(count.alice).to.be(1);
                    expect(check.bob.age).to.be(29);
                    expect(check.bob.name).to.be("Bob!");
                    expect(
                      "string" == typeof Zen.valid(check.bob.pet),
                    ).to.be.ok();
                    //expect(count.bob).to.be(1);
                    nopasstun(done, zen);
                  }, 10);
                }
              });
          },
          1000,
        );
      });

      it("uncached synchronous map get on", function (done) {
        Zen.statedisk(
          {
            alice: {
              age: 26,
              name: "alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/p",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m/p")
              .map()
              .get("name")
              .on(function (v, f) {
                //console.log("*****************", f, v);
                check[v] = f;
                count[v] = (count[v] || 0) + 1;
                if (check.alice && check.bob) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(check.alice).to.be("name");
                    expect(check.bob).to.be("name");
                    //expect(count.alice).to.be(1);
                    //expect(count.bob).to.be(1);
                    nopasstun(done, zen);
                  }, 10);
                }
              });
          },
          1000,
        );
      });

      it("uncached synchronous map get on node", function (done) {
        Zen.statedisk(
          {
            alice: {
              age: 26,
              name: "alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/p/n",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m/p/n")
              .map()
              .get("pet")
              .on(function (v, f) {
                //console.log("********************", f,v);
                check[v.name] = v;
                count[v.name] = (count[v.name] || 0) + 1;
                if (check.Fluffy && check.Frisky) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(check.Fluffy.a).to.be(1);
                    expect(check.Frisky.b).to.be(2);
                    //expect(count.Fluffy).to.be(1);
                    //expect(count.Frisky).to.be(1);
                    //expect(count['undefined']).to.not.be.ok();
                    if (done.c) {
                      return;
                    }
                    done.c = 1;
                    nopasstun(done, zen);
                  }, 10);
                }
              });
          },
          1000,
        );
      });

      it("uncached synchronous map get on node get", function (done) {
        var zen = Zen();
        Zen.statedisk(
          {
            alice: {
              age: 26,
              name: "alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/p/n/p",
          function () {
            var check = {},
              count = {};
            //console.debug.i=1;console.log('-------------------');
            zen
              .get("u/m/p/n/p")
              .map()
              .get("pet")
              .get("name")
              .on(function (v, f) {
                check[v] = f;
                count[v] = (count[v] || 0) + 1;
                //console.log("*****************", f, v);
                if (check.Fluffy && check.Frisky) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(check.Fluffy).to.be("name");
                    expect(check.Frisky).to.be("name");
                    //console.log("????", zen._.graph);
                    //Zen.obj.map(zen._.graph, function(n,s){
                    Object.keys(zen._.graph).forEach(function (s, n) {
                      n = zen._.graph[s];
                      if ("u/m/p/n/p" === s) {
                        return;
                      }
                      var a = Object.keys(n); //Zen.obj.map(n, function(v,f,t){t(v)});
                      expect(a.length).to.be(2); // make sure that ONLY the selected properties were loaded, not the whole node.
                    });
                    //expect(count.Fluffy).to.be(1);
                    //expect(count.Frisky).to.be(1);
                    nopasstun(done, zen);
                  }, 10);
                }
              });
          },
          1000,
        );
      });

      it("uncached synchronous map on mutate", function (done) {
        Zen.statedisk(
          {
            alice: {
              age: 26,
              name: "Alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "Bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/mutate",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m/mutate")
              .map()
              .get("name")
              .get(function (v, f) {
                //console.log("****************", f,v);
                check[v] = f;
                count[v] = (count[v] || 0) + 1;
                if (check.Alice && check.Bob && check["undefined"]) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    //expect(count.Alice).to.be(1);
                    //expect(count.Bob).to.be(1);
                    //expect(count['undefined']).to.be(1);
                    if (done.c) {
                      return;
                    }
                    done.c = 1;
                    nopasstun(done, zen);
                  }, 10);
                }
              });
            setTimeout(function () {
              zen.get("u/m/mutate").get("alice").put(7);
            }, 300);
          },
          1000,
        );
      });

      it("uncached synchronous map on mutate node", function (done) {
        Zen.statedisk(
          {
            alice: {
              _: { "#": "umaliceo" },
              age: 26,
              name: "Alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "Bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/mutate/n",
          function () {
            var check = {},
              count = {},
              mutate,
              t0 = +new Date(),
              seen = [], // every emission, with when it landed
              storeLog = [],
              _opt = zen._.opt || {},
              _store = _opt.store,
              _get = _store && _store.get;
            // The premise still to test: does the storage layer answer this
            // read empty on the runs that fail? Everything downstream of an
            // empty answer is understood; whether it happens is not.
            if (_get) {
              _store.get = function (file, cb) {
                return _get.call(_store, file, function (e, d) {
                  storeLog.push(
                    String(file).slice(-20) +
                      (d ? ":" + d.length + "b" : ":EMPTY"),
                  );
                  cb(e, d);
                });
              };
            }
            var unwrap = function () {
              if (_get) {
                _store.get = _get;
              }
            };
            // This only ever fails on Windows CI, as a bare 9s timeout that
            // says nothing about which of the four expected emissions never
            // arrived. Report the collected state just before mocha gives up.
            // The first round of this told us the missing ones are the
            // *initial* values, not the late one, so also record the order and
            // timing -- that says whether the initial load never delivered or
            // was delivered and dropped.
            var diag = setTimeout(function () {
              console.log(
                "DIAG uncached synchronous map on mutate node: counts=" +
                  JSON.stringify(count) +
                  " missing=" +
                  JSON.stringify(
                    ["Alice", "Bob", "undefined", "Alice Zzxyz"].filter(
                      function (k) {
                        return !check[k];
                      },
                    ),
                  ) +
                  " done.last=" +
                  !!done.last +
                  " timeline=" +
                  JSON.stringify(seen) +
                  // Separates "the node never loaded" from "it loaded and the
                  // per-child name reads never delivered" -- the timeline alone
                  // cannot tell those apart.
                  " node=" +
                  JSON.stringify(
                    Object.keys((zen.get("u/m/mutate/n")._ || {}).put || {}),
                  ) +
                  " children=" +
                  JSON.stringify(
                    Object.keys((zen.get("u/m/mutate/n")._ || {}).next || {}),
                  ) +
                  " store=" +
                  JSON.stringify(storeLog.slice(-10)),
              );
              unwrap();
            }, 8000);
            zen
              .get("u/m/mutate/n")
              .map()
              .get("name")
              .get(function (v, f) {
                seen.push(String(v) + "@" + (+new Date() - t0) + "ms");
                check[v] = f;
                count[v] = (count[v] || 0) + 1;
                if (check.Alice && check.Bob && mutate) {
                  mutate();
                }
                //console.log("************", f,v);
                if (
                  check.Alice &&
                  check.Bob &&
                  check["undefined"] &&
                  check["Alice Zzxyz"]
                ) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    clearTimeout(diag);
                    unwrap();
                    expect(done.last).to.be.ok();
                    expect(check["Alice Aabca"]).to.not.be.ok();
                    expect(count.Alice).to.be(1);
                    expect(count.Bob).to.be(1);
                    expect(count["undefined"]).to.be(1);
                    expect(count["Alice Zzxyz"]).to.be(1);
                    nopasstun(done, zen);
                  }, 200);
                }
              });
            // Mutate once the initial load has actually landed, not after a
            // fixed sleep. The sleep was a race inside the test: it assumed
            // storage answers within 300ms, and on a loaded Windows runner the
            // first read of this node can take twice that. Overwriting `alice`
            // before its original value arrives makes that value unobservable
            // for good -- the later state wins -- so the test failed for a
            // reason that had nothing to do with what it is checking.
            //
            // Waiting on the condition keeps the check intact: if the initial
            // load never arrives nothing mutates, and the test still fails on
            // its own timeout, saying exactly that.
            mutate = function () {
              if (mutate.done) {
                return;
              }
              mutate.done = 1;
              zen
                .get("u/m/mutate/n")
                .get("alice")
                .put({
                  _: { "#": "u/m/m/n/soul" },
                  name: "Alice Zzxyz",
                });
              setTimeout(function () {
                zen.get("umaliceo").put({
                  name: "Alice Aabca",
                });
                done.last = true;
              }, 10);
            };
            if (check.Alice && check.Bob) {
              mutate();
            }
          },
          1000,
        );
      });

      it("uncached synchronous map on mutate node uncached", function (done) {
        Zen.statedisk(
          {
            alice: {
              _: { "#": "umaliceo1" },
              age: 26,
              name: "Alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "Bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/mutate/n/u",
          function () {
            var check = {},
              count = {},
              t0 = +new Date(),
              seen = [];
            // Same story as the sibling test above: Windows-only, and the bare
            // timeout hides which emission went missing. Say so, with the order
            // and timing of what did arrive.
            var diag = setTimeout(function () {
              console.log(
                "DIAG uncached synchronous map on mutate node uncached: counts=" +
                  JSON.stringify(count) +
                  " missing=" +
                  JSON.stringify(
                    ["Alice", "Bob", "Alice Zzxyz"].filter(function (k) {
                      return !check[k];
                    }),
                  ) +
                  " done.last=" +
                  !!done.last +
                  " timeline=" +
                  JSON.stringify(seen) +
                  " node=" +
                  JSON.stringify(
                    Object.keys((zen.get("u/m/mutate/n/u")._ || {}).put || {}),
                  ) +
                  " children=" +
                  JSON.stringify(
                    Object.keys((zen.get("u/m/mutate/n/u")._ || {}).next || {}),
                  ),
              );
            }, 8000);
            zen
              .get("u/m/mutate/n/u")
              .map()
              .on(function (v, f) {
                seen.push(
                  String(v && v.name) + "@" + (+new Date() - t0) + "ms",
                );
                check[v.name] = f;
                count[v.name] = (count[v.name] || 0) + 1;
                if (check.Alice && check.Bob && check["Alice Zzxyz"]) {
                  clearTimeout(done.to);
                  //console.log("****", f, v)
                  done.to = setTimeout(function () {
                    clearTimeout(diag);
                    expect(done.last).to.be.ok();
                    //expect(check['Alice Aabca']).to.not.be.ok();
                    //expect(count['Alice']).to.be(1);
                    //expect(count['Bob']).to.be(1);
                    //expect(count['Alice Zzxyz']).to.be(1);
                    if (done.c) {
                      return;
                    }
                    done.c = 1;
                    nopasstun(done, zen);
                  }, 200);
                }
              });
            setTimeout(function () {
              Zen.statedisk(
                { name: "Alice Zzxyz" },
                "u/m/m/n/u/soul",
                function () {
                  //console.debug.i=1;console.log("---------------");
                  zen.get("u/m/mutate/n/u").put({
                    alice: { "#": "u/m/m/n/u/soul" },
                  });
                  /*
						{
							users: {_:#users
								alice: {#newalice}
							}
						}
					*/
                  setTimeout(function () {
                    zen.get("umaliceo1").put({
                      name: "Alice Aabca",
                    });
                    done.last = true;
                  }, 10);
                },
                1000,
              );
            }, 300);
          },
          1000,
        );
      });

      it("uncached synchronous map on get mutate node uncached", function (done) {
        Zen.statedisk(
          {
            alice: {
              _: { "#": "umaliceo2" },
              age: 26,
              name: "Alice",
              pet: { a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "Bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/p/mutate/n/u",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m/p/mutate/n/u")
              .map()
              .get("name")
              .on(function (v, f) {
                check[v] = f;
                count[v] = (count[v] || 0) + 1;
                //console.log("*************", f,v);
                if (check.Alice && check.Bob && check["Alice Zzxyz"]) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    var a = Object.keys(zen._.graph["u/m/p/m/n/u/soul"]); //Zen.obj.map(zen._.graph['u/m/p/m/n/u/soul'], function(v,f,t){t(v)});
                    expect(a.length).to.be(2);
                    expect(done.last).to.be.ok();
                    expect(check["Alice Aabca"]).to.not.be.ok();
                    //expect(count.Alice).to.be(1);
                    //expect(count.Bob).to.be(1);
                    //expect(count['Alice Zzxyz']).to.be(1);
                    nopasstun(done, zen);
                  }, 200);
                }
              });
            setTimeout(function () {
              Zen.statedisk(
                { name: "Alice Zzxyz", age: 34 },
                "u/m/p/m/n/u/soul",
                function () {
                  zen.get("u/m/p/mutate/n/u").put({
                    alice: { "#": "u/m/p/m/n/u/soul" },
                  });
                  setTimeout(function () {
                    zen.get("umaliceo2").put({
                      name: "Alice Aabca",
                    });
                    done.last = true;
                  }, 10);
                },
                1000,
              );
            }, 300);
          },
          1000,
        );
      });

      it("uncached synchronous map on get node mutate node uncached", function (done) {
        Zen.statedisk(
          {
            alice: {
              _: { "#": "umaliceo3" },
              age: 26,
              name: "Alice",
              pet: { _: { "#": "sflufso" }, a: 1, name: "Fluffy" },
            },
            bob: {
              age: 29,
              name: "Bob",
              pet: { b: 2, name: "Frisky" },
            },
          },
          "u/m/p/n/mutate/n/u",
          function () {
            var check = {},
              count = {};
            zen
              .get("u/m/p/n/mutate/n/u")
              .map()
              .get("pet")
              .on(function (v, f) {
                check[v.name] = f;
                count[v.name] = (count[v.name] || 0) + 1;
                //console.log("*****************", f,v, check);
                if (check.Fluffy && check.Frisky && check.Fuzzball) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(done.last).to.be.ok();
                    expect(check["Fluffs"]).to.not.be.ok();
                    expect(count.Fluffy).to.be(1);
                    expect(count.Frisky).to.be(1);
                    expect(count.Fuzzball).to.be(1);
                    nopasstun(done, zen);
                  }, 200);
                }
              });
            setTimeout(function () {
              Zen.statedisk(
                {
                  name: "Alice Zzxyz",
                  age: 34,
                  pet: { c: 3, name: "Fuzzball" },
                },
                "alice/fuzz/soul",
                function () {
                  zen.get("u/m/p/n/mutate/n/u").put({
                    alice: { "#": "alice/fuzz/soul" },
                  });
                  setTimeout(function () {
                    zen.get("sflufso").put({
                      name: "Fluffs",
                    });
                    done.last = true;
                  }, 10);
                },
                1000,
              );
            }, 300);
          },
          1000,
        );
      });

      it("unlink deeply nested", function (done) {
        Zen.statedisk(
          {
            a: {
              _: { "#": "audn" },
              age: 26,
              name: "Alice",
              b: {
                _: { "#": "budn" },
                c: { _: { "#": "cudn" }, id: "first", level: 3 },
                level: 2,
              },
            },
          },
          "udn",
          function () {
            var check = {},
              count = {};
            zen
              .get("udn")
              .get("a")
              .get("b")
              .get("c")
              .on(function (data) {
                if (done.c) {
                  return;
                }
                //console.log("udn.a.b.c:", data);
                check[data.id] = 1;
                count[data.id] = (count[data.id] || 0) + 1;
                //console.log("*****************", f,v, check);
                if (check.first && check.other) {
                  clearTimeout(done.to);
                  done.to = setTimeout(function () {
                    expect(done.last).to.be.ok();
                    expect(check.firsta).to.not.be.ok();
                    expect(count.first).to.be(1);
                    expect(count.other).to.be(1);
                    if (done.c) {
                      return;
                    }
                    done.c = 1;
                    nopasstun(done, zen);
                  }, 200);
                }
              });
            setTimeout(function () {
              Zen.statedisk(
                {
                  name: "Alice2",
                  age: 34,
                  b: {
                    _: { "#": "2budn" },
                    c: { _: { "#": "2cudn" }, id: "other", level: 3 },
                    level: 2,
                  },
                },
                "2audn",
                function () {
                  //console.only.i=1;console.log('=============================');
                  zen.get("udn").put({
                    a: { "#": "2audn" },
                  });
                  setTimeout(function () {
                    //console.log("- - - - - - - - - - - -");
                    zen.get("cudn").put({ id: "firsta", foo: "bar" });
                    done.last = 1;
                  }, 50);
                },
              );
            }, 50);
          },
        );
      });

      it("get before put in memory", function (done) {
        var zen = Zen();
        var check = {};
        var count = {};
        zen
          .get("g/n/m/f/l/n/r")
          .map()
          .on(function (v, f) {
            //console.log("***********", f,v);
            check[f] = v;
            count[f] = (count[f] || 0) + 1;
            if (check.alice && check.bob && check.alice.PhD) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice.age).to.be(24);
                expect(check.bob.age).to.be(26);
                expect(check.alice.PhD).to.be(true);
                //expect(count.alice).to.be(2);
                //expect(count.bob).to.be(1);
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 50);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/r" },
          alice: {
            _: { "#": "GALICE1" },
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: {
                name: "ACME INC",
              },
            },
          },
        });
        setTimeout(function () {
          zen.get("GALICE1").put({ PhD: true });
        }, 300);
      });

      it("in memory get after", function (done) {
        var zen = Zen();
        zen.put({
          _: { "#": "g/n/m/f/l/n" },
          alice: {
            _: { "#": "GALICE2" },
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: {
                name: "ACME INC",
              },
            },
          },
        });
        var check = {};
        //zen.get('g/n/m/f/l/n').get('bob.spouse.work').on(function(v,f){ console.log("!!!!!!!!!", f, v);});return;
        zen
          .get("g/n/m/f/l/n")
          .map()
          .on(function (v, f) {
            if (done.c) {
              return;
            }
            check[f] = v;
            //console.log("*******************", f, v);
            if (check.alice && check.bob && check.alice.PhD) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice.age).to.be(24);
                expect(check.bob.age).to.be(26);
                expect(check.alice.PhD).to.be(true);
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        // Use 1000ms to give the map subscription enough time to fully
        // set up on slow CI machines (Windows disk I/O can exceed 300ms).
        setTimeout(function () {
          zen.get("GALICE2").put({ PhD: true });
        }, 1000);
      });

      it("in memory get before map get", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/p")
          .map()
          .get("name")
          .on(function (v, f) {
            check[v] = f;
            //console.log("****************", f,v, zen);
            if (check.alice && check.bob && check.Alice) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice).to.be("name");
                expect(check.bob).to.be("name");
                expect(check.Alice).to.be("name");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/p" },
          alice: {
            _: { "#": "GALICE3" },
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: {
                name: "ACME INC",
              },
            },
          },
        });
        setTimeout(function () {
          //console.debug.i=1;console.log("-------------");
          zen.get("GALICE3").put({ name: "Alice" });
        }, 300);
      });

      it("in memory get after map get", function (done) {
        var zen = Zen();
        zen.put({
          _: { "#": "g/n/m/f/l/n/m/p" },
          alice: {
            _: { "#": "GALICE4" },
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: {
                name: "ACME INC",
              },
            },
          },
        });
        var check = {};
        zen
          .get("g/n/m/f/l/n/m/p")
          .map()
          .get("name")
          .on(function (v, f) {
            if (done.c) {
              return;
            }
            check[v] = f;
            //console.log("*****************", f,v);
            if (check.alice && check.bob && check.Alice) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice).to.be("name");
                expect(check.bob).to.be("name");
                expect(check.Alice).to.be("name");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("GALICE4").put({ name: "Alice" });
        }, 300);
      });

      it("in memory get before map get get", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/p/p/p")
          .map()
          .get("spouse")
          .get("work")
          .on(function (v, f) {
            check[v.name] = f;
            //console.log("*******", f, v);
            if (check["ZEN INC"] && check["ACME INC"] && check["ACME INC."]) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check["ZEN INC"]).to.be("work");
                expect(check["ACME INC"]).to.be("work");
                expect(check["ACME INC."]).to.be("work");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/p/p/p" },
          alice: {
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: { _: { "#": "CCINEMA1" }, name: "ACME INC" },
            },
          },
        });
        setTimeout(function () {
          //console.debug.i=1;console.log("----------------");
          zen.get("CCINEMA1").put({ name: "ACME INC." });
        }, 300);
      });

      it("in memory get after map get get", function (done) {
        var zen = Zen();
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/p/p/p/a" },
          alice: {
            name: "alice",
            age: 24,
            spouse: {
              name: "carl",
              age: 25,
              work: {
                name: "ZEN INC",
              },
            },
            bout: { huh: 1 },
          },
          bob: {
            name: "bob",
            age: 26,
            spouse: {
              name: "diana",
              age: 27,
              work: { _: { "#": "CCINEMA2" }, name: "ACME INC" },
            },
          },
        });
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/p/p/p/a")
          .map()
          .get("spouse")
          .get("work")
          .on(function (v, f) {
            check[v.name] = f;
            if (check["ZEN INC"] && check["ACME INC"] && check["ACME INC."]) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check["ZEN INC"]).to.be("work");
                expect(check["ACME INC"]).to.be("work");
                expect(check["ACME INC."]).to.be("work");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("CCINEMA2").put({ name: "ACME INC." });
        }, 300);
      });

      it("in memory get before map map", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/a/m/m")
          .map()
          .map()
          .on(function (v, f) {
            check[f] = v;
            //console.log("****************", f,v);
            if (
              check.alice &&
              check.bob &&
              check.ZEN &&
              check.ACME &&
              check.ACME.corp
            ) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice.name).to.be("alice");
                expect(check.alice.age).to.be(24);
                expect(
                  "string" == typeof Zen.valid(check.alice.spouse),
                ).to.be.ok();
                expect(check.bob.name).to.be("bob");
                expect(check.bob.age).to.be(26);
                expect(
                  "string" == typeof Zen.valid(check.bob.spouse),
                ).to.be.ok();
                expect(check.ZEN.name).to.be("ZEN");
                expect(check.ACME.name).to.be("ACME");
                expect(check.ACME.corp).to.be("C");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        //console.debug.i=1;console.log("------------------------");
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
            },
            ACME: { _: { "#": "CCINEMA3" }, name: "ACME" },
          },
        });
        setTimeout(function () {
          //console.debug.i=1;console.log("-------------");
          zen.get("CCINEMA3").put({ corp: "C" });
        }, 300);
      });

      it("in memory get after map map", function (done) {
        var zen = Zen();
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/m/m" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
            },
            ACME: { _: { "#": "CCINEMA4" }, name: "ACME" },
          },
        });
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/m/m")
          .map()
          .map()
          .on(function (v, f) {
            if (done.c) {
              return;
            }
            check[f] = v;
            //console.log("***************", f,v);
            if (
              check.alice &&
              check.bob &&
              check.ZEN &&
              check.ACME &&
              check.ACME.corp
            ) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice.name).to.be("alice");
                expect(check.alice.age).to.be(24);
                expect(
                  "string" == typeof Zen.valid(check.alice.spouse),
                ).to.be.ok();
                expect(check.bob.name).to.be("bob");
                expect(check.bob.age).to.be(26);
                expect(
                  "string" == typeof Zen.valid(check.bob.spouse),
                ).to.be.ok();
                expect(check.ZEN.name).to.be("ZEN");
                expect(check.ACME.name).to.be("ACME");
                expect(check.ACME.corp).to.be("C");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("CCINEMA4").put({ corp: "C" });
        }, 300);
      });

      it("in memory get before map map get", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/m/m/p")
          .map()
          .map()
          .get("name")
          .on(function (v, f) {
            check[v] = f;
            //console.log("***********", f,v);
            if (
              check.alice &&
              check.bob &&
              check.ZEN &&
              check.ACME &&
              check.ACMEINC
            ) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice).to.be("name");
                expect(check.bob).to.be("name");
                expect(check.ZEN).to.be("name");
                expect(check.ACME).to.be("name");
                expect(check.ACMEINC).to.be("name");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/m/m/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
            },
            ACME: { _: { "#": "CCINEMA5" }, name: "ACME" },
          },
        });
        setTimeout(function () {
          zen.get("CCINEMA5").put({ name: "ACMEINC" });
        }, 300);
      });

      it("in memory get after map map get", function (done) {
        var zen = Zen();
        var check = {};
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
            },
            ACME: { _: { "#": "CCINEMA6" }, name: "ACME" },
          },
        });
        zen
          .get("g/n/m/f/l/n/b/a/m/m/p")
          .map()
          .map()
          .get("name")
          .on(function (v, f) {
            check[v] = f;
            //console.log("************", f,v);
            if (
              check.alice &&
              check.bob &&
              check.ZEN &&
              check.ACME &&
              check.ACMEINC
            ) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.alice).to.be("name");
                expect(check.bob).to.be("name");
                expect(check.ZEN).to.be("name");
                expect(check.ACME).to.be("name");
                expect(check.ACMEINC).to.be("name");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("CCINEMA6").put({ name: "ACMEINC" });
        }, 300);
      });

      it("in memory get before map map get get", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/m/m/p/p")
          .map()
          .map()
          .get("address")
          .get("state")
          .on(function (v, f) {
            check[v] = f;
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("state");
                expect(check.NY).to.be("state");
                expect(check.CA).to.be("state");
                expect(check.TX).to.be("state");
                expect(check.MA).to.be("state");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/m/m/p/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: { _: { "#": "QUANGO" }, state: "MA" },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: "TX",
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: "CA",
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: "NY",
              },
            },
          },
        });
        setTimeout(function () {
          zen.get("QUANGO").put({ state: "QR" });
        }, 300);
      });

      it("in memory get after map map get get", function (done) {
        var zen = Zen();
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m/p/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: { _: { "#": "QUANGO1" }, state: "MA" },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: "TX",
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: "CA",
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: "NY",
              },
            },
          },
        });
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/a/m/m/p/p")
          .map()
          .map()
          .get("address")
          .get("state")
          .on(function (v, f) {
            check[v] = f;
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("state");
                expect(check.NY).to.be("state");
                expect(check.CA).to.be("state");
                expect(check.TX).to.be("state");
                expect(check.MA).to.be("state");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("QUANGO1").put({ state: "QR" });
        }, 300);
      });

      it("in memory get before map map get get get", function (done) {
        this.timeout(20000);
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/m/m/p/p/p")
          .map()
          .map()
          .get("address")
          .get("state")
          .get("code")
          .on(function (v, f) {
            check[v] = f;
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("code");
                expect(check.NY).to.be("code");
                expect(check.CA).to.be("code");
                expect(check.TX).to.be("code");
                expect(check.MA).to.be("code");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/m/m/p/p/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: {
                state: {
                  _: { "#": "HIPPOM" },
                  code: "MA",
                  county: {
                    MA1: "First",
                  },
                },
              },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: {
                  code: "TX",
                  county: {
                    TX1: "First",
                  },
                },
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: {
                  code: "CA",
                  county: {
                    CA1: "First",
                  },
                },
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: {
                  code: "NY",
                  county: {
                    NY1: "First",
                  },
                },
              },
            },
          },
        });
        setTimeout(function () {
          zen.get("HIPPOM").put({ code: "QR" });
        }, 300);
      });

      it("in memory get before after map map get get get", function (done) {
        var zen = Zen();
        var check = {};
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m/p/p/p" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: {
                state: {
                  _: { "#": "HIPPOM1" },
                  code: "MA",
                  county: {
                    MA1: "First",
                  },
                },
              },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: {
                  code: "TX",
                  county: {
                    TX1: "First",
                  },
                },
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: {
                  code: "CA",
                  county: {
                    CA1: "First",
                  },
                },
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: {
                  code: "NY",
                  county: {
                    NY1: "First",
                  },
                },
              },
            },
          },
        });
        zen
          .get("g/n/m/f/l/n/b/a/m/m/p/p/p")
          .map()
          .map()
          .get("address")
          .get("state")
          .get("code")
          .on(function (v, f) {
            check[v] = f;
            //console.log("***********", f,v);
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("code");
                expect(check.NY).to.be("code");
                expect(check.CA).to.be("code");
                expect(check.TX).to.be("code");
                expect(check.MA).to.be("code");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("HIPPOM1").put({ code: "QR" });
        }, 300);
      });

      it("in memory get before map map get get node", function (done) {
        var zen = Zen();
        var check = {};
        zen
          .get("g/n/m/f/l/n/b/m/m/p/p/n")
          .map()
          .map()
          .get("address")
          .get("state")
          .on(function (v, f) {
            check[v.code] = f;
            //console.log("************", f, v);
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("state");
                expect(check.NY).to.be("state");
                expect(check.CA).to.be("state");
                expect(check.TX).to.be("state");
                expect(check.MA).to.be("state");
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              }, 10);
            }
          });
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/m/m/p/p/n" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: {
                state: {
                  _: { "#": "HIPPOM3" },
                  code: "MA",
                  county: {
                    MA1: "First",
                  },
                },
              },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: {
                  code: "TX",
                  county: {
                    TX1: "First",
                  },
                },
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: {
                  code: "CA",
                  county: {
                    CA1: "First",
                  },
                },
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: {
                  code: "NY",
                  county: {
                    NY1: "First",
                  },
                },
              },
            },
          },
        });
        setTimeout(function () {
          zen.get("HIPPOM3").put({ code: "QR" });
        }, 300);
      });

      it("in memory get before after map map get get node", function (done) {
        var zen = Zen();
        var check = {};
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m/p/p/n" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: {
                state: {
                  _: { "#": "HIPPOM4" },
                  code: "MA",
                  county: {
                    MA1: "First",
                  },
                },
              },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: {
                  code: "TX",
                  county: {
                    TX1: "First",
                  },
                },
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: {
                  code: "CA",
                  county: {
                    CA1: "First",
                  },
                },
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: {
                  code: "NY",
                  county: {
                    NY1: "First",
                  },
                },
              },
            },
          },
        });
        zen
          .get("g/n/m/f/l/n/b/a/m/m/p/p/n")
          .map()
          .map()
          .get("address")
          .get("state")
          .on(function (v, f) {
            check[v.code] = f;
            //console.log("**********", f, v);
            if (check.QR && check.NY && check.CA && check.TX && check.MA) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.QR).to.be("state");
                expect(check.NY).to.be("state");
                expect(check.CA).to.be("state");
                expect(check.TX).to.be("state");
                expect(check.MA).to.be("state");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("HIPPOM4").put({ code: "QR" });
        }, 300);
      });

      it("in memory get after map map get get get map", function (done) {
        var zen = Zen();
        var check = {};
        zen.put({
          _: { "#": "g/n/m/f/l/n/b/a/m/m/p/p/p/n" },
          users: {
            alice: {
              name: "alice",
              age: 24,
              address: {
                state: {
                  code: "MA",
                  county: {
                    MA1: "First",
                    MA2: "Second",
                  },
                },
              },
              spouse: {
                name: "carl",
                age: 25,
              },
              bout: { huh: 1 },
            },
            bob: {
              name: "bob",
              age: 26,
              address: {
                state: {
                  code: "TX",
                  county: {
                    TX1: "First",
                    TX2: "Second",
                  },
                },
              },
              spouse: {
                name: "diana",
                age: 27,
              },
            },
          },
          companies: {
            ZEN: {
              name: "ZEN",
              address: {
                state: {
                  code: "CA",
                  county: {
                    CA1: "First",
                    CA2: "Second",
                  },
                },
              },
            },
            ACME: {
              name: "ACME",
              address: {
                state: {
                  code: "NY",
                  county: {
                    _: { "#": "NYCOUNT" },
                    NY1: "First",
                    NY2: "Second",
                  },
                },
              },
            },
          },
        });
        zen
          .get("g/n/m/f/l/n/b/a/m/m/p/p/p/n")
          .map()
          .map()
          .get("address")
          .get("state")
          .get("county")
          .map()
          .on(function (v, f) {
            check[f] = v;
            //console.log("****************", f,v);
            if (
              check.MA1 &&
              check.MA2 &&
              check.TX1 &&
              check.TX2 &&
              check.CA1 &&
              check.CA2 &&
              check.NY1 &&
              check.NY2 &&
              check.NY3
            ) {
              clearTimeout(done.to);
              done.to = setTimeout(function () {
                expect(check.MA1).to.be("First");
                expect(check.TX1).to.be("First");
                expect(check.CA1).to.be("First");
                expect(check.NY1).to.be("First");
                expect(check.MA2).to.be("Second");
                expect(check.TX2).to.be("Second");
                expect(check.CA2).to.be("Second");
                expect(check.NY2).to.be("Second");
                expect(check.NY3).to.be("Third");
                nopasstun(done, zen);
              }, 10);
            }
          });
        setTimeout(function () {
          zen.get("NYCOUNT").put({ NY3: "Third" });
        }, 300);
      });
    });

    it("once put once", function (done) {
      zen.get("opo").get("a").put("yay!");
      var ref = zen.get("opo").get("a");
      setTimeout(function () {
        ref.once(function (data) {
          //console.log("************", data);
          expect(data).to.be("yay!");

          setTimeout(function () {
            console.only(3, ".put z");
            zen.get("opo").get("a").put("z");

            setTimeout(function () {
              ref.once(function (data) {
                //console.log("*+*+*+*+*+**+*+*");
                expect(data).to.be("z");
                nopasstun(done);
              });
            }, 25);
          }, 25);
        });
      }, 25);
    });

    /*it('get recursive map', function(done){
			var teams = {red: {}, blue: {}};
			var alice = {age: 27, name: "Alice"};
			var bob = {age: 29, name: "Bob"};
			alice.spouse = bob;
			bob.spouse = alice;
			var carl = {age: 31, name: "Carl"};
			teams.blue.alice = alice;
			teams.blue.carl = carl;
			teams.red.bob = bob;
			teams.red.carl = carl;
			console.only.i=1;console.log("===============");
			Zen.statedisk(teams, 'gerema', function(ack){
				console.log("VVVVVVVVVVVVVVVVVVV", ack);
				setTimeout(function(){

				zen.get('gerema').map().map().get('spouse').on(function(data){
					console.only(2,'hi', data);
					console.only(1,'hi', data);
						console.log("*****************", data);return;
				});
			},500);});
		});*/

    it("get node after recursive field", function (done) {
      var bob = { age: 29, name: "Bob!" };
      var cat = { name: "Fluffy", species: "kitty" };
      var user = { bob: bob };
      bob.pet = cat;
      cat.slave = bob;
      Zen.statedisk(user, "nodecircle", function () {
        //console.only.i=1;console.log("=============", zen);
        //zen.get('nodecircle').get('bob').once(function(data){
        zen
          .get("nodecircle")
          .get("bob")
          .get("pet")
          .get("slave")
          .once(function (data) {
            //console.log("*****************", data, done.to);return;
            expect(done.to).to.not.be.ok();
            done.to = setTimeout(function () {
              expect(data.age).to.be(29);
              expect(data.name).to.be("Bob!");
              expect("string" == typeof Zen.valid(data.pet)).to.ok();
              nopasstun(done);
            }, 300);
          });
      });
    });

    it("recursive put", function (done) {
      //localStorage.clear();
      var zen = Zen();

      var parent = zen.get("parent");
      var child = zen.get("child");

      child.put({
        way: "down",
      });

      parent.get("sub").put(child);
      //console.only.i=1;console.log("============================");
      parent.get("sub").on(function (data) {
        //console.log("*********sub", data);
        done.sub = data;
      });
      child.on(function (data) {
        done.child = data;
        //console.log("*********child", data);
      });
      parent.on(function (data) {
        done.parent = data;
        //console.log("*********parent", data);
        if (done.c) {
          return;
        }
        done.c = 1;
        nopasstun(done, zen); // TODO: Add more meaningful checks!
      });
    });

    it("empty val followed", function (done) {
      var zen = Zen();

      zen
        .get("val/follow")
        .once(function (data) {
          //console.log("val", data);
        })
        .get(function (at) {
          //console.log("?????", at);
          if (done.c) {
            return;
          }
          done.c = 1;
          nopasstun(done, zen);
        });
    });

    it("map val get put", function (done) {
      var zen = Zen().get("chat/asdf");

      var check = {},
        count = {};
      zen.map().once(function (v, f) {
        check[f] = v;
        count[f] = (count[f] || 0) + 1;
        //console.log("**************", f, v);
        if (check["1-1"] && check["2-2"]) {
          clearTimeout(done.to);
          done.to = setTimeout(function () {
            expect(check["1-1"].what).to.be("hi");
            expect(check["2-2"].what).to.be("you.");
            expect(count["1-1"]).to.be(1);
            expect(count["2-2"]).to.be(1);
            nopasstun(done, zen);
          }, 50);
        }
      });
      setTimeout(function () {
        zen.get("1-1").put({ what: "hi" });
        setTimeout(function () {
          zen.get("2-2").put({ what: "you." });
        }, 40);
      }, 40);
    });

    it("get list set map simple", function (done) {
      var zen = Zen();

      var list = zen.get("list-s");

      list.set(
        zen
          .get("alice-s")
          .put({ name: "Alice", group: "awesome", married: true }),
      );
      var check = {},
        count = {};
      list.map().on(function (data, id) {
        if (done.c) {
          return;
        }
        expect(data.group).to.be("awesome");
        expect(data.married).to.be(true);
        expect(data.name).to.be("Alice");
        done.c = 1;
        nopasstun(done, zen);
      });
    });

    it("get list set map val", function (done) {
      var zen = Zen();

      var list = zen.get("list");

      list.set(
        zen
          .get("alice")
          .put({ name: "Alice", group: "awesome", married: true }),
      );
      list.set(
        zen.get("bob").put({ name: "Bob", group: "cool", married: true }),
      );
      list.set(
        zen.get("carl").put({ name: "Carl", group: "cool", married: false }),
      );
      list.set(
        zen.get("dave").put({ name: "Dave", group: "awesome", married: true }),
      );

      var check = {},
        count = {};
      list.map().once(function (data, id) {
        check[id] = data;
        count[id] = (count[id] || 0) + 1;
        if (check.alice && check.bob && check.carl && check.dave) {
          clearTimeout(done.to);
          done.to = setTimeout(function () {
            expect(count.alice).to.be(1);
            expect(check.alice.name).to.be("Alice");
            expect(check.alice.group).to.be("awesome");
            expect(check.alice.married).to.be(true);
            expect(count.bob).to.be(1);
            expect(check.bob.name).to.be("Bob");
            expect(check.bob.group).to.be("cool");
            expect(check.bob.married).to.be(true);
            expect(count.carl).to.be(1);
            expect(check.carl.name).to.be("Carl");
            expect(check.carl.group).to.be("cool");
            expect(check.carl.married).to.be(false);
            expect(count.dave).to.be(1);
            expect(check.dave.name).to.be("Dave");
            expect(check.dave.group).to.be("awesome");
            expect(check.dave.married).to.be(true);
            nopasstun(done, zen);
          }, 50);
        }
      });
      /*
				Have we asked for this yet? No.
				Do we have it cached? No.
				Is its parent cached? Yes.
				Reply immediately with that cache for map to process.
			*/

      /*
				chain has a root // all
				an ID // all
				a back // all
				inputs // all
				and outputs // all
				acks // any
				echo // any
				next // any
				cache or map of many ones // only a one can have a cache, only a map can have many, and they must be ones. However any chain might have neither. By default a chain is a many, unless it is designated as a one.

				zen.get('alice').also('bob').path('name').on(cb);
				zen.get('users').map().path('friends').map().on(cb);

				friends is a map, it has an echo
				{name: "alice", friends: []}
					{name: "xavier"}
					{name: "yara"}
					{name: "zack"}
				{name: "bob", friends: []}
					{name: "xavier"}
					{name: "yara"}
					{name: "zack"}
				{name: "carl", friends: []}
					{name: "xavier"}
					{name: "yara"}
					{name: "zack"}
			*/
    });

    it("get get get set root get put", function (done) {
      var zen = Zen().get("app");
      //console.only.i=1;console.log('---------------');
      zen
        .get("alias")
        .get("mark")
        .set(
          zen.back(-1).get("pub").put({
            alias: "mark",
            auth: "encrypt", // oops
            born: 1,
            pub: "pub",
            salt: "random",
          }),
        );
      setTimeout(function () {
        //console.debug.i=1;console.log('---------------');
        zen.get(function (data) {
          //console.log("*", data);//return;
          done.app = done.app || (data && data.alias);
        });
        zen
          .back(-1)
          .get("pub")
          .get(function (data) {
            //console.log("**", data && data.auth);
            done.pub = done.pub || (data && data.auth);
          });
        zen
          .get("alias")
          .get(function (data) {
            //console.log("***", data);
            done.alias = done.alias || (data && data.mark);
            //!console.debug.i&&(console.debug.i=1)&&console.log("---------------------");
          })
          .get("mark")
          .on(function (data) {
            //console.log("************", data);//return;
            clearTimeout(done.to);
            done.to = setTimeout(function () {
              done.mark = done.mark || data.pub;
              //console.log('???', done.app, done.pub, done.alias);
              expect(Zen.valid(done.mark)).to.be("pub");
              expect(done.app).to.be.ok();
              expect(done.pub).to.be.ok();
              expect(done.alias).to.be.ok();
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            }, 100);
          });
      }, 100);
    });

    it("get put get get put reload get get then get", function (done) {
      this.timeout(9000);
      var zen = Zen();

      zen.get("stef").put({ name: "Stef" });
      var address = {
        country: "Netherlands",
        zip: "999999",
      };
      zen.get("stef").get("address").put(address);

      // reload
      setTimeout(function () {
        var gun2 = Zen();
        //console.log(load('fs').readFileSync('./radata/!').toString());
        gun2
          .get("stef")
          .get("address")
          .once(function (data) {
            // Object {_: Object, country: "Netherlands", zip: "1766KP"} "address"
            //console.log("******", data);return;
            done.a = true;
            expect(data.country).to.be("Netherlands");
            expect(data.zip).to.be("999999");
            if (!done.s) {
              return;
            }
            if (done.c) {
              return;
            }
            done.c = 1;
            nopasstun(done, gun2);
          });
        gun2.get("stef").once(function (data) {
          //Object {_: Object, address: Object} "stef"
          //console.log("**************", data);return;
          done.s = true;
          expect(data.name).to.be("Stef");
          expect(data.address).to.be.ok();
          if (!done.a) {
            return;
          }
          if (done.c) {
            return;
          }
          done.c = 1;
          nopasstun(done, gun2);
        });
      }, 1200);
    });

    it("get get get any parallel", function (done) {
      Zen.statedisk(
        {
          bob: {
            age: 29,
            name: "Bob!",
          },
        },
        "parallel",
        function () {
          zen
            .get("parallel")
            .get("bob")
            .get("age")
            .get(function (data, field) {
              //console.log("*****", field, data);return;
              expect(data).to.be(29);
              expect(field).to.be("age");
              done.age = true;
            });
          //console.log("-----------------------");
          zen
            .get("parallel")
            .get("bob")
            .get("name")
            .get(function (data, field) {
              //console.log("***********", field, data);return;
              expect(data).to.be("Bob!");
              expect(field).to.be("name");
              done.named = true;
              expect(done.age).to.be.ok();
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            });
        },
      );
    });

    it("get get get any later", function (done) {
      Zen.statedisk(
        {
          bob: { _: { "#": "ddfsa" }, age: 29, name: "Bob!" },
        },
        "parallel/later",
        function () {
          zen
            .get("parallel/later")
            .get("bob")
            .get("age")
            .get(function (data, field) {
              //console.log("*****", field, data); return;
              expect(data).to.be(29);
              expect(field).to.be("age");
              done.age = true;
            });
          setTimeout(function () {
            zen
              .get("parallel/later")
              .get("bob")
              .get("name")
              .get(function (data, field) {
                //console.log("***********", field, data); return;
                expect(data).to.be("Bob!");
                expect(field).to.be("name");
                done.named = true;
                expect(done.age).to.be.ok();
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              });
          }, 400);
        },
      );
    });

    it("get get get any not", function (done) {
      zen
        .get("parallel/not")
        .get("bob")
        .get("age")
        .get(function (data, field) {
          //console.log("***** age", data);
          expect(data).to.be(undefined);
          expect(field).to.be("age");
          done.age = true;
        });
      zen
        .get("parallel/not")
        .get("bob")
        .get("name")
        .get(function (data, field) {
          //console.log("*********** name", data);
          expect(data).to.be(undefined);
          expect(field).to.be("name");
          done.named = true;
          expect(done.age).to.be.ok();
          if (done.c) {
            return;
          }
          done.c = 1;
          nopasstun(done, zen);
        });
    });

    it("get get get any not later", function (done) {
      zen
        .get("parallel/not/later")
        .get("bob")
        .get("age")
        .get(function (data, field) {
          //console.log("***** age", data);
          expect(data).to.be(undefined);
          expect(field).to.be("age");
          done.age = true;
        });
      setTimeout(function () {
        //console.debug.i=1;console.log('---------------');
        zen
          .get("parallel/not/later")
          .get("bob")
          .get("name")
          .get(function (data, field) {
            //console.log("*********** name", field, data);
            expect(data).to.be(undefined);
            expect(field).to.be("name");
            done.named = true;
            expect(done.age).to.be.ok();
            if (done.c) {
              return;
            }
            done.c = 1;
            nopasstun(done, zen);
          });
      }, 400);
    });

    it("get any any", function (done) {
      Zen.statedisk(
        {
          hello: "world",
          goodbye: "mars",
        },
        "full",
        function () {
          zen.get("full").get(function (data) {
            //console.log("*****1", data);return;
            expect(data.hello).to.be("world");
            expect(data.goodbye).to.be("mars");
          });
          zen.get("full").get(function (data) {
            //console.log("*****2", data);return;
            expect(data.hello).to.be("world");
            expect(data.goodbye).to.be("mars");
            if (done.c) {
              return;
            }
            done.c = 1;
            nopasstun(done, zen);
          });
        },
      );
    });

    it("get any any later", function (done) {
      Zen.statedisk(
        {
          hello: "world",
          goodbye: "mars",
        },
        "full/later",
        function () {
          zen.get("full/later").get(function (data) {
            //console.log("*****", data);
            expect(data.hello).to.be("world");
            expect(data.goodbye).to.be("mars");
          });
          setTimeout(function () {
            zen.get("full/later").get(function (data) {
              //console.log("*****2", data);
              expect(data.hello).to.be("world");
              expect(data.goodbye).to.be("mars");
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            });
          }, 400);
        },
      );
    });

    it("multiple times map", function (done) {
      var zen = Zen();

      zen.get("A").put({
        B: {
          C: {
            pub: "fdsa",
            y: "mark",
          },
        },
        timber: {
          asdf: {
            pub: "asdf",
            name: "timber",
          },
        },
      });

      var check = { on: {}, once: {} };
      setTimeout(function () {
        //console.log('A=2, map=7, map.map=8, A.B=3, AB=5, A.B.C=4, ABC=6, AB.C=?', zen);
        zen
          .get("A")
          .map()
          .map()
          .on(function (data, key) {
            //console.log(111111, key, data);
            check.on[data.pub] = true;
          });
      }, 900);

      setTimeout(function () {
        zen
          .get("A")
          .map()
          .map()
          .once(function (data, key) {
            //console.log(22222, key, data);
            check.once[data.pub] = true;
            //console.log(check);
            if (
              check.on["asdf"] &&
              check.on["fdsa"] &&
              check.once["asdf"] &&
              check.once["fdsa"]
            ) {
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            }
          });
      }, 1200);
    });

    it("many maps with @rogowski!", function (done) {
      var zen = Zen();
      var data = {
        levelA2: { levelA3: { levelA4: { levelA5: "hello" } } },
        levelB2: { levelB3: { levelB4: { levelB5: "world" } } },
      };

      Zen.statedisk(
        data,
        "level1",
        function () {
          var check = {},
            log = function () {} || console.log;
          zen.get("level1").on(function (v, k) {
            log(1, k, v);
            check[k] = v;
          });
          zen
            .get("level1")
            .map()
            .on(function (v, k) {
              log("*", 2, k, v);
              check[k] = v;
            });
          zen
            .get("level1")
            .map()
            .map()
            .on(function (v, k) {
              log("*", 3, k, v);
              check[k] = v;
            });
          zen
            .get("level1")
            .map()
            .map()
            .map()
            .on(function (v, k) {
              log("*", 4, k, v);
              check[k] = v;
              check.to =
                check.to ||
                setTimeout(function () {
                  expect(check.level1.levelA2).to.eql({
                    "#": "level1/levelA2",
                  });
                  expect(check.level1.levelB2).to.eql({
                    "#": "level1/levelB2",
                  });

                  expect(check.levelA2.levelA3).to.eql({
                    "#": "level1/levelA2/levelA3",
                  });
                  expect(check.levelA3.levelA4).to.eql({
                    "#": "level1/levelA2/levelA3/levelA4",
                  });
                  expect(check.levelA4.levelA5).to.be("hello");

                  expect(check.levelB2.levelB3).to.eql({
                    "#": "level1/levelB2/levelB3",
                  });
                  expect(check.levelB3.levelB4).to.eql({
                    "#": "level1/levelB2/levelB3/levelB4",
                  });
                  expect(check.levelB4.levelB5).to.be("world");

                  nopasstun(done, zen);
                }, 250);
            });
          zen
            .get("level1")
            .map()
            .map()
            .map()
            .map()
            .on(function (v, k) {
              log("*", 5, k, v);
              check[k] = v;
            }); // see test below
        },
        1000,
      );
    });

    it("many maps with @rogowski last value map", function (done) {
      var zen = Zen();
      var data = {
        levelA2: { levelA3: { levelA4: { levelA5: "hello" } } },
        levelB2: { levelB3: { levelB4: { levelB5: "world" } } },
      };

      Zen.statedisk(
        data,
        "level1p",
        function () {
          var check = {},
            log = function () {} || console.log;
          //console.log("======================");
          zen
            .get("level1p")
            .map()
            .map()
            .map()
            .map()
            .on(function (v, k) {
              log("level5", k, v);
              check[k] = v;
              check.to =
                check.to ||
                setTimeout(function () {
                  expect(check.levelA5).to.be("hello");
                  expect(check.levelB5).to.be("world");
                  nopasstun(done, zen);
                }, 20);
            });
        },
        1000,
      );
    });

    it("multiple map test with @rogowski!", function (done) {
      var check = {};
      var gun1 = Zen();
      gun1.get("mmA").put({
        B: {
          C: {
            pub: "fdsa",
            y: "mark",
          },
        },
        timber: {
          asdf: {
            pub: "asdf",
            name: "timber",
          },
        },
      });
      //console.only.i=1;console.log("--------------------", gun1._.next);
      gun1
        .get("mmA")
        .map()
        .map()
        .on(function (data, has) {
          check[has + 1] = data;
          //console.log('first test ONLY get called with C/asdf = pub:fdsa/pub:asdf......', has, data);
        });

      setTimeout(function () {
        var gun2 = Zen();
        gun2.get("2mmA").put({
          nest: {
            B: {
              C: {
                pub: "fdsa",
                y: "mark",
              },
            },
            timber: {
              asdf: {
                pub: "asdf",
                name: "timber",
              },
            },
          },
        });
        //console.only.i=101;console.log("------------------");
        //console.log("CHAIN ID: 2mma = 2, 2mmA.nest = 3, map=4, map.map=5, 2mmaNBC=17, 2mma.nest.b.c=8, 2mmanest=9, 2mmanest.b=11", gun2._);

        gun2
          .get("2mmA")
          .get("nest")
          .map()
          .map()
          .on(function (data, has) {
            check[has + 2] = data;
            //console.log('should log pub=fdsa/asdf....', has, data);
            check.to =
              check.to ||
              setTimeout(function () {
                if (
                  check.C1.pub === "fdsa" &&
                  check.C1.y === "mark" &&
                  check.asdf1.pub === "asdf" &&
                  check.asdf1.name === "timber" &&
                  check.C2.pub === "fdsa" &&
                  check.C2.y === "mark" &&
                  check.asdf2.pub === "asdf" &&
                  check.asdf2.name === "timber"
                ) {
                  nopasstun(done, gun2);
                }
              }, 150);
          });
      }, 70);
    });

    it("multiple times", function (done) {
      var zen = Zen();
      var app = zen.get("mult/times");

      app
        .get("alias")
        .get("mark")
        .set(
          zen.get("ASDF").put({
            pub: "ASDF",
            alias: "mark",
            born: 1,
          }),
        );

      zen.get("mult/times").get("alias").map().map().get("pub").on(foo);
      function foo(data) {
        done.one = done.one || data;
        if (!done.one || !done.two) {
          return;
        }
        expect(done.one).to.be("ASDF");
        expect(done.two).to.be("mark");
        if (done.c) {
          return;
        }
        done.c = 1;
        nopasstun(done, zen);
      }

      setTimeout(function () {
        app
          .get("alias")
          .map()
          .map()
          .get("alias")
          .on(function (data) {
            done.two = data;
            foo();
          });
      }, 10);
    });

    it("multiple times partial", function (done) {
      Zen.statedisk(
        {
          alias: {
            mark: {
              pub: { _: { "#": "PUB" }, pub: "asdf", alias: "mark", born: 1 },
            },
          },
        },
        "mult/times/part",
        function () {
          var zen = Zen();
          var app = zen.get("mult/times/part");

          //console.debug.i=1;console.log("===================");
          // 			app.get('alias').get('mark').once(function(alias){
          // 				console.log("***111 ", alias);
          // 				done.alias = alias;
          // 			});
          app
            .get("alias")
            .get("mark")
            .map()
            .once(function (alias) {
              // 				console.log("***>>>> ", alias);
              //         if (typeof alias !== 'undefined') {
              done.alias = alias;
              //         }
            });

          setTimeout(function () {
            app
              .get("alias")
              .map()
              .map()
              .get("born")
              .on(function (data) {
                // 					console.log("*******", data);
                expect(data).to.be(1);
                expect(done.alias.pub).to.be("asdf");
                expect(done.alias.alias).to.be("mark");
                expect(done.alias.born).to.be(1);
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              });
          }, 400);
        },
        1000,
      );
    });

    it("put on a put", function (done) {
      try {
        var zen = Zen();
        var foo = zen.get("put/on/put").get("a").get("b");
        var bar = zen.get("put/on/put/ok").get("a").get("b");

        bar.put({ a: 1 });
        //console.log("vvvvvvvvv");
        bar.on(function (data) {
          //console.log("***", data);
          if (1 === data.a && 3 === data.c) {
            if (done.c) {
              return;
            }
            done.c = 1;
            //console.log("-------");
            nopasstun(done, zen);
          }
        });

        foo.on(function (ack) {
          //console.log("*", ack);
          bar.put({ c: 3 });
        });
        foo.put({ b: 2 });
      } catch (e) {
        console.log("!!!!!!!!!!!", e);
      }
    });

    it("map with map function", function (done) {
      var zen = Zen(),
        s = "map/mapfunc",
        u;
      var app = zen.get(s);
      var list = app.get("list");

      var check = {};
      list
        .map(function (user) {
          /*console.log("****", user);*/ return user.age === 27
            ? user.name + "thezombie"
            : u;
        })
        .on(function (data) {
          //console.log('+++++', data);
          check[data] = true;
          if (check.alicethezombie && check.bobthezombie) {
            if (done.c) {
              return;
            }
            done.c = 1;
            setTimeout(function () {
              nopasstun(done, zen);
            }, 99);
          }
        });
      //console.debug.i=1;console.log("--------------");
      list.set({ name: "alice", age: 27 }); // on put, table-scan flag doesn't get set, but is needed for initial!??
      list.set({ name: "bob", age: 27 });
      list.set({ name: "carl", age: 29 });
      list.set({ name: "dave", age: 25 });
    });

    it("once map function once", function (done) {
      var zen = Zen(),
        s = "o/mf/o",
        u;
      var app = zen.get(s);
      var list = app.get("list");

      var check = {};
      zen
        .get("useromo")
        .get("alice")
        .put({ name: "Alice", email: "alice@example.com" });
      zen
        .get("useromo")
        .get("bob")
        .put({ name: "Bob", email: "bob@example.com" });
      zen
        .get("useromo")
        .get("carl")
        .put({ name: "Carl", email: "carl@example.com" });

      zen
        .get("useromo")
        .once()
        .map((v) => {
          //console.log('this gets called', v);
          return v;
        })
        .once((v, k) => {
          //console.log('this is never called', k, v);
          check[k] = (check[k] || 0) + 1;
          if (1 === check.alice && 1 === check.bob && 1 === check.carl) {
            if (done.c) {
              return;
            }
            done.c = 1;
            setTimeout(function () {
              nopasstun(done, zen);
            }, 9);
          }
        });
    });

    it("val and then map", function (done) {
      this.timeout(5000);
      var zen = Zen(),
        s = "val/then/map",
        u;
      var list = zen.get(s);

      list.set(zen.get("alice").put({ name: "alice", age: 27 }));
      list.set(zen.get("bob").put({ name: "bob", age: 27 }));
      list.set(zen.get("carl").put({ name: "carl", age: 29 }));
      list.set(zen.get("dave").put({ name: "dave", age: 25 }));

      var check = {};
      list
        .once()
        .map()
        .on(function (data, key) {
          check[key] = data;
          clearTimeout(done.to);
          //console.log("*****", key, data);
          done.to = setTimeout(function () {
            if (
              check.alice &&
              check.bob &&
              check.carl &&
              check.dave &&
              done.last
            ) {
              expect(check.alice.age).to.be(27);
              expect(check.bob.age).to.be(28);
              expect(check.carl.age).to.be(29);
              expect(check.dave.age).to.be(25);
              expect(check.eve).to.not.be.ok();
              if (done.c) {
                return;
              }
              done.c = 1;
              setTimeout(function () {
                nopasstun(done, zen);
              }, 9);
            }
          }, 600);
        });
      setTimeout(function () {
        list.set(zen.get("eve").put({ name: "eve", age: 30 }));
        zen.get("bob").get("age").put(28);
        done.last = true;
      }, 300);
    });

    it("check null on map", function (done) {
      var list = zen.get("myList");
      list.map(function (value, id) {
        if ("hello world" === value) {
          done.one = true;
        }
        if (null === value) {
          done.two = true;
        }
        if (done.one && done.two) {
          if (done.c) {
            return;
          }
          done.c = 1;
          nopasstun(done, zen);
        }
      });
      list.get("message").put("hello world"); // outputs "message: hello world"
      list.get("message").put(null); // throws Uncaught TypeError: Cannot read property '#' of null
    });

    it("Check multi instance message passing", function (done) {
      this.timeout(10000);
      // NOTICE: The behavior of this test changed from v0.2020.520 to version after.
      var stopped = false;
      var optHook = Zen.on("opt", function (ctx) {
        ctx.on("out", function (msg) {
          this.to.next(msg);
          if (stopped) {
            return;
          }
          var onGun = ctx;
          var tmp = {};
          Object.keys(msg).forEach(function (k) {
            tmp[k] = msg[k];
          });
          delete tmp.out;
          delete tmp._;
          msg = tmp; // copy message.
          if (onGun.$ === b || onGun.$ === (b && b._graph)) {
            if (d) {
              //console.log("b can send to d....", JSON.parse(JSON.stringify(msg)));
              msg.$ = d;
              d.on("in", msg);
            }
          } else if (onGun.$ === d || onGun.$ === (d && d._graph)) {
            //console.log("d sends to b....", JSON.parse(JSON.stringify(msg)));
            msg.$ = b;
            b.on("in", msg);
          }
        });
      });

      var b = Zen({ radisk: false, peers: [], localStorage: false });
      var d = null;

      var bb = b.get("key");
      bb.put({ msg: "hello" });

      d = Zen({ radisk: false, peers: [], localStorage: false });
      var db = d.get("key");
      db.map().on(function (val, field) {
        expect(val).to.be("hello");
        if (done.c) {
          return;
        }
        done.c = 1;
        setTimeout(function () {
          stopped = true;
          try {
            optHook.off();
          } catch (e) {}
          done();
        }, 500);
      });
    });

    it("val should now get called if no data is found", function (done) {
      var zen = Zen();

      zen
        .get("nv/foo")
        .get("bar")
        .get("baz")
        .once(function (val, key) {
          //console.log('*******', key, val);
          expect(val).to.be(undefined);
          done.fbb = true;
        });

      zen.get("nv/totesnothing").once(function (val, key) {
        //console.log('***********', key, val);
        expect(val).to.be(undefined);
        done.t = true;
      });

      zen
        .get("nv/bz")
        .get("lul")
        .once(function (val, key) {
          //console.log('*****************', key, val);
          expect(val).to.be(undefined);
          done.bzl = true;
          setTimeout(function () {
            if (done.fbb && done.t && done.bzl) {
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            }
          }, 100);
        });
    });

    it("Callbacks should have database safe data copies", function (done) {
      var zen = Zen();

      zen.get("ds/safe").put({ a: 1 });

      zen.get("ds/safe").on(function (data) {
        data.b = 2;
      });

      zen.get("ds/safe").once(function (data) {
        expect(zen.back(-1)._.graph["ds/safe"].b).to.not.be.ok();
        if (done.c) {
          return;
        }
        done.c = 1;
        nopasstun(done, zen);
      });
    });

    it("If chain cannot be called, ack", function (done) {
      this.timeout(9000);
      Zen.statedisk(
        { wat: 1, a: true },
        "nl/app",
        function () {
          var zen = Zen(),
            u;
          var app = zen.get("nl/app");

          app.get(
            function (d) {
              if (done.a) {
                return;
              }
              d = (d.$$ || d.$)._.put;
              //console.log('*', d);
              expect(d.wat).to.be(1);
              expect(d.a).to.be(true);
              done.a = 1;
              app
                .get("a")
                .get("b")
                .get(
                  function (d) {
                    d = (d.$$ || d.$)._.put;
                    //console.log('****', d);
                    expect(d).to.be(u);
                    done.b = (done.b || 0) + 1;
                    if (done.phase) {
                      return;
                    }
                    done.phase = 1;
                    setTimeout(function () {
                      // adding more rigorous test!
                      //console.log("------------");
                      app
                        .get("a")
                        .get("b")
                        .get(
                          function (d) {
                            d = (d.$$ || d.$)._.put;
                            //console.log('****::::', d);
                            expect(done.b).to.be(1);
                            done.c = (done.c || 0) + 1;
                          },
                          { v2020: 1 },
                        );

                      app.get("a").put("lol");

                      setTimeout(function () {
                        expect(done.c).to.be(1);
                        if (done.d) {
                          return;
                        }
                        done.d = 1;
                        nopasstun(done, zen);
                      }, 50);
                    }, 50);
                  },
                  { v2020: 1 },
                );
            },
            { v2020: 1 },
          );
        },
        1000,
      );
    });

    it("Chain on known nested object should ack", function (done) {
      Zen.statedisk(
        { bar: { wat: 1 } },
        "nl/app",
        function () {
          var zen = Zen(),
            u;
          var app = zen.get("nl/app").get("bar");

          app.get(function (d) {
            if (!d || !d.wat) {
              return;
            }
            //console.log('************ should be called: {wat:1}=', d);
            expect(d.wat).to.be(1);
            done.a = 1;
            if (!done.u) {
              return;
            }
            expect(done.u).to.be.ok();
            if (done.c) {
              return;
            }
            done.c = 1;
            nopasstun(done, zen);
          }); //, {v2020:1});

          //console.log("----------");
          app
            .get("a")
            .get("b")
            .get(
              function (d) {
                //console.log("************ empty/clear: undefined=", d.put);
                //d = (d.$$||d.$)._.put;
                expect(d.put).to.be(u);
                done.u = true;
                if (!done.a) {
                  return;
                }
                expect(done.a).to.be.ok();
                if (done.c) {
                  return;
                }
                done.c = 1;
                nopasstun(done, zen);
              },
              { v2020: 1 },
            );
        },
        1000,
      );
    });

    it("Soul above but not beneath", function (done) {
      this.timeout(5000);
      var zen = Zen();
      var a = zen.get("sabnb");
      a.get("profile").put({ _: { "#": "sabnbprofile" }, name: "Plum" });

      setTimeout(function () {
        a.get("profile").get("said").get("asdf").put("yes");
        setTimeout(function () {
          a.once(function (data) {
            expect(data.profile).to.be.eql({ "#": "sabnbprofile" });
            if (done.c) {
              return;
            }
            done.c = 1;
            nopasstun(done, zen);
          });
        }, 100);
      }, 100);
    });

    it("users map map who said map on", function (done) {
      this.timeout(1000 * 9);
      var zen = Zen();

      zen.get("users/mm").put({
        alice: {
          _: { "#": "alias/alice" },
          "pub/asdf": { _: { "#": "pub/asdf" }, pub: "asdf" },
        },
        bob: {
          _: { "#": "alias/bob" },
          "pub/fdsa": { _: { "#": "pub/fdsa" }, pub: "fdsa" },
        },
      });

      var check = {},
        c = 0,
        end,
        queued = false,
        sub;
      // `check` is transiently empty between writes — a key is only added just
      // before each set() — so "empty" alone does not mean the run finished.
      // Wait for every write to be issued too, and let done() fire only once:
      // a late emission after an early finish used to call it a second time.
      var settle = function () {
        clearTimeout(end);
        end = setTimeout(function () {
          //console.log("?", c, check, Object.keys(check), zen._.graph);
          if (!queued || !Object.empty(check)) {
            return;
          } //if(Zen.obj.map(check, function(v){ if(v){ return v } })){ return }
          if (done.c) {
            return;
          }
          done.c = 1;
          if (sub && sub.off) {
            sub.off();
          }
          nopasstun(done, zen);
        }, 9);
      };
      //console.log(check, zen._.graph);
      zen
        .get("users/mm")
        .map()
        .map()
        .get("who")
        .get("said")
        .map()
        .on(function (msg) {
          //console.log("------>", msg);
          if (check[msg.num]) {
            //console.log("!!!!", msg.num, "!!!!");
          }
          sub = this;
          delete check[msg.num];
          c++;
          settle();
        });

      var said = zen.get("pub/asdf").get("who").get("said");

      function run(i) {
        //if(i > 1){ return } // DEBUGGING!
        //console.log("----", i, "----");
        //2 === i && (console.only.i = 1) && console.only(1, '==========');
        //(console.debug.i = console.debug.i || 1);
        said.set({
          what: i + " Hello world!",
          num: i,
          who: "asdf",
          id: "alice",
        });
      }

      var i = 0,
        m = 9,
        to = setTimeout(function frame() {
          if (m <= i) {
            queued = true;
            clearTimeout(to);
            settle(); // in case every emission already landed before this point
            return;
          }
          i++;
          check[i] = true;
          run(i);
          setTimeout(frame, 1);
        }, 1);
    });

    it("get map should not slowdown", function (done) {
      this.timeout(9000);
      var zen = Zen({ test_no_peer: true }).get("g/m/no/slow");
      //console.log("---------- setup data done -----------");
      var prev,
        diff,
        max = 500,
        total = 500,
        largest = -1,
        gone = {},
        u;
      //var prev, diff, max = Infinity, total = 10000, largest = -1, gone = {};
      // TODO: It would be nice if we could change these numbers for different platforms/versions of javascript interpreters so we can squeeze as much out of them.
      var hist = zen.get("history").map();
      var slowFailed = false;
      hist.on(function (time, index) {
        if (slowFailed) {
          return;
        }
        diff = +new Date() - time;
        //console.log(">>>", index, time, diff);//return;
        var err = null;
        if (gone[index]) {
          err = new Error("duplicate index " + index);
        }
        gone[index] = diff;
        largest = largest < diff ? diff : largest;
        if (!err && diff > max) {
          err = new Error(
            "diff " + diff + "ms > max " + max + "ms at index " + index,
          );
        }
        if (err) {
          slowFailed = true;
          hist.off();
          clearTimeout(many);
          if (!done.c) {
            done.c = 1;
            done(err);
          }
        }
      });
      //console.only.i=1;
      var turns = 0;
      var many = setTimeout(function go() {
        // TODO: NOTE: BUG? using setInterval caused poor CPU scheduling that did fail this test, it is possible actual apps might use that approach even tho for now they should use this adjusted version, so maybe we need to review a test in future for that?
        if (done.c) {
          return;
        } // test already finished (passed or failed)
        if (turns > total || (diff || 0) > max + 5) {
          if (u === diff) {
            return;
          }
          clearTimeout(many);
          hist.off();
          if (done.c) {
            return;
          }
          done.c = 1;
          if ("number" !== typeof diff) {
            done(new Error("diff is not a number"));
            return;
          }
          nopasstun(done, zen);
          return;
        }
        prev = +new Date();
        var put = {};
        put[(turns += 1)] = prev;
        zen.put({ history: put });
        many = setTimeout(go, 1); // see above NOTE, increasing total runs to compensate.
      }, 1);
    });

    it("Check put callback", function (done) {
      var zen = Zen();

      zen
        .get("c/p/c")
        .get("a")
        .put("lol", function (ack) {
          nopasstun(done, zen);
        });
    });

    it("Resume reads after put error", function (done) {
      var zen = Zen();

      zen.get("r/r/a/p/e").put({ a: 1 });
      setTimeout(function () {
        zen
          .get("r/r/a/p/e")
          .get("a")
          .get("b")
          .get("c")
          .put([], function (ack) {
            expect(ack.err).to.be.ok();
            expect(ack.err.toLowerCase().indexOf("array") >= 0).to.be.ok();
            expect(ack.err.toLowerCase().indexOf("a.b.c") >= 0).to.be.ok();
            setTimeout(function () {
              zen.get("r/r/a/p/e").once(function (data) {
                expect(data.a).to.be(1);
                //console.log("data!", data);
                nopasstun(done, zen);
              });
            }, 50);
          });
      }, 100);
    });

    it("Multiple subscribes should trigger", function (done) {
      // thanks to @ivkan for reporting and providing test.
      var zen = Zen();
      var check = {};
      zen.get("m/s/key").put({ property: "value" });

      var subA = zen.get("m/s/key").on(function (data, key) {
        check["a" + data.property] = 1;
      });

      var subB = zen.get("m/s/key").on(function (data, key) {
        check["b" + data.property] = 1;
        if (
          check.avalue &&
          check.bvalue &&
          check.anewValue &&
          check.bnewValue
        ) {
          if (done.c) {
            return;
          }
          done.c = true;
          subA.off();
          subB.off();
          nopasstun(done, zen);
        }
      });

      setTimeout(function () {
        zen.get("m/s/key").put({ property: "newValue" });
      }, 1000);
    });

    it("Deep puts with peer should work", function (done) {
      // tests in async mode now automatically connect to localhost peer.
      //var zen = Zen('http://localhost:8420/zen');
      var zen = Zen();
      //var user = zen.user();
      //user.create('alice', 'password', function(){
      zen
        .get("who")
        .get("all")
        .put({ what: "hello world!", when: Zen.state() }, function (ack) {
          //user.get('who').get('all').put({what: "hello world!", when: Zen.state()}, function(ack){
          zen
            .get("who")
            .get("all")
            .once(function (data) {
              expect(data.what).to.be.ok();
              expect(data.when).to.be.ok();
              nopasstun(done, zen);
            });
        });
      //});
    });

    it("Set a ref should be found", function (done) {
      var zen = Zen();
      var msg = { what: "hello world" };
      //var ref = user.get('who').get('all').set(msg);
      //user.get('who').get('said').set(ref);
      var ref = zen
        .get("s/r/who")
        .get("all")
        .set(msg, function (ack) {
          /*console.log('@@@', ack)*/
        });
      zen
        .get("s/r/who")
        .get("said")
        .set(ref, function (ack) {
          /*console.log('###', ack)*/
        });
      zen
        .get("s/r/who")
        .get("said")
        .map()
        .once(function (data) {
          expect(data.what).to.be.ok();
          nopasstun(done, zen);
        });
    });

    /*describe('talk to live server tests', function(){
			this.timeout(1000 * 9);
			it.only('Second once on undefined should call', function(done){ // this test is passing when it fails by hand?
				var zen = Zen('https://gunjs.herokuapp.com/zen');
				zen.get('~@O8H2BJa4pNfecWamWN7efd888Pg1@hackernoon').once(function(data){
					console.log(1, data);
					expect(data).to.not.be.ok();
					setTimeout(function(){
						zen.get('~@O8H2BJa4pNfecWamWN7efd888Pg1@hackernoon').once(function(data){
							console.log(2, data);
							expect(data).to.not.be.ok();
							done();
						});
					}, 3000);
				});
			});
		});*/
    it("Nested listener should be called", function (done) {
      var zen = Zen();
      /*
			var app = zen.get('nl/app').get('bar');

			app.on(function(d){
				console.log("!!", d);
			})

			app.put({wat: 1});

			console.debug.i=1;console.log("------------");
			console.log(zen._.now);
			app.put({a: {b:2}});
			console.log('_______________________');
			return;*/

      var app = zen.get("nl/app");
      var node = app.get("watcher/1").put({ stats: { num: 3 }, name: "trex" });
      var C = 0;

      app
        .get("watcher/1")
        .get("stats")
        .on(function (v, k) {
          if (++C === 1) {
            expect(v.num).to.be(3);
            // Trigger the update only after the first notification fires,
            // to avoid a race with slow disk I/O on CI machines.
            setTimeout(function () {
              app.get("watcher/1").put({ stats: { num: 4 }, name: "trexxx" });
            }, 50);
            return;
          }
          expect(v.num).to.be(4);
          if (done.c) {
            return;
          }
          done.c = 1;
          this.off();
          nopasstun(done, zen);
        });
      //return;
    });
    //return;

    /*it.skip('Memory management', function(done){
			this.timeout(9999999);
			var zen = Zen(), c = 100000, big = "big";
			while(--c){big += "big"}
			c = 0;
			setInterval(function(){
				var key = Zen.text.random(5);
				zen.get(key).put({data: big});
				setTimeout(function(){
					zen.get(key).off();
				},10);
				if(typeof process === 'undefined'){ return }
				var mem = process.memoryUsage();
				console.log(((mem.heapUsed / mem.heapTotal) * 100).toFixed(0) + '% memory');
				console.log(Object.keys(zen._.graph).length, 'item in memory graph:', Object.keys(zen._.graph));
			},25);
		});

		it('Custom extensions are chainable', function(done){
			Zen.chain.filter = function(filter){
			  var chain = this.chain();
			  var context = this;
			  var _tags;
			  context.once(function(obj, key){
			    if(!obj.tags){
			      console.warn('Not tagged to anything!');
			      context._.valid = false;
			      chain._.on('in', {get: key, $: this});
			      return false;
			    } else { 
			     _tags = Zen.obj.ify(obj.tags);
			      if(Array.isArray(filter)){
			        context._.valid = filter.every(function(f){ return ( _tags[f] && _tags[f]==1) });
			        if(context._.valid){
			          chain._.on('in', {get: key, put: obj, $: this});
			          return context;
			        } else {
			          console.log("that was wrong");
			          chain._.on('in', {get: key, put: undefined, $: this});
			        }
			        return false;
			      } else {
			        console.warn('filter should be an Array');
			        return false;
			      }
			    }
			  });
			  return chain;
			}

			var zen = Zen();

			var fake1 = zen.get('fake1').put({name:'faker1',tags:JSON.stringify({a:1,b:0,c:1})});
			var fake2 = zen.get('fake2').put({name:'faker2',tags:JSON.stringify({a:1,b:1,c:1})});
			var list = zen.get('list');
			list.set(fake1);
			list.set(fake2);

			zen.get('fake1')//.map()
			      .filter(['a','b'])  // Zen.chain.filter = function(tags){ .... }
			      .get(function(no){console.log("NO!", no)})
			      .once(function(yes){console.log("YES!", yes)})
		}); */

    it("Check that events are called with multiple instances", function (done) {
      var gunA = Zen({ file: "tmp/A.json" });
      var gunB = Zen({ file: "tmp/B.json" });
      var gunC = Zen({ file: "tmp/C.json" });

      var check = {};

      gunA.get("some path A").map(function (v, f) {
        check.A = v; /*console.log( "event on A: ", f, v )*/
      });
      gunB.get("some path B").map(function (v, f) {
        check.B = v; /*console.log( "event on B: ", f, v )*/
      });
      gunC.get("some path C").map(function (v, f) {
        check.C = v; /*console.log( "event on C: ", f, v )*/
      });

      gunA.get("some path A").put({ simple: "message" });
      gunB.get("some path B").put({ simple: "message" });
      gunC.get("some path C").put({ simple: "message" });
      setTimeout(function () {
        expect(check.A).to.be("message");
        expect(check.B).to.be("message");
        expect(check.C).to.be("message");
        nopasstun(0, gunA);
        nopasstun(0, gunB);
        nopasstun(done, gunC);
      }, 100);
    });

    it("ack aggregation bypass", function (done) {
      var alice = new ZEN({
        localStorage: false,
        file: false,
        rad: false,
        radisk: false,
      });
      var carl = new ZEN({
        localStorage: false,
        file: false,
        rad: false,
        radisk: false,
      });

      var amesh = alice._.opt.mesh;
      var cmesh = carl._.opt.mesh;
      var croot = carl._;

      // Wire alice <-> carl with proper peer objects so ack routing works
      var c2a = {
        wire: {
          send: function (raw) {
            amesh.hear(raw, a2c);
          },
        },
      };
      var a2c = {
        wire: {
          send: function (raw) {
            cmesh.hear(raw, c2a);
          },
        },
      };
      amesh.hi(a2c);
      cmesh.hi(c2a);

      // Prepend an "in" listener on carl that intercepts incoming puts and
      // replies with a custom BANANA ack using the original ask ID
      var inTag = croot.tag["in"];
      var interceptor = {
        next: function (msg) {
          if (msg.put && msg._.via) {
            var askId = msg["#"];
            setTimeout(function () {
              croot.on("out", { "@": String(askId), ok: { BANANA: 9 } });
            }, 10);
          }
          if (this.to) {
            this.to.next(msg);
          }
        },
        to: inTag.to,
        back: inTag,
        the: inTag,
        on: croot,
      };
      inTag.to = interceptor;

      alice.get("test").put(
        { a: 1, b: 2, c: 3 },
        function (ack) {
          if (ack.ok && ack.ok.BANANA) {
            done();
          }
        },
        { acks: 99 },
      );
    });

    /*it.only('Make sure circular contexts are not copied', function(done){
			//let's define an appropriate deep default database...
			var dfltSansUsers = { 1: { name : "org1", sites : { 1: {name : "site1"} } } };

			var alice =  {name: "alice" }

			var zen = Zen();

			var root = zen.get( "root" );
			root.put( dfltSansUsers );

			var alice = zen.get( "alice" ).put( { name: "alice" } );
			console.log( "Failed after this" );
			root.get("1").get("sites").get("1").get("users" ).put( { 1: alice } );
			console.log( "Failed before this" );
		});*/

    it("get any any none", function (done) {
      zen.get("full/none").get(function (data) {
        //console.log("*****", data);
        expect(data).to.be(undefined);
      });
      zen.get("full/none").get(function (data) {
        //console.log("*****2", data);
        expect(data).to.be(undefined);
        if (done.c) {
          return;
        }
        done.c = 1;
        nopasstun(done, zen);
      });
    });

    it("get any any none later", function (done) {
      zen.get("full/none/later").get(function (data) {
        //console.log("*****", data);
        expect(data).to.be(undefined);
      });
      setTimeout(function () {
        zen.get("full/none/later").get(function (data) {
          //console.log("*****2", data);
          expect(data).to.be(undefined);
          nopasstun(done, zen);
        });
      }, 400);
    });

    it("get get any parallel", function (done) {
      Zen.statedisk(
        { bob: { age: 29, name: "Bob!" } },
        "parallel/get/get",
        function () {
          zen
            .get("parallel/get/get")
            .get("bob")
            .get(function (data) {
              //console.log("***** 1", data);
              expect(data.age).to.be(29);
              expect(data.name).to.be("Bob!");
            });
          zen
            .get("parallel/get/get")
            .get("bob")
            .get(function (data) {
              //console.log("***** 2", data);
              expect(data.age).to.be(29);
              expect(data.name).to.be("Bob!");
              if (done.c) {
                return;
              }
              done.c = 1;
              nopasstun(done, zen);
            });
        },
        1000,
      );
    });

    it("once on link to nothing @mimiza", function (done) {
      zen.get("oltn").put({ "#": "this-does-not-exist" });

      zen.get("oltn").once((response) => {
        //console.log('did we call?', response) ;
        expect(response).to.not.be.ok();
        nopasstun(done, zen);
      });
    });

    it("once on link to nothing deep @mimiza", function (done) {
      zen.get("oltnd").get("deep").put({ "#": "this-does-not-exist" });

      zen
        .get("oltnd")
        .get("deep")
        .once((response) => {
          //console.log('did we call?', response) ;
          expect(response).to.not.be.ok();
          nopasstun(done, zen);
        });
    });
  });

  describe("localStorage", function () {
    it("err", function (done) {
      this.timeout(9000);
      var localStorage = localStorage || { clear: function () {} };
      localStorage.clear();
      var zen = Zen();
      var text = String.random(1024 * 1024 * 6);
      zen.put({ i: text }, function (ack) {
        var err = ack.err,
          ok = ack.ok;
        if (done.c) {
          return;
        }
        if (!err) {
          return done();
        }
        var text =
          "If you are seeing this message, it means the localStorage error was caught successfully rather than it crashing and stopping replication to peers. Also, the error is now reported back to you via the put callback. Here it is!";
        localStorage.clear();
        done();
        done.c = 1;
      });
    });
    it("ack", function (done) {
      this.timeout(9000);
      var localStorage = localStorage || { clear: function () {} };
      localStorage.clear();
      var zen = Zen();
      var i = 999,
        obj = {};
      while (--i) {
        obj[i] = String.random(99);
      }
      //console.log("save:", obj);
      zen.get("lSack").put(obj, function (ack) {
        //console.log("ack:", ack)
        done();
        done.c = 1;
      });
    });
  });

  describe("Node Links", function () {
    it("put node link then read through link resolves in plain scope", function (done) {
      var g = Zen();
      var target = g.get("target").get("12345");
      target.put("hello world", function (ack) {
        expect(ack.err).to.not.be.ok();
        g.get("plain")
          .get("link")
          .put(target, function (ack2) {
            expect(ack2.err).to.not.be.ok();
            g.get("plain")
              .get("link")
              .once(function (data) {
                expect(data).to.be("hello world");
                done();
              });
          });
      });
    });
    it("slash path resolves same chain as chained get", function (done) {
      var g = Zen();
      var chain1 = g.get("p").get("q");
      var chain2 = g.get("p/q");
      expect(chain1).to.be(chain2);
      done();
    });
    it("read through slash path returns primitive value", function (done) {
      var g = Zen();
      g.get("x")
        .get("y")
        .put(42, function (ack) {
          g.get("x/y").once(function (v) {
            expect(v).to.be(42);
            done();
          });
        });
    });
    it("three-level slash path resolves correctly", function (done) {
      var g = Zen();
      g.get("deep")
        .get("lvl2")
        .get("lvl3")
        .put("hello", function (ack) {
          g.get("deep/lvl2/lvl3").once(function (v) {
            expect(v).to.be("hello");
            done();
          });
        });
    });
    it("link to primitive has-chain resolves via slash path", function (done) {
      var g = Zen();
      var scope = g.get("a").get("b");
      scope.put("hello", function (ack) {
        g.get("link")
          .get("test")
          .put(scope, function (ack2) {
            g.get("link")
              .get("test")
              .once(function (v) {
                expect(v).to.be("hello");
                done();
              });
          });
      });
    });
    it(".on() subscription resolves primitive via slash-path link", function (done) {
      this.timeout(5000);
      var g = Zen();
      var scope = g.get("aOn").get("bOn");
      scope.put("world", function (ack) {
        expect(ack.err).to.not.be.ok();
        g.get("linkOn")
          .get("testOn")
          .put(scope, function (ack2) {
            expect(ack2.err).to.not.be.ok();
            g.get("linkOn")
              .get("testOn")
              .on(function (v) {
                if (v === undefined) {
                  return;
                }
                expect(v).to.be("world");
                this.off();
                done();
              });
          });
      });
    });
  });
});
