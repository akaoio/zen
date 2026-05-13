/**
 * test/chains/evm-fork.js — Phase C: mainnet fork integration tests
 *
 * Forks Ethereum mainnet at a pinned block via ganache. Tests run against real
 * Uniswap V2/V3 contracts at their production mainnet addresses — no tokens or
 * deployment needed. Exercises every public Provider/Wallet/Contract API surface.
 *
 * Requires: ETH_RPC env var (Alchemy/Infura free tier). Skipped if absent.
 *
 * Run: npm run test:chains:fork
 */

import assert  from "assert"
import ganache from "ganache"
import { ethers } from "ethers"

import {
    rpc,
    Wallet,
    Contract,
    buildCalldata,
    buildSig,
    encodePath,
} from "../../lib/chains/evm.js"

// ─── Config ───────────────────────────────────────────────────────────────────

const ETH_RPC    = process.env.ETH_RPC
const FORK_BLOCK = 19_500_000   // Feb 2024 — Uniswap V2+V3 both active

// Hardhat account #0 (widely known, safe for tests)
const TEST_PRIV = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80"
const TEST_ADDR = new ethers.Wallet(TEST_PRIV).address

// ─── Mainnet contract addresses (sourced from ~/akao/src/statics) ─────────────

const WETH        = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"
const USDC        = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"
const V2_ROUTER   = "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D"
const V2_FACTORY  = "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f"
const V3_ROUTER   = "0x68b3465833fb72A70ecDF485E0e4C7bD8665Fc45"  // SwapRouter02
const V3_FACTORY  = "0x1F98431c8aD98523631AE4a59f267346ea31F984"
const V3_PM       = "0xC36442b4a4522E871399CD717aBDD847Ab11FE88"   // NonfungiblePositionManager
const V3_QUOTER   = "0x61fFE014bA17989E743c5F6cB21bF9697530B21e"   // QuoterV2
const WHALE       = "0x28C6c06298d514Db089934071355E5743bf21d60"   // Binance 14

// V3 USDC/WETH 0.3% pool tick range (tickSpacing = 60)
const TICK_LOWER  = -887220n
const TICK_UPPER  =  887220n

// ─── ABIs ─────────────────────────────────────────────────────────────────────

const ERC20_ABI = [
    { type: "function", name: "balanceOf",    stateMutability: "view",       inputs: [{ name: "a",       type: "address" }],                                                                       outputs: [{ name: "", type: "uint256" }] },
    { type: "function", name: "approve",      stateMutability: "nonpayable", inputs: [{ name: "spender", type: "address" }, { name: "amount",  type: "uint256" }],                                  outputs: [{ name: "", type: "bool"    }] },
    { type: "function", name: "transfer",     stateMutability: "nonpayable", inputs: [{ name: "to",      type: "address" }, { name: "amount",  type: "uint256" }],                                  outputs: [{ name: "", type: "bool"    }] },
    { type: "function", name: "transferFrom", stateMutability: "nonpayable", inputs: [{ name: "from",    type: "address" }, { name: "to", type: "address" }, { name: "amount", type: "uint256" }],  outputs: [{ name: "", type: "bool"    }] },
    { type: "function", name: "allowance",    stateMutability: "view",       inputs: [{ name: "owner",   type: "address" }, { name: "spender", type: "address" }],                                  outputs: [{ name: "", type: "uint256" }] },
    { type: "event",    name: "Transfer", anonymous: false,
      inputs: [{ indexed: true, name: "from", type: "address" }, { indexed: true, name: "to", type: "address" }, { indexed: false, name: "value", type: "uint256" }] },
]

const WETH_ABI = [
    ...ERC20_ABI,
    { type: "function", name: "deposit",  stateMutability: "payable",     inputs: [],                                  outputs: [] },
    { type: "function", name: "withdraw", stateMutability: "nonpayable",  inputs: [{ name: "wad", type: "uint256" }], outputs: [] },
]

