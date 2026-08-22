#!/usr/bin/env node

// Prints what `zen doctor` should say about TLS, one line per finding:
//
//     level <TAB> label <TAB> detail
//
// The judging lives in lib/tlsdoctor.js and the aggregating lives in zen.sh,
// which already owns the exit code. This only wires real dns and a real
// filesystem to the one and hands its answers to the other.

import dns from "dns";
import os from "os";
import path from "path";

import * as xdg from "../lib/xdg.js";
import { report } from "../lib/tlsdoctor.js";

const domain = process.argv[2];
if (!domain) process.exit(0);

const lines = await report({
  domain,
  acmeDir: process.env.ACME_HOME || path.join(os.homedir(), ".acme.sh"),
  certPath: process.env.HTTPS_CERT || path.join(xdg.config(), "cert.pem"),
  resolveNs: dns.promises.resolveNs,
});

for (const l of lines) {
  process.stdout.write(`${l.level}\t${l.label}\t${l.detail}\n`);
}
