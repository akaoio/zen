/**
 * lib/chains/evm.js — EVM chain adapter for ZEN
 *
 * Standalone module. NOT bundled into zen.js — import explicitly:
 *   import chains from "@akaoio/zen/lib/chains.js"
 *   const provider = chains.evm.rpc("https://rpc.example.com")
 *
 * Drop-in replacement for ethers.js connector in akao.
 * Reuses ZEN's secp256k1 + keccak256 WASM primitives. Zero external deps.
 *
 * Migration note from ethers:
 *   - contract.interface.encodeFunctionData() now returns a Promise (keccak is async).
 *     Callers must `await` it.
 *   - new Wallet(priv) sets .address asynchronously; await wallet._ready before
 *     accessing .address, or use await Wallet.create(priv).
 */

import ZEN from "../../zen.js"

// ─── Byte utilities ───────────────────────────────────────────────────────────

function hexToBytes(hex) {
    const h = typeof hex === "string" && hex.startsWith("0x") ? hex.slice(2) : String(hex || "")
    const s = h.length % 2 ? "0" + h : h
    const out = new Uint8Array(s.length / 2)
    for (let i = 0; i < out.length; i++) out[i] = parseInt(s.slice(i * 2, i * 2 + 2), 16)
    return out
}

function bytesToHex(bytes, prefix = true) {
    const hex = Array.from(bytes).map(b => b.toString(16).padStart(2, "0")).join("")
    return prefix ? "0x" + hex : hex
}

function bigIntToBytes(n, len) {
    const hex = n.toString(16).padStart(len * 2, "0").slice(-(len * 2))
    return hexToBytes(hex)
}

function bigIntToMinimalBytes(n) {
    if (n === 0n) return new Uint8Array(0)
    const hex = n.toString(16)
    return hexToBytes(hex.length % 2 ? "0" + hex : hex)
}

function bytesToBigInt(bytes) {
    if (!bytes || !bytes.length) return 0n
    return BigInt("0x" + bytesToHex(bytes, false))
}

function concatBytes(...parts) {
    const len = parts.reduce((s, p) => s + p.length, 0)
    const out = new Uint8Array(len)
    let pos = 0
    for (const p of parts) { out.set(p, pos); pos += p.length }
    return out
}

function strToBytes(str) {
    return new TextEncoder().encode(str)
}

// ─── Address utilities ────────────────────────────────────────────────────────

async function checksumAddress(addr) {
    const hex = (addr.startsWith("0x") ? addr.slice(2) : addr).toLowerCase().padStart(40, "0")
    const hashHex = await ZEN.hash(strToBytes(hex), null, null, {name: "keccak256", encode: "hex", input: "raw"})
    let out = "0x"
    for (let i = 0; i < 40; i++) out += parseInt(hashHex[i], 16) >= 8 ? hex[i].toUpperCase() : hex[i]
    return out
}

// Lowercase-only address (no EIP-55) — used synchronously in ABI decoding
function addressFromSlot(slot32) {
    return "0x" + bytesToHex(slot32.slice(12), false).toLowerCase()
}

async function privateToAddress(privHex) {
    const pair = await ZEN.pair(null, {priv: privHex, format: "evm"})
    return pair.address
}

function isAddress(str) {
    return typeof str === "string" && /^0x[0-9a-fA-F]{40}$/.test(str)
}

// ─── Tier 1 utilities ─────────────────────────────────────────────────────────

async function _keccak256(data) {
    const bytes = typeof data === "string" ? hexToBytes(data) : data
    return "0x" + await ZEN.hash(bytes, null, null, {name: "keccak256", encode: "hex", input: "raw"})
}

function _parseWordBits(type, kind) {
    if (type === kind) return 256
    const m = new RegExp("^" + kind + "(\\d+)$").exec(type)
    if (!m) return null
    const bits = Number(m[1])
    if (bits < 8 || bits > 256 || bits % 8 !== 0) throw new Error("Invalid Solidity type: " + type)
    return bits
}

function _twosComplementBytes(value, bits) {
    let n = typeof value === "bigint" ? value : BigInt(value)
    const max = 1n << BigInt(bits)
    if (n < 0n) n = max + n
    return bigIntToBytes(n & (max - 1n), bits / 8)
}

function solidityPacked(types, values) {
    if (!Array.isArray(types) || !Array.isArray(values) || types.length !== values.length)
        throw new Error("solidityPacked: types and values length mismatch")
    const parts = types.map((type, i) => {
        const value = values[i]
        if (type === "address") {
            const bytes = hexToBytes(String(value).toLowerCase())
            if (bytes.length !== 20) throw new Error("solidityPacked: address must be 20 bytes")
            return bytes
        }
        if (type === "bool") return new Uint8Array([value ? 1 : 0])
        if (type === "string") return strToBytes(String(value))
        if (type === "bytes") return typeof value === "string" ? hexToBytes(value) : value
        const bytesN = /^bytes(\d+)$/.exec(type)
        if (bytesN) {
            const size = Number(bytesN[1])
            if (size < 1 || size > 32) throw new Error("solidityPacked: invalid bytesN type " + type)
            const bytes = typeof value === "string" ? hexToBytes(value) : value
            if (bytes.length > size) throw new Error("solidityPacked: value too large for " + type)
            const out = new Uint8Array(size)
            out.set(bytes.slice(0, size), 0)
            return out
        }
        const uintBits = _parseWordBits(type, "uint")
        if (uintBits != null) return bigIntToBytes(BigInt(value), uintBits / 8)
        const intBits = _parseWordBits(type, "int")
        if (intBits != null) return _twosComplementBytes(value, intBits)
        throw new Error("solidityPacked: unsupported type " + type)
    })
    return bytesToHex(concatBytes(...parts))
}

async function solidityPackedKeccak256(types, values) {
    return _keccak256(solidityPacked(types, values))
}

async function getCreate2Address(factory, salt, initCodeHash) {
    const factoryBytes = hexToBytes(factory)
    const saltBytes = typeof salt === "bigint" ? bigIntToBytes(salt, 32) : hexToBytes(salt)
    const initCodeHashBytes = hexToBytes(initCodeHash)
    if (factoryBytes.length !== 20) throw new Error("getCreate2Address: factory must be 20 bytes")
    if (saltBytes.length !== 32) throw new Error("getCreate2Address: salt must be 32 bytes")
    if (initCodeHashBytes.length !== 32) throw new Error("getCreate2Address: initCodeHash must be 32 bytes")
    const hash = await _keccak256(concatBytes(new Uint8Array([0xff]), factoryBytes, saltBytes, initCodeHashBytes))
    return checksumAddress("0x" + hash.slice(-40))
}

