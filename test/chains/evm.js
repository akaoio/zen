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
    Provider,
    WsProvider,
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

describe("formatUnits / parseUnits", () => {
    it("formatUnits: bigint → string", () => {
        assert.strictEqual(formatUnits(0n, 6),                    "0.0")
        assert.strictEqual(formatUnits(1500000n, 6),              "1.5")
        assert.strictEqual(formatUnits(1000000n, 6),              "1.0")
        assert.strictEqual(formatUnits(123456789n, 6),            "123.456789")
        assert.strictEqual(formatUnits(10n ** 18n, "ether"),      "1.0")
        assert.strictEqual(formatUnits(1500000000000000000n, 18), "1.5")
        assert.strictEqual(formatUnits(9n, 9),                    "0.000000009")
    })

    it("parseUnits: string → bigint", () => {
        assert.strictEqual(parseUnits("0", 6),          0n)
        assert.strictEqual(parseUnits("1.5", 6),        1500000n)
        assert.strictEqual(parseUnits("1", 6),          1000000n)
        assert.strictEqual(parseUnits("123.456789", 6), 123456789n)
        assert.strictEqual(parseUnits("1.5", "ether"),  1500000000000000000n)
        assert.strictEqual(parseUnits("1", "ether"),    10n ** 18n)
    })

    it("parseUnits truncates extra decimal digits", () => {
        // "1.1234567" with 6 decimals → truncate to "123456"
        assert.strictEqual(parseUnits("1.1234567", 6), 1123456n)
    })

    it("formatEther / parseEther are ether aliases", () => {
        assert.strictEqual(formatEther(10n ** 18n), "1.0")
        assert.strictEqual(parseEther("1.5"),       1500000000000000000n)
    })

    it("round-trip consistency", () => {
        const cases = ["1.5", "0.000001", "123456.789"]
        for (const v of cases) {
            assert.strictEqual(formatUnits(parseUnits(v, 6), 6), v)
        }
    })
})


describe("solidityPacked", function () {
    const paddedBytes32 = ethers.zeroPadBytes("0x1234", 32)
    const cases = [
        { name: "address", types: ["address"], values: [TEST_ADDR] },
        { name: "bool", types: ["bool"], values: [true] },
        { name: "uint256", types: ["uint256"], values: [123456789n] },
        { name: "uint8", types: ["uint8"], values: [255n] },
        { name: "bytes32 right-padded", types: ["bytes32"], values: ["0x1234"], ethersValues: [paddedBytes32] },
        { name: "bytes raw", types: ["bytes"], values: ["0x123456"] },
        { name: "string utf8", types: ["string"], values: ["hello 👋"] },
        { name: "mixed types", types: ["address", "bool", "uint256", "uint8", "bytes32", "bytes", "string"], values: [TEST_ADDR, true, 123n, 7n, "0x1234", "0xabcd", "zen"], ethersValues: [TEST_ADDR, true, 123n, 7n, paddedBytes32, "0xabcd", "zen"] },
    ]

    for (const testCase of cases) {
        it(`matches ethers for ${testCase.name}`, function () {
            const ours = solidityPacked(testCase.types, testCase.values)
            const theirs = ethers.solidityPacked(testCase.types, testCase.ethersValues || testCase.values)
            assert.strictEqual(ours.toLowerCase(), theirs.toLowerCase())
        })
    }
})

describe("solidityPackedKeccak256", function () {
    it("matches ethers for mixed values", async function () {
        const types = ["address", "uint256", "string"]
        const values = [TEST_ADDR, 42n, "hello"]
        assert.strictEqual(
            (await solidityPackedKeccak256(types, values)).toLowerCase(),
            ethers.solidityPackedKeccak256(types, values).toLowerCase(),
        )
    })

    it("matches ethers for bytes32", async function () {
        const paddedBytes32 = ethers.zeroPadBytes("0x1234", 32)
        assert.strictEqual(
            (await solidityPackedKeccak256(["bytes32"], ["0x1234"])).toLowerCase(),
            ethers.solidityPackedKeccak256(["bytes32"], [paddedBytes32]).toLowerCase(),
        )
    })
})

