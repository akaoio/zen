import fs from "node:fs";
import nodePath from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = fileURLToPath(import.meta.url);
const rootDir = nodePath.resolve(nodePath.dirname(here), "..", "..");
const zenUrl = pathToFileURL(nodePath.join(rootDir, "zen.js")).href;
const zenMinUrl = pathToFileURL(nodePath.join(rootDir, "zen.min.js")).href;
const libDir = nodePath.join(rootDir, "lib") + nodePath.sep;

// The suite must exercise what ships. zen.js has a minified twin and so does
// every file under lib/, and a bug can live in one and not the other: a fix
// applied to the source alone reads as effective here while the bundle a user
// installs still carries it. So every import of a source file is answered with
// its .min.js, and the build that produces them runs before the tests.
const minified = new Map();
function minFor(url) {
  if (minified.has(url)) {
    return minified.get(url);
  }
  let out = null;
  if (url === zenUrl) {
    out = zenMinUrl;
  } else {
    let file;
    try {
      file = fileURLToPath(url);
    } catch (e) {
      file = null;
    }
    if (
      file &&
      file.startsWith(libDir) &&
      file.endsWith(".js") &&
      !file.endsWith(".min.js")
    ) {
      const min = file.slice(0, -3) + ".min.js";
      if (fs.existsSync(min)) {
        out = pathToFileURL(min).href;
      }
    }
  }
  minified.set(url, out);
  return out;
}

export async function resolve(specifier, context, defaultResolve) {
  const resolved = await defaultResolve(specifier, context, defaultResolve);
  const min = minFor(resolved.url);
  return min ? { ...resolved, url: min } : resolved;
}