function _enrichBlock(block) {
    if (!block || block.timestamp == null) return block
    const ts = typeof block.timestamp === "string" ? parseInt(block.timestamp, 16) : Number(block.timestamp)
    block.date = new Date(ts * 1000)
    return block
}

// ─── Human-readable ABI parser ─────────────────────────────────────────────

function _extractParenContent(str) {
    const start = str.indexOf("(")
    if (start === -1) throw new Error("Expected '('")
    let depth = 0
    for (let i = start; i < str.length; i++) {
        const ch = str[i]
        if (ch === "(") depth++
        else if (ch === ")") {
            depth--
            if (depth === 0) return [str.slice(start + 1, i), str.slice(i + 1)]
        }
    }
    throw new Error("Unbalanced parentheses: " + str)
}

function _splitTopLevel(str) {
    const out = []
    let cur = ""
    let paren = 0
    let bracket = 0
    for (const ch of str) {
        if (ch === "," && paren === 0 && bracket === 0) {
            if (cur.trim()) out.push(cur.trim())
            cur = ""
            continue
        }
        if (ch === "(") paren++
        else if (ch === ")") paren--
        else if (ch === "[") bracket++
        else if (ch === "]") bracket--
        cur += ch
    }
    if (cur.trim()) out.push(cur.trim())
    return out
}

function _parseType(typeStr) {
    const src = String(typeStr || "").trim()
    if (!src) return { type: "" }
    if (src.startsWith("(")) {
        const [inner, rest] = _extractParenContent(src)
        const components = _splitTopLevel(inner).map(_parseParam)
        return { type: "tuple" + rest.trim(), components }
    }
    return { type: src }
}

function _parseParam(str) {
    const src = String(str || "").trim()
    if (!src) return { name: "", type: "" }
    if (src.startsWith("(")) {
        const [inner, rest0] = _extractParenContent(src)
        let rest = rest0.trim()
        let suffix = ""
        while (rest.startsWith("[")) {
            const end = rest.indexOf("]")
            if (end === -1) throw new Error("Unbalanced brackets: " + src)
            suffix += rest.slice(0, end + 1)
            rest = rest.slice(end + 1).trim()
        }
        const tokens = rest ? rest.split(/\s+/).filter(Boolean) : []
        const indexed = tokens[0] === "indexed"
        const name = indexed ? (tokens[1] || "") : (tokens[0] || "")
        const typeInfo = _parseType("(" + inner + ")" + suffix)
        const out = { name, type: typeInfo.type }
        if (typeInfo.components) out.components = typeInfo.components
        if (indexed) out.indexed = true
        return out
    }
    const tokens = src.split(/\s+/).filter(Boolean)
    const typeInfo = _parseType(tokens.shift() || "")
    const indexed = tokens[0] === "indexed"
    if (indexed) tokens.shift()
    const name = tokens.join(" ")
    const out = { name, type: typeInfo.type }
    if (typeInfo.components) out.components = typeInfo.components
    if (indexed) out.indexed = true
    return out
}

function parseAbiItem(sig) {
    const src = String(sig || "").trim().replace(/;$/, "")
    if (!src) throw new Error("parseAbiItem: empty signature")
    if (!/^(function|event|error|constructor|receive|fallback)\b/.test(src)) {
        return parseAbiItem("function " + src)
    }
    if (src.startsWith("function ")) {
        const rest = src.slice(9).trim()
        const name = rest.slice(0, rest.indexOf("(")).trim()
        const [inputsRaw, afterInputs0] = _extractParenContent(rest.slice(name.length))
        const inputs = inputsRaw.trim() ? _splitTopLevel(inputsRaw).map(_parseParam) : []
        const afterInputs = afterInputs0.trim()
        let outputs = []
        const returnsIdx = afterInputs.indexOf("returns")
        if (returnsIdx !== -1) {
            const afterReturns = afterInputs.slice(returnsIdx + 7).trim()
            const [outputsRaw] = _extractParenContent(afterReturns)
            outputs = outputsRaw.trim() ? _splitTopLevel(outputsRaw).map(_parseParam) : []
        }
        let stateMutability = "nonpayable"
        if (/\bview\b/.test(afterInputs)) stateMutability = "view"
        else if (/\bpure\b/.test(afterInputs)) stateMutability = "pure"
        else if (/\bpayable\b/.test(afterInputs)) stateMutability = "payable"
        return { type: "function", name, inputs, outputs, stateMutability }
    }
    if (src.startsWith("event ")) {
        const rest = src.slice(6).trim()
        const name = rest.slice(0, rest.indexOf("(")).trim()
        const [inputsRaw, afterInputs] = _extractParenContent(rest.slice(name.length))
        const inputs = inputsRaw.trim() ? _splitTopLevel(inputsRaw).map(_parseParam) : []
        const out = { type: "event", name, inputs }
        if (/\banonymous\b/.test(afterInputs)) out.anonymous = true
        return out
    }
    if (src.startsWith("error ")) {
        const rest = src.slice(6).trim()
        const name = rest.slice(0, rest.indexOf("(")).trim()
        const [inputsRaw] = _extractParenContent(rest.slice(name.length))
        const inputs = inputsRaw.trim() ? _splitTopLevel(inputsRaw).map(_parseParam) : []
        return { type: "error", name, inputs }
    }
    if (src.startsWith("constructor")) {
        const [inputsRaw, afterInputs] = _extractParenContent(src.slice("constructor".length))
        const inputs = inputsRaw.trim() ? _splitTopLevel(inputsRaw).map(_parseParam) : []
        return { type: "constructor", inputs, stateMutability: /\bpayable\b/.test(afterInputs) ? "payable" : "nonpayable" }
    }
    if (src.startsWith("receive")) return { type: "receive", stateMutability: /\bpayable\b/.test(src) ? "payable" : "nonpayable" }
    if (src.startsWith("fallback")) return { type: "fallback", stateMutability: /\bpayable\b/.test(src) ? "payable" : "nonpayable" }
    throw new Error("parseAbiItem: unsupported signature " + sig)
}

function parseAbi(abi) {
    if (typeof abi === "string") return [parseAbiItem(abi.trim())]
    if (!Array.isArray(abi)) return abi
    return abi.map(item => typeof item === "string" ? parseAbiItem(item.trim()) : item)
}

// ─── RLP encoder ─────────────────────────────────────────────────────────────

function rlpLengthPrefix(len, offset) {
    if (len < 56) return new Uint8Array([offset + len])
    const lb = bigIntToMinimalBytes(BigInt(len))
    return new Uint8Array([offset + 55 + lb.length, ...lb])
}

