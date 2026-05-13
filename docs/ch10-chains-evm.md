# Ch 10 — EVM Chain Adapter (`lib/chains/evm.js`)

**Thay thế hoàn toàn `ethers.js` bằng ZEN crypto primitives — không có external dependency.**

---

## Tại sao thay ethers.js?

`ethers.js` v6 (~144 KB compressed) là một thư viện xuất sắc nhưng có một số vấn đề khi dùng trong hệ sinh thái ZEN:

| Vấn đề | ethers.js | zen/chains |
|---|---|---|
| Bundle size | ~144 KB compressed | Không bundle — explicit import |
| Dependency | External npm package | Dùng `zen.js` WASM primitives đã có sẵn |
| BigNumber | Trước v6 dùng class riêng | Native `BigInt` từ đầu |
| secp256k1 | Dùng pure-JS hoặc native module | WASM (crypto.wasm) — nhanh hơn |
| keccak256 | Pure-JS | WASM-accelerated |
| Kiến trúc | Class-based, OOP deep hierarchy | Thin adapter, mọi crypto từ `ZEN.*` |

ZEN đã có sẵn secp256k1 + keccak256 WASM. `lib/chains/evm.js` chỉ là lớp adapter mỏng phía trên, cung cấp đúng API mà DeFi developer cần.

---

## Import

`lib/chains/evm.js` **không** được bundle vào `zen.js`. Import riêng:

```js
// Default export: { rpc, wsRpc, Wallet, Contract, isAddress, connector }
import chains from "@akaoio/zen/lib/chains.js"

// Hoặc named imports
import {
    rpc, wsRpc, Wallet, Contract,
    isAddress, checksumAddress, privateToAddress,
    formatUnits, parseUnits, formatEther, parseEther,
    solidityPacked, solidityPackedKeccak256, getCreate2Address,
    parseAbi, parseAbiItem,
    hashStruct, hashDomain, hashTypedData, encodeTypedData, recoverTypedDataAddress,
    encodePath, buildCalldata, decodeReturnData,
    buildSig, rlpEncode, signTransaction,
} from "@akaoio/zen/lib/chains/evm.js"
```

**Ràng buộc kiến trúc quan trọng:**
- `lib/chains/evm.js` KHÔNG import từ `src/`. Chỉ import `ZEN from "../../zen.js"` (bundled).
- Mọi secp256k1 / keccak256 đều đi qua `ZEN.pair`, `ZEN.sign`, `ZEN.hash`.

---

## Cấu trúc file

```
lib/chains/evm.js  (1461 dòng)
├── Byte utilities         — hexToBytes, bytesToHex, bigIntToBytes, concatBytes, strToBytes
├── Address utilities      — checksumAddress, isAddress, addressFromSlot, privateToAddress
├── Tier 1 utilities       — solidityPacked, solidityPackedKeccak256, getCreate2Address, block.date
├── Human-readable ABI     — parseAbi, parseAbiItem, _splitTopLevel, _parseParam, _parseType
├── RLP encoder            — rlpEncode (EIP-155 transaction serialization)
├── ABI encoder/decoder    — encodeParams, encodeStatic, isDynamicType, decodeReturnData
├── JSON-RPC Provider      — Provider class (HTTP)
├── WebSocket Provider     — WsProvider class (WS + auto-reconnect)
├── EIP-155 tx signing     — signTransaction
├── Wallet                 — Wallet class (sign & send)
├── Event decoding         — buildEventTopicMap, decodeEventLog, decodeReceiptEvents
├── Contract               — Contract class (read/write/queryFilter)
├── Connector shim         — MULTICALL3_ABI, connector object
├── Unit conversion        — formatUnits, parseUnits, formatEther, parseEther
├── V3 path encoding       — encodePath
└── EIP-712                — hashStruct, hashDomain, hashTypedData, encodeTypedData, recoverTypedDataAddress
```

---

## Provider (HTTP)

```js
const provider = rpc("https://mainnet.infura.io/v3/<KEY>")
// Hoặc với network hint để skip eth_chainId call:
const provider = rpc("https://mainnet.infura.io/v3/<KEY>", { chainId: 1 })
```

### Methods

