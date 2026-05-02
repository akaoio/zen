import "./lib/open.js";
import "./lib/load.js";
import ip from "ip";
import fs from "fs";
import panicmanager from "panic-manager";
import fsrm from "./lib/fsrm";
import nodehttp from "http";
import zenapp from "./index.js";
import ZEN from "./zen";
import panic from "panic-server";
import { fileURLToPath } from "node:url";
import { dirname as dirnameOf } from "node:path";
const filemodname = fileURLToPath(import.meta.url);
const __dirname = dirnameOf(filemodname);

var config = {
  IP: ip.address(),
  port: 8420,
  servers: 4,
  browsers: 0,
  route: {
    "/": __dirname + "/index.html",
    "/zen.js": __dirname + "/../../zen.js",
    "/jquery.js": __dirname + "/../../examples/jquery.js",
  },
  names: ["alice", "bob"],
  keys: {
    PROFILE: "Profile",
    DISPLAY_NAME: "displayName",
    CURRENT_HANDSHAKE_ADDRESS: "currentHandshakeAddress",
    HANDSHAKE_NODES: "handshakeNodes",
    CURRENT_ORDER_ADDRESS: "currentOrderAddress",
    BIO: "bio",
    USER_TO_INCOMING: "userToIncoming",
    RECIPIENT_TO_OUTGOING: "recipientToOutgoing",
    USER_TO_LAST_REQUEST_SENT: "USER_TO_LAST_REQUEST_SENT",
    OUTGOINGS: "outgoings",
    MESSAGES: "messages",
    STORED_REQS: "storedReqs",
  },
  stoppers: {},
};

var testData = Array(config.servers - 2).fill();

const rands = (length) => {
  var result = "";
  var characters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  var charactersLength = characters.length;
  for (var i = 0; i < length; i++) {
    result += characters.charAt(Math.floor(Math.random() * charactersLength));
  }
  return result;
};

panic
  .server()
  .on("request", function (req, res) {
    config.route[req.url] &&
      fs.createReadStream(config.route[req.url]).pipe(res);
  })
  .listen(config.port);

var clients = panic.clients;
var manager = panicmanager();

manager.start({
  clients: Array(config.servers)
    .fill()
    .map(function (u, i) {
      return {
        type: "node",
        port: config.port + (i + 1),
      };
    }),
  panic: "http://" + config.IP + ":" + config.port,
});

var servers = clients.filter("Node.js");
var peer1 = servers.pluck(1);
var peer2 = servers.excluding(peer1).pluck(1);
var peerClients = new panic.ClientList([peer1, peer2]);
var serverClients = servers.excluding(new panic.ClientList([peer1, peer2]));
var alice = serverClients.pluck(1);
var bob = serverClients.excluding(alice).pluck(1);
var again = {};

