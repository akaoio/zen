import assert from "assert";
import {
  BOOT,
  bootstrapDisabled,
  mergePeers,
  resolveBootstrapPeers,
} from "../../src/bootstrap.js";

describe("bootstrap peer resolution", function () {
  it("keeps built-in bootstrap peers when runtime config is empty", function () {
    assert.deepStrictEqual(resolveBootstrapPeers([]), BOOT);
  });

  it("merges configured peers with bootstrap peers without duplicates", function () {
    const configured = ["wss://peer1.akao.io", "wss://custom.akao.io:8420/zen"];
    assert.deepStrictEqual(resolveBootstrapPeers(configured), [
      "wss://zen.akao.io",
      "wss://peer0.akao.io",
      "wss://peer1.akao.io",
      "wss://custom.akao.io:8420/zen",
    ]);
  });

  it("supports isolated deployments by disabling bootstrap peers explicitly", function () {
    assert.deepStrictEqual(
      resolveBootstrapPeers(["wss://custom.akao.io:8420/zen"], {
        includeBootstrap: false,
      }),
      ["wss://custom.akao.io:8420/zen"],
    );
  });

  it("dedupes merged peer lists while preserving first-seen order", function () {
    assert.deepStrictEqual(
      mergePeers(
        [" wss://zen.akao.io ", "wss://peer0.akao.io"],
        ["wss://peer0.akao.io", "wss://custom.akao.io:8420/zen"],
      ),
      [
        "wss://zen.akao.io",
        "wss://peer0.akao.io",
        "wss://custom.akao.io:8420/zen",
      ],
    );
  });

  it("parses NO_BOOTSTRAP and BOOTSTRAP env flags", function () {
    assert.strictEqual(bootstrapDisabled({ NO_BOOTSTRAP: "1" }), true);
    assert.strictEqual(bootstrapDisabled({ NO_BOOTSTRAP: "0" }), false);
    assert.strictEqual(bootstrapDisabled({ BOOTSTRAP: "off" }), true);
    assert.strictEqual(bootstrapDisabled({ BOOTSTRAP: "on" }), false);
  });
});