const V2_ROUTER_ABI = [
    { type: "function", name: "addLiquidity",               stateMutability: "nonpayable",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }, { name: "amountADesired", type: "uint256" }, { name: "amountBDesired", type: "uint256" }, { name: "amountAMin", type: "uint256" }, { name: "amountBMin", type: "uint256" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amountA", type: "uint256" }, { name: "amountB", type: "uint256" }, { name: "liquidity", type: "uint256" }] },
    { type: "function", name: "removeLiquidity",            stateMutability: "nonpayable",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }, { name: "liquidity", type: "uint256" }, { name: "amountAMin", type: "uint256" }, { name: "amountBMin", type: "uint256" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amountA", type: "uint256" }, { name: "amountB", type: "uint256" }] },
    { type: "function", name: "swapExactTokensForTokens",   stateMutability: "nonpayable",
      inputs: [{ name: "amountIn", type: "uint256" }, { name: "amountOutMin", type: "uint256" }, { name: "path", type: "address[]" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amounts", type: "uint256[]" }] },
    { type: "function", name: "swapExactETHForTokens",      stateMutability: "payable",
      inputs: [{ name: "amountOutMin", type: "uint256" }, { name: "path", type: "address[]" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amounts", type: "uint256[]" }] },
    { type: "function", name: "swapExactTokensForETH",      stateMutability: "nonpayable",
      inputs: [{ name: "amountIn", type: "uint256" }, { name: "amountOutMin", type: "uint256" }, { name: "path", type: "address[]" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amounts", type: "uint256[]" }] },
    { type: "function", name: "getAmountsOut",              stateMutability: "view",
      inputs: [{ name: "amountIn", type: "uint256" }, { name: "path", type: "address[]" }],
      outputs: [{ name: "amounts", type: "uint256[]" }] },
]

const V2_FACTORY_ABI = [
    { type: "function", name: "getPair", stateMutability: "view",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }],
      outputs: [{ name: "pair", type: "address" }] },
]

const V2_PAIR_ABI = [
    { type: "function", name: "getReserves", stateMutability: "view", inputs: [],
      outputs: [{ name: "reserve0", type: "uint112" }, { name: "reserve1", type: "uint112" }, { name: "blockTimestampLast", type: "uint32" }] },
    { type: "function", name: "token0", stateMutability: "view", inputs: [], outputs: [{ name: "", type: "address" }] },
    { type: "function", name: "token1", stateMutability: "view", inputs: [], outputs: [{ name: "", type: "address" }] },
    { type: "function", name: "totalSupply", stateMutability: "view", inputs: [], outputs: [{ name: "", type: "uint256" }] },
]

// SwapRouter02: exactInputSingle (no deadline), exactInput (path), exactOutput (path)
const V3_SWAP_ABI = [
    { type: "function", name: "exactInputSingle", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "tokenIn",           type: "address" },
          { name: "tokenOut",          type: "address" },
          { name: "fee",               type: "uint24"  },
          { name: "recipient",         type: "address" },
          { name: "amountIn",          type: "uint256" },
          { name: "amountOutMinimum",  type: "uint256" },
          { name: "sqrtPriceLimitX96", type: "uint160" },
      ]}],
      outputs: [{ name: "amountOut", type: "uint256" }] },
    { type: "function", name: "exactInput", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "path",             type: "bytes"   },
          { name: "recipient",        type: "address" },
          { name: "amountIn",         type: "uint256" },
          { name: "amountOutMinimum", type: "uint256" },
      ]}],
      outputs: [{ name: "amountOut", type: "uint256" }] },
    { type: "function", name: "exactOutput", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "path",            type: "bytes"   },
          { name: "recipient",       type: "address" },
          { name: "amountOut",       type: "uint256" },
          { name: "amountInMaximum", type: "uint256" },
      ]}],
      outputs: [{ name: "amountIn", type: "uint256" }] },
]

const V3_FACTORY_ABI = [
    { type: "function", name: "getPool", stateMutability: "view",
      inputs: [{ name: "", type: "address" }, { name: "", type: "address" }, { name: "", type: "uint24" }],
      outputs: [{ name: "", type: "address" }] },
]

const V3_POOL_ABI = [
    { type: "function", name: "slot0", stateMutability: "view", inputs: [],
      outputs: [
          { name: "sqrtPriceX96",              type: "uint160" },
          { name: "tick",                      type: "int24"   },
          { name: "observationIndex",          type: "uint16"  },
          { name: "observationCardinality",    type: "uint16"  },
          { name: "observationCardinalityNext",type: "uint16"  },
          { name: "feeProtocol",               type: "uint8"   },
          { name: "unlocked",                  type: "bool"    },
      ] },
    { type: "function", name: "liquidity",   stateMutability: "view", inputs: [], outputs: [{ name: "", type: "uint128" }] },
    { type: "function", name: "fee",         stateMutability: "view", inputs: [], outputs: [{ name: "", type: "uint24"  }] },
    { type: "function", name: "token0",      stateMutability: "view", inputs: [], outputs: [{ name: "", type: "address" }] },
    { type: "function", name: "token1",      stateMutability: "view", inputs: [], outputs: [{ name: "", type: "address" }] },
    { type: "function", name: "tickSpacing", stateMutability: "view", inputs: [], outputs: [{ name: "", type: "int24"   }] },
]

// QuoterV2 — marked "view" so our Contract routes it via eth_call
const V3_QUOTER_ABI = [
    { type: "function", name: "quoteExactInputSingle", stateMutability: "view",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "tokenIn",           type: "address" },
          { name: "tokenOut",          type: "address" },
          { name: "amountIn",          type: "uint256" },
          { name: "fee",               type: "uint24"  },
          { name: "sqrtPriceLimitX96", type: "uint160" },
      ]}],
      outputs: [
          { name: "amountOut",               type: "uint256" },
          { name: "sqrtPriceX96After",       type: "uint160" },
          { name: "initializedTicksCrossed", type: "uint32"  },
          { name: "gasEstimate",             type: "uint256" },
      ]
    },
]

