/**
 * Cross-machine WebRTC browser-to-browser test.
 *
 * Opens one Chromium browser on each of the 3 relay machines:
 *   zen.akao.io   — controlled directly by this Playwright spec
 *   peer0.akao.io — started via SSH using webrtc-p2p-runner.js
 *   peer1.akao.io — started via SSH using webrtc-p2p-runner.js
 *
 * Each browser connects to its machine's relay. The relay mesh propagates
 * browser peer IDs (bpids) via PEX, triggering automatic WebRTC
 * RTCPeerConnection offers. The test asserts that DataChannels open
 * across all 3 physical machines.
 */

import { test, expect } from "@playwright/test";
import { spawn } from "child_process";

const RELAYS = {
  zen:   "wss://zen.akao.io:8420/zen",
  peer0: "wss://peer0.akao.io:8420/zen",
  peer1: "wss://peer1.akao.io:8420/zen",
};

function remoteRunnerCmd(host, relayKey) {
  const relay   = RELAYS[relayKey];
  const nvmInit = "source ~/.nvm/nvm.sh 2>/dev/null || true; nvm use 24 --silent 2>/dev/null || true";
  return (
    `sshpass -p 's2e2r0v9er@' ssh -o StrictHostKeyChecking=no x@${host} ` +
    `'${nvmInit}; cd ~/zen && node test/browser/webrtc-p2p-runner.js ` +
    `--relay=${relay} --min=1 --port=8767 --timeout=60000'`
  );
}

const procs = {};
const stdout = { peer0: "", peer1: "" };

test.describe("WebRTC browser-to-browser across 3 physical machines", () => {

  test.beforeAll(async () => {
    for (const [key, host] of [["peer0", "peer0.akao.io"], ["peer1", "peer1.akao.io"]]) {
      const proc = spawn("bash", ["-c", remoteRunnerCmd(host, key)], {
        stdio: ["ignore", "pipe", "pipe"],
      });
      proc.stdout.on("data", d => {
        stdout[key] += d.toString();
        console.log(`[${key}]`, d.toString().trim());
      });
      proc.stderr.on("data", d => console.error(`[${key}-err]`, d.toString().trim()));
      procs[key] = proc;
    }
    // Allow SSH sessions to connect and servers to start
    await new Promise(r => setTimeout(r, 4000));
  });

  test.afterAll(async () => {
    for (const proc of Object.values(procs)) {
      if (proc && !proc.killed) proc.kill();
    }
  });

  test("DataChannels open on all 3 machines", async ({ page }) => {
    test.setTimeout(90000);

    // zen browser connects to zen relay
    await page.goto("/webrtc-p2p.html?relay=" + encodeURIComponent(RELAYS.zen));
    await expect(page.locator("#ready")).toHaveText("ready", { timeout: 15000 });
    console.log("[zen] browser ready");

    // Wait for zen to have DataChannels to BOTH peer0 and peer1
    const zenPeers = await page.evaluate(function () {
      return window.webrtcTest.waitForDC(2, 60000);
    });
    console.log("[zen] DC peers (" + zenPeers.length + "):", zenPeers);
    expect(zenPeers.length).toBeGreaterThanOrEqual(2);

    // Give remote runners up to 15 s to also confirm at least 1 DC each
    const deadline = Date.now() + 15000;
    while (Date.now() < deadline) {
      if (stdout.peer0.includes("DC_OPEN") && stdout.peer1.includes("DC_OPEN")) break;
      await page.waitForTimeout(500);
    }

    console.log("[peer0] last output:", stdout.peer0.split("\n").filter(Boolean).pop());
    console.log("[peer1] last output:", stdout.peer1.split("\n").filter(Boolean).pop());

    expect(stdout.peer0, "peer0 should report DC_OPEN").toContain("DC_OPEN");
    expect(stdout.peer1, "peer1 should report DC_OPEN").toContain("DC_OPEN");
  });

});