| Method | Signature | Mô tả |
|---|---|---|
| `getChainId` | `() → BigInt` | Cached sau lần đầu |
| `getBlock` | `(tag) → Block` | `tag`: "latest", "earliest", hex, number. Block có thêm `block.date` (Date) |
| `getBalance` | `(address) → BigInt` | Wei |
| `getTransactionCount` | `(address, tag?) → BigInt` | Nonce |
| `getGasPrice` | `() → BigInt` | EIP-1559-aware: nếu `eth_gasPrice < baseFee` thì trả `baseFee * 1.2` |
| `getFeeData` | `() → { gasPrice, maxFeePerGas, maxPriorityFeePerGas }` | |
| `estimateGas` | `(tx) → BigInt` | tx: `{ from?, to, data, value? }` |
| `call` | `(tx, blockTag?) → hex` | Gọi view function. blockTag có thể là số, BigInt, hoặc string |
| `getLogs` | `(filter) → Log[]` | filter: `{ address?, topics?, fromBlock?, toBlock? }` |
| `getTransactionReceipt` | `(hash) → Receipt \| null` | |
| `waitForTransaction` | `(hash, timeout?) → Receipt` | Poll mỗi 2s. Throw nếu `status=0x0` (revert). Default timeout 120s |
| `sendRawTransaction` | `(rawHex) → txHash` | |
| `multicall` | `(calls) → results[]` | Xem phần multicall bên dưới |
| `send` | `(method, params) → any` | Raw JSON-RPC call |

### block.date

Mọi block trả về từ `getBlock()` tự động có `block.date`:

```js
const block = await provider.getBlock("latest")
console.log(block.number)    // "0x12a5b4f"
console.log(block.timestamp) // "0x680e1234"
console.log(block.date)      // Date: 2026-05-13T...
```

### waitForTransaction và revert detection

```js
const { hash, wait } = await wallet.sendTransaction({ to, data })
const receipt = await wait()  // throw nếu tx revert!
// receipt.status sẽ là "0x1" (success)

// Nếu tx revert:
try {
    const receipt = await wait()
} catch (err) {
    console.log(err.message)  // "Transaction reverted: 0x..."
    console.log(err.receipt)  // raw receipt object
}
```

---

## WsProvider (WebSocket)

```js
const ws = wsRpc("wss://mainnet.infura.io/ws/v3/<KEY>")
```

Có đầy đủ HTTP provider methods + subscriptions. Điểm nổi bật:

### Auto-reconnect

Nếu WebSocket bị ngắt kết nối (server restart, network flicker), WsProvider tự động:
1. Đợi `reconnectDelay` ms (mặc định 1000ms)
2. Reconnect
3. Re-subscribe tất cả subscriptions đã đăng ký
4. Delay tăng theo exponential backoff (x2 mỗi lần), tối đa 30 giây

```js
const ws = wsRpc("wss://...")
// Đăng ký theo dõi block mới
const subId = await ws.on("block", (blockNumber) => {
    console.log("New block:", blockNumber)
})
// Nếu WS bị ngắt, callback vẫn nhận blocks sau khi reconnect tự động.

// Hủy subscription
await ws.off("block")

// Dừng hoàn toàn (không reconnect nữa)
await ws.destroy()
```

### Subscribe events

```js
// Block mới
await ws.on("block", (blockNumber) => { ... })

// Error handler
await ws.on("error", (err) => { ... })

// Hủy theo subscription ID
const subId = await ws.on("block", handler)
await ws.off(subId)
```

---

## Wallet

```js
// Từ private key (hex)
const wallet = new Wallet("0xac0974bec39a17e36ba4a6b4d238ff944bacb478...")
await wallet._ready  // address được compute async

// Factory — address guaranteed ready on return
const wallet = await Wallet.create("0x...")
console.log(wallet.address)  // "0x..."

// Kết nối với provider
const connected = wallet.connect(provider)
```

### Ký và gửi transaction

```js
const { hash, wait } = await wallet.sendTransaction({
    to: "0xRecipient...",
    value: parseEther("0.1"),
    data: "0x",
    // Optional overrides:
    gasLimit: 21000n,
    gasPrice: parseUnits("20", "gwei"),
})
const receipt = await wait()
```

`sendTransaction` tự động:
1. Lấy `nonce` từ `eth_getTransactionCount`
2. Lấy `gasPrice` (EIP-1559-aware)
3. Estimate gas với 20% buffer nếu không có `gasLimit`
4. Ký với `ZEN.sign(hash, pair, null, { prehash: true, encode: "raw" })`
5. Broadcast với `eth_sendRawTransaction`
6. Trả về `{ hash, wait() }` — gọi `wait()` để poll receipt

