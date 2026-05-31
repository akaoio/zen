import fs from "fs";
import nodePath from "path";
import { minify } from "terser";
import { fileURLToPath } from "node:url";
import { dirname as __dirnameOf } from "node:path";

const __filename = fileURLToPath(import.meta.url);
const __dirname = __dirnameOf(__filename);
const rootDir = nodePath.resolve(__dirname, "..");
const libDir = nodePath.join(rootDir, "lib");

function eachJs(dir, files = []) {
  for (const name of fs.readdirSync(dir)) {
    const absPath = nodePath.join(dir, name);
    const stat = fs.statSync(absPath);
    if (stat.isDirectory()) {
      eachJs(absPath, files);
      continue;
    }
    if (!name.endsWith(".js") || name.endsWith(".min.js")) {
      continue;
    }
    files.push(absPath);
  }
  return files;
}

function isModule(code) {
  return /^\s*(import|export)\s/m.test(code);
}

function rewriteImportsToMin(code) {
  return code.replace(/(['"])(\.\.?\/[^'"]*?)\.js\1/g, function (match, q, path) {
    if (path.endsWith(".min")) return match;
    return q + path + ".min.js" + q;
  });
}

async function minifyFile(absPath) {
  const code = fs.readFileSync(absPath, "utf8");
  const outPath = absPath.replace(/\.js$/, ".min.js");
  const mode = fs.statSync(absPath).mode;
  const result = await minify(code, {
    module: isModule(code),
    compress: true,
    mangle: true,
  });
  if (!result.code) {
    console.warn(
      `Warning: copied unminified fallback for ${nodePath.relative(rootDir, absPath)}`,
    );
    fs.writeFileSync(outPath, rewriteImportsToMin(code));
    fs.chmodSync(outPath, mode);
    return nodePath.relative(rootDir, outPath);
  }
  fs.writeFileSync(outPath, rewriteImportsToMin(result.code));
  fs.chmodSync(outPath, mode);
  return nodePath.relative(rootDir, outPath);
}

// Also minify root-level standalone files (index.js, etc.)
// Exclude zen.js (built separately by build:zen), chains.js (not a standard ES module),
// and non-source files.
const ROOT_EXCLUDE = new Set(["zen.js", "chains.js", "clean.js", "minify.js", "playwright.config.js"]);
const rootEntries = fs.readdirSync(rootDir)
  .filter((f) => f.endsWith(".js") && !f.endsWith(".min.js") && !ROOT_EXCLUDE.has(f))
  .map((f) => nodePath.join(rootDir, f))
  .filter((f) => fs.statSync(f).isFile());

const outputs = await Promise.all(
  [...eachJs(libDir), ...rootEntries].map(minifyFile),
);
console.log(`Minified ${outputs.length} lib files`);
