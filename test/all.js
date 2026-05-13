import { spawn } from "node:child_process";

const suites = [
  { name: "PEN unit",      args: ["run", "test:pen:unit"] },
  { name: "ZEN unit",      args: ["run", "test:zen:unit"] },
  { name: "core",          args: ["run", "test:core"] },
  { name: "discover",      args: ["run", "test:discover"] },
  { name: "MCP",           args: ["run", "test:mcp"] },
  { name: "DHT",           args: ["run", "test:dht"] },
  { name: "chains",        args: ["run", "test:chains"] },
  { name: "chains:fork",   args: ["run", "test:chains:fork"] },
];

var totalPassing = 0;
var totalPending = 0;
var totalFailing = 0;

function runsuite(suite) {
  return new Promise(function (resolve) {
    var proc = spawn("npm", suite.args, { shell: true, stdio: ["inherit", "pipe", "inherit"], env: Object.assign({}, process.env, { FORCE_COLOR: "1" }) });
    var output = "";
    proc.stdout.on("data", function (chunk) {
      process.stdout.write(chunk);
      output += chunk.toString();
    });
    proc.on("close", function (code) {
      var m;
      var re;
      re = /(\d+) passing/g;
      while ((m = re.exec(output))) totalPassing += parseInt(m[1]);
      re = /(\d+) pending/g;
      while ((m = re.exec(output))) totalPending += parseInt(m[1]);
      re = /(\d+) failing/g;
      while ((m = re.exec(output))) totalFailing += parseInt(m[1]);
      resolve(code);
    });
  });
}

var codes = [];
for (var i = 0; i < suites.length; i++) {
  codes.push(await runsuite(suites[i]));
}

var line = "=".repeat(50);
console.log("\n" + line);
console.log(
  "  " + totalPassing + " passing" +
  (totalPending ? ", " + totalPending + " pending" : "") +
  (totalFailing ? ", " + totalFailing + " failing" : "")
);
console.log(line);

if (codes.some(function (c) { return c !== 0; })) process.exit(1);
