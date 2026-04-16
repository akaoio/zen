/**
 * Test for ZEN.wallet derivation
 * Run with: node test/zen/test-wallet-zen.js
 */

import ZEN from '../../zen.js';
import '../../lib/wallet-zen.js';

// Injection per ESM dove non esiste l'oggetto global condiviso implicitamente, bind manuale per i moduli lib
if (!ZEN.wallet && typeof globalThis !== 'undefined' && globalThis.Gun && globalThis.Gun.ZEN) {
  // Try to use attached functions manually if necessary
}

async function runTests() {
  console.log('=== Testing ZEN.wallet ===\n');
  
  let passed = 0;
  let failed = 0;
  
  const assert = (condition, msg) => {
    if (condition) {
      console.log('  ✓ PASS:', msg);
      passed++;
    } else {
      console.log('  ✗ FAIL:', msg);
      failed++;
    }
  };

  try {
    // Siccome lib/wallet-zen.js in ESM non espone module.exports e riceve ZEN dal global object,
    // garantiamo che l'attachement a ZEN funzioni iniettandolo direttamente:
    if (!ZEN.wallet) {
      console.log("ZEN.wallet is loosely attached, fallback mapping..");
      // wallet-zen attacca la funzione a window.ZEN o global.ZEN
      // quindi ZEN importato qui = riferimento all'istanza.
    }

    console.log('Test 1: Wallet generation returns a valid hex string');
    const pair = await ZEN.pair();
    const pk = await ZEN.wallet(pair);
    assert(typeof pk === 'string' && pk.startsWith('0x') && pk.length === 66, 'Should return a 66-character hex string starting with 0x');

    console.log('\nTest 2: Deterministic generation');
    const pk2 = await ZEN.wallet(pair);
    assert(pk === pk2, 'Same pair should generate the exact same private key');

    console.log('\nTest 3: Different ID generates different keys');
    const pk3 = await ZEN.wallet(pair, 'wallet-2');
    assert(pk !== pk3, 'Different IDs should generate different private keys');

    console.log('\nTest 4: Requires pair with priv');
    try {
      await ZEN.wallet({ pub: 'only_pub' });
      assert(false, 'Should throw if priv is missing');
    } catch(e) {
      assert(e.message.includes('priv'), 'Properly threw error for missing priv');
    }

  } catch(e) {
    console.log('  ✗ FAIL: Unexpected error:', e);
    failed++;
  }
  
  // Summary
  console.log('\n=== Results ===');
  console.log(`Passed: ${passed}/${passed + failed}`);
  console.log(`Failed: ${failed}/${passed + failed}`);
  
  process.exit(failed > 0 ? 1 : 0);
}

// Inizializza global.ZEN prima di far valutare il js
global.ZEN = ZEN;

// Ora che global.ZEN esiste importiamo dinamicamente in modo che il modulo lib trovi ZEN
import('../../lib/wallet-zen.js').then(() => {
  if (!ZEN.wallet && typeof globalThis !== 'undefined' && globalThis.zenWalletDerive) {
    ZEN.wallet = globalThis.zenWalletDerive;
  }
  runTests().catch(console.error);
});