function rlpEncode(input) {
    if (typeof input === "bigint") {
        if (input === 0n) return new Uint8Array([0x80])
        return rlpEncode(bigIntToMinimalBytes(input))
    }
    if (typeof input === "number") return rlpEncode(BigInt(input))
    if (input instanceof Uint8Array) {
        if (input.length === 0) return new Uint8Array([0x80])
        if (input.length === 1 && input[0] < 0x80) return new Uint8Array([input[0]])
        return concatBytes(rlpLengthPrefix(input.length, 0x80), input)
    }
    if (Array.isArray(input)) {
        const parts = input.map(rlpEncode)
        const total = parts.reduce((s, p) => s + p.length, 0)
        return concatBytes(rlpLengthPrefix(total, 0xc0), ...parts)
    }
    throw new Error("RLP: unsupported type: " + typeof input)
}

// ─── ABI encoder / decoder ────────────────────────────────────────────────────

function _parseArrayType(type) {
    const m = /^(.*)\[(.*?)\]$/.exec(type || "")
    if (!m) return null
    return { baseType: m[1], length: m[2] === "" ? null : Number(m[2]) }
}

function _isTupleType(type) {
    return /^tuple(\[[0-9]*\])*$/.test(type || "")
}

function _staticWords(type, components) {
    if (!type) return 1
    const arr = _parseArrayType(type)
    if (arr) {
        if (arr.length == null) return 1
        if (isDynamicType(arr.baseType, components)) return 1
        return arr.length * _staticWords(arr.baseType, components)
    }
    if (_isTupleType(type)) {
        if (isDynamicType(type, components)) return 1
        return (components || []).reduce((sum, c) => sum + _staticWords(c.type, c.components), 0)
    }
    return 1
}

function isDynamicType(type, components) {
    if (!type) return false
    if (type === "bytes" || type === "string") return true
    const arr = _parseArrayType(type)
    if (arr) return arr.length === null || isDynamicType(arr.baseType, components)
    if (_isTupleType(type)) return (components || []).some(c => isDynamicType(c.type, c.components))
    return false
}

function encodeStatic(type, value) {
    if (type === "address") {
        const bytes = hexToBytes(value)
        const slot = new Uint8Array(32)
        slot.set(bytes.slice(-20), 12)
        return slot
    }
    if (type === "bool") {
        const slot = new Uint8Array(32)
        slot[31] = value ? 1 : 0
        return slot
    }
    if (type.startsWith("uint") || type === "uint" || type.startsWith("int") || type === "int") {
        let n = typeof value === "bigint" ? value : BigInt(value.toString())
        if (n < 0n) n = (1n << 256n) + n
        return bigIntToBytes(n & ((1n << 256n) - 1n), 32)
    }
    if (type === "bytes32" || /^bytes\d+$/.test(type)) {
        const bytes = typeof value === "string" ? hexToBytes(value) : value
        const slot = new Uint8Array(32)
        slot.set(bytes.slice(0, 32), 0)
        return slot
    }
    throw new Error("ABI encode: unsupported static type: " + type)
}

function _normalizeArrayValue(value) {
    if (Array.isArray(value)) return value
    if (value == null) return []
    return Array.from(value)
}

function encodeDynamic(type, value, components) {
    if (type === "bytes") {
        const bytes = typeof value === "string" ? hexToBytes(value) : value
        const lenSlot = bigIntToBytes(BigInt(bytes.length), 32)
        const padded = new Uint8Array(Math.ceil(bytes.length / 32) * 32)
        padded.set(bytes)
        return concatBytes(lenSlot, padded)
    }
    if (type === "string") {
        const bytes = strToBytes(value)
        const lenSlot = bigIntToBytes(BigInt(bytes.length), 32)
        const padded = new Uint8Array(Math.ceil(bytes.length / 32) * 32)
        padded.set(bytes)
        return concatBytes(lenSlot, padded)
    }
    const arr = _parseArrayType(type)
    if (arr) {
        const values = _normalizeArrayValue(value)
        if (arr.length != null && values.length !== arr.length)
            throw new Error("ABI encode: array length mismatch for " + type)
        const encoded = encodeParams(values.map(v => ({ type: arr.baseType, value: v, components })))
        return arr.length == null
            ? concatBytes(bigIntToBytes(BigInt(values.length), 32), encoded)
            : encoded
    }
    throw new Error("ABI encode: unsupported dynamic type: " + type)
}

function encodeParams(params) {
    const headWords = params.reduce((sum, p) => sum + _staticWords(p.type, p.components), 0)
    const headParts = new Array(params.length)
    const tailParts = []
    let tailOffset = headWords * 32

    for (let i = 0; i < params.length; i++) {
        const { type, value, components } = params[i]
        const arr = _parseArrayType(type)
        if (_isTupleType(type) && !arr) {
            const comp = (components || []).map((c, j) => ({
                type: c.type,
                value: Array.isArray(value) ? value[j] : value?.[c.name],
                components: c.components
            }))
            const tupleData = encodeParams(comp)
            if (isDynamicType(type, components)) {
                headParts[i] = bigIntToBytes(BigInt(tailOffset), 32)
                tailParts.push(tupleData)
                tailOffset += tupleData.length
            } else {
                headParts[i] = tupleData
            }
        } else if (arr) {
            const data = encodeDynamic(type, value, components)
            if (isDynamicType(type, components)) {
                headParts[i] = bigIntToBytes(BigInt(tailOffset), 32)
                tailParts.push(data)
                tailOffset += data.length
            } else {
                headParts[i] = data
            }
        } else if (isDynamicType(type, components)) {
            headParts[i] = bigIntToBytes(BigInt(tailOffset), 32)
            const tail = encodeDynamic(type, value, components)
            tailParts.push(tail)
            tailOffset += tail.length
        } else {
            headParts[i] = encodeStatic(type, value)
        }
    }
    return concatBytes(...headParts, ...tailParts)
}

function _canonicalType(inp) {
    const m = /^tuple((\[[0-9]*\])*)$/.exec(inp.type)
    if (!m) return inp.type
    return "(" + (inp.components || []).map(_canonicalType).join(",") + ")" + (m[1] || "")
}

function buildSig(abiFunc) {
    const types = abiFunc.inputs.map(_canonicalType)
    return abiFunc.name + "(" + types.join(",") + ")"
}

async function buildCalldata(sig, inputs, values) {
    const sel = hexToBytes(await _keccak256(strToBytes(sig))).slice(0, 4)
    if (!inputs || !inputs.length) return bytesToHex(sel)
    const params = inputs.map((inp, i) => ({
        type: inp.type,
        value: values[i],
        components: inp.components
    }))
    return bytesToHex(concatBytes(sel, encodeParams(params)))
}