describe("getCreate2Address", function () {
    it("matches the known Uniswap V2 USDC/WETH pair address", async function () {
        const factory = "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f"
        const token0 = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"
        const token1 = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"
        const salt = ethers.solidityPackedKeccak256(["address", "address"], [token0, token1])
        const initCodeHash = "0x96e8ac4277198ff8b6f785478aa9a39f403cb768dd02cbee326c3e7da348845f"
        const pair = await getCreate2Address(factory, salt, initCodeHash)
        assert.strictEqual(pair, "0xB4e16d0168e52d35CaCD2c6185b44281Ec28C9Dc")
    })

    it("matches ethers for a simple BigInt salt case", async function () {
        const factory = "0x000000000000000000000000000000000000dEaD"
        const salt = 123n
        const initCodeHash = ethers.keccak256("0x1234")
        const ours = await getCreate2Address(factory, salt, initCodeHash)
        const theirs = ethers.getCreate2Address(factory, ethers.zeroPadValue(ethers.toBeHex(salt), 32), initCodeHash)
        assert.strictEqual(ours, theirs)
    })
})

describe("block.date", function () {
    it("Provider.getBlock enriches blocks with a Date", async function () {
        class MockProvider extends Provider {
            constructor() { super("http://example") }
            async send() { return { timestamp: "0x5e9c0c00", number: "0x1" } }
        }
        const block = await new MockProvider().getBlock("latest")
        const expected = new Date(parseInt("0x5e9c0c00", 16) * 1000)
        assert.ok(block.date instanceof Date)
        assert.strictEqual(block.date.toISOString(), expected.toISOString())
    })

    it("WsProvider.getBlock enriches blocks with a Date", async function () {
        class MockWsProvider extends WsProvider {
            constructor() { super("ws://example") }
            async send() { return { timestamp: "0x5e9c0c00", number: "0x1" } }
        }
        const block = await new MockWsProvider().getBlock("latest")
        const expected = new Date(parseInt("0x5e9c0c00", 16) * 1000)
        assert.ok(block.date instanceof Date)
        assert.strictEqual(block.date.toISOString(), expected.toISOString())
    })
})

describe("parseAbi", function () {
    it("parses a simple function signature", function () {
        assert.deepStrictEqual(
            parseAbiItem("function transfer(address to, uint256 amount) returns (bool)"),
            {
                type: "function",
                name: "transfer",
                inputs: [
                    { name: "to", type: "address" },
                    { name: "amount", type: "uint256" },
                ],
                outputs: [{ name: "", type: "bool" }],
                stateMutability: "nonpayable",
            },
        )
    })

    it("parses a view function", function () {
        const item = parseAbiItem("function balanceOf(address) view returns (uint256)")
        assert.strictEqual(item.stateMutability, "view")
        assert.strictEqual(item.inputs[0].type, "address")
        assert.strictEqual(item.outputs[0].type, "uint256")
    })

    it("parses a payable function", function () {
        assert.strictEqual(parseAbiItem("function deposit() payable").stateMutability, "payable")
    })

    it("parses an event with indexed params", function () {
        const item = parseAbiItem("event Transfer(address indexed from, address indexed to, uint256 value)")
        assert.strictEqual(item.type, "event")
        assert.strictEqual(item.inputs[0].indexed, true)
        assert.strictEqual(item.inputs[1].indexed, true)
        assert.strictEqual(item.inputs[2].indexed, undefined)
    })

    it("parses an error", function () {
        const item = parseAbiItem("error InsufficientBalance(address account, uint256 balance)")
        assert.strictEqual(item.type, "error")
        assert.strictEqual(item.inputs[0].name, "account")
        assert.strictEqual(item.inputs[1].type, "uint256")
    })

    it("parses tuple inputs", function () {
        const item = parseAbiItem("function exactInputSingle((address tokenIn, address tokenOut, uint24 fee) params) returns (uint256)")
        assert.strictEqual(item.inputs[0].type, "tuple")
        assert.deepStrictEqual(item.inputs[0].components, [
            { name: "tokenIn", type: "address" },
            { name: "tokenOut", type: "address" },
            { name: "fee", type: "uint24" },
        ])
    })

    it("passes through existing object ABI items unchanged", function () {
        const abiItem = { type: "event", name: "X", inputs: [] }
        const parsed = parseAbi([abiItem])
        assert.strictEqual(parsed[0], abiItem)
    })

    it("handles mixed string/object ABI arrays", function () {
        const abiItem = { type: "event", name: "X", inputs: [] }
        const parsed = parseAbi(["function balanceOf(address) view returns (uint256)", abiItem])
        assert.strictEqual(parsed.length, 2)
        assert.strictEqual(parsed[0].name, "balanceOf")
        assert.strictEqual(parsed[1], abiItem)
    })

    it("Contract constructor accepts string ABI and builds methods", async function () {
        const provider = {
            async call() {
                return ethers.AbiCoder.defaultAbiCoder().encode(["uint256"], [123n])
            }
        }
        const contract = new Contract(TEST_ADDR, ["function balanceOf(address) view returns (uint256)"], provider)
        assert.strictEqual(typeof contract.balanceOf, "function")
        assert.strictEqual(await contract.balanceOf(TEST_ADDR), 123n)
    })
})

