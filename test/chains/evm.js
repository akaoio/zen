/**
 * test/chains/evm.js — Behavioral comparison: zen chains.evm vs ethers.js
 *
 * Tests verify that zen's EVM implementation produces identical results
 * to ethers.js for all core operations: address derivation, ABI encoding,
 * RLP encoding, transaction signing, and live provider calls via ganache.
 *
 * Run: npm run test:chains
 */

import assert from "assert"
import ganache from "ganache"
import { ethers } from "ethers"

// zen chains module — source (not bundled)
import evm, {
    rpc,
    Wallet,
    Contract,
    isAddress,
    checksumAddress,
    privateToAddress,
    buildCalldata,
    decodeReturnData,
    rlpEncode,
    signTransaction,
    buildSig,
    decodeEventLog,
    buildEventTopicMap,
} from "../../lib/chains/evm.js"

// ─── Test vector ──────────────────────────────────────────────────────────────

// Hardhat/Ganache account #0 — widely known, safe for tests
const TEST_PRIV = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80"
// Properly checksummed address for TEST_PRIV (derived by ethers to be EIP-55 correct)
const TEST_ADDR = new ethers.Wallet(TEST_PRIV).address

// Minimal read-only ABI used for encoding/decoding tests
const ERC20_ABI = [
    { type: "function", name: "name",        stateMutability: "view",       inputs: [],                                                                      outputs: [{ name: "", type: "string" }] },
    { type: "function", name: "decimals",    stateMutability: "view",       inputs: [],                                                                      outputs: [{ name: "", type: "uint8" }] },
    { type: "function", name: "totalSupply", stateMutability: "view",       inputs: [],                                                                      outputs: [{ name: "", type: "uint256" }] },
    { type: "function", name: "balanceOf",   stateMutability: "view",       inputs: [{ name: "account", type: "address" }],                                  outputs: [{ name: "", type: "uint256" }] },
    { type: "function", name: "transfer",    stateMutability: "nonpayable", inputs: [{ name: "to", type: "address" }, { name: "amount", type: "uint256" }],  outputs: [{ name: "", type: "bool" }] },
    { type: "function", name: "approve",     stateMutability: "nonpayable", inputs: [{ name: "spender", type: "address" }, { name: "amount", type: "uint256" }], outputs: [{ name: "", type: "bool" }] },
    { type: "event",    name: "Transfer",    inputs: [{ name: "from", type: "address", indexed: true }, { name: "to", type: "address", indexed: true }, { name: "value", type: "uint256", indexed: false }] },
]

// ─── 1. Address derivation ────────────────────────────────────────────────────

describe("evm: address derivation", function () {
    this.timeout(10000)

    it("privateToAddress matches ethers", async function () {
        const zenAddr = await privateToAddress(TEST_PRIV)
        assert.strictEqual(zenAddr.toLowerCase(), TEST_ADDR.toLowerCase())
    })

    it("EIP-55 checksum matches ethers", async function () {
        const raw = TEST_ADDR.toLowerCase()
        const zen = await checksumAddress(raw)
        const eth = ethers.getAddress(raw)
        assert.strictEqual(zen, eth)
    })

    it("isAddress accepts valid addresses", function () {
        assert.strictEqual(isAddress(TEST_ADDR), true)
        assert.strictEqual(isAddress(TEST_ADDR.toLowerCase()), true)
    })

    it("isAddress rejects invalid", function () {
        assert.strictEqual(isAddress("0xabc"), false)
        assert.strictEqual(isAddress("not-an-address"), false)
        assert.strictEqual(isAddress(""), false)
    })

    it("Wallet.create derives correct address", async function () {
        const w = await Wallet.create(TEST_PRIV)
        assert.strictEqual(w.address.toLowerCase(), TEST_ADDR.toLowerCase())
    })
})

// ─── 2. ABI encoding ─────────────────────────────────────────────────────────