function decodeSingle(type, bytes, offset) {
    if (type === "address") return addressFromSlot(bytes.slice(offset, offset + 32))
    if (type === "bool") return bytes[offset + 31] !== 0
    if (type.startsWith("uint") || type === "uint") return bytesToBigInt(bytes.slice(offset, offset + 32))
    if (type.startsWith("int") || type === "int") {
        const n = bytesToBigInt(bytes.slice(offset, offset + 32))
        return n >= (1n << 255n) ? n - (1n << 256n) : n
    }
    if (type === "bytes32" || /^bytes\d+$/.test(type)) return bytesToHex(bytes.slice(offset, offset + 32))
    if (type === "bytes" || type === "string") {
        const ptr = Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
        const len = Number(bytesToBigInt(bytes.slice(ptr, ptr + 32)))
        const raw = bytes.slice(ptr + 32, ptr + 32 + len)
        return type === "string" ? new TextDecoder().decode(raw) : bytesToHex(raw)
    }
    throw new Error("ABI decode: unsupported type: " + type)
}

function _decodeArray(param, bytes, start, length, baseOffset) {
    const out = []
    const step = _staticWords(param.type, param.components) * 32
    let cursor = start
    for (let i = 0; i < length; i++) {
        out.push(_decodeAbi(param, bytes, cursor, baseOffset))
        cursor += step
    }
    return out
}

function _decodeAbi(param, bytes, offset, baseOffset = 0) {
    const { type, components } = param
    const arr = _parseArrayType(type)
    if (arr) {
        const elem = { type: arr.baseType, components }
        if (arr.length == null) {
            const ptr = Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
            const arrStart = baseOffset + ptr
            const len = Number(bytesToBigInt(bytes.slice(arrStart, arrStart + 32)))
            const headStart = arrStart + 32
            return _decodeArray(elem, bytes, headStart, len, headStart)
        }
        const start = isDynamicType(type, components)
            ? baseOffset + Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
            : offset
        return _decodeArray(elem, bytes, start, arr.length, start)
    }
    if (_isTupleType(type)) {
        const tupleStart = isDynamicType(type, components)
            ? baseOffset + Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
            : offset
        const result = {}
        let cursor = tupleStart
        for (const c of (components || [])) {
            const val = _decodeAbi(c, bytes, cursor, tupleStart)
            result[c.name] = val
            cursor += _staticWords(c.type, c.components) * 32
        }
        return result
    }
    if (type === "bytes" || type === "string") {
        const ptr = Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
        const start = baseOffset + ptr
        const len = Number(bytesToBigInt(bytes.slice(start, start + 32)))
        const raw = bytes.slice(start + 32, start + 32 + len)
        return type === "string" ? new TextDecoder().decode(raw) : bytesToHex(raw)
    }
    return decodeSingle(type, bytes, offset)
}

function decodeTuple(components, bytes, startOffset) {
    return _decodeAbi({ type: "tuple", components }, bytes, startOffset, startOffset)
}

function decodeReturnData(outputs, raw) {
    if (!raw || raw === "0x" || !outputs || !outputs.length) return null
    const bytes = hexToBytes(raw)
    if (outputs.length === 1) return _decodeAbi(outputs[0], bytes, 0, 0)
    const result = []
    let offset = 0
    for (const out of outputs) {
        const val = _decodeAbi(out, bytes, offset, 0)
        result.push(val)
        if (out.name) result[out.name] = val
        offset += _staticWords(out.type, out.components) * 32
    }
    return result
}

// ─── JSON-RPC HTTP Provider ───────────────────────────────────────────────────

let _globalReqId = 1

class Provider {
    constructor(url, network) {
        this.url = url
        this.network = network || null
        this._chainId = network?.chainId ? BigInt(network.chainId) : null
    }

    async send(method, params = []) {
        const id = _globalReqId++
        const res = await fetch(this.url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ jsonrpc: "2.0", id, method, params })
        })
        if (!res.ok) throw new Error("RPC HTTP " + res.status + ": " + this.url)
        const json = await res.json()
        if (json.error) throw new Error("RPC [" + json.error.code + "] " + json.error.message)
        return json.result
    }

    async getChainId() {
        if (this._chainId) return this._chainId
        this._chainId = BigInt(await this.send("eth_chainId", []))
        return this._chainId
    }

    async getBlock(tag) {
        const t = typeof tag === "number" ? "0x" + tag.toString(16) : tag
        return _enrichBlock(await this.send("eth_getBlockByNumber", [t, false]))
    }

    async getBalance(address) {
        return BigInt(await this.send("eth_getBalance", [address, "latest"]))
    }

    async getTransactionCount(address, tag = "latest") {
        return BigInt(await this.send("eth_getTransactionCount", [address, tag]))
    }

    async getGasPrice() {
        const suggested = BigInt(await this.send("eth_gasPrice", []))
        try {
            const block = await this.send("eth_getBlockByNumber", ["latest", false])
            if (block?.baseFeePerGas) {
                const baseFee = BigInt(block.baseFeePerGas)
                if (suggested < baseFee) return baseFee * 12n / 10n
            }
        } catch {}
        return suggested
    }

    async getFeeData() {
        const gasPrice = await this.getGasPrice()
        return { gasPrice, maxFeePerGas: null, maxPriorityFeePerGas: null }
    }

    async estimateGas(tx) {
        const obj = {}
        if (tx.from)             obj.from  = tx.from
        if (tx.to)               obj.to    = tx.to
        if (tx.data)             obj.data  = tx.data
        if (tx.value !== undefined) obj.value = "0x" + BigInt(tx.value).toString(16)
        return BigInt(await this.send("eth_estimateGas", [obj]))
    }

    async call(tx, blockTag = "latest") {
        const obj = { to: tx.to, data: tx.data }
        if (tx.from) obj.from = tx.from
        if (tx.value !== undefined) obj.value = "0x" + BigInt(tx.value).toString(16)
        const tag = typeof blockTag === "bigint" || typeof blockTag === "number"
            ? "0x" + BigInt(blockTag).toString(16)
            : blockTag
        return this.send("eth_call", [obj, tag])
    }

    async getLogs(filter) {
        const f = {}
        if (filter.address) f.address = filter.address
        if (filter.topics) f.topics = filter.topics
        if (filter.fromBlock !== undefined) f.fromBlock = typeof filter.fromBlock === "number" ? "0x" + filter.fromBlock.toString(16) : filter.fromBlock
        if (filter.toBlock !== undefined) f.toBlock = typeof filter.toBlock === "number" ? "0x" + filter.toBlock.toString(16) : filter.toBlock
        return this.send("eth_getLogs", [f])
    }

    async multicall(calls) {
        const callItems = []
        for (const call of calls) {
            const parsedAbi = parseAbi(call.abi)
            const abi = Array.isArray(parsedAbi) ? parsedAbi : [parsedAbi]
            const item = abi.find(entry => entry.type === "function" && (entry.name === call.method || buildSig(entry) === call.method))
            if (!item) throw new Error("multicall: unknown method " + call.method)
            const callData = await buildCalldata(buildSig(item), item.inputs || [], call.args || [])
            callItems.push({ ...call, abi, item, callData })
        }
        const aggregate = MULTICALL3_ABI[0]
        const data = await buildCalldata(buildSig(aggregate), aggregate.inputs, [callItems.map(call => ({
            target: call.address,
            allowFailure: !!call.allowFailure,
            callData: call.callData,
        }))])
        const raw = await this.call({ to: MULTICALL3_ADDRESS, data })
        const decoded = decodeReturnData(aggregate.outputs, raw) || []
        return decoded.map((result, i) => {
            if (!result?.success) return null
            const outputs = callItems[i].item.outputs || []
            return decodeReturnData(outputs, result.returnData)
        })
    }

    async sendRawTransaction(rawHex) {
        return this.send("eth_sendRawTransaction", [rawHex])
    }

    async getTransactionReceipt(hash) {
        return this.send("eth_getTransactionReceipt", [hash])
    }

    async waitForTransaction(hash, timeout = 120000) {
        const start = Date.now()
        while (Date.now() - start < timeout) {
            const receipt = await this.getTransactionReceipt(hash)
            if (receipt) {
                if (parseInt(receipt.status, 16) === 0)
                    throw Object.assign(new Error("Transaction reverted: " + hash), { receipt })
                return receipt
            }
            await new Promise(r => setTimeout(r, 2000))
        }
        throw new Error("Transaction timeout: " + hash)
    }
}

