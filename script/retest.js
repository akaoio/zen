#!/usr/bin/env node

import { spawn } from "node:child_process";

var count = Number.parseInt(process.argv[2] || "20", 10);

if (!Number.isInteger(count) || count < 1) {
  console.error("Usage: node script/retest.js <count>");
  process.exit(1);
}

var npmExecPath = process.env.npm_execpath;

function runOnce(iteration) {
  return new Promise(function (resolve) {
    var started = Date.now();
    var child;

    console.log("\n=== npm test run " + iteration + "/" + count + " ===\n");

    if (npmExecPath) {
      child = spawn(process.execPath, [npmExecPath, "test"], {
        stdio: "inherit",
        env: process.env,
      });
    } else {
      child = spawn("npm", ["test"], {
        stdio: "inherit",
        env: process.env,
        shell: process.platform === "win32",
      });
    }

    child.on("exit", function (code, signal) {
      var took = ((Date.now() - started) / 1000).toFixed(1);
      if (code === 0) {
        console.log("\n=== run " + iteration + " passed in " + took + "s ===\n");
        return resolve({ ok: true });
      }
      console.error(
        "\n=== run " + iteration + " failed in " + took + "s" +
        (signal ? " (signal " + signal + ")" : " (exit " + code + ")") +
        " ===\n",
      );
      resolve({ ok: false, code: code || 1, signal: signal || null, iteration: iteration });
    });
  });
}

async function main() {
  var iteration;
  var failed;
  for (iteration = 1; iteration <= count; iteration += 1) {
    failed = await runOnce(iteration);
    if (!failed.ok) {
      process.exit(failed.code);
    }
  }
  console.log("All " + count + " npm test runs passed.");
}

main().catch(function (err) {
  console.error(err);
  process.exit(1);
});