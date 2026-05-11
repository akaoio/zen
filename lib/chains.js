// lib/chains.js — Multi-chain adapter entry point for ZEN
//
// Standalone module — NOT bundled into zen.js.
// Import directly: import chains from "@akaoio/zen/lib/chains.js"
//
// Currently supports EVM-compatible chains (Ethereum, Polygon, BSC, etc.).
// Future adapters (Bitcoin, Solana, …) can be added as lib/chains/{name}.js.

export { default as evm } from "./chains/evm.js"

// Named re-exports for direct access
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
} from "./chains/evm.js"

import evm from "./chains/evm.js"
export default { evm }