describe("queryFilter", function () {
    it("decodes event logs from Provider.getLogs", async function () {
        const iface = new ethers.Interface(["event Transfer(address indexed from, address indexed to, uint256 value)"])
        const event = iface.getEvent("Transfer")
        const encoded = iface.encodeEventLog(event, [TEST_ADDR, "0x70997970C51812dc3A010C7d01b50e0d17dc79C8", 42n])
        class MockProvider extends Provider {
            constructor() {
                super("http://example")
                this.filter = null
            }
            async send(method, params) {
                if (method === "eth_getLogs") {
                    this.filter = params[0]
                    return [{
                        address: TEST_ADDR,
                        topics: encoded.topics,
                        data: encoded.data,
                        blockNumber: "0xa",
                        transactionHash: "0x" + "ab".repeat(32),
                    }]
                }
                throw new Error("Unexpected RPC method: " + method)
            }
        }
        const provider = new MockProvider()
        const contract = new Contract(TEST_ADDR, ["event Transfer(address indexed from, address indexed to, uint256 value)"], provider)
        const events = await contract.queryFilter("Transfer", 10, 20)
        assert.strictEqual(provider.filter.address, TEST_ADDR)
        assert.strictEqual(provider.filter.fromBlock, "0xa")
        assert.strictEqual(provider.filter.toBlock, "0x14")
        assert.strictEqual(events.length, 1)
        assert.strictEqual(events[0].from.toLowerCase(), TEST_ADDR.toLowerCase())
        assert.strictEqual(events[0].to.toLowerCase(), "0x70997970c51812dc3a010c7d01b50e0d17dc79c8")
        assert.strictEqual(events[0].value, 42n)
        assert.strictEqual(events[0].blockNumber, "0xa")
    })
})

describe("multicall", function () {
    it("encodes aggregate3 and decodes results", async function () {
        const erc20 = new ethers.Interface(["function balanceOf(address) view returns (uint256)"])
        const multicall = new ethers.Interface([
            "function aggregate3((address target,bool allowFailure,bytes callData)[] calls) view returns ((bool success,bytes returnData)[] returnData)"
        ])
        class MockProvider extends Provider {
            constructor() {
                super("http://example")
                this.tx = null
            }
            async call(tx) {
                this.tx = tx
                return ethers.AbiCoder.defaultAbiCoder().encode(
                    ["tuple(bool success, bytes returnData)[]"],
                    [[
                        { success: true, returnData: erc20.encodeFunctionResult("balanceOf", [11n]) },
                        { success: false, returnData: "0x" },
                    ]],
                )
            }
        }
        const provider = new MockProvider()
        const calls = [
            {
                address: TEST_ADDR,
                abi: ["function balanceOf(address) view returns (uint256)"],
                method: "balanceOf",
                args: [TEST_ADDR],
            },
            {
                address: TEST_ADDR,
                abi: ["function balanceOf(address) view returns (uint256)"],
                method: "balanceOf",
                args: ["0x70997970C51812dc3A010C7d01b50e0d17dc79C8"],
                allowFailure: true,
            },
        ]
        const results = await provider.multicall(calls)
        const expectedData = multicall.encodeFunctionData("aggregate3", [[
            { target: TEST_ADDR, allowFailure: false, callData: erc20.encodeFunctionData("balanceOf", [TEST_ADDR]) },
            { target: TEST_ADDR, allowFailure: true, callData: erc20.encodeFunctionData("balanceOf", ["0x70997970C51812dc3A010C7d01b50e0d17dc79C8"]) },
        ]])
        assert.strictEqual(provider.tx.to, "0xcA11bde05977b3631167028862bE2a173976CA11")
        assert.strictEqual(provider.tx.data.toLowerCase(), expectedData.toLowerCase())
        assert.deepStrictEqual(results, [11n, null])
    })
})