---

## Contract

```js
// JSON ABI (truyền thống)
const erc20 = new Contract(tokenAddress, ERC20_ABI, provider)

// Human-readable ABI strings (mới — DX tốt hơn nhiều)
const erc20 = new Contract(tokenAddress, [
    "function balanceOf(address) view returns (uint256)",
    "function transfer(address to, uint256 amount) returns (bool)",
    "function approve(address spender, uint256 amount) returns (bool)",
    "event Transfer(address indexed from, address indexed to, uint256 value)",
], provider)

// Với signer (write access)
const erc20 = new Contract(tokenAddress, ABI, wallet)
```

### Read methods (view/pure)

```js
const balance = await erc20.balanceOf("0xAddr...")  // → BigInt
// Với blockTag
const historical = await erc20.balanceOf("0xAddr...", { blockTag: 19500000 })
```

### Write methods

```js
const receipt = await erc20.transfer("0xRecipient...", parseUnits("100", 6))
// receipt.events chứa các event đã decode
console.log(receipt.events.Transfer.returnValues)
// → { from: "0x...", to: "0x...", value: 100000000n }

// Với overrides
const receipt = await erc20.transfer("0xTo...", 1000000n, {
    gasLimit: 80000n,
    value: 0n,
})
```

### contract.interface

```js
const calldata = await erc20.interface.encodeFunctionData("transfer", [to, amount])
// → "0xa9059cbb..."
```

### queryFilter — Event history

```js
// Lấy tất cả Transfer events trong block range
const events = await erc20.queryFilter("Transfer", 19000000, 19500000)
// Hoặc với ABI event object
const events = await erc20.queryFilter(TRANSFER_EVENT_ABI, fromBlock, toBlock)

// Mỗi event:
// { from, to, value, _log: rawLog, blockNumber, transactionHash }
for (const ev of events) {
    console.log(`${formatUnits(ev.value, 6)} USDC: ${ev.from} → ${ev.to}`)
}
```

### connect — thay đổi signer/provider

```js
const readOnly  = new Contract(addr, ABI, provider)
const withSigner = readOnly.connect(wallet)
```

---

## Human-readable ABI (`parseAbi` / `parseAbiItem`)

```js
import { parseAbi, parseAbiItem } from "@akaoio/zen/lib/chains/evm.js"

// Parse 1 item
const fnAbi = parseAbiItem("function transfer(address to, uint256 amount) returns (bool)")
// → { type: "function", name: "transfer", inputs: [...], outputs: [...], stateMutability: "nonpayable" }

// Parse array (mixed strings + objects)
const abi = parseAbi([
    "function balanceOf(address account) view returns (uint256)",
    "function transfer(address to, uint256 amount) returns (bool)",
    "event Transfer(address indexed from, address indexed to, uint256 value)",
    "error ERC20InsufficientBalance(address sender, uint256 balance, uint256 needed)",
    { type: "function", name: "name", stateMutability: "view", inputs: [], outputs: [{ type: "string" }] }
])
```

### Cú pháp hỗ trợ

| Loại | Ví dụ |
|---|---|
| Function (view) | `"function balanceOf(address) view returns (uint256)"` |
| Function (write) | `"function transfer(address, uint256) returns (bool)"` |
| Function (payable) | `"function deposit() payable"` |
| Event | `"event Transfer(address indexed from, address indexed to, uint256 value)"` |
| Event (anonymous) | `"event Sync(uint112 reserve0, uint112 reserve1) anonymous"` |
| Error | `"error InsufficientBalance(address, uint256)"` |
| Constructor | `"constructor(address _token) payable"` |
| Receive | `"receive() external payable"` |
| Fallback | `"fallback() external"` |
| Tuple params | `"function mint((address recipient, uint256 amount, uint24 fee) params) returns (uint256)"` |
| Array params | `"function foo(uint256[] amounts, address[3] addrs)"` |
| Named tuple | `"function exactInputSingle((address tokenIn, address tokenOut, uint24 fee, address recipient, uint256 deadline, uint256 amountIn, uint256 amountOutMinimum, uint160 sqrtPriceLimitX96) params)"` |