describe("evm: ABI encoding", function () {
    this.timeout(10000)

    const IFACE = new ethers.Interface(ERC20_ABI)

    it("encodes balanceOf(address)", async function () {
        const abiItem = ERC20_ABI.find(i => i.name === "balanceOf")
        const zen     = await buildCalldata(buildSig(abiItem), abiItem.inputs, [TEST_ADDR])
        const eth     = IFACE.encodeFunctionData("balanceOf", [TEST_ADDR])
        assert.strictEqual(zen.toLowerCase(), eth.toLowerCase())
    })

    it("encodes transfer(address,uint256)", async function () {
        const amount  = 1000000n
        const abiItem = ERC20_ABI.find(i => i.name === "transfer")
        const zen     = await buildCalldata(buildSig(abiItem), abiItem.inputs, [TEST_ADDR, amount])
        const eth     = IFACE.encodeFunctionData("transfer", [TEST_ADDR, amount])
        assert.strictEqual(zen.toLowerCase(), eth.toLowerCase())
    })

    it("encodes totalSupply() — no args", async function () {
        const abiItem = ERC20_ABI.find(i => i.name === "totalSupply")
        const zen     = await buildCalldata(buildSig(abiItem), abiItem.inputs, [])
        const eth     = IFACE.encodeFunctionData("totalSupply", [])
        assert.strictEqual(zen.toLowerCase(), eth.toLowerCase())
    })

    it("encodes address[] dynamic array", async function () {
        const abi   = [{ type: "function", name: "addrs", inputs: [{ name: "list", type: "address[]" }], outputs: [] }]
        const iface = new ethers.Interface(abi)
        const addr2 = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8"
        const list  = [TEST_ADDR, addr2]
        const item  = abi[0]
        const zen   = await buildCalldata(buildSig(item), item.inputs, [list])
        const eth   = iface.encodeFunctionData("addrs", [list])
        assert.strictEqual(zen.toLowerCase(), eth.toLowerCase())
    })
})

// ─── 3. ABI decoding ─────────────────────────────────────────────────────────

describe("evm: ABI decoding", function () {
    it("decodes uint256 return", function () {
        const supply = 1000000000000000000000000n
        const enc    = ethers.AbiCoder.defaultAbiCoder().encode(["uint256"], [supply])
        const zen    = decodeReturnData([{ name: "", type: "uint256" }], enc)
        assert.strictEqual(zen.toString(), supply.toString())
    })

    it("decodes address return (case-insensitive match)", function () {
        const enc = ethers.AbiCoder.defaultAbiCoder().encode(["address"], [TEST_ADDR])
        const zen = decodeReturnData([{ name: "", type: "address" }], enc)
        assert.strictEqual(zen.toLowerCase(), TEST_ADDR.toLowerCase())
    })

    it("decodes bool return — true", function () {
        const enc = ethers.AbiCoder.defaultAbiCoder().encode(["bool"], [true])
        assert.strictEqual(decodeReturnData([{ name: "", type: "bool" }], enc), true)
    })

    it("decodes bool return — false", function () {
        const enc = ethers.AbiCoder.defaultAbiCoder().encode(["bool"], [false])
        assert.strictEqual(decodeReturnData([{ name: "", type: "bool" }], enc), false)
    })

    it("decodes int24 (signed) — positive", function () {
        const enc = ethers.AbiCoder.defaultAbiCoder().encode(["int24"], [1000])
        assert.strictEqual(decodeReturnData([{ name: "", type: "int24" }], enc), 1000n)
    })

    it("decodes int24 (signed) — negative", function () {
        const enc = ethers.AbiCoder.defaultAbiCoder().encode(["int24"], [-887220])
        assert.strictEqual(decodeReturnData([{ name: "", type: "int24" }], enc), -887220n)
    })

    it("decodes uint256[] dynamic array", function () {
        const vals = [100n, 200n, 300n]
        const enc  = ethers.AbiCoder.defaultAbiCoder().encode(["uint256[]"], [vals])
        const zen  = decodeReturnData([{ name: "", type: "uint256[]" }], enc)
        assert.ok(Array.isArray(zen), "result should be an array, got: " + typeof zen)
        assert.deepStrictEqual(zen.map(v => v.toString()), vals.map(v => v.toString()))
    })

    it("decodes address[] dynamic array", function () {
        const addr2 = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8"
        const addrs = [TEST_ADDR, addr2]
        const enc   = ethers.AbiCoder.defaultAbiCoder().encode(["address[]"], [addrs])
        const zen   = decodeReturnData([{ name: "", type: "address[]" }], enc)
        assert.ok(Array.isArray(zen), "result should be an array")
        assert.deepStrictEqual(zen.map(a => a.toLowerCase()), addrs.map(a => a.toLowerCase()))
    })

    it("decodes multi-return (uint112, uint112, uint32) — V2 getReserves style", function () {
        const r0   = 5000000000000000000000n
        const r1   = 1000000000000000000n
        const ts   = 1700000000n
        const enc  = ethers.AbiCoder.defaultAbiCoder().encode(["uint112","uint112","uint32"], [r0, r1, ts])
        const outs = [
            { name: "_reserve0", type: "uint112" },
            { name: "_reserve1", type: "uint112" },
            { name: "_blockTimestampLast", type: "uint32" }
        ]
        const zen = decodeReturnData(outs, enc)
        assert.strictEqual(zen[0].toString(), r0.toString())
        assert.strictEqual(zen[1].toString(), r1.toString())
        assert.strictEqual(zen[2].toString(), ts.toString())
    })
})

