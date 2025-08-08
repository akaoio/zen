#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load manifest
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

console.log('Updating MANIFEST.json: converting init_* functions to *_new pattern...\n');

let updateCount = 0;

// Process each file in the manifest
for (const [filePath, fileInfo] of Object.entries(manifest.files)) {
    if (fileInfo.functions) {
        for (const func of fileInfo.functions) {
            // Check if function name starts with init_
            if (func.name.startsWith('init_')) {
                const oldName = func.name;
                const oldSignature = func.signature;
                
                // Extract the part after init_
                const baseName = func.name.substring(5); // Remove 'init_'
                const newName = baseName + '_new';
                
                // Update function name
                func.name = newName;
                
                // Update signature - replace init_* with *_new
                func.signature = func.signature.replace(/\binit_(\w+)/, '$1_new');
                
                console.log(`${filePath}:`);
                console.log(`  ${oldName} → ${newName}`);
                console.log(`  ${oldSignature} → ${func.signature}\n`);
                
                updateCount++;
            }
        }
    }
}

// Also update function declarations in headers
for (const [headerPath, headerInfo] of Object.entries(manifest.headers)) {
    if (headerInfo.function_declarations) {
        for (const func of headerInfo.function_declarations) {
            if (func.name.startsWith('init_')) {
                const oldName = func.name;
                const oldSignature = func.signature;
                
                // Extract the part after init_
                const baseName = func.name.substring(5);
                const newName = baseName + '_new';
                
                // Update function name
                func.name = newName;
                
                // Update signature
                func.signature = func.signature.replace(/\binit_(\w+)/, '$1_new');
                
                console.log(`${headerPath} (header):`);
                console.log(`  ${oldName} → ${newName}`);
                console.log(`  ${oldSignature} → ${func.signature}\n`);
                
                updateCount++;
            }
        }
    }
}

// Write updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log(`\nUpdated ${updateCount} function references from init_* to *_new pattern.`);
console.log('MANIFEST.json has been updated successfully!');