---

## Unit conversion

```js
import { formatUnits, parseUnits, formatEther, parseEther } from "@akaoio/zen/lib/chains/evm.js"

// formatUnits(bigint, decimals) → string
formatUnits(1500000n, 6)           // → "1.5"      (USDC: 6 decimals)
formatUnits(10n ** 18n, "ether")   // → "1.0"
formatUnits(9n, 9)                 // → "0.000000009"
formatUnits(0n, 6)                 // → "0.0"

// parseUnits(string, decimals) → bigint
parseUnits("1.5", 6)              // → 1500000n
parseUnits("50", "gwei")          // → 50000000000n
parseUnits("1", "ether")          // → 1000000000000000000n

// Aliases cho ether
formatEther(10n ** 18n)            // → "1.0"
parseEther("1.5")                  // → 1500000000000000000n
```

Tên decimals hỗ trợ: `wei` (0), `kwei` (3), `mwei` (6), `gwei` (9), `szabo` (12), `finney` (15), `ether` (18).

---

## solidityPacked / solidityPackedKeccak256

Tương đương `abi.encodePacked` trong Solidity — raw bytes, **không** có padding.

```js
import { solidityPacked, solidityPackedKeccak256 } from "@akaoio/zen/lib/chains/evm.js"

// Uniswap V3 path encoding (thủ công, không cần encodePath helper)
const packed = solidityPacked(
    ["address", "uint24", "address"],
    ["0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2", 3000, "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"]
)
// → "0xC02aaA39b...000BB8A0b86..." (43 bytes hex)

// Hash của packed (async — dùng ZEN.hash)
const hash = await solidityPackedKeccak256(
    ["address", "uint24", "address"],
    [WETH, 3000, USDC]
)
```

### Kiểu dữ liệu hỗ trợ

| Solidity type | Kích thước | Notes |
|---|---|---|
| `address` | 20 bytes | Không padding |
| `bool` | 1 byte | `0x00` hoặc `0x01` |
| `uint8`..`uint256` | 1..32 bytes | Big-endian, kích thước = bits/8 |
| `int8`..`int256` | 1..32 bytes | Two's complement |
| `bytes1`..`bytes32` | 1..32 bytes | Left-aligned, right-padded với zero |
| `bytes` | N bytes | Raw, không length prefix |
| `string` | N bytes | UTF-8, không length prefix |

---

## getCreate2Address

Tính địa chỉ contract được deploy bằng CREATE2 offline — không cần RPC call.

```js
import { getCreate2Address } from "@akaoio/zen/lib/chains/evm.js"

// Uniswap V2 pair address
const pairAddress = await getCreate2Address(
    "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f",  // V2 Factory
    "0xb67c9dbc52ef98b6cde15a39e6efa0b7ab050f61...", // salt = keccak256(token0 ++ token1)
    "0x96e8ac4277198ff8b6f785478aa9a39f403cb768dd02cbee326c3e7da348845f"  // initCodeHash
)
// → "0xB4e16d0168e52d35CaCD2c6185b44281Ec28C9Dc" (USDC/WETH V2 pair)

// salt có thể là hex string (32 bytes) hoặc BigInt
const addr = await getCreate2Address(factory, saltBigInt, initCodeHash)
```

**Formula:** `keccak256(0xff ++ factory(20 bytes) ++ salt(32 bytes) ++ initCodeHash(32 bytes))`, lấy 20 bytes cuối.

---

## Multicall

