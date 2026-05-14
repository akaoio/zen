// Example: minimal relay server
// For production use, see script/server.js at the project root.
import ZEN from "../index.js";
import http from "http";
import { fileURLToPath } from "node:url";
import { dirname } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));

const zen = new ZEN({
  web: http.createServer(ZEN.serve(__dirname)).listen(8420),
});
zen.chain();
console.log("ZEN relay on port 8420");
