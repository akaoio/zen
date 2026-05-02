// test/identity.js — Hardware identity module tests
import { getOrCreateIdentity, getIdentity } from "../lib/identity.js";
import { hwid } from "../lib/discover.js";
import fs from "fs";
import path from "path";
import * as xdg from "../lib/xdg.js";

console.log("Testing hardware identity module...\n");

// Test 1: Check if hardware entropy is available
console.log("Test 1: Hardware entropy availability");
const hraw = hwid();
if (hraw) {
  console.log("✓ Hardware entropy available:", hraw.slice(0, 30) + "...");
} else {
  console.log("⚠ Hardware entropy not available (container environment?)");
  console.log("  Tests will be skipped");
  process.exit(0);
}

// Test 2: Create identity for the first time
console.log("\nTest 2: Create identity");
const identity1 = await getOrCreateIdentity();
if (!identity1) {
  console.log("✗ Failed to create identity");
  process.exit(1);
}
console.log("✓ Identity created");
console.log("  pub:", identity1.pair.pub.slice(0, 20) + "...");
console.log("  epub:", identity1.pair.epub.slice(0, 20) + "...");
console.log("  seed:", identity1.seed.slice(0, 20) + "...");

// Test 3: Verify identity file was created
console.log("\nTest 3: Identity persistence");
const statePath = path.join(xdg.state(), "identity.json");
if (!fs.existsSync(statePath)) {
  console.log("✗ Identity file was not created");
  process.exit(1);
}
const stored = JSON.parse(fs.readFileSync(statePath, "utf8"));
console.log("✓ Identity file exists at:", statePath);
console.log("  stored pub:", stored.pub.slice(0, 20) + "...");

// Test 4: Retrieve same identity
console.log("\nTest 4: Identity consistency");
const identity2 = await getOrCreateIdentity();
if (identity2.pair.pub !== identity1.pair.pub) {
  console.log("✗ Public key mismatch!");
  console.log("  First:", identity1.pair.pub.slice(0, 20) + "...");
  console.log("  Second:", identity2.pair.pub.slice(0, 20) + "...");
  process.exit(1);
}
if (identity2.pair.priv !== identity1.pair.priv) {
  console.log("✗ Private key mismatch!");
  process.exit(1);
}
console.log("✓ Identity is consistent across calls");

// Test 5: Verify getIdentity returns same identity
console.log("\nTest 5: getIdentity() function");
const identity3 = await getIdentity();
if (identity3.pair.pub !== identity1.pair.pub) {
  console.log("✗ getIdentity() returned different public key");
  process.exit(1);
}
console.log("✓ getIdentity() returns same identity");

// Test 6: Verify keypair is valid (can sign/verify)
console.log("\nTest 6: Keypair functionality");
try {
  const ZEN = (await import("../index.js")).default;
  const testData = "test message";
  const signed = await ZEN.sign(testData, identity1.pair);
  const verified = await ZEN.verify(signed, identity1.pair.pub);
  if (verified !== testData) {
    console.log("✗ Sign/verify failed");
    process.exit(1);
  }
  console.log("✓ Keypair can sign and verify");
} catch (e) {
  console.log("✗ Sign/verify error:", e.message);
  process.exit(1);
}

console.log("\n✓ All tests passed!");
console.log("\nNote: To clean up, remove:", statePath);
