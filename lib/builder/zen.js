import fs from "fs";
import nodePath from "path";
import { fileURLToPath } from "node:url";
import { dirname as dirnameOf } from "node:path";
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirnameOf(__filename);
const rootDir = nodePath.resolve(__dirname, "../..");
const entryFile = nodePath.join(rootDir, "src/index.js");
const zenOutput = nodePath.join(rootDir, "zen.js");
const penWasmSource = nodePath.join(rootDir, "src/pen.wasm");
const penWasmOutput = nodePath.join(rootDir, "pen.wasm");

function read(absPath) {
  return fs
    .readFileSync(absPath, "utf8")
    .replace(/\r\n/g, "\n")
    .replace(/\r/g, "\n");
}

function write(absPath, data) {
  fs.writeFileSync(absPath, data);
}

function normalize(absPath) {
  return nodePath.resolve(absPath);
}

function relId(absPath) {
  return (
    "./" + nodePath.relative(rootDir, absPath).split(nodePath.sep).join("/")
  );
}

function resolveImport(fromFile, spec) {
  if (spec.startsWith(".")) {
    return normalize(nodePath.resolve(nodePath.dirname(fromFile), spec));
  }
  return spec;
}

function aliased(absPath) {
  return absPath;
}

function parseExportList(raw) {
  return raw
    .split(",")
    .map(function (part) {
      return part.trim();
    })
    .filter(Boolean)
    .map(function (part) {
      var bits = part.split(/\s+as\s+/);
      return { local: bits[0].trim(), exported: (bits[1] || bits[0]).trim() };
    });
}

function transformModule(source, absPath) {
  var exportLines = [];

  source = source.replace(
    /^\s*import\s+\{([^}]+)\}\s+from\s+['"]([^'"]+)['"];\s*$/gm,
    function (_, names, spec) {
      if (spec.startsWith(".")) {
        var target = relId(aliased(resolveImport(absPath, spec)));
        return "var {" + names + "} = reqmod('" + target + "');";
      }
      throw new Error(
        "External import not supported in zen bundle: " +
          spec +
          " (in " +
          absPath +
          ")",
      );
    },
  );

  source = source.replace(
    /^\s*import\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"];\s*$/gm,
    function (_, name, spec) {
      if (spec.startsWith(".")) {
        var target = relId(aliased(resolveImport(absPath, spec)));
        return "var " + name + " = reqmod('" + target + "').default;";
      }
      throw new Error(
        "External import not supported in zen bundle: " +
          spec +
          " (in " +
          absPath +
          ")",
      );
    },
  );

  source = source.replace(
    /^\s*import\s+['"]([^'"]+)['"];\s*$/gm,
    function (_, spec) {
      if (spec.startsWith(".")) {
        var target = relId(aliased(resolveImport(absPath, spec)));
        return "reqmod('" + target + "');";
      }
      throw new Error(
        "External import not supported in zen bundle: " +
          spec +
          " (in " +
          absPath +
          ")",
      );
    },
  );

  source = source.replace(/export\s*\{([\s\S]*?)\};?/gm, function (_, names) {
    parseExportList(names).forEach(function (item) {
      exportLines.push("exp." + item.exported + " = " + item.local + ";");
    });
    return "";
  });

  source = source.replace(
    /export default async function\s+([A-Za-z_$][\w$]*)\s*\(/g,
    function (_, name) {
      exportLines.push("exp.default = " + name + ";");
      return "async function " + name + "(";
    },
  );

  source = source.replace(
    /export default function\s+([A-Za-z_$][\w$]*)\s*\(/g,
    function (_, name) {
      exportLines.push("exp.default = " + name + ";");
      return "function " + name + "(";
    },
  );

  source = source.replace(/export default\s+([^;]+);/g, function (_, expr) {
    return "exp.default = " + expr + ";";
  });

  if (exportLines.length) {
    source += "\n" + exportLines.join("\n") + "\n";
  }
  return source.trim() + "\n";
}

function collect(file, seen, ordered) {
  var absPath = aliased(normalize(file));
  if (typeof seen[absPath] !== "undefined") {
    return;
  }
  var source = read(absPath);
  seen[absPath] = source;
  source.replace(
    /^\s*import\s+(?:[^'"]*?\s+from\s+)?['"]([^'"]+)['"];\s*$/gm,
    function (_, spec) {
      if (spec.startsWith(".")) {
        collect(resolveImport(absPath, spec), seen, ordered);
      }
    },
  );
  ordered.push(absPath);
}

function bundle() {
  var seen = {};
  var ordered = [];
  collect(entryFile, seen, ordered);

  var out = "";
  out += "const mods = Object.create(null);\n";
  out +=
    "function defmod(id, fn){ mods[id] = { fn: fn, exports: {}, loaded: false }; }\n";
  out +=
    "function reqmod(id){ var mod = mods[id]; if(!mod){ throw new Error('Missing module: ' + id); } if(!mod.loaded){ mod.loaded = true; mod.fn(mod, mod.exports); } return mod.exports; }\n\n";

  ordered.forEach(function (absPath) {
    var id = relId(absPath);
    var source = transformModule(seen[absPath], absPath);
    out += "defmod('" + id + "', function(module, exp){\n";
    out += source
      .split("\n")
      .map(function (line) {
        return line ? "  " + line : "";
      })
      .join("\n");
    out += "});\n\n";
  });

  out += "const zenmod = reqmod('" + relId(entryFile) + "');\n";
  out += "const ZEN = zenmod.default;\n";
  out += "export { ZEN };\n";
  out += "export default ZEN;\n";

  write(zenOutput, out);
  fs.copyFileSync(penWasmSource, penWasmOutput);
  console.log("Built zen.js");
  console.log("Copied pen.wasm");
}

if (process.argv[1] && nodePath.resolve(process.argv[1]) === __filename) {
  bundle();
}

export default bundle;