function rpc(url, network) { return new Provider(url, network) }

// ─── WebSocket Provider ───────────────────────────────────────────────────────

class WsProvider {
    constructor(url, network) {
        this.url = url
        this.network = network || null
        this._ws = null
        this._pending = new Map()
        this._subs = new Map()
        this._subTopics = new Map()
        this._subHandlers = new Map()
        this._reqId = 1
        this._ready = null
        this._reconnectDelay = 1000
        this._maxReconnectDelay = 30000
        this._reconnecting = false
        this._destroyed = false
        this._errorHandler = null
    }

    _connect() {
        if (this._ready) return this._ready
        this._ready = new Promise((resolve, reject) => {
            const ws = new WebSocket(this.url)
            this._ws = ws
            let settled = false
            let opened = false
            ws.onopen = () => {
                opened = true
                this._reconnecting = false
                this._reconnectDelay = 1000
                if (!settled) {
                    settled = true
                    resolve()
                }
            }
            ws.onerror = (ev) => {
                if (this._errorHandler) this._errorHandler(ev)
                if (!opened && !settled) {
                    settled = true
                    this._ready = null
                    reject(new Error("WS connect failed: " + this.url))
                }
            }
            ws.onmessage = (ev) => {
                try {
                    const msg = JSON.parse(ev.data)
                    if (msg.id && this._pending.has(msg.id)) {
                        const { resolve: res, reject: rej } = this._pending.get(msg.id)
                        this._pending.delete(msg.id)
                        if (msg.error) rej(new Error(msg.error.message))
                        else res(msg.result)
                    } else if (msg.method === "eth_subscription" && msg.params) {
                        const handler = this._subs.get(msg.params.subscription)
                        if (handler) handler(msg.params.result)
                    }
                } catch {}
            }
            ws.onclose = () => {
                this._ready = null
                if (!settled) {
                    settled = true
                    reject(new Error("WS closed: " + this.url))
                }
                for (const { reject: rej } of this._pending.values()) rej(new Error("WS closed"))
                this._pending.clear()
                this._subs.clear()
                this._subTopics.clear()
                if (!this._destroyed) this._scheduleReconnect()
            }
        })
        return this._ready
    }

    _scheduleReconnect() {
        if (this._reconnecting || this._destroyed) return
        this._reconnecting = true
        const delay = this._reconnectDelay
        this._reconnectDelay = Math.min(this._reconnectDelay * 2, this._maxReconnectDelay)
        setTimeout(async () => {
            try {
                await this._connect()
                await this._resubscribeAll()
                this._reconnecting = false
            } catch {
                this._ready = null
                this._reconnecting = false
                if (!this._destroyed) this._scheduleReconnect()
            }
        }, delay)
    }

    async _subscribeTopic(topic, handler) {
        const subId = await this.send("eth_subscribe", [topic])
        this._subs.set(subId, handler)
        this._subTopics.set(subId, topic)
        return subId
    }

    async _resubscribeAll() {
        this._subs.clear()
        this._subTopics.clear()
        for (const [topic, handler] of this._subHandlers.entries()) {
            await this._subscribeTopic(topic, handler)
        }
    }

    async send(method, params = []) {
        await this._connect()
        return new Promise((resolve, reject) => {
            const id = this._reqId++
            this._pending.set(id, { resolve, reject })
            this._ws.send(JSON.stringify({ jsonrpc: "2.0", id, method, params }))
        })
    }

    async on(event, handler) {
        await this._connect()
        if (event === "block") {
            const wrapped = result => handler(result?.number != null ? parseInt(result.number, 16) : result)
            this._subHandlers.set("newHeads", wrapped)
            return this._subscribeTopic("newHeads", wrapped)
        }
        if (event === "error") {
            this._errorHandler = handler
            return null
        }
    }

    async off(subIdOrEvent) {
        if (subIdOrEvent === "block") {
            this._subHandlers.delete("newHeads")
            for (const [subId, topic] of this._subTopics.entries()) {
                if (topic === "newHeads") {
                    try { await this.send("eth_unsubscribe", [subId]) } catch {}
                    this._subs.delete(subId)
                    this._subTopics.delete(subId)
                }
            }
            return
        }
        if (subIdOrEvent && typeof subIdOrEvent === "string" && subIdOrEvent.startsWith("0x")) {
            const topic = this._subTopics.get(subIdOrEvent)
            try { await this.send("eth_unsubscribe", [subIdOrEvent]) } catch {}
            this._subs.delete(subIdOrEvent)
            this._subTopics.delete(subIdOrEvent)
            if (topic) this._subHandlers.delete(topic)
        }
    }

