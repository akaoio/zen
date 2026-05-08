import assert from "assert";
import {
  BOOT,
  bootstrapDisabled,
  mergePeers,
  parsePeerEnv,
  resolveBootstrapPeers,
  resolveEnvPeers,
} from "../../src/bootstrap.js";

describe("bootstrap peer resolution", function () {
  it("keeps built-in bootstrap peers when runtime config is empty", function () {
    assert.deepStrictEqual(resolveBootstrapPeers([]), BOOT);
  });

  it("merges configured peers with bootstrap peers without duplicates", function () {
    const configured = ["https://peer1.akao.io:8420/zen", "wss://custom.akao.io:8420/zen"];
    // BOOT is empty — configured peers are returned as-is (no duplicates to remove)
    assert.deepStrictEqual(resolveBootstrapPeers(configured), [
      "https://peer1.akao.io:8420/zen",
      "wss://custom.akao.io:8420/zen",
    ]);
  });

  it("supports isolated deployments by disabling bootstrap peers explicitly", function () {
    assert.deepStrictEqual(
      resolveBootstrapPeers(["https://custom.akao.io:8420/zen"], {
        includeBootstrap: false,
      }),
      ["https://custom.akao.io:8420/zen"],
    );
  });

  it("dedupes merged peer lists while preserving first-seen order", function () {
    assert.deepStrictEqual(
      mergePeers(
        [" https://zen.akao.io ", "https://peer0.akao.io"],
        ["https://peer0.akao.io", "https://custom.akao.io:8420/zen"],
      ),
      [
        "https://zen.akao.io",
        "https://peer0.akao.io",
        "https://custom.akao.io:8420/zen",
      ],
    );
  });

  it("parses NO_BOOTSTRAP and BOOTSTRAP env flags", function () {
    assert.strictEqual(bootstrapDisabled({ NO_BOOTSTRAP: "1" }), true);
    assert.strictEqual(bootstrapDisabled({ NO_BOOTSTRAP: "0" }), false);
    assert.strictEqual(bootstrapDisabled({ BOOTSTRAP: "off" }), true);
    assert.strictEqual(bootstrapDisabled({ BOOTSTRAP: "on" }), false);
  });

  it("parses PEERS env into a trimmed list", function () {
    assert.deepStrictEqual(
      parsePeerEnv(" https://peer1.akao.io:8420/zen, wss://custom.akao.io:8420/zen ,, "),
      ["https://peer1.akao.io:8420/zen", "wss://custom.akao.io:8420/zen"],
    );
  });

  it("resolveEnvPeers leaves peers undefined when env is empty so constructor keeps BOOT", function () {
    assert.strictEqual(resolveEnvPeers({}), undefined);
  });

  it("resolveEnvPeers merges explicit PEERS with BOOT by default", function () {
    // BOOT is empty, so only the explicit PEERS are returned
    assert.deepStrictEqual(
      resolveEnvPeers({ PEERS: "wss://custom.akao.io:8420/zen" }),
      ["wss://custom.akao.io:8420/zen"],
    );
  });

  it("resolveEnvPeers keeps isolated mode when NO_BOOTSTRAP=1 and PEERS is unset", function () {
    assert.deepStrictEqual(resolveEnvPeers({ NO_BOOTSTRAP: "1" }), []);
  });

  it("resolveEnvPeers uses explicit PEERS only when bootstrap is disabled", function () {
    assert.deepStrictEqual(
      resolveEnvPeers({
        NO_BOOTSTRAP: "1",
        PEERS: "wss://custom.akao.io:8420/zen,https://peer1.akao.io:8420/zen",
      }),
      ["wss://custom.akao.io:8420/zen", "https://peer1.akao.io:8420/zen"],
    );
  });
});