// ─── 4. RLP encoding ─────────────────────────────────────────────────────────

describe("evm: RLP encoding", function () {
    it("encodes empty byte array → 0x80", function () {
        assert.strictEqual(rlpEncode(new Uint8Array(0))[0], 0x80)
    })

    it("encodes single byte < 0x80 as-is", function () {
        const out = rlpEncode(new Uint8Array([0x42]))
        assert.strictEqual(out.length, 1)
        assert.strictEqual(out[0], 0x42)
    })

    it("encodes 0n as 0x80 (empty string)", function () {
        assert.strictEqual(rlpEncode(0n)[0], 0x80)
    })

    it("encodes 1n as single byte 0x01", function () {
        const out = rlpEncode(1n)
        assert.strictEqual(out.length, 1)
        assert.strictEqual(out[0], 0x01)
    })

    it("encodes list of two items", function () {
        const out = rlpEncode([new Uint8Array([0x01]), new Uint8Array([0x02])])
        // [0xc2, 0x01, 0x02]
        assert.strictEqual(out[0], 0xc2)
        assert.strictEqual(out[1], 0x01)
        assert.strictEqual(out[2], 0x02)
    })
})

// ─── 5. Transaction signing ───────────────────────────────────────────────────

describe("evm: transaction signing (EIP-155)", function () {
    this.timeout(15000)

    it("signTransaction output matches ethers Wallet.signTransaction", async function () {
        const chainId  = 1n
        const nonce    = 0n
        const gasPrice = 20000000000n     // 20 gwei
        const gasLimit = 21000n
        const to       = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8"
        const value    = 1000000000000000000n  // 1 ETH

        const zenRaw  = await signTransaction({ nonce, gasPrice, gasLimit, to, value, data: "0x" }, TEST_PRIV, chainId)
        const ethersW = new ethers.Wallet(TEST_PRIV)
        const ethRaw  = await ethersW.signTransaction({ nonce: 0, gasPrice, gasLimit, to, value, chainId: 1, type: 0 })

        assert.strictEqual(zenRaw.toLowerCase(), ethRaw.toLowerCase())
    })

    it("sequential nonces produce different raw txs", async function () {
        const base = { gasPrice: 1000000000n, gasLimit: 21000n, to: "0x70997970C51812dc3A010C7d01b50e0d17dc79C8", value: 1n }
        const raw0 = await signTransaction({ ...base, nonce: 0n }, TEST_PRIV, 1n)
        const raw1 = await signTransaction({ ...base, nonce: 1n }, TEST_PRIV, 1n)
        assert.notStrictEqual(raw0, raw1)
    })
})

// ─── 6. Provider + Wallet via ganache ────────────────────────────────────────