    async getChainId() { return BigInt(await this.send("eth_chainId", [])) }
    async getBlock(tag) {
        const t = typeof tag === "number" ? "0x" + tag.toString(16) : tag
        return _enrichBlock(await this.send("eth_getBlockByNumber", [t, false]))
    }
    async getBalance(addr) { return BigInt(await this.send("eth_getBalance", [addr, "latest"])) }
    async getGasPrice() { return BigInt(await this.send("eth_gasPrice", [])) }
    async getFeeData() { return { gasPrice: await this.getGasPrice(), maxFeePerGas: null, maxPriorityFeePerGas: null } }
    async getTransactionCount(addr, tag = "latest") {
        return BigInt(await this.send("eth_getTransactionCount", [addr, tag]))
    }
    async call(tx, blockTag = "latest") {
        const obj = { to: tx.to, data: tx.data }
        if (tx.from) obj.from = tx.from
        if (tx.value !== undefined) obj.value = "0x" + BigInt(tx.value).toString(16)
        const tag = typeof blockTag === "bigint" || typeof blockTag === "number"
            ? "0x" + BigInt(blockTag).toString(16) : blockTag
        return this.send("eth_call", [obj, tag])
    }
    async getLogs(filter) {
        const f = {}
        if (filter.address) f.address = filter.address
        if (filter.topics) f.topics = filter.topics
        if (filter.fromBlock !== undefined) f.fromBlock = typeof filter.fromBlock === "number" ? "0x" + filter.fromBlock.toString(16) : filter.fromBlock
        if (filter.toBlock !== undefined) f.toBlock = typeof filter.toBlock === "number" ? "0x" + filter.toBlock.toString(16) : filter.toBlock
        return this.send("eth_getLogs", [f])
    }
    async estimateGas(tx) {
        const obj = {}
        if (tx.from) obj.from = tx.from
        if (tx.to)   obj.to   = tx.to
        if (tx.data) obj.data = tx.data
        if (tx.value !== undefined) obj.value = "0x" + BigInt(tx.value).toString(16)
        return BigInt(await this.send("eth_estimateGas", [obj]))
    }
    async destroy() {
        this._destroyed = true
        if (this._ws) this._ws.close()
    }
}

function wsRpc(url, network) { return new WsProvider(url, network) }

// ─── EIP-155 transaction signing ─────────────────────────────────────────────

async function signTransaction(tx, privHex, chainId) {
    const cid      = typeof chainId === "bigint" ? chainId : BigInt(chainId)
    const nonce    = typeof tx.nonce    === "bigint" ? tx.nonce    : BigInt(tx.nonce    ?? 0)
    const gasPrice = typeof tx.gasPrice === "bigint" ? tx.gasPrice : BigInt(tx.gasPrice ?? 0)
    const gasLimit = typeof tx.gasLimit === "bigint" ? tx.gasLimit : BigInt(tx.gasLimit ?? 21000)
    const to20     = tx.to ? hexToBytes(tx.to) : new Uint8Array(0)
    const value    = typeof tx.value === "bigint" ? tx.value : BigInt(tx.value ?? 0)
    const data     = tx.data ? hexToBytes(tx.data) : new Uint8Array(0)

    // EIP-155: pre-signing RLP includes chainId, 0, 0
    const unsigned = rlpEncode([nonce, gasPrice, gasLimit, to20, value, data, cid, 0n, 0n])
    const hashHex  = await ZEN.hash(unsigned, null, null, {name: "keccak256", encode: "hex", input: "raw"})
    const pair     = await ZEN.pair(null, {priv: privHex, format: "evm"})
    const sig      = await ZEN.sign("0x" + hashHex, pair, null, {prehash: true, encode: "raw"})

    const vBig   = cid * 2n + 35n + BigInt(sig.v)   // EIP-155 replay protection
    const signed = rlpEncode([nonce, gasPrice, gasLimit, to20, value, data, vBig, BigInt(sig.r), BigInt(sig.s)])
    return bytesToHex(signed)
}

// ─── Wallet ───────────────────────────────────────────────────────────────────

class Wallet {
    constructor(privHex, provider) {
        // Accept "0x" + 64 hex chars or bare 64 hex chars
        const h = String(privHex || "")
        this._priv    = /^0x/i.test(h) ? h : "0x" + h
        this._provider = provider || null
        this.address   = null
        this._ready    = this._init()
    }

    async _init() {
        this.address = await privateToAddress(this._priv)
        return this
    }

    connect(provider) {
        const w = new Wallet(this._priv, provider)
        // If address already computed, carry it over to skip async work
        if (this.address) {
            w.address = this.address
            w._ready  = Promise.resolve(w)
        }
        return w
    }

    async sendTransaction(tx) {
        const provider = this._provider
        if (!provider) throw new Error("Wallet: no provider attached. Use wallet.connect(provider).")
        await this._ready

        const chainId  = await provider.getChainId()
        const nonce    = tx.nonce    != null ? BigInt(tx.nonce)    : await provider.getTransactionCount(this.address, "latest")
        const gasPrice = tx.gasPrice != null ? BigInt(tx.gasPrice) : await provider.getGasPrice()
        let   gasLimit = tx.gasLimit != null ? BigInt(tx.gasLimit) : null

        if (!gasLimit) {
            try {
                const est = await provider.estimateGas({ ...tx, from: this.address })
                gasLimit = est * 120n / 100n   // 20 % buffer
            } catch {
                gasLimit = 200000n
            }
        }

        const raw  = await signTransaction({ ...tx, nonce, gasPrice, gasLimit }, this._priv, chainId)
        const hash = await provider.sendRawTransaction(raw)

        const wait = async () => {
            const receipt = await provider.waitForTransaction(hash)
            return receipt
        }
        return { hash, wait }
    }

    // Async factory — address is guaranteed ready on return
    static async create(privHex, provider) {
        const w = new Wallet(privHex, provider)
        await w._ready
        return w
    }
}

// ─── Event decoding ───────────────────────────────────────────────────────────

// Returns Map<topic0Hex, abiEventEntry>
async function buildEventTopicMap(abi) {
    const map = new Map()
    for (const item of abi) {
        if (item.type !== "event") continue
        const topic0 = await _keccak256(strToBytes(buildSig(item)))
        map.set(topic0.toLowerCase(), item)
    }
    return map
}

function decodeEventLog(abiEvent, log) {
    const rv = {}
    const topics = log.topics || []
    const data   = log.data ? hexToBytes(log.data) : new Uint8Array(0)
    let   topicIdx  = 1
    let   dataOffset = 0

    for (const inp of abiEvent.inputs) {
        if (inp.indexed) {
            if (topicIdx < topics.length) {
                const tb = hexToBytes(topics[topicIdx++])
                rv[inp.name] = decodeSingle(inp.type, tb, 0)
            }
        } else {
            rv[inp.name] = decodeSingle(inp.type, data, dataOffset)
            dataOffset += 32
        }
    }
    return rv
}