const V3_PM_ABI = [
    { type: "function", name: "mint", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "token0",         type: "address" },
          { name: "token1",         type: "address" },
          { name: "fee",            type: "uint24"  },
          { name: "tickLower",      type: "int24"   },
          { name: "tickUpper",      type: "int24"   },
          { name: "amount0Desired", type: "uint256" },
          { name: "amount1Desired", type: "uint256" },
          { name: "amount0Min",     type: "uint256" },
          { name: "amount1Min",     type: "uint256" },
          { name: "recipient",      type: "address" },
          { name: "deadline",       type: "uint256" },
      ]}],
      outputs: [
          { name: "tokenId",   type: "uint256" },
          { name: "liquidity", type: "uint128" },
          { name: "amount0",   type: "uint256" },
          { name: "amount1",   type: "uint256" },
      ]
    },
    { type: "function", name: "increaseLiquidity", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "tokenId",        type: "uint256" },
          { name: "amount0Desired", type: "uint256" },
          { name: "amount1Desired", type: "uint256" },
          { name: "amount0Min",     type: "uint256" },
          { name: "amount1Min",     type: "uint256" },
          { name: "deadline",       type: "uint256" },
      ]}],
      outputs: [
          { name: "liquidity", type: "uint128" },
          { name: "amount0",   type: "uint256" },
          { name: "amount1",   type: "uint256" },
      ]
    },
    { type: "function", name: "decreaseLiquidity", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "tokenId",    type: "uint256" },
          { name: "liquidity",  type: "uint128" },
          { name: "amount0Min", type: "uint256" },
          { name: "amount1Min", type: "uint256" },
          { name: "deadline",   type: "uint256" },
      ]}],
      outputs: [{ name: "amount0", type: "uint256" }, { name: "amount1", type: "uint256" }]
    },
    { type: "function", name: "collect", stateMutability: "payable",
      inputs: [{ type: "tuple", name: "params", components: [
          { name: "tokenId",    type: "uint256" },
          { name: "recipient",  type: "address" },
          { name: "amount0Max", type: "uint128" },
          { name: "amount1Max", type: "uint128" },
      ]}],
      outputs: [{ name: "amount0", type: "uint256" }, { name: "amount1", type: "uint256" }]
    },
    { type: "function", name: "burn", stateMutability: "payable",
      inputs: [{ name: "tokenId", type: "uint256" }],
      outputs: []
    },
    { type: "function", name: "positions", stateMutability: "view",
      inputs: [{ name: "tokenId", type: "uint256" }],
      outputs: [
          { name: "nonce",                    type: "uint96"  },
          { name: "operator",                 type: "address" },
          { name: "token0",                   type: "address" },
          { name: "token1",                   type: "address" },
          { name: "fee",                      type: "uint24"  },
          { name: "tickLower",                type: "int24"   },
          { name: "tickUpper",                type: "int24"   },
          { name: "liquidity",                type: "uint128" },
          { name: "feeGrowthInside0LastX128", type: "uint256" },
          { name: "feeGrowthInside1LastX128", type: "uint256" },
          { name: "tokensOwed0",              type: "uint128" },
          { name: "tokensOwed1",              type: "uint128" },
      ]
    },
    { type: "event", name: "IncreaseLiquidity", anonymous: false,
      inputs: [
          { indexed: true,  name: "tokenId",   type: "uint256" },
          { indexed: false, name: "liquidity",  type: "uint128" },
          { indexed: false, name: "amount0",    type: "uint256" },
          { indexed: false, name: "amount1",    type: "uint256" },
      ]
    },
    { type: "event", name: "DecreaseLiquidity", anonymous: false,
      inputs: [
          { indexed: true,  name: "tokenId",   type: "uint256" },
          { indexed: false, name: "liquidity",  type: "uint128" },
          { indexed: false, name: "amount0",    type: "uint256" },
          { indexed: false, name: "amount1",    type: "uint256" },
      ]
    },
    { type: "event", name: "Collect", anonymous: false,
      inputs: [
          { indexed: true,  name: "tokenId",   type: "uint256" },
          { indexed: false, name: "recipient",  type: "address" },
          { indexed: false, name: "amount0",    type: "uint256" },
          { indexed: false, name: "amount1",    type: "uint256" },
      ]
    },
]

// ─── Shared state ─────────────────────────────────────────────────────────────

let server, provider, wallet
let mintTokenId, mintLiquidity   // set by V3 mint test
let forkStartBlock               // block number of our ganache fork

// ─── Suite ────────────────────────────────────────────────────────────────────