describe("evm: provider + wallet (ganache)", function () {
    this.timeout(60000)

    let server, zenProv, ethersProv
    const RECIPIENT = "0x70997970C51812dc3A010C7d01b50e0d17dc79C8"

    before(async function () {
        server = ganache.server({
            accounts: [{ balance: "0x" + (100n * 10n ** 18n).toString(16), secretKey: TEST_PRIV }],
            chain:    { chainId: 1337 },
            logging: { quiet: true }
        })
        // port 0 → OS assigns a free port (safe for parallel CI runs)
        await new Promise(resolve => server.listen(0, resolve))
        const port = server.address().port
        zenProv    = rpc(`http://127.0.0.1:${port}`)
        ethersProv = new ethers.JsonRpcProvider(`http://127.0.0.1:${port}`, { chainId: 1337, name: "ganache" }, { staticNetwork: true })
    })

    after(async function () {
        if (server) await server.close()
    })

    it("getChainId returns 1337n", async function () {
        assert.strictEqual(await zenProv.getChainId(), 1337n)
    })

    it("getBalance matches ethers", async function () {
        const [zen, eth] = await Promise.all([
            zenProv.getBalance(TEST_ADDR),
            ethersProv.getBalance(TEST_ADDR)
        ])
        assert.strictEqual(zen.toString(), eth.toString())
    })

    it("getTransactionCount starts at 0n", async function () {
        assert.strictEqual(await zenProv.getTransactionCount(TEST_ADDR, "latest"), 0n)
    })

    it("getGasPrice returns a positive BigInt", async function () {
        const gp = await zenProv.getGasPrice()
        assert.ok(typeof gp === "bigint" && gp > 0n, "gasPrice should be positive bigint")
    })

    it("Wallet.create + sendTransaction: native ETH transfer", async function () {
        const value  = 10n ** 17n   // 0.1 ETH
        const before = await zenProv.getBalance(RECIPIENT)
        const wallet = await Wallet.create(TEST_PRIV, zenProv)
        const txResp = await wallet.sendTransaction({ to: RECIPIENT, value })
        const rec    = await txResp.wait()
        assert.ok(rec, "receipt exists")
        const after = await zenProv.getBalance(RECIPIENT)
        assert.strictEqual((after - before).toString(), value.toString())
    })

    it("nonce increments after send", async function () {
        const nonce = await zenProv.getTransactionCount(TEST_ADDR, "latest")
        assert.ok(nonce > 0n, "nonce should be > 0 after first transfer")
    })

    it("recipient balance matches ethers after transfer", async function () {
        const [zen, eth] = await Promise.all([
            zenProv.getBalance(RECIPIENT),
            ethersProv.getBalance(RECIPIENT)
        ])
        assert.strictEqual(zen.toString(), eth.toString())
    })
})

// ─── 7. ABI encoding (tuple / struct) ────────────────────────────────────────

describe("evm: ABI encoding (tuple)", function () {
    // Uniswap V3 Router exactInputSingle ABI
    const V3_EXACT_INPUT_SINGLE = {
        type: "function",
        name: "exactInputSingle",
        stateMutability: "payable",
        inputs: [{
            type: "tuple",
            name: "params",
            components: [
                { name: "tokenIn",           type: "address" },
                { name: "tokenOut",          type: "address" },
                { name: "fee",               type: "uint24"  },
                { name: "recipient",         type: "address" },
                { name: "amountIn",          type: "uint256" },
                { name: "amountOutMinimum",  type: "uint256" },
                { name: "sqrtPriceLimitX96", type: "uint160" }
            ]
        }],
        outputs: [{ name: "amountOut", type: "uint256" }]
    }

    const TOKEN_A  = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"
    const TOKEN_B  = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"

    it("selector matches ethers encodeFunctionData selector", async function () {
        const params = {
            tokenIn:           TOKEN_A,
            tokenOut:          TOKEN_B,
            fee:               3000n,
            recipient:         TEST_ADDR,
            amountIn:          1000000000000000000n,
            amountOutMinimum:  0n,
            sqrtPriceLimitX96: 0n
        }

        const zenCalldata   = await buildCalldata(buildSig(V3_EXACT_INPUT_SINGLE), V3_EXACT_INPUT_SINGLE.inputs, [params])
        const ethersCalldata = new ethers.Interface([V3_EXACT_INPUT_SINGLE]).encodeFunctionData("exactInputSingle", [params])

        // 4-byte selector must match
        assert.strictEqual(zenCalldata.slice(0, 10).toLowerCase(), ethersCalldata.slice(0, 10).toLowerCase())
    })

    it("full calldata matches ethers for exactInputSingle", async function () {
        const params = {
            tokenIn:           TOKEN_A,
            tokenOut:          TOKEN_B,
            fee:               500n,
            recipient:         TEST_ADDR,
            amountIn:          2500000000000000000n,
            amountOutMinimum:  1000000000n,
            sqrtPriceLimitX96: 0n
        }

        const zenCalldata    = await buildCalldata(buildSig(V3_EXACT_INPUT_SINGLE), V3_EXACT_INPUT_SINGLE.inputs, [params])
        const ethersCalldata = new ethers.Interface([V3_EXACT_INPUT_SINGLE]).encodeFunctionData("exactInputSingle", [params])

        assert.strictEqual(zenCalldata.toLowerCase(), ethersCalldata.toLowerCase())
    })

    it("accepts struct as array (positional) as well as object", async function () {
        const sig = buildSig(V3_EXACT_INPUT_SINGLE)

        // Object form
        const paramsObj = {
            tokenIn:           TOKEN_A,
            tokenOut:          TOKEN_B,
            fee:               3000n,
            recipient:         TEST_ADDR,
            amountIn:          1n,
            amountOutMinimum:  0n,
            sqrtPriceLimitX96: 0n
        }
        // Array form (positional)
        const paramsArr = [TOKEN_A, TOKEN_B, 3000n, TEST_ADDR, 1n, 0n, 0n]

        const cdObj = await buildCalldata(sig, V3_EXACT_INPUT_SINGLE.inputs, [paramsObj])
        const cdArr = await buildCalldata(sig, V3_EXACT_INPUT_SINGLE.inputs, [paramsArr])

        assert.strictEqual(cdObj.toLowerCase(), cdArr.toLowerCase())
    })
})