async function decodeReceiptEvents(abi, receipt) {
    const topicMap = await buildEventTopicMap(abi)
    const events   = {}
    for (const log of (receipt.logs || [])) {
        if (!log.topics?.length) continue
        const abiEvent = topicMap.get(log.topics[0].toLowerCase())
        if (!abiEvent) continue
        events[abiEvent.name] = { returnValues: decodeEventLog(abiEvent, log), raw: log }
    }
    return events
}

// ─── Contract ─────────────────────────────────────────────────────────────────

class Contract {
    constructor(address, abi, providerOrWallet) {
        this.address  = address
        const parsedAbi = parseAbi(abi)
        this.abi      = Array.isArray(parsedAbi) ? parsedAbi : [parsedAbi]
        this._signer  = providerOrWallet instanceof Wallet ? providerOrWallet : null
        this._provider = providerOrWallet instanceof Wallet
            ? providerOrWallet._provider
            : providerOrWallet

        this.interface = {
            encodeFunctionData: (name, args) => {
                const item = this.abi.find(i => i.type === "function" && i.name === name)
                if (!item) throw new Error("Contract.interface: unknown function " + name)
                return buildCalldata(buildSig(item), item.inputs, args)
            }
        }

        this._buildMethods()
    }

    _buildMethods() {
        for (const item of this.abi) {
            if (item.type !== "function") continue
            const name = item.name
            if (name in this && typeof this[name] === "function" && !this[name]._abiGenerated) continue

            const isReadOnly = item.stateMutability === "view" || item.stateMutability === "pure"
            const method = async (...args) => {
                const sig      = buildSig(item)
                const calldata = await buildCalldata(sig, item.inputs, args)

                if (isReadOnly) {
                    const hasOverrides = args.length > item.inputs.length && args[args.length - 1] !== null
                        && typeof args[args.length - 1] === "object" && !Array.isArray(args[args.length - 1])
                    const overrides  = hasOverrides ? args[args.length - 1] : {}
                    const blockTag   = overrides.blockTag ?? "latest"
                    const raw = await this._provider.call({ to: this.address, data: calldata }, blockTag)
                    return decodeReturnData(item.outputs, raw)
                }
                if (!this._signer) throw new Error("Contract: no signer for write method '" + name + "'")
                await this._signer._ready
                const hasWriteOverrides = args.length > item.inputs.length
                    && args[args.length - 1] !== null
                    && typeof args[args.length - 1] === "object"
                    && !Array.isArray(args[args.length - 1])
                const ov = hasWriteOverrides ? args[args.length - 1] : {}
                const txResp = await this._signer.sendTransaction({
                    to:       this.address,
                    data:     calldata,
                    value:    ov.value    != null ? BigInt(ov.value)    : 0n,
                    gasLimit: ov.gasLimit != null ? BigInt(ov.gasLimit) : undefined,
                    gasPrice: ov.gasPrice != null ? BigInt(ov.gasPrice) : undefined,
                })
                const receipt = await txResp.wait()
                receipt.events = await decodeReceiptEvents(this.abi, receipt)
                return receipt
            }
            method._abiGenerated = true
            this[name] = method
        }
    }

    async queryFilter(eventAbi, fromBlock = 0, toBlock = "latest") {
        const abiEvent = typeof eventAbi === "string"
            ? this.abi.find(item => item.type === "event" && item.name === eventAbi)
            : eventAbi
        if (!abiEvent) throw new Error("Contract.queryFilter: unknown event " + eventAbi)
        const topic0 = abiEvent.anonymous ? null : await _keccak256(strToBytes(buildSig(abiEvent)))
        const logs = await this._provider.getLogs({
            address: this.address,
            topics: topic0 ? [topic0] : undefined,
            fromBlock,
            toBlock,
        })
        return logs.map(log => Object.assign({}, decodeEventLog(abiEvent, log), {
            _log: log,
            blockNumber: log.blockNumber,
            transactionHash: log.transactionHash,
        }))
    }

    connect(signerOrProvider) {
        return new Contract(this.address, this.abi, signerOrProvider)
    }
}

// ─── Ethers-compatible connector shim ─────────────────────────────────────────
// Drop-in for `connector: ethers` in akao's EVM.js.
// Usage: replace `import { ethers } from "../Ethers.js"` and
//        change `connector: ethers` to `connector: evmConnector`.

const MULTICALL3_ADDRESS = "0xcA11bde05977b3631167028862bE2a173976CA11"
const MULTICALL3_ABI = [
    {
        type: "function", name: "aggregate3", stateMutability: "view",
        inputs: [{ name: "calls", type: "tuple[]", components: [
            { name: "target",       type: "address" },
            { name: "allowFailure", type: "bool"    },
            { name: "callData",     type: "bytes"   },
        ]}],
        outputs: [{ name: "returnData", type: "tuple[]", components: [
            { name: "success",    type: "bool"  },
            { name: "returnData", type: "bytes" },
        ]}],
    }
]

const connector = {
    JsonRpcProvider:   Provider,
    WebSocketProvider: WsProvider,
    Wallet,
    Contract,
    isAddress,
}

// ─── Unit conversion (ethers-compatible) ─────────────────────────────────────

const _UNIT_DECIMALS = { wei: 0, kwei: 3, mwei: 6, gwei: 9, szabo: 12, finney: 15, ether: 18 }

// formatUnits(1500000n, 6) → "1.5"   formatUnits(10n**18n, "ether") → "1.0"
function formatUnits(value, decimals = 18) {
    const d = typeof decimals === "string" ? _UNIT_DECIMALS[decimals] : Number(decimals)
    const n = typeof value === "bigint" ? value : BigInt(value)
    if (d === 0) return n.toString()
    const factor = 10n ** BigInt(d)
    const abs    = n < 0n ? -n : n
    const whole  = abs / factor
    const frac   = (abs % factor).toString().padStart(d, "0").replace(/0+$/, "") || "0"
    return (n < 0n ? "-" : "") + whole.toString() + "." + frac
}

