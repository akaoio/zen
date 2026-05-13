const mods = Object.create(null);
function defmod(id, fn){ mods[id] = { fn, exports: {}, loaded: false }; }
function reqmod(id){ var m = mods[id]; if(!m) throw new Error('chains: missing module ' + id); if(!m.loaded){ m.loaded=true; m.fn(m, m.exports); } return m.exports; }

defmod('./lib/chains/evm.js', function(module, exp){
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

  import keccak256 from "../../src/keccak256.js"
  import secp256k1 from "../../src/curves/secp256k1.js"

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
      const hash = await keccak256(strToBytes(hex))
      const hashHex = bytesToHex(hash, false)
      let out = "0x"
      for (let i = 0; i < 40; i++) out += parseInt(hashHex[i], 16) >= 8 ? hex[i].toUpperCase() : hex[i]
      return out
  }

  // Lowercase-only address (no EIP-55) — used synchronously in ABI decoding
  function addressFromSlot(slot32) {
      return "0x" + bytesToHex(slot32.slice(12), false).toLowerCase()
  }

  async function privateToAddress(privHex) {
      const priv = secp256k1.parseScalar(privHex)
      const pub = secp256k1.pointMultiplyG(priv)
      const raw = new Uint8Array(64)
      raw.set(secp256k1.bigIntToBytes(pub.x, 32), 0)
      raw.set(secp256k1.bigIntToBytes(pub.y, 32), 32)
      const hash = await keccak256(raw)
      return checksumAddress(bytesToHex(hash.slice(-20)))
  }

  function isAddress(str) {
      return typeof str === "string" && /^0x[0-9a-fA-F]{40}$/.test(str)
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

  // Whether a type requires a dynamic (offset+data) slot
  function isDynamicType(type) {
      if (!type) return false
      if (type === "bytes" || type === "string") return true
      if (type.endsWith("[]")) return true     // unbounded dynamic array
      // fixed arrays, tuples: static
      return false
  }

  // Encode one static value → 32-byte slot
  function encodeStatic(type, value) {
      if (type === "address") {
          const bytes = hexToBytes(value)        // accept "0x..." or raw hex
          const slot = new Uint8Array(32)
          slot.set(bytes.slice(-20), 12)
          return slot
      }
      if (type === "bool") {
          const slot = new Uint8Array(32)
          slot[31] = value ? 1 : 0
          return slot
      }
      if (type.startsWith("uint") || type.startsWith("int")) {
          let n = typeof value === "bigint" ? value : BigInt(value.toString())
          if (n < 0n) {
              // 2's complement — ABI sign-extends to 256 bits
              n = (1n << 256n) + n
          }
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

  // Encode one dynamic value → byte blob (length prefix + padded data)
  function encodeDynamic(type, value) {
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
      if (type.endsWith("[]")) {
          const elemType = type.slice(0, -2)
          const arr = Array.isArray(value) ? value : Array.from(value)
          const lenSlot = bigIntToBytes(BigInt(arr.length), 32)
          const elems = arr.map(v => encodeStatic(elemType, v))
          return concatBytes(lenSlot, ...elems)
      }
      throw new Error("ABI encode: unsupported dynamic type: " + type)
  }

  // Encode an ordered list of (type, value, components?) params → bytes
  // Handles: static types, dynamic types, inline tuples (all-static components)
  function encodeParams(params) {
      const N = params.length
      const headParts = new Array(N)
      const tailParts = []
      let tailOffset = N * 32

      for (let i = 0; i < N; i++) {
          const { type, value, components } = params[i]
          if (type === "tuple") {
              // All-static tuple: encode components inline into the head
              const comp = (components || []).map((c, j) => ({
                  type: c.type,
                  value: Array.isArray(value) ? value[j] : value[c.name],
                  components: c.components
              }))
              headParts[i] = encodeParams(comp)
          } else if (isDynamicType(type)) {
              headParts[i] = bigIntToBytes(BigInt(tailOffset), 32)
              const tail = encodeDynamic(type, value)
              tailParts.push(tail)
              tailOffset += tail.length
          } else {
              headParts[i] = encodeStatic(type, value)
          }
      }
      return concatBytes(...headParts, ...tailParts)
  }

  // Build canonical function signature string from ABI entry
  // e.g. { name: "transfer", inputs: [{ type: "address" }, { type: "uint256" }] }
  //   → "transfer(address,uint256)"
  function buildSig(abiFunc) {
      const types = abiFunc.inputs.map(inp => {
          if (inp.type === "tuple") {
              return "(" + inp.components.map(c => c.type).join(",") + ")"
          }
          return inp.type
      })
      return abiFunc.name + "(" + types.join(",") + ")"
  }

  // Build calldata: 4-byte selector + encoded params
  async function buildCalldata(sig, inputs, values) {
      const sel = (await keccak256(strToBytes(sig))).slice(0, 4)
      if (!inputs || !inputs.length) return bytesToHex(sel)
      const params = inputs.map((inp, i) => ({
          type: inp.type,
          value: values[i],
          components: inp.components
      }))
      return bytesToHex(concatBytes(sel, encodeParams(params)))
  }

  // Decode a single ABI value from data at offset
  function decodeSingle(type, bytes, offset) {
      // Check array types first — before uint/int prefix checks
      if (type.endsWith("[]")) {
          const elemType = type.slice(0, -2)
          const ptr = Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
          const len = Number(bytesToBigInt(bytes.slice(ptr, ptr + 32)))
          const arr = []
          for (let i = 0; i < len; i++) {
              arr.push(decodeSingle(elemType, bytes, ptr + 32 + i * 32))
          }
          return arr
      }
      if (type === "address") {
          const slot = bytes.slice(offset, offset + 32)
          return addressFromSlot(slot)
      }
      if (type === "bool") {
          return bytes[offset + 31] !== 0
      }
      if (type.startsWith("uint")) {
          return bytesToBigInt(bytes.slice(offset, offset + 32))
      }
      if (type.startsWith("int")) {
          // ABI sign-extends all intN to 256 bits
          const n = bytesToBigInt(bytes.slice(offset, offset + 32))
          return n >= (1n << 255n) ? n - (1n << 256n) : n
      }
      if (type === "bytes32" || /^bytes\d+$/.test(type)) {
          return bytesToHex(bytes.slice(offset, offset + 32))
      }
      if (type === "bytes" || type === "string") {
          const ptr = Number(bytesToBigInt(bytes.slice(offset, offset + 32)))
          const len = Number(bytesToBigInt(bytes.slice(ptr, ptr + 32)))
          const raw = bytes.slice(ptr + 32, ptr + 32 + len)
          return type === "string" ? new TextDecoder().decode(raw) : bytesToHex(raw)
      }
      throw new Error("ABI decode: unsupported type: " + type)
  }

  // Decode a tuple with named components from a byte slice at startOffset
  function decodeTuple(components, bytes, startOffset) {
      const result = {}
      let offset = startOffset
      for (const c of components) {
          result[c.name] = decodeSingle(c.type, bytes, offset)
          offset += 32
      }
      return result
  }

  // Decode raw return hex into JS value(s) based on output ABI spec
  function decodeReturnData(outputs, raw) {
      if (!raw || raw === "0x" || !outputs || !outputs.length) return null
      const bytes = hexToBytes(raw)
      if (outputs.length === 1) {
          const out = outputs[0]
          if (out.type === "tuple" && out.components) return decodeTuple(out.components, bytes, 0)
          return decodeSingle(out.type, bytes, 0)
      }
      // Multi-return: array with named properties (ethers-compatible)
      const result = []
      let offset = 0
      for (const out of outputs) {
          let val
          if (out.type === "tuple" && out.components) {
              val = decodeTuple(out.components, bytes, offset)
              offset += out.components.length * 32
          } else {
              val = decodeSingle(out.type, bytes, offset)
              offset += 32
          }
          result.push(val)
          if (out.name) result[out.name] = val
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
          return this.send("eth_getBlockByNumber", [t, false])
      }

      async getBalance(address) {
          return BigInt(await this.send("eth_getBalance", [address, "latest"]))
      }

      async getTransactionCount(address, tag = "latest") {
          return BigInt(await this.send("eth_getTransactionCount", [address, tag]))
      }

      async getGasPrice() {
          return BigInt(await this.send("eth_gasPrice", []))
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
              if (receipt) return receipt
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
          this._reqId = 1
          this._ready = null
      }

      _connect() {
          if (this._ready) return this._ready
          this._ready = new Promise((resolve, reject) => {
              const ws = new WebSocket(this.url)
              this._ws = ws
              ws.onopen  = () => resolve()
              ws.onerror = () => reject(new Error("WS connect failed: " + this.url))
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
                  for (const { reject: rej } of this._pending.values()) rej(new Error("WS closed"))
                  this._pending.clear()
              }
          })
          return this._ready
      }

      async send(method, params = []) {
          await this._connect()
          return new Promise((resolve, reject) => {
              const id = this._reqId++
              this._pending.set(id, { resolve, reject })
              this._ws.send(JSON.stringify({ jsonrpc: "2.0", id, method, params }))
          })
      }

      // Subscribe to named events: "block" | "error"
      async on(event, handler) {
          await this._connect()
          if (event === "block") {
              const subId = await this.send("eth_subscribe", ["newHeads"])
              this._subs.set(subId, result => handler(result?.number != null ? parseInt(result.number, 16) : result))
              return subId
          }
          if (event === "error") {
              if (this._ws) this._ws.onerror = e => handler(e)
              return null
          }
      }

      async off(subIdOrEvent) {
          if (subIdOrEvent && typeof subIdOrEvent === "string" && subIdOrEvent.startsWith("0x")) {
              await this.send("eth_unsubscribe", [subIdOrEvent])
              this._subs.delete(subIdOrEvent)
          }
      }

      // Standard provider methods delegated over WS
      async getChainId() { return BigInt(await this.send("eth_chainId", [])) }
      async getBlock(tag) {
          const t = typeof tag === "number" ? "0x" + tag.toString(16) : tag
          return this.send("eth_getBlockByNumber", [t, false])
      }
      async getBalance(addr) { return BigInt(await this.send("eth_getBalance", [addr, "latest"])) }
      async getGasPrice() { return BigInt(await this.send("eth_gasPrice", [])) }
      async getFeeData() { return { gasPrice: await this.getGasPrice(), maxFeePerGas: null, maxPriorityFeePerGas: null } }
      async getTransactionCount(addr, tag = "latest") {
          return BigInt(await this.send("eth_getTransactionCount", [addr, tag]))
      }
      async call(tx, blockTag = "latest") {
          const obj = { to: tx.to, data: tx.data }
          const tag = typeof blockTag === "bigint" || typeof blockTag === "number"
              ? "0x" + BigInt(blockTag).toString(16) : blockTag
          return this.send("eth_call", [obj, tag])
      }
      async estimateGas(tx) {
          const obj = {}
          if (tx.from) obj.from = tx.from
          if (tx.to)   obj.to   = tx.to
          if (tx.data) obj.data = tx.data
          if (tx.value !== undefined) obj.value = "0x" + BigInt(tx.value).toString(16)
          return BigInt(await this.send("eth_estimateGas", [obj]))
      }
      async destroy() { if (this._ws) this._ws.close() }
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
      const unsigned  = rlpEncode([nonce, gasPrice, gasLimit, to20, value, data, cid, 0n, 0n])
      const hashBytes = await keccak256(unsigned)

      const priv = secp256k1.parseScalar(privHex)
      let v, r, s
      for (let attempt = 0; attempt < 16; attempt++) {
          const k  = await secp256k1.deterministicK(priv, hashBytes, attempt)
          const pt = secp256k1.pointMultiplyG(k)
          if (!pt) continue
          r = secp256k1.mod(pt.x, secp256k1.N)
          if (!r) continue
          s = secp256k1.mod(
              secp256k1.modInv(k, secp256k1.N) *
              (secp256k1.mod(secp256k1.bytesToBigInt(hashBytes), secp256k1.N) + r * priv),
              secp256k1.N
          )
          if (!s) continue
          v = Number(pt.y & 1n)
          if (s > secp256k1.HALF_N) { s = secp256k1.N - s; v ^= 1 }
          break
      }

      const vBig   = cid * 2n + 35n + BigInt(v)   // EIP-155 replay protection
      const signed = rlpEncode([nonce, gasPrice, gasLimit, to20, value, data, vBig, r, s])
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
          const sig     = item.name + "(" + item.inputs.map(i => i.type).join(",") + ")"
          const topic0  = bytesToHex(await keccak256(strToBytes(sig)))
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
          this.abi      = abi
          this._signer  = providerOrWallet instanceof Wallet ? providerOrWallet : null
          this._provider = providerOrWallet instanceof Wallet
              ? providerOrWallet._provider
              : providerOrWallet

          // ethers-compatible .interface property
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
                      // Extract optional overrides from last arg if it's a plain object (ethers compat)
                      const hasOverrides = args.length > item.inputs.length && args[args.length - 1] !== null
                          && typeof args[args.length - 1] === "object" && !Array.isArray(args[args.length - 1])
                      const overrides  = hasOverrides ? args[args.length - 1] : {}
                      const blockTag   = overrides.blockTag ?? "latest"
                      const raw = await this._provider.call({ to: this.address, data: calldata }, blockTag)
                      return decodeReturnData(item.outputs, raw)
                  } else {
                      if (!this._signer) throw new Error("Contract: no signer for write method '" + name + "'")
                      await this._signer._ready
                      const txResp    = await this._signer.sendTransaction({ to: this.address, data: calldata, value: 0n })
                      const receipt   = await txResp.wait()
                      receipt.events  = await decodeReceiptEvents(this.abi, receipt)
                      return receipt
                  }
              }
              method._abiGenerated = true
              this[name] = method
          }
      }

      // Returns a new Contract bound to a different signer or provider
      connect(signerOrProvider) {
          return new Contract(this.address, this.abi, signerOrProvider)
      }
  }

  // ─── Ethers-compatible connector shim ─────────────────────────────────────────
  // Drop-in for `connector: ethers` in akao's EVM.js.
  // Usage: replace `import { ethers } from "../Ethers.js"` and
  //        change `connector: ethers` to `connector: evmConnector`.

  const connector = {
      JsonRpcProvider:   Provider,
      WebSocketProvider: WsProvider,
      Wallet,
      Contract,
      isAddress,
  }

  // ─── Exports ──────────────────────────────────────────────────────────────────



  export default { rpc, wsRpc, Wallet, Contract, isAddress, connector }

  exp.rpc = rpc;
  exp.wsRpc = wsRpc;
  exp.Wallet = Wallet;
  exp.Contract = Contract;
  exp.isAddress = isAddress;
  exp.connector = connector;
  exp.Provider = Provider;
  exp.WsProvider = WsProvider;
  exp.signTransaction = signTransaction;
  exp.checksumAddress = checksumAddress;
  exp.privateToAddress = privateToAddress;
  exp.buildCalldata = buildCalldata;
  exp.decodeReturnData = decodeReturnData;
  exp.rlpEncode = rlpEncode;
  exp.buildSig = buildSig;
  exp.encodeParams = encodeParams;
  exp.encodeStatic = encodeStatic;
  exp.isDynamicType = isDynamicType;
  exp.decodeEventLog = decodeEventLog;
  exp.buildEventTopicMap = buildEventTopicMap;
  exp.decodeReceiptEvents = decodeReceiptEvents;
});

const _entry = reqmod('./lib/chains/evm.js');
export const evm        = _entry.default;
export const rpc        = _entry.rpc;
export const wsRpc      = _entry.wsRpc;
export const Wallet     = _entry.Wallet;
export const Contract   = _entry.Contract;
export const isAddress       = _entry.isAddress;
export const connector       = _entry.connector;
export const privateToAddress = _entry.privateToAddress;
export default { evm: _entry.default, rpc: _entry.rpc, wsRpc: _entry.wsRpc, Wallet: _entry.Wallet, Contract: _entry.Contract, isAddress: _entry.isAddress, connector: _entry.connector, privateToAddress: _entry.privateToAddress };