Batch nhiều read calls thành 1 RPC request bằng [Multicall3](https://www.multicall3.com/) (`0xcA11bde05977b3631167028862bE2a173976CA11`).

```js
const results = await provider.multicall([
    {
        address: USDC_ADDRESS,
        abi: ["function balanceOf(address) view returns (uint256)"],
        method: "balanceOf",
        args: [addr1],
    },
    {
        address: USDC_ADDRESS,
        abi: ["function balanceOf(address) view returns (uint256)"],
        method: "balanceOf",
        args: [addr2],
    },
    {
        address: WETH_ADDRESS,
        abi: ["function totalSupply() view returns (uint256)"],
        method: "totalSupply",
        args: [],
        allowFailure: true,  // không fail cả batch nếu call này lỗi
    },
])
// results[0] → BigInt  (USDC balance of addr1)
// results[1] → BigInt  (USDC balance of addr2)
// results[2] → BigInt | null  (WETH totalSupply, null nếu call fail)
```

**Lợi ích so với ethers.js:**
- ethers.js không có built-in multicall — cần package ngoài
- zen's multicall là first-class method của Provider
- Tự động dùng `aggregate3` (allowFailure per call) thay vì `aggregate` cũ

**Multicall3 được deploy tại cùng địa chỉ trên:**
Ethereum mainnet, Goerli, Sepolia, Polygon, Arbitrum, Optimism, Base, BSC, Avalanche, và hầu hết EVM chains.

---

## encodePath (Uniswap V3 multi-hop)

```js
import { encodePath } from "@akaoio/zen/lib/chains/evm.js"

// Single hop: WETH → USDC qua pool 0.3%
const path = encodePath([WETH, USDC], [3000])
// → "0xC02aaA39b...000BB8A0b..." (43 bytes)

// Multi-hop: WETH → USDT → USDC
const path = encodePath([WETH, USDT, USDC], [3000, 500])

// exactOutput: path phải NGƯỢC — tokenOut trước
const reversePath = encodePath([WETH, USDC], [3000])  // tokenOut=WETH, tokenIn=USDC
```

Format: `token0 (20 bytes) + fee0 (3 bytes, uint24) + token1 (20 bytes) + fee1 + ...`

---

## ABI Encoder / Decoder

Dùng nội bộ bởi `Contract`, nhưng có thể gọi trực tiếp:

```js
import { buildCalldata, buildSig, decodeReturnData, encodeParams } from "@akaoio/zen/lib/chains/evm.js"

// Build function selector + encoded args
const calldata = await buildCalldata(
    "transfer(address,uint256)",
    [{ name: "to", type: "address" }, { name: "amount", type: "uint256" }],
    ["0xRecipient...", 1000000n]
)
// → "0xa9059cbb000000...000000..."

// Build function signature string từ ABI item
const sig = buildSig({ name: "transfer", inputs: [{ type: "address" }, { type: "uint256" }] })
// → "transfer(address,uint256)"

// Decode return data
const [value] = decodeReturnData([{ name: "", type: "uint256" }], "0x0000...0001")
// → 1n
```

### Kiểu ABI được hỗ trợ

| Solidity type | Encoding |
|---|---|
| `uint8`..`uint256`, `int8`..`int256` | 32-byte slot, big-endian |
| `address` | 32-byte slot, right-aligned 20 bytes |
| `bool` | 32-byte slot, `0` or `1` |
| `bytes1`..`bytes32` | 32-byte slot, left-aligned |
| `bytes` | dynamic: offset → length → data (padded to 32) |
| `string` | dynamic: offset → length → UTF-8 bytes (padded to 32) |
| `T[]` | dynamic: offset → count → T0, T1, ... |
| `T[N]` | static if T static: N slots inline |
| `tuple(T1,T2,...)` | static tuple: components inline; dynamic tuple: offset → data |

**Dynamic tuple** (tuple chứa `bytes`/`string`/dynamic array) được encode đúng cách với offset pointer — đây là bug phổ biến trong các ABI encoder tự làm.

---

## EIP-712 TypedDataEncoder

Hỗ trợ signing structured data (EIP-712) — dùng cho Uniswap Permit2, DAI permit, OpenSea orders, v.v.

```js
import { hashStruct, hashDomain, hashTypedData, encodeTypedData, recoverTypedDataAddress }
    from "@akaoio/zen/lib/chains/evm.js"

const domain = {
    name: "Ether Mail",
    version: "1",
    chainId: 1,
    verifyingContract: "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC",
}

const types = {
    Mail: [
        { name: "from",     type: "Person"  },
        { name: "to",       type: "Person"  },
        { name: "contents", type: "string"  },
    ],
    Person: [
        { name: "name",   type: "string"  },
        { name: "wallet", type: "address" },
    ],
}

const value = {
    from:     { name: "Cow", wallet: "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826" },
    to:       { name: "Bob", wallet: "0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB" },
    contents: "Hello, Bob!",
}

// Hash từng phần
const domainHash = await hashDomain(domain)
const structHash  = await hashStruct("Mail", types, value)

// Hash cuối cùng để ký (= "\x19\x01" + domainHash + structHash)
const digest = await hashTypedData(domain, types, value)
// Khớp với ethers.TypedDataEncoder.hash(domain, types, value) ✓

// Ký với zen
const pair = await ZEN.pair(null, { priv: privHex, format: "evm" })
const sig = await ZEN.sign(digest, pair, null, { prehash: true, encode: "raw" })

// Recover signer address từ signature
const signer = await recoverTypedDataAddress(domain, types, value, sig)
// → "0xCow..." (địa chỉ của Cow)
```

### Quy tắc encode trong hashStruct

| Solidity type | EIP-712 encoding |
|---|---|
| `uint/int/bool/address/bytesN` | 32-byte ABI-encoded slot |
| `string` | `keccak256(utf8 bytes)` |
| `bytes` | `keccak256(raw bytes)` |
| `T[]` hoặc `T[N]` | `keccak256(enc(T0) ++ enc(T1) ++ ...)` |
| Nested struct | `hashStruct(type, types, value)` (recursive) |

### primaryType auto-detection

Khi không truyền `primaryType`, hàm tự detect bằng cách tìm type không được referenced bởi type nào khác:

```js
// "Mail" references "Person", "Person" không references ai → primaryType = "Mail"
const digest = await hashTypedData(domain, types, value)  // tự detect "Mail"
const digest = await hashTypedData(domain, types, value, "Mail")  // explicit
```

---

## RLP Encoder

```js
import { rlpEncode } from "@akaoio/zen/lib/chains/evm.js"

// EIP-155 raw transaction
const encoded = rlpEncode([nonce, gasPrice, gasLimit, to, value, data, chainId, 0n, 0n])
```

Dùng nội bộ bởi `signTransaction`. Hỗ trợ BigInt, Uint8Array, và mảng nested.

---

## signTransaction (low-level)

```js
import { signTransaction } from "@akaoio/zen/lib/chains/evm.js"

const rawHex = await signTransaction({
    to:       "0xRecipient...",
    value:    parseEther("0.1"),
    data:     "0x",
    nonce:    5n,
    gasPrice: parseUnits("20", "gwei"),
    gasLimit: 21000n,
}, privKey, chainId)
// → "0x02..." (signed EIP-155 raw transaction)
```

---

## Event decoding

```js
import { buildEventTopicMap, decodeEventLog, decodeReceiptEvents } from "@akaoio/zen/lib/chains/evm.js"

// Build topic0 map từ ABI
const topicMap = await buildEventTopicMap(ABI)

// Decode 1 log
const values = decodeEventLog(TRANSFER_EVENT_ABI, log)
// → { from: "0x...", to: "0x...", value: 1000000n }

// Decode tất cả events trong receipt
const events = await decodeReceiptEvents(ABI, receipt)
// → { Transfer: { returnValues: { from, to, value }, raw: log }, ... }
```

---

## Ethers-compatible connector shim

Cho `akao` dùng để drop-in replace ethers:

```js
import evm from "@akaoio/zen/lib/chains/evm.js"
const connector = evm.connector

// connector tương thích với ethers:
connector.JsonRpcProvider   === Provider
connector.WebSocketProvider === WsProvider
connector.Wallet            === Wallet
connector.Contract          === Contract
connector.isAddress         === isAddress
```

Trong `akao/src/EVM.js`:
```js
// Trước
import { ethers } from "../Ethers.js"
const chain = new Chain({ connector: ethers, ... })

// Sau
import evm from "@akaoio/zen/lib/chains/evm.js"
const chain = new Chain({ connector: evm.connector, ... })
```

---

## Migration guide từ ethers.js

### Provider

```js
// ethers
const provider = new ethers.JsonRpcProvider(url)
const balance = await provider.getBalance(addr)

// zen
const provider = rpc(url)
const balance = await provider.getBalance(addr)  // ← identical API
```

### Wallet

```js
// ethers
const wallet = new ethers.Wallet(privKey, provider)

// zen
const wallet = await Wallet.create(privKey, provider)
// Chú ý: Wallet.create là async (address derived từ ZEN.pair)
// hoặc:
const wallet = new Wallet(privKey, provider)
await wallet._ready  // wait for address
```

### Contract

```js
// ethers
const contract = new ethers.Contract(address, ABI, provider)
const balance = await contract.balanceOf(addr)

// zen — identical
const contract = new Contract(address, ABI, provider)
const balance = await contract.balanceOf(addr)
```

### ABI strings

```js
// ethers — human-readable ABI
const ABI = ["function transfer(address to, uint256 amount) returns (bool)"]
const contract = new ethers.Contract(address, ABI, provider)

// zen — identical (parseAbi được gọi tự động trong Contract constructor)
const contract = new Contract(address, ABI, provider)
```

### formatUnits / parseUnits

```js
// ethers
ethers.formatUnits(1500000n, 6)   // → "1.5"
ethers.parseUnits("1.5", 6)        // → 1500000n

// zen — identical API
formatUnits(1500000n, 6)           // → "1.5"
parseUnits("1.5", 6)               // → 1500000n
```

### Điều cần lưu ý khi migration

| Thay đổi | ethers | zen |
|---|---|---|
| `contract.interface.encodeFunctionData` | sync | **async** (keccak là async) |
| `new Wallet(priv)` | `.address` available instantly | await `wallet._ready` hoặc dùng `Wallet.create` |
| `provider.getBlock()` | block bình thường | block có thêm `.date` |
| Revert detection | không throw | `wait()` **throw** nếu status=0x0 |
| `queryFilter` | Trả `EventLog[]` | Trả `{ ...decoded, _log, blockNumber, txHash }[]` |
| `solidityPacked` | `ethers.solidityPacked(...)` | `solidityPacked(...)` (sync, không cần ethers prefix) |
| `TypedDataEncoder` | `new TypedDataEncoder(types)` class | `hashTypedData(domain, types, value)` function |

---

## Test coverage

| Suite | Số test | Chạy bằng |
|---|---|---|
| Unit (evm.js) | 76 | `npm run test:chains` |
| Fork integration | 38 | `ETH_RPC=... npm run test:chains:fork` |

Fork tests dùng ganache fork mainnet tại block 19,500,000 qua Infura, test đầy đủ:
- Provider primitives (getBalance, getCode, getBlock, estimateGas)
- ETH transfer, WETH wrap/unwrap
- ERC20 transfer, transferFrom, Transfer event
- Uniswap V2 pool reads, swap quote, swaps (3 variants), liquidity
- Uniswap V3 QuoterV2, pool reads, exactInputSingle, exactInput (bytes path), exactOutput
- V3 PositionManager round-trip: mint → positions → increaseLiquidity → decreaseLiquidity → collect → burn
- Multicall3 (3 calls in 1 RPC)
- queryFilter (Transfer events in block range)

---

## Kiến trúc nội bộ — ZEN primitives

```
lib/chains/evm.js
    ↓ uses only
zen.js (bundled)
    ↓ wraps
crypto.wasm (Zig-compiled)
    ├── secp256k1 (keypair, ECDSA sign)
    └── keccak256 (hashing)
```

Mọi crypto operation đi qua:

```js
// Keccak256 hash (bytes input)
const hex = await ZEN.hash(bytes, null, null, { name: "keccak256", encode: "hex", input: "raw" })

// Keypair từ private key
const pair = await ZEN.pair(null, { priv: "0x...", format: "evm" })
// pair.address → EIP-55 checksum address
// pair.priv    → hex private key

// ECDSA signature (prehashed input)
const sig = await ZEN.sign("0x" + hashHex, pair, null, { prehash: true, encode: "raw" })
// sig.r, sig.s, sig.v (recovery bit)
```

`lib/chains/evm.js` không bao giờ import `src/keccak256.js`, `src/curves/secp256k1.js`, hay bất kỳ internal nào. Đây là **contract kiến trúc quan trọng** — đảm bảo rằng:
1. Khi zen.js được nâng cấp (kể cả WASM), chains/evm.js tự động được hưởng lợi
2. Người dùng chỉ cần bundle `zen.js` + `lib/chains/evm.js` — không có transitive deps
3. ABI encoder và crypto layer hoàn toàn độc lập — dễ test và debug riêng

---

## See also

- `lib/chains.js` — re-export entry point: `import chains from "@akaoio/zen/lib/chains.js"`
- `test/chains/evm.js` — 76 unit tests
- `test/chains/evm-fork.js` — 38 mainnet fork integration tests
- `docs/ch03-crypto.md` — ZEN crypto primitives (`pair`, `sign`, `hash`, v.v.)
- GitHub Issue #27 — `ethers-replacement` branch tracking