describe("WsProvider auto-reconnect", function () {
    it("tracks reconnect settings and saved subscriptions", async function () {
        const provider = new WsProvider("ws://example")
        let subCounter = 0
        provider._connect = async () => Promise.resolve()
        provider.send = async (method) => method === "eth_subscribe" ? `0xsub${++subCounter}` : true
        const subId = await provider.on("block", () => {})
        assert.strictEqual(subId, "0xsub1")
        assert.strictEqual(provider._reconnectDelay, 1000)
        assert.strictEqual(provider._maxReconnectDelay, 30000)
        assert.strictEqual(typeof provider._scheduleReconnect, "function")
        assert.ok(provider._subHandlers.has("newHeads"))
        provider._subs.clear()
        provider._subTopics.clear()
        await provider._resubscribeAll()
        assert.ok(provider._subs.size >= 1)
    })
})

describe("EIP-712 TypedDataEncoder", function () {
    this.timeout(20000)

    const domain = {
        name: "Ether Mail",
        version: "1",
        chainId: 1,
        verifyingContract: "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC",
    }
    const types = {
        Person: [
            { name: "name", type: "string" },
            { name: "wallet", type: "address" },
        ],
        Mail: [
            { name: "from", type: "Person" },
            { name: "to", type: "Person" },
            { name: "contents", type: "string" },
        ],
    }
    const value = {
        from: { name: "Cow", wallet: "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826" },
        to: { name: "Bob", wallet: "0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB" },
        contents: "Hello, Bob!",
    }

    it("_encodeType builds the canonical type string", function () {
        assert.strictEqual(
            _encodeType("Mail", types),
            "Mail(Person from,Person to,string contents)Person(string name,address wallet)",
        )
    })

    it("hashStruct matches the EIP-712 spec example", async function () {
        assert.strictEqual(
            await hashStruct("Mail", types, value),
            "0xc52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e",
        )
    })

    it("hashDomain matches the EIP-712 spec example", async function () {
        assert.strictEqual(
            await hashDomain(domain),
            "0xf2cee375fa42b42143804025fc449deafd50cc031ca257e0b194a650a912090f",
        )
    })

    it("hashTypedData matches the EIP-712 spec example", async function () {
        assert.strictEqual(
            await hashTypedData(domain, types, value),
            "0xbe609aee343fb3c4b28e1df9e632fca64fcfaede20f02e86244efddf30957bd2",
        )
    })

    it("encodeTypedData returns 0x1901-prefixed bytes", async function () {
        assert.ok((await encodeTypedData(domain, types, value)).startsWith("0x1901"))
    })

    it("matches ethers TypedDataEncoder.hash", async function () {
        assert.strictEqual(
            await hashTypedData(domain, types, value),
            ethers.TypedDataEncoder.hash(domain, types, value),
        )
    })

    it("recovers the signer address from a typed data signature", async function () {
        const wallet = new ethers.Wallet(TEST_PRIV)
        const sig = await wallet.signTypedData(domain, types, value)
        assert.strictEqual(
            (await recoverTypedDataAddress(domain, types, value, sig)).toLowerCase(),
            wallet.address.toLowerCase(),
        )
    })
})
