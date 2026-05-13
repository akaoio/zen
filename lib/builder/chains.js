// lib/builder/chains.js — Bundles lib/chains/evm.js → chains.js + chains.min.js
//
// Follows the same module-inlining pattern as lib/builder/zen.js.
// Run: node lib/builder/chains.js
//
// Output:
//   chains.js     — bundled, self-contained ES module (Node + browser)
//   chains.min.js — minified version (requires terser)

import fs from "fs"
import nodePath from "path"
import { fileURLToPath } from "node:url"
import { dirname as dirnameOf } from "node:path"
import { spawnSync } from "node:child_process"

const __filename = fileURLToPath(import.meta.url)
const __dirname  = dirnameOf(__filename)
const rootDir    = nodePath.resolve(__dirname, "../..")
const entryFile  = nodePath.join(rootDir, "lib/chains/evm.js")
const output     = nodePath.join(rootDir, "chains.js")
const outputMin  = nodePath.join(rootDir, "chains.min.js")

function read(p)       { return fs.readFileSync(p, "utf8").replace(/\r\n/g, "\n").replace(/\r/g, "\n") }
function write(p, d)   { fs.writeFileSync(p, d) }
function normalize(p)  { return nodePath.resolve(p) }
function relId(p)      { return "./" + nodePath.relative(rootDir, p).split(nodePath.sep).join("/") }

function resolveImport(fromFile, spec) {
    if (spec.startsWith(".")) return normalize(nodePath.resolve(nodePath.dirname(fromFile), spec))
    return spec
}

function parseExportList(raw) {
    return raw.split(",").map(p => p.trim()).filter(Boolean).map(p => {
        const bits = p.split(/\s+as\s+/)
        return { local: bits[0].trim(), exported: (bits[1] || bits[0]).trim() }
    })
}

function transformModule(source, absPath) {
    const exportLines = []

    // import { a, b } from "./x"  →  var { a, b } = reqmod('./x');
    source = source.replace(
        /^\s*import\s+\{([^}]+)\}\s+from\s+['"]([^'"]+)['"];\s*$/gm,
        (_, names, spec) => {
            if (!spec.startsWith(".")) throw new Error("External import in chains bundle: " + spec + " (" + absPath + ")")
            return "var {" + names + "} = reqmod('" + relId(resolveImport(absPath, spec)) + "');"
        }
    )

    // import X from "./x"  →  var X = reqmod('./x').default;
    source = source.replace(
        /^\s*import\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"];\s*$/gm,
        (_, name, spec) => {
            if (!spec.startsWith(".")) throw new Error("External import in chains bundle: " + spec + " (" + absPath + ")")
            return "var " + name + " = reqmod('" + relId(resolveImport(absPath, spec)) + "').default;"
        }
    )

    // import "x"  →  reqmod('x');
    source = source.replace(
        /^\s*import\s+['"]([^'"]+)['"];\s*$/gm,
        (_, spec) => {
            if (!spec.startsWith(".")) throw new Error("External import in chains bundle: " + spec + " (" + absPath + ")")
            return "reqmod('" + relId(resolveImport(absPath, spec)) + "');"
        }
    )

    // export { a, b as c }
    source = source.replace(/export\s*\{([\s\S]*?)\};?/gm, (_, names) => {
        parseExportList(names).forEach(item => exportLines.push("exp." + item.exported + " = " + item.local + ";"))
        return ""
    })

    // export default async function name(
    source = source.replace(/export default async function\s+([A-Za-z_$][\w$]*)\s*\(/g, (_, name) => {
        exportLines.push("exp.default = " + name + ";")
        return "async function " + name + "("
    })

    // export default function name(
    source = source.replace(/export default function\s+([A-Za-z_$][\w$]*)\s*\(/g, (_, name) => {
        exportLines.push("exp.default = " + name + ";")
        return "function " + name + "("
    })

    // export async function name( or export function name(
    source = source.replace(/export\s+(async\s+)?function\s+([A-Za-z_$][\w$]*)\s*\(/g, (_, async_, name) => {
        exportLines.push("exp." + name + " = " + name + ";")
        return (async_ || "") + "function " + name + "("
    })

    // export class Name
    source = source.replace(/export\s+class\s+([A-Za-z_$][\w$]*)/g, (_, name) => {
        exportLines.push("exp." + name + " = " + name + ";")
        return "class " + name
    })

    // export const / export let
    source = source.replace(/export\s+(const|let|var)\s+([A-Za-z_$][\w$]*)/g, (_, kw, name) => {
        exportLines.push("exp." + name + " = " + name + ";")
        return kw + " " + name
    })

    // export default expr;
    source = source.replace(/export default\s+([^;{][^;]*);/g, (_, expr) => {
        return "exp.default = " + expr + ";"
    })

    if (exportLines.length) source += "\n" + exportLines.join("\n") + "\n"
    return source.trim() + "\n"
}

function collect(file, seen, ordered) {
    const absPath = normalize(file)
    if (seen[absPath] !== undefined) return
    const source = read(absPath)
    seen[absPath] = source
    source.replace(/^\s*import\s+(?:[^'"]*?\s+from\s+)?['"]([^'"]+)['"];\s*$/gm, (_, spec) => {
        if (spec.startsWith(".")) collect(resolveImport(absPath, spec), seen, ordered)
    })
    ordered.push(absPath)
}

function bundle() {
    const seen    = {}
    const ordered = []
    collect(entryFile, seen, ordered)

    let out = ""
    out += "const mods = Object.create(null);\n"
    out += "function defmod(id, fn){ mods[id] = { fn, exports: {}, loaded: false }; }\n"
    out += "function reqmod(id){ var m = mods[id]; if(!m) throw new Error('chains: missing module ' + id); if(!m.loaded){ m.loaded=true; m.fn(m, m.exports); } return m.exports; }\n\n"

    ordered.forEach(absPath => {
        const id     = relId(absPath)
        const source = transformModule(seen[absPath], absPath)
        out += "defmod('" + id + "', function(module, exp){\n"
        out += source.split("\n").map(l => l ? "  " + l : "").join("\n")
        out += "});\n\n"
    })

    out += "const _entry = reqmod('" + relId(entryFile) + "');\n"
    out += "export const evm        = _entry.default;\n"
    out += "export const rpc        = _entry.rpc;\n"
    out += "export const wsRpc      = _entry.wsRpc;\n"
    out += "export const Wallet     = _entry.Wallet;\n"
    out += "export const Contract   = _entry.Contract;\n"
    out += "export const isAddress       = _entry.isAddress;\n";
    out += "export const connector       = _entry.connector;\n";
    out += "export const privateToAddress = _entry.privateToAddress;\n";
    out += "export default { evm: _entry.default, rpc: _entry.rpc, wsRpc: _entry.wsRpc, Wallet: _entry.Wallet, Contract: _entry.Contract, isAddress: _entry.isAddress, connector: _entry.connector, privateToAddress: _entry.privateToAddress };\n";

    write(output, out)
    console.log("Built chains.js (" + (out.length / 1024).toFixed(1) + " KB)")

    // Minify if terser is available
    const terser = nodePath.join(rootDir, "node_modules/.bin/terser")
    if (fs.existsSync(terser)) {
        const result = spawnSync(terser, [output, "--module", "-o", outputMin, "--compress", "--mangle"], { stdio: "inherit" })
        if (result.status === 0) console.log("Built chains.min.js")
        else console.warn("terser minification failed")
    } else {
        console.log("terser not found — skipping chains.min.js")
    }
}

bundle()