// parseUnits("1.5", 6) → 1500000n    parseUnits("1.5", "ether") → 1500000000000000000n
function parseUnits(value, decimals = 18) {
    const d   = typeof decimals === "string" ? _UNIT_DECIMALS[decimals] : Number(decimals)
    const str = String(value).trim()
    const neg = str.startsWith("-")
    const abs = neg ? str.slice(1) : str
    const dot = abs.indexOf(".")
    if (dot === -1) {
        const result = BigInt(abs) * 10n ** BigInt(d)
        return neg ? -result : result
    }
    const whole = abs.slice(0, dot)
    const frac  = abs.slice(dot + 1).slice(0, d).padEnd(d, "0")
    const result = BigInt(whole) * 10n ** BigInt(d) + BigInt(frac)
    return neg ? -result : result
}

const formatEther = v => formatUnits(v, 18)
const parseEther  = v => parseUnits(v, 18)

// ─── V3 path encoding ─────────────────────────────────────────────────────────

// Encodes a V3 multi-hop path for exactInput / exactOutput:
//   tokens: [tokenA, tokenB, tokenC, ...]  (n addresses)
//   fees:   [feeAB, feeBC, ...]            (n-1 uint24 fee tiers)
// Returns 0x-prefixed hex: tokenA ++ feeAB ++ tokenB ++ feeBC ++ tokenC ...
function encodePath(tokens, fees) {
    if (tokens.length !== fees.length + 1)
        throw new Error("encodePath: tokens.length must be fees.length + 1")
    let hex = ""
    for (let i = 0; i < tokens.length; i++) {
        hex += tokens[i].replace("0x", "").toLowerCase().padStart(40, "0")
        if (i < fees.length)
            hex += Number(fees[i]).toString(16).padStart(6, "0")
    }
    return "0x" + hex
}

// ─── EIP-712 TypedDataEncoder ─────────────────────────────────────────────

function _stripArraySuffix(type) {
    return String(type || "").replace(/\[[^\]]*\]/g, "")
}

function _encodeType(primaryType, types) {
    const seen = new Set()
    const visit = (typeName) => {
        if (seen.has(typeName) || !types[typeName]) return
        seen.add(typeName)
        for (const field of types[typeName]) {
            const base = _stripArraySuffix(field.type)
            if (base !== typeName && types[base]) visit(base)
        }
    }
    visit(primaryType)
    const ordered = [primaryType, ...Array.from(seen).filter(t => t !== primaryType).sort()]
    return ordered.map(typeName => (
        typeName + "(" + (types[typeName] || []).map(field => field.type + " " + field.name).join(",") + ")"
    )).join("")
}

async function _typeHash(primaryType, types) {
    return _keccak256(strToBytes(_encodeType(primaryType, types)))
}

async function _encodeTypedValue(type, types, value) {
    const arr = _parseArrayType(type)
    if (arr) {
        const parts = []
        for (const item of _normalizeArrayValue(value)) {
            const encoded = await _encodeTypedValue(arr.baseType, types, item)
            parts.push(encoded.type === "bytes32" ? hexToBytes(encoded.value) : encodeStatic(encoded.type, encoded.value))
        }
        return { type: "bytes32", value: await _keccak256(concatBytes(...parts)) }
    }
    const base = _stripArraySuffix(type)
    if (types[base]) return { type: "bytes32", value: await hashStruct(base, types, value) }
    if (type === "string") return { type: "bytes32", value: await _keccak256(strToBytes(String(value ?? ""))) }
    if (type === "bytes") return { type: "bytes32", value: await _keccak256(typeof value === "string" ? hexToBytes(value) : value) }
    return { type, value }
}

async function _encodeData(primaryType, types, value) {
    const fields = types[primaryType] || []
    const params = []
    for (const field of fields) {
        const encoded = await _encodeTypedValue(field.type, types, value?.[field.name])
        params.push(encoded)
    }
    return encodeParams(params)
}

async function hashStruct(primaryType, types, value) {
    return _keccak256(concatBytes(hexToBytes(await _typeHash(primaryType, types)), await _encodeData(primaryType, types, value)))
}

async function hashDomain(domain) {
    const domainFields = []
    if (domain.name !== undefined) domainFields.push({ name: "name", type: "string" })
    if (domain.version !== undefined) domainFields.push({ name: "version", type: "string" })
    if (domain.chainId !== undefined) domainFields.push({ name: "chainId", type: "uint256" })
    if (domain.verifyingContract !== undefined) domainFields.push({ name: "verifyingContract", type: "address" })
    if (domain.salt !== undefined) domainFields.push({ name: "salt", type: "bytes32" })
    return hashStruct("EIP712Domain", { EIP712Domain: domainFields }, domain)
}

function _detectPrimaryType(types) {
    const candidates = Object.keys(types || {}).filter(t => t !== "EIP712Domain")
    const refs = new Set()
    for (const typeName of candidates) {
        for (const field of (types[typeName] || [])) {
            const base = _stripArraySuffix(field.type)
            if (types[base] && base !== "EIP712Domain") refs.add(base)
        }
    }
    return candidates.find(typeName => !refs.has(typeName)) || candidates[0]
}

async function encodeTypedData(domain, types, value, primaryType) {
    const root = primaryType || _detectPrimaryType(types)
    const domainHash = await hashDomain(domain || {})
    const valueHash = await hashStruct(root, types, value)
    return bytesToHex(concatBytes(new Uint8Array([0x19, 0x01]), hexToBytes(domainHash), hexToBytes(valueHash)))
}

async function hashTypedData(domain, types, value, primaryType) {
    return _keccak256(await encodeTypedData(domain, types, value, primaryType))
}

async function recoverTypedDataAddress(domain, types, value, sig, primaryType) {
    const digest = await hashTypedData(domain, types, value, primaryType)
    // Delegate to ZEN.recover with prehash:true — uses WASM-accelerated Jacobian secp256k1
    return ZEN.recover(sig, null, { prehash: true, hash: digest, format: "evm" })
}

// ─── Exports ──────────────────────────────────────────────────────────────────

export {
    rpc,
    wsRpc,
    Wallet,
    Contract,
    isAddress,
    connector,
    Provider,
    WsProvider,
    signTransaction,
    checksumAddress,
    privateToAddress,
    buildCalldata,
    decodeReturnData,
    rlpEncode,
    buildSig,
    encodeParams,
    encodeStatic,
    isDynamicType,
    decodeEventLog,
    buildEventTopicMap,
    decodeReceiptEvents,
    encodePath,
    formatUnits,
    parseUnits,
    formatEther,
    parseEther,
    solidityPacked,
    solidityPackedKeccak256,
    getCreate2Address,
    parseAbi,
    parseAbiItem,
    _encodeType,
    hashStruct,
    hashDomain,
    encodeTypedData,
    hashTypedData,
    recoverTypedDataAddress,
}

export default { rpc, wsRpc, Wallet, Contract, isAddress, connector }