// ─── 8. Event decoding (V3 PositionManager Collect) ──────────────────────────

describe("evm: event decoding (V3 Collect)", function () {
    // Uniswap V3 PositionManager Collect event ABI
    const COLLECT_EVENT = {
        type: "event",
        name: "Collect",
        anonymous: false,
        inputs: [
            { indexed: true,  name: "tokenId",  type: "uint256" },
            { indexed: false, name: "recipient", type: "address" },
            { indexed: false, name: "amount0",   type: "uint256" },
            { indexed: false, name: "amount1",   type: "uint256" }
        ]
    }

    it("buildEventTopicMap produces topic0 matching ethers", async function () {
        const topicMap  = await buildEventTopicMap([COLLECT_EVENT])
        const ethersTopic0 = ethers.id("Collect(uint256,address,uint256,uint256)")
        assert.ok(topicMap.has(ethersTopic0.toLowerCase()), "topic0 should be in map")
    })

    it("decodeEventLog extracts indexed uint256 (tokenId) and non-indexed fields", async function () {
        const tokenId  = 42n
        const amount0  = 100000000000000000n   // 0.1 ETH
        const amount1  = 250000000n            // 250 USDC (6 decimals)

        // Build a synthetic log
        const topic0 = ethers.id("Collect(uint256,address,uint256,uint256)")
        const topic1 = "0x" + tokenId.toString(16).padStart(64, "0")          // indexed tokenId
        const data   = ethers.AbiCoder.defaultAbiCoder().encode(
            ["address", "uint256", "uint256"],
            [TEST_ADDR, amount0, amount1]
        )

        const log = { topics: [topic0, topic1], data }
        const rv  = decodeEventLog(COLLECT_EVENT, log)

        assert.strictEqual(rv.tokenId.toString(),  tokenId.toString())
        assert.strictEqual(rv.recipient.toLowerCase(), TEST_ADDR.toLowerCase())
        assert.strictEqual(rv.amount0.toString(),  amount0.toString())
        assert.strictEqual(rv.amount1.toString(),  amount1.toString())
    })

    it("decodeReceiptEvents wires events into receipt by name", async function () {
        const { decodeReceiptEvents: dre } = await import("../../lib/chains/evm.js")

        const tokenId  = 7n
        const amount0  = 500000000000000000n
        const amount1  = 100000000n

        const topic0 = ethers.id("Collect(uint256,address,uint256,uint256)")
        const topic1 = "0x" + tokenId.toString(16).padStart(64, "0")
        const data   = ethers.AbiCoder.defaultAbiCoder().encode(
            ["address", "uint256", "uint256"],
            [TEST_ADDR, amount0, amount1]
        )

        const receipt = { logs: [{ topics: [topic0, topic1], data }] }
        const events  = await dre([COLLECT_EVENT], receipt)

        assert.ok(events.Collect, "Collect event should be decoded")
        assert.strictEqual(events.Collect.returnValues.tokenId.toString(),  tokenId.toString())
        assert.strictEqual(events.Collect.returnValues.amount0.toString(),  amount0.toString())
        assert.strictEqual(events.Collect.returnValues.amount1.toString(),  amount1.toString())
    })
})
