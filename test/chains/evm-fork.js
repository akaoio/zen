/**
 * test/chains/evm-fork.js — Phase C: mainnet fork integration tests
 *
 * Forks Ethereum mainnet at a pinned block via ganache. Tests run against real
 * Uniswap V2/V3 contracts at their production mainnet addresses — no tokens or
 * deployment needed.
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
} from "../../lib/chains/evm.js"

// ─── Config ───────────────────────────────────────────────────────────────────

const ETH_RPC    = process.env.ETH_RPC
const FORK_BLOCK = 19_500_000   // Feb 2024 — Uniswap V2+V3 both active

// Hardhat account #0 (widely known, safe for tests)
const TEST_PRIV = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80"
const TEST_ADDR = new ethers.Wallet(TEST_PRIV).address

// ─── Mainnet contract addresses ───────────────────────────────────────────────

const WETH          = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"
const USDC          = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"
const V2_ROUTER     = "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D"
const V2_FACTORY    = "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f"
const V3_ROUTER     = "0x68b3465833fb72A70ecDF485E0e4C7bD8665Fc45"  // SwapRouter02 (no deadline)
const V3_PM         = "0xC36442b4a4522E871399CD717aBDD847Ab11FE88"   // NonfungiblePositionManager
const V3_QUOTER     = "0x61fFE014bA17989E743c5F6cB21bF9697530B21e"   // QuoterV2 (tuple-based, akao statics)
const WHALE         = "0x28C6c06298d514Db089934071355E5743bf21d60"   // Binance 14 — large USDC holder

// V3 USDC/WETH 0.3% pool tick range (tickSpacing = 60)
const TICK_LOWER    = -887220n
const TICK_UPPER    =  887220n

// ─── ABIs ─────────────────────────────────────────────────────────────────────

const ERC20_ABI = [
    { type: "function", name: "balanceOf",  stateMutability: "view",        inputs: [{ name: "a",       type: "address" }],                                                             outputs: [{ name: "", type: "uint256" }] },
    { type: "function", name: "approve",    stateMutability: "nonpayable",  inputs: [{ name: "spender", type: "address" }, { name: "amount", type: "uint256" }],                        outputs: [{ name: "", type: "bool"    }] },
    { type: "function", name: "transfer",   stateMutability: "nonpayable",  inputs: [{ name: "to",      type: "address" }, { name: "amount", type: "uint256" }],                        outputs: [{ name: "", type: "bool"    }] },
    { type: "function", name: "allowance",  stateMutability: "view",        inputs: [{ name: "owner",   type: "address" }, { name: "spender", type: "address" }],                       outputs: [{ name: "", type: "uint256" }] },
]

const WETH_ABI = [
    ...ERC20_ABI,
    { type: "function", name: "deposit",    stateMutability: "payable",     inputs: [],                                                                                                  outputs: [] },
]

const V2_ROUTER_ABI = [
    { type: "function", name: "addLiquidity",    stateMutability: "nonpayable",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }, { name: "amountADesired", type: "uint256" }, { name: "amountBDesired", type: "uint256" }, { name: "amountAMin", type: "uint256" }, { name: "amountBMin", type: "uint256" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amountA", type: "uint256" }, { name: "amountB", type: "uint256" }, { name: "liquidity", type: "uint256" }] },
    { type: "function", name: "removeLiquidity",  stateMutability: "nonpayable",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }, { name: "liquidity", type: "uint256" }, { name: "amountAMin", type: "uint256" }, { name: "amountBMin", type: "uint256" }, { name: "to", type: "address" }, { name: "deadline", type: "uint256" }],
      outputs: [{ name: "amountA", type: "uint256" }, { name: "amountB", type: "uint256" }] },
]

const V2_FACTORY_ABI = [
    { type: "function", name: "getPair", stateMutability: "view",
      inputs: [{ name: "tokenA", type: "address" }, { name: "tokenB", type: "address" }],
      outputs: [{ name: "pair", type: "address" }] },
]

// SwapRouter02 exactInputSingle (no deadline field — router uses block.timestamp)
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

// QuoterV2 — tuple-based quoteExactInputSingle; marked "view" to route via eth_call
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

// ─── Shared state ─────────────────────────────────────────────────────────────

let server, provider, wallet
let mintTokenId, mintLiquidity  // set by V3 mint test, used by collect/burn

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

        // Give whale a small ETH balance for gas (from our test account)
        await wallet.sendTransaction({ to: WHALE, value: 5n * 10n**16n })  // 0.05 ETH

        // Transfer 5000 USDC from whale to test wallet (eth_sendTransaction, no signing needed)
        const usdcTransferAbi = { name: "transfer", inputs: [{ name: "to", type: "address" }, { name: "amount", type: "uint256" }] }
        const transferData    = await buildCalldata(buildSig(usdcTransferAbi), usdcTransferAbi.inputs, [TEST_ADDR, 5000n * 10n**6n])
        await provider.send("eth_sendTransaction", [{ from: WHALE, to: USDC, data: transferData, gas: "0x30000" }])

        // Wrap 2 ETH → 2 WETH (tests payable Contract call with value override)
        const weth = new Contract(WETH, WETH_ABI, wallet)
        await weth.deposit({ value: 2n * 10n**18n })
    })

    after("close ganache fork", async function () {
        if (server) await server.close()
    })

    // ─── 1. blockTag ─────────────────────────────────────────────────────────

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
            // Both are valid BigInts — values may differ (Binance is active)
            assert.ok(typeof latest  === "bigint")
            assert.ok(typeof earlier === "bigint")
        })
    })

    // ─── 2. V3 exactInputSingle swap ─────────────────────────────────────────

    describe("V3 exactInputSingle swap", function () {
        it("WETH→USDC swap succeeds and returns amountOut > 0", async function () {
            const weth     = new Contract(WETH,      WETH_ABI,    wallet)
            const v3Router = new Contract(V3_ROUTER, V3_SWAP_ABI, wallet)
            const AMOUNT_IN = 100_000_000_000_000_000n  // 0.1 WETH

            const deadline    = BigInt(Math.floor(Date.now() / 1000) + 3600)
            const usdcBefore  = await new Contract(USDC, ERC20_ABI, provider).balanceOf(TEST_ADDR)

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
            assert.ok(usdcAfter > usdcBefore, "USDC balance should increase after swap")
        })
    })

    // ─── 3. V3 NonfungiblePositionManager round-trip ─────────────────────────

    describe("V3 NonfungiblePositionManager round-trip", function () {
        it("mint: creates position, IncreaseLiquidity event decoded", async function () {
            const weth = new Contract(WETH, WETH_ABI, wallet)
            const usdc = new Contract(USDC, ERC20_ABI, wallet)
            const pm   = new Contract(V3_PM, V3_PM_ABI, wallet)

            const AMOUNT_WETH = 300_000_000_000_000_000n   // 0.3 WETH (token1)
            const AMOUNT_USDC = 1000n * 10n**6n             // 1000 USDC (token0)
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

        it("positions(): liquidity matches IncreaseLiquidity event", async function () {
            assert.ok(mintTokenId != null, "tokenId must be set from mint test")
            const pm  = new Contract(V3_PM, V3_PM_ABI, provider)
            const pos = await pm.positions(mintTokenId)
            assert.strictEqual(pos.liquidity.toString(), mintLiquidity.toString())
            assert.strictEqual(pos.token0.toLowerCase(), USDC.toLowerCase())
            assert.strictEqual(pos.token1.toLowerCase(), WETH.toLowerCase())
        })

        it("decreaseLiquidity: removes all liquidity from position", async function () {
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
            assert.strictEqual(posAfter.liquidity.toString(), "0", "liquidity should be 0 after decrease")
        })

        it("collect: Collect event decoded with tokenId and recipient", async function () {
            assert.ok(mintTokenId != null)
            const pm = new Contract(V3_PM, V3_PM_ABI, wallet)
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
            assert.strictEqual(rv.tokenId.toString(),  mintTokenId.toString(), "tokenId matches")
            assert.strictEqual(rv.recipient.toLowerCase(), TEST_ADDR.toLowerCase(), "recipient matches")
            assert.ok(typeof rv.amount0 === "bigint", "amount0 is BigInt")
            assert.ok(typeof rv.amount1 === "bigint", "amount1 is BigInt")
        })

        it("burn: removes position NFT", async function () {
            assert.ok(mintTokenId != null)
            const pm      = new Contract(V3_PM, V3_PM_ABI, wallet)
            const receipt = await pm.burn(mintTokenId)
            assert.ok(receipt, "burn receipt exists")

            // After burn, positions() should revert — ganache returns 0x, decode returns null
            const posAfter = await new Contract(V3_PM, V3_PM_ABI, provider).positions(mintTokenId).catch(() => null)
            // Either returns null (reverted) or tokensOwed fields are zero
            if (posAfter !== null) {
                assert.strictEqual(posAfter.liquidity.toString(), "0")
            }
        })
    })

    // ─── 4. V3 QuoterV2 ──────────────────────────────────────────────────────
    //
    // QuoterV2 (0x61fFE...) uses a revert-based simulation under the hood,
    // but is intended to be called via eth_call (marked "view" in our ABI).
    // Tests: tuple input encoding, 4-value return decoding, real WETH→USDC price.

    describe("V3 QuoterV2", function () {
        it("quoteExactInputSingle: WETH→USDC returns 4-value result with sensible amountOut", async function () {
            const quoter = new Contract(V3_QUOTER, V3_QUOTER_ABI, provider)
            const result = await quoter.quoteExactInputSingle({
                tokenIn:           WETH,
                tokenOut:          USDC,
                amountIn:          100_000_000_000_000_000n,  // 0.1 WETH
                fee:               3000n,
                sqrtPriceLimitX96: 0n,
            })
            assert.ok(typeof result.amountOut               === "bigint", "amountOut is BigInt")
            assert.ok(typeof result.sqrtPriceX96After       === "bigint", "sqrtPriceX96After is BigInt")
            assert.ok(typeof result.initializedTicksCrossed === "bigint", "initializedTicksCrossed is BigInt")
            assert.ok(typeof result.gasEstimate             === "bigint", "gasEstimate is BigInt")
            // Block 19_500_000 ≈ Feb 2024; ETH ≈ $3500 → 0.1 WETH ≈ 300–400 USDC (6 decimals)
            assert.ok(result.amountOut > 200n * 10n**6n, `amountOut ${result.amountOut} should be > 200 USDC`)
            assert.ok(result.amountOut < 500n * 10n**6n, `amountOut ${result.amountOut} should be < 500 USDC`)
            assert.ok(result.sqrtPriceX96After > 0n, "sqrtPriceX96After > 0")
            assert.ok(result.gasEstimate       > 0n, "gasEstimate > 0")
        })
    })

    // ─── 5. V2 addLiquidity + removeLiquidity ────────────────────────────────

    describe("V2 addLiquidity + removeLiquidity round-trip", function () {
        let lpTokenAddr

        it("addLiquidity: LP tokens received", async function () {
            const weth      = new Contract(WETH,      WETH_ABI,      wallet)
            const usdc      = new Contract(USDC,      ERC20_ABI,     wallet)
            const v2Router  = new Contract(V2_ROUTER, V2_ROUTER_ABI, wallet)
            const v2Factory = new Contract(V2_FACTORY, V2_FACTORY_ABI, provider)

            const AMOUNT_WETH = 200_000_000_000_000_000n  // 0.2 WETH
            const AMOUNT_USDC = 500n * 10n**6n             // 500 USDC
            const deadline    = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await weth.approve(V2_ROUTER, AMOUNT_WETH)
            await usdc.approve(V2_ROUTER, AMOUNT_USDC)

            const receipt = await v2Router.addLiquidity(
                WETH, USDC,
                AMOUNT_WETH, AMOUNT_USDC,
                0n, 0n,
                TEST_ADDR,
                deadline
            )
            assert.ok(receipt, "addLiquidity receipt exists")

            lpTokenAddr = await v2Factory.getPair(WETH, USDC)
            const lpToken = new Contract(lpTokenAddr, ERC20_ABI, provider)
            const lpBal   = await lpToken.balanceOf(TEST_ADDR)
            assert.ok(lpBal > 0n, "LP token balance should be > 0 after addLiquidity")
        })

        it("removeLiquidity: tokens returned to wallet", async function () {
            assert.ok(lpTokenAddr, "lpTokenAddr must be set from addLiquidity test")
            const weth     = new Contract(WETH,       WETH_ABI,      wallet)
            const lpToken  = new Contract(lpTokenAddr, ERC20_ABI,    wallet)
            const v2Router = new Contract(V2_ROUTER,  V2_ROUTER_ABI, wallet)

            const lpBal   = await new Contract(lpTokenAddr, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            const wethBefore = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            const deadline   = BigInt(Math.floor(Date.now() / 1000) + 3600)

            await lpToken.approve(V2_ROUTER, lpBal)
            const receipt = await v2Router.removeLiquidity(
                WETH, USDC,
                lpBal,
                0n, 0n,
                TEST_ADDR,
                deadline
            )
            assert.ok(receipt, "removeLiquidity receipt exists")

            const wethAfter = await new Contract(WETH, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.ok(wethAfter > wethBefore, "WETH balance should increase after removeLiquidity")

            const lpBalAfter = await new Contract(lpTokenAddr, ERC20_ABI, provider).balanceOf(TEST_ADDR)
            assert.strictEqual(lpBalAfter.toString(), "0", "LP balance should be 0 after full removal")
        })
    })
})
