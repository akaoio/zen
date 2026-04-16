/*
 * Gun SEA Wallet Utility
 * Estrae una Private Key compatibile (seed a 32-byte) per wallet Ethereum 
 * a partire da un pair SEA di Gun. Non accorpa librerie esterne.
 */
;(function(){
    var Gun = (typeof window !== "undefined") ? window.Gun : require('../gun');
    var SEA = Gun.SEA || (typeof window !== "undefined" ? window.SEA : require('../sea'));

    // Funzione helper: hash SHA-256 della private key SEA (con id opzionale) per generare il seed/pk
    async function getSeed(priv, id) {
        var text = String(priv) + (id || '');
        var hashBuffer;
        
        if (typeof crypto !== "undefined" && crypto.subtle) {
            // Ambienti Browser: WebCrypto API
            var msgUint8 = new TextEncoder().encode(text);
            hashBuffer = await crypto.subtle.digest('SHA-256', msgUint8);
        } else {
            // Ambiente Node.js: modulo crypto nativo
            var cryptoLib = require('crypto');
            hashBuffer = cryptoLib.createHash('sha256').update(text).digest();
        }
        
        // Converte l'array di byte in stringa esadecimale compatibile "0x..."
        var hashArray = Array.from(new Uint8Array(hashBuffer));
        var hashHex = hashArray.map(function(b) {
            return b.toString(16).padStart(2, '0');
        }).join('');
        
        return "0x" + hashHex;
    }

    if (SEA) {
        /**
         * Metodo per derivare una Private Key Ethereum-compatibile da un pair SEA
         * @param {Object} pair - Il pair generato da Gun.SEA.pair()
         * @param {String} id - (Opzionale) alias o identificativo per derivare chiavi diverse dallo stesso pair
         * @returns {String} - Ritorna la private key in formato hex pronta per essere usata con ethers (es. "0x...")
         */
        SEA.wallet = async function(pair, id) {
            if (!pair || !pair.priv) {
                throw new Error("È richiesto un SEA pair valido, contenente la chiave 'priv'.");
            }
            
            // Genera l'hash SHA-256 (32 byte conformi alle PK di Ethereum) ricalcando la logica wallet deterministica
            var privateKey = await getSeed(pair.priv, id);
            
            return privateKey;
        };
    }

    if (typeof module !== "undefined" && module.exports) {
        module.exports = SEA ? SEA.wallet : undefined;
    }
}());