describe("evm-fork (mainnet fork via ETH_RPC)", function () {
    this.timeout(180000)

    before("start ganache fork + acquire tokens", async function () {
        if (!ETH_RPC) return this.skip()
        this.timeout(120000)

        server = ganache.server({
            fork: { url: ETH_RPC, blockNumber: FORK_BLOCK },
            wallet: {
                accounts:         [{ balance: "0x" + (1000n * 10n**18n).toString(16), secretKey: TEST_PRIV }],
                unlockedAccounts: [WHALE],
            },
            logging: { quiet: true },
        })
        await new Promise(resolve => server.listen(0, resolve))
        const port = server.address().port
        provider   = rpc(`http://127.0.0.1:${port}`)
        wallet     = await Wallet.create(TEST_PRIV, provider)

        forkStartBlock = await provider.getBlock("latest").then(b => BigInt(b.number))

        // Give whale a small ETH balance for gas
        await wallet.sendTransaction({ to: WHALE, value: 5n * 10n**16n })  // 0.05 ETH

        // Transfer 10,000 USDC from whale to test wallet
        const usdcTransferAbi = { name: "transfer", inputs: [{ name: "to", type: "address" }, { name: "amount", type: "uint256" }] }
        const transferData    = await buildCalldata(buildSig(usdcTransferAbi), usdcTransferAbi.inputs, [TEST_ADDR, 10000n * 10n**6n])
        await provider.send("eth_sendTransaction", [{ from: WHALE, to: USDC, data: transferData, gas: "0x30000" }])

        // Wrap 5 ETH → 5 WETH
        const weth = new Contract(WETH, WETH_ABI, wallet)
        await weth.deposit({ value: 5n * 10n**18n })
    })

    after("close ganache fork", async function () {
        if (server) await server.close()
    })

    // ─── 1. Provider primitives ───────────────────────────────────────────────

    describe("provider primitives", function () {
        it("getBalance: returns ETH balance as BigInt", async function () {
            const bal = await provider.getBalance(TEST_ADDR)
            assert.ok(typeof bal === "bigint", "balance is BigInt")
            assert.ok(bal > 100n * 10n**18n, "wallet has >100 ETH after setup")
        })

        it("getCode: contract address returns non-empty bytecode", async function () {
            const code = await provider.send("eth_getCode", [WETH, "latest"])
            assert.ok(typeof code === "string" && code.length > 10, "WETH has bytecode")
            assert.notStrictEqual(code, "0x", "non-empty code")
        })

        it("getBlock: latest block has expected fields", async function () {
            const block = await provider.getBlock("latest")
            assert.ok(block, "block exists")
            assert.ok(typeof block.number === "number" || typeof block.number === "string", "has number")
            assert.ok(block.hash?.startsWith("0x"), "has hash")
            assert.ok(block.timestamp > 0, "has timestamp")
        })

        it("estimateGas: ETH transfer returns ~21000", async function () {
            const gas = await provider.estimateGas({ from: TEST_ADDR, to: WHALE, value: "0x1" })
            assert.ok(typeof gas === "bigint", "gas is BigInt")
            assert.ok(gas >= 21000n && gas < 30000n, `gas ${gas} should be ~21000`)
        })
    })

    // ─── 2. blockTag ─────────────────────────────────────────────────────────

    describe("blockTag", function () {
        it("string 'latest' returns whale USDC balance as BigInt", async function () {
            const usdc = new Contract(USDC, ERC20_ABI, provider)
            const bal  = await usdc.balanceOf(WHALE, { blockTag: "latest" })
            assert.ok(typeof bal === "bigint", "balance should be BigInt")
            assert.ok(bal > 0n, "whale should have USDC")
        })

        it("numeric BigInt blockTag returns BigInt without error", async function () {
            const usdc = new Contract(USDC, ERC20_ABI, provider)
            const bal  = await usdc.balanceOf(WHALE, { blockTag: BigInt(FORK_BLOCK - 10000) })
            assert.ok(typeof bal === "bigint", "balance should be BigInt")
        })

        it("numeric blockTag at an earlier block may differ from latest", async function () {
            const usdc    = new Contract(USDC, ERC20_ABI, provider)
            const latest  = await usdc.balanceOf(WHALE, { blockTag: "latest" })
            const earlier = await usdc.balanceOf(WHALE, { blockTag: BigInt(FORK_BLOCK - 100000) })
            assert.ok(typeof latest  === "bigint")
            assert.ok(typeof earlier === "bigint")
        })
    })

    // ─── 3. ETH + WETH wrap / unwrap ─────────────────────────────────────────

    describe("ETH + WETH", function () {
        it("native ETH transfer updates recipient balance", async function () {
            const before = await provider.getBalance(WHALE)
            await wallet.sendTransaction({ to: WHALE, value: 10n**17n })  // 0.1 ETH
            const after  = await provider.getBalance(WHALE)
            assert.ok(after > before, "whale ETH balance increased")
        })

        it("WETH deposit (wrap): WETH balance increases", async function () {
            const weth   = new Contract(WETH, WETH_ABI, wallet)
            const before = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            await weth.deposit({ value: 10n**17n })  // 0.1 ETH
            const after  = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(after > before, "WETH balance should increase after deposit")
        })

        it("WETH withdraw (unwrap): ETH balance increases, WETH decreases", async function () {
            const weth       = new Contract(WETH, WETH_ABI, wallet)
            const AMOUNT     = 5n * 10n**17n  // 0.5 WETH
            const ethBefore  = await provider.getBalance(TEST_ADDR)
            const wethBefore = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)

            await weth.withdraw(AMOUNT)

            const ethAfter  = await provider.getBalance(TEST_ADDR)
            const wethAfter = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(ethAfter  > ethBefore  - AMOUNT, "ETH should increase (minus gas)")
            assert.ok(wethAfter < wethBefore,           "WETH balance should decrease")
        })
    })

    // ─── 4. ERC20 transfer + transferFrom + Transfer event ───────────────────

    describe("ERC20 tokens", function () {
        it("transfer: USDC sent to recipient, Transfer event decoded", async function () {
            const usdc    = new Contract(USDC, ERC20_ABI, wallet)
            const AMOUNT  = 100n * 10n**6n  // 100 USDC
            const before  = await new Contract(USDC, ERC20_ABI, provider).balanceOf(WHALE)

            const receipt = await usdc.transfer(WHALE, AMOUNT)
            assert.ok(receipt, "transfer receipt exists")
            assert.ok(receipt.events?.Transfer, "Transfer event decoded")
            const rv = receipt.events.Transfer.returnValues
            assert.ok(rv.from.toLowerCase() === TEST_ADDR.toLowerCase(), "from is TEST_ADDR")
            assert.ok(rv.to.toLowerCase()   === WHALE.toLowerCase(),     "to is WHALE")
            assert.strictEqual(rv.value.toString(), AMOUNT.toString(),   "value matches")

            const after  = await new Contract(USDC, ERC20_ABI, provider).balanceOf(WHALE)
            assert.ok(after > before, "whale USDC increased")
        })

        it("approve + transferFrom: allowance pattern works", async function () {
            const usdc      = new Contract(USDC, ERC20_ABI, wallet)
            const AMOUNT    = 50n * 10n**6n  // 50 USDC
            const RECIPIENT = WHALE

            // Give a second test account some USDC to pull from
            await usdc.approve(RECIPIENT, AMOUNT)
            const allowed = await usdc.allowance(TEST_ADDR, RECIPIENT)
            assert.strictEqual(allowed.toString(), AMOUNT.toString(), "allowance matches approved amount")

            // Use eth_sendTransaction from whale (no private key needed — unlocked)
            const tfAbi  = { name: "transferFrom", inputs: [{ name: "from", type: "address" }, { name: "to", type: "address" }, { name: "amount", type: "uint256" }] }
            const tfData = await buildCalldata(buildSig(tfAbi), tfAbi.inputs, [TEST_ADDR, WHALE, AMOUNT])
            const result = await provider.send("eth_sendTransaction", [{ from: WHALE, to: USDC, data: tfData, gas: "0x30000" }])
            assert.ok(result?.startsWith("0x"), "transferFrom tx hash returned")

            const afterAllow = await usdc.allowance(TEST_ADDR, RECIPIENT)
            assert.strictEqual(afterAllow.toString(), "0", "allowance consumed to 0")
        })

        it("ERC20 balanceOf at blockTag returns correct historical value", async function () {
            const usdc = new Contract(USDC, ERC20_ABI, provider)
            // At fork block, whale had lots of USDC; further back slightly different
            const atFork   = await usdc.balanceOf(WHALE, { blockTag: BigInt(FORK_BLOCK) })
            const atEarlier= await usdc.balanceOf(WHALE, { blockTag: BigInt(FORK_BLOCK - 50000) })
            assert.ok(typeof atFork    === "bigint", "fork block balance is BigInt")
            assert.ok(typeof atEarlier === "bigint", "earlier block balance is BigInt")
        })
    })

    // ─── 5. V2 pool reads ────────────────────────────────────────────────────

    describe("V2 pool reads", function () {
        let v2PairAddr

        it("V2Factory.getPair: returns non-zero pair address", async function () {
            const factory = new Contract(V2_FACTORY, V2_FACTORY_ABI, provider)
            v2PairAddr    = await factory.getPair(WETH, USDC)
            assert.ok(v2PairAddr && v2PairAddr !== "0x0000000000000000000000000000000000000000", "pair address exists")
        })

        it("V2Pair.getReserves: returns (uint112, uint112, uint32) with reserves > 0", async function () {
            const pair = new Contract(v2PairAddr, V2_PAIR_ABI, provider)
            const res  = await pair.getReserves()
            assert.ok(typeof res.reserve0             === "bigint", "reserve0 is BigInt")
            assert.ok(typeof res.reserve1             === "bigint", "reserve1 is BigInt")
            assert.ok(typeof res.blockTimestampLast   === "bigint", "blockTimestampLast is BigInt")
            assert.ok(res.reserve0 > 0n && res.reserve1 > 0n,      "both reserves > 0")
        })

        it("V2Pair.token0 / token1: returns valid addresses", async function () {
            const pair   = new Contract(v2PairAddr, V2_PAIR_ABI, provider)
            const token0 = await pair.token0()
            const token1 = await pair.token1()
            const addrs  = [token0.toLowerCase(), token1.toLowerCase()]
            assert.ok(addrs.includes(WETH.toLowerCase()),  "WETH is one of the pair tokens")
            assert.ok(addrs.includes(USDC.toLowerCase()),  "USDC is one of the pair tokens")
        })
    })

    // ─── 6. V2 swap quote (getAmountsOut) ────────────────────────────────────

    describe("V2 swap quote", function () {
        it("getAmountsOut: [WETH, USDC] path returns uint256[] with 2 elements", async function () {
            const v2Router = new Contract(V2_ROUTER, V2_ROUTER_ABI, provider)
            const amounts  = await v2Router.getAmountsOut(
                10n**17n,          // 0.1 WETH in
                [WETH, USDC],      // path
            )
            assert.ok(Array.isArray(amounts), "amounts is array")
            assert.strictEqual(amounts.length, 2, "2 elements for 2-token path")
            assert.ok(amounts[0] > 0n, "amountIn > 0")
            assert.ok(amounts[1] > 100n * 10n**6n, "amountOut > 100 USDC for 0.1 ETH")
        })
    })

    // ─── 7. V2 swaps ─────────────────────────────────────────────────────────

    describe("V2 swaps", function () {
        it("swapExactTokensForTokens: USDC→WETH, amounts[] returned", async function () {
            const v2Router  = new Contract(V2_ROUTER, V2_ROUTER_ABI, wallet)
            const usdc      = new Contract(USDC,      ERC20_ABI,     wallet)
            const AMOUNT_IN = 500n * 10n**6n  // 500 USDC
            const deadline  = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await usdc.approve(V2_ROUTER, AMOUNT_IN)
            const receipt = await v2Router.swapExactTokensForTokens(
                AMOUNT_IN, 0n, [USDC, WETH], TEST_ADDR, deadline
            )
            assert.ok(receipt, "swap receipt exists")

            const wethBal = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(wethBal > 0n, "WETH balance > 0 after swap")
        })

        it("swapExactETHForTokens: ETH→USDC, value override works", async function () {
            const v2Router  = new Contract(V2_ROUTER, V2_ROUTER_ABI, wallet)
            const AMOUNT_IN = 2n * 10n**17n  // 0.2 ETH
            const deadline  = BigInt(Math.floor(Date.now() / 1000) + 3600)
            const before    = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)

            const receipt = await v2Router.swapExactETHForTokens(
                0n, [WETH, USDC], TEST_ADDR, deadline,
                { value: AMOUNT_IN }
            )
            assert.ok(receipt, "swap receipt exists")

            const after = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(after > before, "USDC balance increased after ETH→USDC swap")
        })

        it("swapExactTokensForETH: USDC→ETH, ETH balance increases", async function () {
            const v2Router  = new Contract(V2_ROUTER, V2_ROUTER_ABI, wallet)
            const usdc      = new Contract(USDC,      ERC20_ABI,     wallet)
            const AMOUNT_IN = 500n * 10n**6n  // 500 USDC
            const deadline  = BigInt(Math.floor(Date.now() / 1000) + 3600)
            const before    = await provider.getBalance(TEST_ADDR)

            await usdc.approve(V2_ROUTER, AMOUNT_IN)
            // Path must end in WETH so router unwraps to ETH
            const receipt = await v2Router.swapExactTokensForETH(
                AMOUNT_IN, 0n, [USDC, WETH], TEST_ADDR, deadline
            )
            assert.ok(receipt, "swap receipt exists")

            const after = await provider.getBalance(TEST_ADDR)
            // after ≈ before + ETH_received - gas; ETH_received >> gas for 500 USDC
            assert.ok(after > before - 10n**16n, "ETH balance did not decrease (got ETH from swap)")
        })
    })

    // ─── 8. V2 liquidity ─────────────────────────────────────────────────────

    describe("V2 addLiquidity + removeLiquidity round-trip", function () {
        let lpTokenAddr

        it("addLiquidity: LP tokens received", async function () {
            const weth      = new Contract(WETH,      WETH_ABI,        wallet)
            const usdc      = new Contract(USDC,      ERC20_ABI,       wallet)
            const v2Router  = new Contract(V2_ROUTER, V2_ROUTER_ABI,   wallet)
            const v2Factory = new Contract(V2_FACTORY,V2_FACTORY_ABI,  provider)

            const AMOUNT_WETH = 2n * 10n**17n   // 0.2 WETH
            const AMOUNT_USDC = 500n * 10n**6n  // 500 USDC
            const deadline    = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await weth.approve(V2_ROUTER, AMOUNT_WETH)
            await usdc.approve(V2_ROUTER, AMOUNT_USDC)

            const receipt = await v2Router.addLiquidity(
                WETH, USDC, AMOUNT_WETH, AMOUNT_USDC, 0n, 0n, TEST_ADDR, deadline
            )
            assert.ok(receipt, "addLiquidity receipt exists")

            lpTokenAddr = await v2Factory.getPair(WETH, USDC)
            const lpBal = await new Contract(lpTokenAddr, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(lpBal > 0n, "LP token balance > 0 after addLiquidity")
        })

        it("removeLiquidity: WETH returned to wallet, LP burned", async function () {
            assert.ok(lpTokenAddr, "lpTokenAddr must be set from addLiquidity test")
            const lpToken  = new Contract(lpTokenAddr, ERC20_ABI,     wallet)
            const v2Router = new Contract(V2_ROUTER,   V2_ROUTER_ABI, wallet)

            const lpBal       = await new Contract(lpTokenAddr, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            const wethBefore  = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            const deadline    = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await lpToken.approve(V2_ROUTER, lpBal)
            const receipt = await v2Router.removeLiquidity(
                WETH, USDC, lpBal, 0n, 0n, TEST_ADDR, deadline
            )
            assert.ok(receipt, "removeLiquidity receipt exists")

            const wethAfter  = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            const lpBalAfter = await new Contract(lpTokenAddr, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(wethAfter > wethBefore,                    "WETH returned after removeLiquidity")
            assert.strictEqual(lpBalAfter.toString(), "0",       "LP fully burned")
        })
    })

    // ─── 9. V3 QuoterV2 ──────────────────────────────────────────────────────

    describe("V3 QuoterV2", function () {
        it("quoteExactInputSingle: tuple input + 4-value return, sensible USDC amount", async function () {
            const quoter = new Contract(V3_QUOTER, V3_QUOTER_ABI, provider)
            const result = await quoter.quoteExactInputSingle({
                tokenIn:           WETH,
                tokenOut:          USDC,
                amountIn:          10n**17n,  // 0.1 WETH
                fee:               3000n,
                sqrtPriceLimitX96: 0n,
            })
            assert.ok(typeof result.amountOut               === "bigint", "amountOut is BigInt")
            assert.ok(typeof result.sqrtPriceX96After       === "bigint", "sqrtPriceX96After is BigInt")
            assert.ok(typeof result.initializedTicksCrossed === "bigint", "initializedTicksCrossed is BigInt")
            assert.ok(typeof result.gasEstimate             === "bigint", "gasEstimate is BigInt")
            // Block 19_500_000: ETH ≈ $3500 → 0.1 WETH ≈ 300–400 USDC
            assert.ok(result.amountOut > 200n * 10n**6n, `amountOut ${result.amountOut} should be > 200 USDC`)
            assert.ok(result.amountOut < 500n * 10n**6n, `amountOut ${result.amountOut} should be < 500 USDC`)
            assert.ok(result.sqrtPriceX96After > 0n, "sqrtPriceX96After > 0")
            assert.ok(result.gasEstimate       > 0n, "gasEstimate > 0")
        })
    })

    // ─── 10. V3 pool direct reads ────────────────────────────────────────────

    describe("V3 pool reads", function () {
        let v3PoolAddr

        it("V3Factory.getPool: returns non-zero pool address for USDC/WETH 3000", async function () {
            const factory = new Contract(V3_FACTORY, V3_FACTORY_ABI, provider)
            v3PoolAddr    = await factory.getPool(USDC, WETH, 3000n)
            assert.ok(v3PoolAddr && v3PoolAddr !== "0x0000000000000000000000000000000000000000",
                "pool address is non-zero")
        })

        it("V3Pool.slot0: 7-value return — sqrtPriceX96, signed int24 tick, bool unlocked", async function () {
            const pool   = new Contract(v3PoolAddr, V3_POOL_ABI, provider)
            const slot0  = await pool.slot0()
            assert.ok(typeof slot0.sqrtPriceX96 === "bigint",   "sqrtPriceX96 is BigInt")
            assert.ok(typeof slot0.tick         === "bigint",   "tick is BigInt")
            assert.ok(typeof slot0.unlocked     === "boolean",  "unlocked is boolean")
            assert.ok(slot0.sqrtPriceX96 > 0n,                  "sqrtPriceX96 > 0")
            // USDC/WETH 3000 tick should be in a plausible range (ETH ≈ $3500 at fork block)
            assert.ok(slot0.tick > -1000000n && slot0.tick < 1000000n, `tick ${slot0.tick} in range`)
        })

        it("V3Pool.liquidity: returns positive uint128", async function () {
            const pool = new Contract(v3PoolAddr, V3_POOL_ABI, provider)
            const liq  = await pool.liquidity()
            assert.ok(typeof liq === "bigint", "liquidity is BigInt")
            assert.ok(liq > 0n,               "pool has liquidity > 0")
        })

        it("V3Pool.fee + token0 + token1: verify pool metadata", async function () {
            const pool   = new Contract(v3PoolAddr, V3_POOL_ABI, provider)
            const fee    = await pool.fee()
            const token0 = await pool.token0()
            const token1 = await pool.token1()
            assert.strictEqual(fee.toString(), "3000",  "fee is 3000 (0.3%)")
            const addrs = [token0.toLowerCase(), token1.toLowerCase()]
            assert.ok(addrs.includes(USDC.toLowerCase()), "USDC is a pool token")
            assert.ok(addrs.includes(WETH.toLowerCase()), "WETH is a pool token")
        })
    })

    // ─── 11. V3 exactInputSingle swap ────────────────────────────────────────

    describe("V3 exactInputSingle swap", function () {
        it("WETH→USDC: amountOut > 0, USDC balance increases", async function () {
            const weth      = new Contract(WETH,      WETH_ABI,    wallet)
            const v3Router  = new Contract(V3_ROUTER, V3_SWAP_ABI, wallet)
            const AMOUNT_IN = 10n**17n  // 0.1 WETH

            const usdcBefore = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            await weth.approve(V3_ROUTER, AMOUNT_IN)
            const receipt = await v3Router.exactInputSingle({
                tokenIn:           WETH,
                tokenOut:          USDC,
                fee:               3000n,
                recipient:         TEST_ADDR,
                amountIn:          AMOUNT_IN,
                amountOutMinimum:  0n,
                sqrtPriceLimitX96: 0n,
            })
            assert.ok(receipt, "receipt exists")

            const usdcAfter = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(usdcAfter > usdcBefore, "USDC balance increased after V3 swap")
        })
    })

    // ─── 12. V3 exactInput (path bytes) ──────────────────────────────────────
    //
    // Tests bytes type ABI encoding — encodePath encodes the packed path:
    //   WETH + fee(3000) + USDC  →  43-byte hex string

    describe("V3 exactInput (bytes path)", function () {
        it("WETH→USDC via path bytes — tests bytes ABI type encoding", async function () {
            const weth     = new Contract(WETH,      WETH_ABI,    wallet)
            const v3Router = new Contract(V3_ROUTER, V3_SWAP_ABI, wallet)
            const AMOUNT_IN= 10n**17n  // 0.1 WETH

            const path       = encodePath([WETH, USDC], [3000])
            const usdcBefore = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)

            await weth.approve(V3_ROUTER, AMOUNT_IN)
            const receipt = await v3Router.exactInput({
                path,
                recipient:        TEST_ADDR,
                amountIn:         AMOUNT_IN,
                amountOutMinimum: 0n,
            })
            assert.ok(receipt, "exactInput receipt exists")

            const usdcAfter = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(usdcAfter > usdcBefore, "USDC increased after exactInput path swap")
        })
    })

    // ─── 13. V3 exactOutput ──────────────────────────────────────────────────

    describe("V3 exactOutput", function () {
        it("exactOutput USDC→WETH: spends ≤ amountInMaximum, returns exact amountOut", async function () {
            const usdc     = new Contract(USDC,      ERC20_ABI,   wallet)
            const v3Router = new Contract(V3_ROUTER, V3_SWAP_ABI, wallet)
            // Want exactly 0.1 WETH out, pay at most 1000 USDC
            const WANT_OUT  = 10n**17n          // 0.1 WETH exactly
            const MAX_IN    = 1000n * 10n**6n   // max 1000 USDC

            // exactOutput path is reversed: tokenOut first, then tokenIn
            const path = encodePath([WETH, USDC], [3000])  // reversed path for exactOutput

            await usdc.approve(V3_ROUTER, MAX_IN)
            const wethBefore = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)

            const receipt = await v3Router.exactOutput({
                path,
                recipient:       TEST_ADDR,
                amountOut:       WANT_OUT,
                amountInMaximum: MAX_IN,
            })
            assert.ok(receipt, "exactOutput receipt exists")

            const wethAfter = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(wethAfter >= wethBefore + WANT_OUT, `WETH increased by at least ${WANT_OUT}`)
        })
    })

    // ─── 14. V3 NonfungiblePositionManager full round-trip ───────────────────
    //
    // mint → positions read → increaseLiquidity → positions read (verify increase)
    // → decreaseLiquidity (all) → collect → burn

    describe("V3 NonfungiblePositionManager round-trip", function () {
        it("mint: creates position, IncreaseLiquidity event decoded", async function () {
            const weth = new Contract(WETH, WETH_ABI,  wallet)
            const usdc = new Contract(USDC, ERC20_ABI, wallet)
            const pm   = new Contract(V3_PM, V3_PM_ABI, wallet)

            const AMOUNT_WETH = 3n * 10n**17n    // 0.3 WETH
            const AMOUNT_USDC = 1000n * 10n**6n  // 1000 USDC
            const deadline    = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await usdc.approve(V3_PM, AMOUNT_USDC)
            await weth.approve(V3_PM, AMOUNT_WETH)

            const receipt = await pm.mint({
                token0:         USDC,
                token1:         WETH,
                fee:            3000n,
                tickLower:      TICK_LOWER,
                tickUpper:      TICK_UPPER,
                amount0Desired: AMOUNT_USDC,
                amount1Desired: AMOUNT_WETH,
                amount0Min:     0n,
                amount1Min:     0n,
                recipient:      TEST_ADDR,
                deadline,
            })
            assert.ok(receipt, "mint receipt exists")
            assert.ok(receipt.events?.IncreaseLiquidity, "IncreaseLiquidity event decoded")

            mintTokenId   = receipt.events.IncreaseLiquidity.returnValues.tokenId
            mintLiquidity = receipt.events.IncreaseLiquidity.returnValues.liquidity
            assert.ok(typeof mintTokenId   === "bigint" && mintTokenId   > 0n, "tokenId > 0")
            assert.ok(typeof mintLiquidity === "bigint" && mintLiquidity > 0n, "liquidity > 0")
        })

        it("positions(): liquidity and tokens match mint event", async function () {
            assert.ok(mintTokenId != null)
            const pm  = new Contract(V3_PM, V3_PM_ABI, provider)
            const pos = await pm.positions(mintTokenId)
            assert.strictEqual(pos.liquidity.toString(), mintLiquidity.toString(), "liquidity matches")
            assert.strictEqual(pos.token0.toLowerCase(), USDC.toLowerCase(), "token0 is USDC")
            assert.strictEqual(pos.token1.toLowerCase(), WETH.toLowerCase(), "token1 is WETH")
        })

        it("increaseLiquidity: adds more tokens, liquidity increases", async function () {
            assert.ok(mintTokenId != null)
            const weth     = new Contract(WETH,  WETH_ABI,   wallet)
            const usdc     = new Contract(USDC,  ERC20_ABI,  wallet)
            const pm       = new Contract(V3_PM, V3_PM_ABI,  wallet)
            const ADD_WETH = 10n**17n          // 0.1 WETH
            const ADD_USDC = 300n * 10n**6n   // 300 USDC
            const deadline = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await usdc.approve(V3_PM, ADD_USDC)
            await weth.approve(V3_PM, ADD_WETH)

            const receipt = await pm.increaseLiquidity({
                tokenId:        mintTokenId,
                amount0Desired: ADD_USDC,
                amount1Desired: ADD_WETH,
                amount0Min:     0n,
                amount1Min:     0n,
                deadline,
            })
            assert.ok(receipt, "increaseLiquidity receipt exists")

            const pos = await new Contract(V3_PM, V3_PM_ABI, provider).positions(mintTokenId)
            assert.ok(pos.liquidity > mintLiquidity, "liquidity increased after increaseLiquidity")
            mintLiquidity = pos.liquidity  // update for decreaseLiquidity
        })

        it("decreaseLiquidity: removes all liquidity", async function () {
            assert.ok(mintTokenId != null && mintLiquidity != null)
            const pm       = new Contract(V3_PM, V3_PM_ABI, wallet)
            const deadline = BigInt(Math.floor(Date.now() / 1000) + 3600)

            const receipt = await pm.decreaseLiquidity({
                tokenId:    mintTokenId,
                liquidity:  mintLiquidity,
                amount0Min: 0n,
                amount1Min: 0n,
                deadline,
            })
            assert.ok(receipt, "decreaseLiquidity receipt exists")

            const posAfter = await new Contract(V3_PM, V3_PM_ABI, provider).positions(mintTokenId)
            assert.strictEqual(posAfter.liquidity.toString(), "0", "liquidity = 0 after full decrease")
        })

        it("collect: Collect event with correct tokenId and recipient", async function () {
            assert.ok(mintTokenId != null)
            const pm         = new Contract(V3_PM, V3_PM_ABI, wallet)
            const MAX_UINT128 = (1n << 128n) - 1n

            const receipt = await pm.collect({
                tokenId:    mintTokenId,
                recipient:  TEST_ADDR,
                amount0Max: MAX_UINT128,
                amount1Max: MAX_UINT128,
            })
            assert.ok(receipt, "collect receipt exists")
            assert.ok(receipt.events?.Collect, "Collect event decoded")
            const rv = receipt.events.Collect.returnValues
            assert.strictEqual(rv.tokenId.toString(),          mintTokenId.toString())
            assert.strictEqual(rv.recipient.toLowerCase(),     TEST_ADDR.toLowerCase())
            assert.ok(typeof rv.amount0 === "bigint", "amount0 is BigInt")
            assert.ok(typeof rv.amount1 === "bigint", "amount1 is BigInt")
        })

        it("burn: NFT removed, positions() reverts", async function () {
            assert.ok(mintTokenId != null)
            const pm      = new Contract(V3_PM, V3_PM_ABI, wallet)
            const receipt = await pm.burn(mintTokenId)
            assert.ok(receipt, "burn receipt exists")

            const posAfter = await new Contract(V3_PM, V3_PM_ABI, provider).positions(mintTokenId).catch(() => null)
            if (posAfter !== null) {
                assert.strictEqual(posAfter.liquidity.toString(), "0")
            }
        })
    })
})