describe("Shocknet Test!", function () {
  //this.timeout(5 * 60 * 1000);
  this.timeout(10 * 60 * 1000);

  it("Servers have joined!", function () {
    return servers.atLeast(config.servers);
  });

  it("Server Peer initialized zen!", function () {
    var tests = [],
      i = 0;
    peerClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            var env = test.props;
            test.async();
            try {
              fs.unlinkSync(3 + env.i + "data");
            } catch (e) {}
            try {
              fs.unlinkSync(env.i + 1 + 3 + "data");
            } catch (e) {}
            try {
              fsrm(3 + env.i + "data");
            } catch (e) {}
            try {
              fsrm(env.i + 1 + 3 + "data");
            } catch (e) {}
            var port = env.config.port + 3 + env.i;
            var server = nodehttp.createServer(function (req, res) {
              res.end("I am " + (3 + env.i) + "!");
            });
            const peerPort = env.i === 1 ? 5 : 4;
            const peerAddr =
              "http://" +
              env.config.IP +
              ":" +
              (env.config.port + peerPort) +
              "/zen";
            var Zen = zenapp;
            var zen = Zen({
              file: 3 + env.i + "data",
              web: server,
              peers: [peerAddr],
              axe: false,
            });
            console.log(
              "\x1b[32m",
              ` I am peer #${env.i}, I am listening on port ${port} and connecting to ${peerAddr}`,
              "\x1b[0m",
            );
            server.listen(port, function () {
              test.done();
            });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients initialized zen!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            //localStorage.clear();
            global.rands = (length) => {
              var result = "";
              var characters =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
              var charactersLength = characters.length;
              for (var i = 0; i < length; i++) {
                result += characters.charAt(
                  Math.floor(Math.random() * charactersLength),
                );
              }
              return result;
            };
            var env = test.props;
            try {
              fs.unlinkSync(env.i + "data");
            } catch (e) {}
            try {
              fs.unlinkSync(env.i + 1 + "data");
            } catch (e) {}
            try {
              fsrm(env.i + "data");
            } catch (e) {}
            try {
              fsrm(env.i + 1 + "data");
            } catch (e) {}
            var Zen = zenapp;
            const peerAddr =
              "http://" +
              env.config.IP +
              ":" +
              (env.config.port + 3 + env.i) +
              "/zen";
            var zen = Zen({
              peers: [peerAddr],
              file: env.i + "data",
              axe: false,
            });
            console.log(
              "\x1b[32m",
              ` I am client #${env.i}, I am connecting to ${peerAddr}`,
              "\x1b[0m",
            );
            global.zen = zen;
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients auth with zen!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client
          .run(
            function (test) {
              const done = test.async();
              //localStorage.clear();
              var env = test.props;
              //try{ load('zen/lib/fsrm')(env.i+'data') }catch(e){}
              //try{ load('zen/lib/fsrm')((env.i+1)+'data') }catch(e){}
              const user = global.zen.user();
              const alias = env.config.names[env.i - 1] + rands(4);
              const pass = rands(8);

              //return new Promise((res,rej) => {
              user.create(alias, pass, (ack) => {
                if (ack.err) {
                  test.fail("error fail ack:" + JSON.stringify(ack.err));
                  //rej(ack.err)
                } else {
                  user.auth(alias, pass, (ack1) => {
                    if (ack1.err) {
                      test.fail(ack1.err);
                      //rej(ack1.err)
                    } else {
                      global.alias = alias;
                      global.pass = pass;
                      global.user = user;
                      console.log(env.i, alias, pass);
                      //done()
                      done({ i: env.i, pub: user.is.pub, alias: alias });
                    }
                  });
                }
              });
              //})
            },
            { i: (i += 1), config: config },
          )
          .then((res) => {
            testData[res.i - 1] = res;
          }),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients generating ZEN secret!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          async function (test) {
            const done = test.async();
            var env = test.props;
            const ZEN = ZEN;
            const user = global.user;
            return ZEN.secret(user._.zen.pub, user._.zen).then((mySecret) => {
              global.ZEN = ZEN;
              global.mySecret = mySecret;
              done();
            });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients set display name!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            const done = test.async();
            var env = test.props;
            var keys = env.config.keys;
            global.user
              .get(keys.PROFILE)
              .get(keys.DISPLAY_NAME)
              .put(global.alias, (ack) => {
                if (ack.err) {
                  console.log(env.i, ack);
                  test.fail("error fail ack:" + JSON.stringify(ack.err));
                } else {
                  done();
                }
              });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Reading alice display name using .on !", function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        global.zen
          .user(pub)
          .get(keys.PROFILE)
          .get(keys.DISPLAY_NAME)
          .on((dn) => {
            if (!dn.startsWith("alice")) {
              test.fail(`display Name > ${dn}`);
            } else {
              console.log(dn);
              done();
            }
          });
      },
      { pub: testData[0].pub, config: config },
    );
  });

  it("Reading bob display name using .once !", function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        global.zen
          .user(pub)
          .get(keys.PROFILE)
          .get(keys.DISPLAY_NAME)
          .once((dn) => {
            if (!dn.startsWith("bob")) {
              test.fail(`display Name > ${dn}`);
            } else {
              console.log(dn);
              done();
            }
          });
      },
      { pub: testData[1].pub, config: config },
    );
  });
  it("Reading bob display name using .then !", function () {
    return alice.run(
      async function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        const dn = await global.zen
          .user(pub)
          .get(keys.PROFILE)
          .get(keys.DISPLAY_NAME)
          .then();
        if (!dn.startsWith("bob")) {
          test.fail(`display Name > ${dn}`);
        } else {
          console.log(dn);
          done();
        }
      },
      { pub: testData[1].pub, config: config },
    );
  });

  it("Server Clients generate handshake node!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            const done = test.async();
            var env = test.props;
            var keys = env.config.keys;
            var address = rands(24);
            global.user
              .get(keys.CURRENT_HANDSHAKE_ADDRESS)
              .put(address, (ack) => {
                if (ack.err) {
                  test.fail("error fail ack:" + JSON.stringify(ack.err));
                } else {
                  global.user
                    .get(keys.HANDSHAKE_NODES)
                    .get(address)
                    .put({ unused: 0 }, (ack1) => {
                      if (ack.err) {
                        test.fail("error fail ack:" + JSON.stringify(ack.err));
                      } else {
                        done();
                      }
                    });
                }
              });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });
  it("Server Clients generate order address!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            const done = test.async();
            var env = test.props;
            var keys = env.config.keys;
            var address = rands(24);
            global.user.get(keys.CURRENT_ORDER_ADDRESS).put(address, (ack) => {
              if (ack.err) {
                test.fail("error fail ack:" + JSON.stringify(ack.err));
              } else {
                done();
              }
            });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients set bio!", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            const done = test.async();
            const env = test.props;
            const keys = env.config.keys;
            const name = env.config.names[env.i - 1];
            const bio = `Hi my name is ${name}`;
            global.user.get(keys.BIO).put(bio, (ack) => {
              if (ack.err) {
                test.fail("error fail ack:" + JSON.stringify(ack.err));
              } else {
                global.user
                  .get(keys.PROFILE)
                  .get(keys.BIO)
                  .put(bio, (ack) => {
                    if (ack.err) {
                      test.fail("error fail ack:" + JSON.stringify(ack.err));
                    } else {
                      done();
                    }
                  });
              }
            });
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });

  it("Server Clients registering helpers", function () {
    var tests = [],
      i = 0;
    serverClients.each(function (client, id) {
      tests.push(
        client.run(
          function (test) {
            const env = test.props;
            const keys = env.config.keys;
            global.pubToEpub = async (pub, fail) => {
              const epub = await global.zen.user(pub).get("epub").then();
              if (typeof epub === "string") {
                return epub;
              } else {
                fail(`Expected epub ti be string found ${typeof epub}`);
              }
            };
            global.successfulHandshakeAlreadyExists = async (pub) => {
              const { user } = global;
              const maybeIncomingID = await user
                .get(keys.USER_TO_INCOMING)
                .get(pub)
                .then();
              const maybeOutgoingID = await user
                .get(keys.RECIPIENT_TO_OUTGOING)
                .get(pub)
                .then();
              return (
                typeof maybeIncomingID === "string" &&
                typeof maybeOutgoingID === "string"
              );
            };
          },
          { i: (i += 1), config: config },
        ),
      );
    });
    return Promise.all(tests);
  });
  var handshakeRequestID;
  const clenUp = (name, testClient, data) => {
    it(`Sending handshake request to ${name} 1: cleanup 1:USER_TO_INCOMING !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;

        global.hsData = {};

        user
          .get(keys.USER_TO_INCOMING)
          .get(pub)
          .put(null, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      }, data);
    });
    it(`Sending handshake request to ${name} 1: cleanup 2:RECIPIENT_TO_OUTGOING !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        user
          .get(keys.RECIPIENT_TO_OUTGOING)
          .get(pub)
          .put(null, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      }, data);
    });
    it(`Sending handshake request to ${name} 1: cleanup 3:USER_TO_LAST_REQUEST_SENT !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        user
          .get(keys.USER_TO_LAST_REQUEST_SENT)
          .get(pub)
          .put(null, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      }, data);
    });
    it(`Sending handshake request to ${name} 1: cleanup 4:recipientToOutgoingID !`, function () {
      return testClient.run(async function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        const maybeEncryptedOutgoingID = await global.user
          .get(keys.RECIPIENT_TO_OUTGOING)
          .get(pub)
          .then();
        if (maybeEncryptedOutgoingID === "string") {
          global.hsData.maybeEncryptedOutgoingID = maybeEncryptedOutgoingID;
          /*global.ZEN.decrypt(maybeEncryptedOutgoingID,global.mySecret)
						.then(outgoingID => outgoingID ? res(outgoingID) : rej(`expected outgoingID to be exist`))*/
        } else {
          global.hsData.outgoingID = maybeEncryptedOutgoingID;
        }
        done();
      }, data);
    });
    it(`Sending handshake request to ${name} 1: cleanup 5:decrypt recipientToOutgoingID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        const { maybeEncryptedOutgoingID } = global.hsData;

        if (maybeEncryptedOutgoingID === "string") {
          global.hsData.maybeEncryptedOutgoingID = maybeEncryptedOutgoingID;
          global.ZEN.decrypt(maybeEncryptedOutgoingID, global.mySecret).then(
            (outgoingID) => {
              if (outgoingID) {
                global.hsData.outgoingID = outgoingID;
                done();
              } else {
                test.fail(`expected outgoingID to exist`);
              }
            },
          );
        } else {
          done();
        }
      }, data);
    });
    it(`Sending handshake request to ${name} 1: cleanup 6:OUTGOINGS !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        const { outgoingID } = global.hsData;

        if (outgoingID) {
          user
            .get(keys.OUTGOINGS)
            .get(outGoingID)
            .put(null, (ack) => {
              if (ack.err) {
                test.fail("error fail ack:" + JSON.stringify(ack.err));
              } else {
                console.log("ok");
                done();
              }
            });
        } else {
          done();
        }
      }, data);
    });
  };
  clenUp("bob", alice, { testData, config: config, index: 1 });

  it("Sending handshake request to bob 2: ourSecret!", function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        const { user, pubToEpub, ZEN } = global;
        pubToEpub(pub, test.fail).then((bobEpub) => {
          ZEN.secret(bobEpub, user._.zen).then((ourSecret) => {
            global.hsData.ourSecret = ourSecret;
            global.hsData.otherEpub = bobEpub;
            done();
          });
        });
      },
      { pub: testData[1].pub, config: config },
    );
  });
  it("Sending handshake request to bob 3: check if handshake already done!", function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        const { successfulHandshakeAlreadyExists } = global;
        return successfulHandshakeAlreadyExists(pub).then(
          (alreadyHandshaked) => {
            if (alreadyHandshaked) {
              test.fail("handshake already exists");
            } else {
              done();
            }
          },
        );
      },
      { pub: testData[1].pub, config: config },
    );
  });

  it("Sending handshake request to bob 4: getting handshake address!", function () {
    return alice.run(
      async function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        const currentHandshakeAddress = await zen
          .user(pub)
          .get(keys.CURRENT_HANDSHAKE_ADDRESS)
          .then();
        if (typeof currentHandshakeAddress !== "string") {
          test.fail("expected current handshake address to be string");
        } else {
          global.hsData.currentHandshakeAddress = currentHandshakeAddress;
          done();
        }
      },
      { pub: testData[1].pub, config: config },
    );
  });

  it("Sending handshake request to bob 5: check if already sent request!", function () {
    return alice.run(
      async function (test) {
        const done = test.async();
        const { pub } = test.props;
        const { keys } = test.props.config;
        const { currentHandshakeAddress } = global.hsData;
        const maybeLastRequestIDSentToUser = await user
          .get(keys.USER_TO_LAST_REQUEST_SENT)
          .get(pub)
          .then();
        if (typeof maybeLastRequestIDSentToUser === "string") {
          if (maybeLastRequestIDSentToUser.length < 5) {
            test.fail("maybeLastRequestIDSentToUser.length < 5");
          }
          const lastRequestIDSentToUser = maybeLastRequestIDSentToUser;
          const hrInHandshakeNode = await zen
            .get(keys.HANDSHAKE_NODES)
            .get(currentHandshakeAddress)
            .get(lastRequestIDSentToUser)
            .then();
          if (typeof hrInHandshakeNode !== "undefined") {
            test.fail("request already sent");
          } else {
            done();
          }
        } else {
          done();
        }
      },
      { pub: testData[1].pub, config: config },
    );
  });
  const createOutgoingFeed = (name, testClient, data) => {
    it(`${name}: create outgoing feed 1:encryptedForMeRecipientPub !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const { mySecret, ZEN } = global;
        ZEN.encrypt(pub, mySecret).then((encryptedForMeRecipientPub) => {
          global.hsData.encryptedForMeRecipientPub = encryptedForMeRecipientPub;
          done();
        });
      }, data);
    });
    it(`${name}: create outgoing feed 2:ourSecret !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const { user, ZEN } = global;
        const { otherEpub } = global.hsData;
        ZEN.secret(otherEpub, user._.zen).then((ourSecret) => {
          global.hsData.ourSecret = ourSecret;
          done();
        });
      }, data);
    });
    it(`${name}: create outgoing feed 2.8:maybeOutgoingID !`, function () {
      return testClient.run(async function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        const maybeEncryptedOutgoingID = await global.user
          .get(keys.RECIPIENT_TO_OUTGOING)
          .get(pub)
          .then();
        if (maybeEncryptedOutgoingID === "string") {
          global.hsData.maybeEncryptedOutgoingID = maybeEncryptedOutgoingID;
          /*global.ZEN.decrypt(maybeEncryptedOutgoingID,global.mySecret)
						.then(outgoingID => outgoingID ? res(outgoingID) : rej(`expected outgoingID to be exist`))*/
        } else {
          global.hsData.maybeOutgoingID = maybeEncryptedOutgoingID;
        }
        done();
      }, data);
    });
    it(`${name}: create outgoing feed 2.9:decrypt maybeOutgoingID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const keys = env.config.keys;
        const { maybeEncryptedOutgoingID } = global.hsData;

        if (maybeEncryptedOutgoingID === "string") {
          global.hsData.maybeEncryptedOutgoingID = maybeEncryptedOutgoingID;
          global.ZEN.decrypt(maybeEncryptedOutgoingID, global.mySecret).then(
            (outgoingID) => {
              if (outgoingID) {
                global.hsData.maybeOutgoingID = outgoingID;
                done();
              } else {
                test.fail(`expected outgoingID to exist`);
              }
            },
          );
        } else {
          done();
        }
      }, data);
    });
    it(`${name}: create outgoing feed 3.1:newOutgoingFeedID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { maybeOutgoingID, encryptedForMeRecipientPub } = global.hsData;
        if (typeof maybeOutgoingID === "string") {
          done();
          return;
        }
        const newPartialOutgoingFeed = {
          with: encryptedForMeRecipientPub,
        };
        const _outFeedNode = user
          .get(keys.OUTGOINGS)
          .set(newPartialOutgoingFeed, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              const newOutgoingFeedID = _outFeedNode._.get;
              if (typeof newOutgoingFeedID !== "string") {
                test.fail('typeof newOutgoingFeedID !== "string"');
              } else {
                global.hsData.newOutgoingFeedID = newOutgoingFeedID;
                done();
              }
            }
          });
      }, data);
    });
    it(`${name}: create outgoing feed 3.2:encryptedForUsInitialMessage !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const { index } = env;
        const { pub } = env.testData[index];
        const { mySecret, ZEN } = global;
        const { ourSecret } = global.hsData;
        ZEN.encrypt("$$__SHOCKWALLET__INITIAL__MESSAGE", ourSecret).then(
          (encryptedForUsInitialMessage) => {
            global.hsData.encryptedForUsInitialMessage =
              encryptedForUsInitialMessage;
            done();
          },
        );
      }, data);
    });
    it(`${name}: create outgoing feed 3.3:initialMsg !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { user, ZEN } = global;
        const {
          maybeOutgoingID,
          newOutgoingFeedID,
          encryptedForUsInitialMessage,
        } = global.hsData;
        if (typeof maybeOutgoingID === "string") {
          done();
          return;
        }
        const initialMsg = {
          body: encryptedForUsInitialMessage,
          timestamp: Date.now(),
        }; //"$$__SHOCKWALLET__INITIAL__MESSAGE"

        user
          .get(keys.OUTGOINGS)
          .get(newOutgoingFeedID)
          .get(keys.MESSAGES)
          .set(initialMsg, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      }, data);
    });
    it(`${name}: create outgoing feed 3.4:encryptedForMeNewOutgoingFeedID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const { mySecret, ZEN } = global;
        const { maybeOutgoingID, newOutgoingFeedID } = global.hsData;
        if (typeof maybeOutgoingID === "string") {
          done();
          return;
        }
        ZEN.encrypt(newOutgoingFeedID, mySecret).then(
          (encryptedForMeNewOutgoingFeedID) => {
            global.hsData.encryptedForMeNewOutgoingFeedID =
              encryptedForMeNewOutgoingFeedID;
            done();
          },
        );
      }, data);
    });
    it(`${name}: create outgoing feed 3.5:save encryptedForMeNewOutgoingFeedID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { user } = global;
        const { maybeOutgoingID, encryptedForMeNewOutgoingFeedID } =
          global.hsData;
        if (typeof maybeOutgoingID === "string") {
          done();
          return;
        }

        user
          .get(keys.RECIPIENT_TO_OUTGOING)
          .get(pub)
          .put(encryptedForMeNewOutgoingFeedID, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      }, data);
    });
    it(`${name}: create outgoing feed 3.6:check OutgoingFeedID !`, function () {
      return testClient.run(function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { user } = global;
        const { maybeOutgoingID, newOutgoingFeedID } = global.hsData;

        if (typeof maybeOutgoingID === "string") {
          global.hsData.OutgoingFeedID = maybeOutgoingID;
          done();
        } else {
          const outgoingFeedID = newOutgoingFeedID;
          if (typeof outgoingFeedID === "undefined") {
            test.fail(
              '__createOutgoingFeed() -> typeof outgoingFeedID === "undefined"',
            );
          }

          if (typeof outgoingFeedID !== "string") {
            test.fail(
              "__createOutgoingFeed() -> expected outgoingFeedID to be an string",
            );
          }

          if (outgoingFeedID.length === 0) {
            test.fail(
              "__createOutgoingFeed() -> expected outgoingFeedID to be a populated string.",
            );
          }
          global.hsData.outgoingFeedID = outgoingFeedID;
          done();
        }
      }, data);
    });
  };
  createOutgoingFeed("Sending handshake request to bob 6", alice, {
    testData,
    config: config,
    index: 1,
  });
  it(`Sending handshake request to bob 6: create outgoing feed 4:encryptedForUsOutgoingFeedID !`, function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { ZEN } = global;
        const { outgoingFeedID, ourSecret } = global.hsData;
        ZEN.encrypt(outgoingFeedID, ourSecret).then(
          (encryptedForUsOutgoingFeedID) => {
            global.hsData.encryptedForUsOutgoingFeedID =
              encryptedForUsOutgoingFeedID;
            done();
          },
        );
      },
      { testData, config: config, index: 1 },
    );
  });
  it(`Sending handshake request to bob 6: create outgoing feed 5:newHandshakeRequestID in zen !`, function () {
    return alice
      .run(
        function (test) {
          const done = test.async();
          const env = test.props;
          const keys = env.config.keys;
          const { index } = env;
          const { pub } = env.testData[index];
          const { user, zen } = global;
          const { currentHandshakeAddress, encryptedForUsOutgoingFeedID } =
            global.hsData;
          const timestamp = Date.now();
          const handshakeRequestData = {
            from: user.is.pub,
            response: encryptedForUsOutgoingFeedID,
            timestamp,
          };
          const hr = zen
            .get(keys.HANDSHAKE_NODES)
            .get(currentHandshakeAddress)
            .set(handshakeRequestData, (ack) => {
              if (ack.err) {
                test.fail("error fail ack:" + JSON.stringify(ack.err));
              } else {
                global.hsData.newHandshakeRequestID = hr._.get;

                global.hsData.timestamp = timestamp;
                done(hr._.get);
              }
            });
        },
        { testData, config: config, index: 1 },
      )
      .then(
        (newHandshakeRequestID) => (handshakeRequestID = newHandshakeRequestID),
      );
  });
  it(`Sending handshake request to bob 6: create outgoing feed 6:newHandshakeRequestID !`, function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { user } = global;
        const { newHandshakeRequestID } = global.hsData;
        user
          .get(keys.USER_TO_LAST_REQUEST_SENT)
          .get(pub)
          .put(newHandshakeRequestID, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      },
      { testData, config: config, index: 1 },
    );
  });
  it(`Sending handshake request to bob 6: create outgoing feed 7:encrypt newHandshakeRequestID !`, function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { ZEN, mySecret } = global;
        const { newHandshakeRequestID } = global.hsData;
        ZEN.encrypt(newHandshakeRequestID, mySecret).then(
          (encryptedHandshakeRequestID) => {
            global.hsData.encryptedHandshakeRequestID =
              encryptedHandshakeRequestID;
            done();
          },
        );
      },
      { testData, config: config, index: 1 },
    );
  });
  it(`Sending handshake request to bob 6: create outgoing feed 8:encrypt currentHandshakeAddress !`, function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { ZEN, mySecret } = global;
        const { currentHandshakeAddress } = global.hsData;
        ZEN.encrypt(currentHandshakeAddress, mySecret).then(
          (encryptedHandshakeAddress) => {
            global.hsData.encryptedHandshakeAddress = encryptedHandshakeAddress;
            done();
          },
        );
      },
      { testData, config: config, index: 1 },
    );
  });
  it(`Sending handshake request to bob 6: create outgoing feed 9:storedReq !`, function () {
    return alice.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { index } = env;
        const { pub } = env.testData[index];
        const { ZEN, mySecret } = global;
        const {
          encryptedHandshakeAddress,
          encryptedForMeRecipientPub,
          encryptedHandshakeRequestID,
          timestamp,
        } = global.hsData;
        const storedReq = {
          sentReqID: encryptedHandshakeRequestID,
          recipientPub: encryptedForMeRecipientPub,
          handshakeAddress: encryptedHandshakeAddress,
          timestamp,
        };
        user.get(keys.STORED_REQS).set(storedReq, (ack) => {
          if (ack.err) {
            test.fail("error fail ack:" + JSON.stringify(ack.err));
          } else {
            done();
          }
        });
      },
      { testData, config: config, index: 1 },
    );
  });
  it(`Accepting handshake request from alice 7: currentHandshakeAddress !`, function () {
    return bob.run(
      async function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { user } = global;
        global.hsData = {};
        const currentHandshakeAddress = await user
          .get(keys.CURRENT_HANDSHAKE_ADDRESS)
          .then();
        if (typeof currentHandshakeAddress !== "string") {
          test.fail("expected currentHandshakeAddress to be string");
        } else {
          global.hsData.currentHandshakeAddress = currentHandshakeAddress;
          done();
        }
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: receivedReqs **using .load!`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { zen } = global;
        const { currentHandshakeAddress } = global.hsData;
        zen
          .get(keys.HANDSHAKE_NODES)
          .get(currentHandshakeAddress)
          .open((receivedReqs) => {
            global.hsData.receivedReqs = receivedReqs;
            done();
          });
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: extract request data`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const { handshakeRequestID } = env;
        const keys = env.config.keys;
        const { zen } = global;
        const { receivedReqs } = global.hsData;
        const data = receivedReqs[handshakeRequestID];
        if (typeof data.from !== "string") {
          test.fail(`expected "from" to be string, found ${typeof data.from}`);
        }
        if (typeof data.response !== "string") {
          test.fail(
            `expected "response" to be string, found ${typeof data.response}`,
          );
        }
        if (typeof data.timestamp !== "number") {
          test.fail(
            `expected "timestamp" to be string, found ${typeof data.timestamp}`,
          );
        }
        global.hsData.handshakeRequestID = handshakeRequestID;
        global.hsData.data = receivedReqs[handshakeRequestID];
        done();
      },
      { testData, config: config, index: 2, handshakeRequestID },
    );
  }); //handshakeRequestID

  it(`Accepting handshake request from alice 7: pub to Epub`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { pubToEpub } = global;
        const { data } = global.hsData;
        pubToEpub(data.from, test.fail).then((aliceEpub) => {
          global.hsData.otherEpub = aliceEpub;
          done();
        });
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: our secret`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { ZEN, user } = global;
        const { otherEpub } = global.hsData;
        ZEN.secret(otherEpub, user._.zen).then((ourSecret) => {
          global.hsData.ourSecret = ourSecret;
          done();
        });
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: decrypt encryptedForUsIncomingID`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const { ZEN, user } = global;
        const { data, ourSecret } = global.hsData;
        ZEN.decrypt(data.response, ourSecret).then((incomingID) => {
          if (typeof incomingID !== "string") {
            test.fail(
              `expected "incomingID" to be string, found ${typeof incomingID}`,
            );
          } else {
            global.hsData.incomingID = incomingID;
            done();
          }
        });
      },
      { testData, config: config, index: 0 },
    );
  });
  createOutgoingFeed("Accepting handshake request from alice 7", bob, {
    testData,
    config: config,
    index: 0,
  });
  it(`Accepting handshake request from alice 7: encrypt encryptedForMeIncomingID`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const { ZEN, mySecret } = global;
        const { incomingID } = global.hsData;
        ZEN.encrypt(incomingID, mySecret).then((encryptedForMeIncomingID) => {
          global.hsData.encryptedForMeIncomingID = encryptedForMeIncomingID;
          done();
        });
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: encryptedForMeIncomingID`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const env = test.props;
        const keys = env.config.keys;
        const { user } = global;
        const { encryptedForMeIncomingID, data } = global.hsData;
        user
          .get(keys.USER_TO_INCOMING)
          .get(data.from)
          .put(encryptedForMeIncomingID, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: encrypt encryptedForUsOutgoingID`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const { ZEN } = global;
        const { ourSecret, outgoingFeedID } = global.hsData;
        ZEN.encrypt(outgoingFeedID, ourSecret).then(
          (encryptedForUsOutgoingID) => {
            global.hsData.encryptedForUsOutgoingID = encryptedForUsOutgoingID;
            done();
          },
        );
      },
      { testData, config: config, index: 0 },
    );
  });
  it(`Accepting handshake request from alice 7: encryptedForUsOutgoingID`, function () {
    return bob.run(
      function (test) {
        const done = test.async();
        const { zen } = global;
        const env = test.props;
        const keys = env.config.keys;
        const {
          encryptedForUsOutgoingID,
          currentHandshakeAddress,
          handshakeRequestID,
        } = global.hsData;
        const data = { response: encryptedForUsOutgoingID };
        zen
          .get(keys.HANDSHAKE_NODES)
          .get(currentHandshakeAddress)
          .get(handshakeRequestID)
          .put(data, (ack) => {
            if (ack.err) {
              test.fail("error fail ack:" + JSON.stringify(ack.err));
            } else {
              done();
            }
          });
      },
      { testData, config: config, index: 0 },
    );
  });
  it("All finished!", function (done) {
    console.log("Done! Cleaning things up...");
    setTimeout(function () {
      done();
    }, 1000);
  });

  after("Everything shut down.", function () {
    return servers.run(function () {
      process.exit();
    });
  });
});
