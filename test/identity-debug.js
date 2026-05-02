// test/identity-debug.js — Debug hardware identity generation
import { hwid } from "../lib/discover.js";
import ZEN from "../index.js";

console.log("Debug: Testing deterministic key generation\n");

const hraw = hwid();
console.log("hwid:", hraw);

const seed1 = await ZEN.hash(hraw, null, null, { name: "SHA-256", encode: "base62" });
console.log("seed (call 1):", seed1);

const seed2 = await ZEN.hash(hraw, null, null, { name: "SHA-256", encode: "base62" });
console.log("seed (call 2):", seed2);
console.log("Seeds match:", seed1 === seed2);

console.log("\nGenerating first keypair...");
const pair1 = await ZEN.pair(null, { seed: seed1 });
console.log("pair1.pub:", pair1.pub);
console.log("pair1.priv:", pair1.priv.slice(0, 20) + "...");

console.log("\nGenerating second keypair...");
const pair2 = await ZEN.pair(null, { seed: seed1 });
console.log("pair2.pub:", pair2.pub);
console.log("pair2.priv:", pair2.priv.slice(0, 20) + "...");

console.log("\nKeys match:", pair1.pub === pair2.pub);

if (pair1.pub !== pair2.pub) {
  console.log("\n✗ FAIL: Keys are not deterministic!");
  process.exit(1);
} else {
  console.log("\n✓ SUCCESS: Keys are deterministic");
}
