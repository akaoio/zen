#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// Load existing manifest to preserve structure
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

// Function to extract function signatures from C files
function extractFunctions(filePath) {
    const content = fs.readFileSync(filePath, 'utf8');
    const functions = [];
    
    // Match function definitions (return_type function_name(params) {)
    const funcRegex = /^([A-Za-z_][A-Za-z0-9_]*\s*\*?\s*)([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*\{/gm;
    let match;
    
    while ((match = funcRegex.exec(content)) !== null) {
        const returnType = match[1].trim();
        const funcName = match[2];
        const params = match[3].trim();
        const signature = `${returnType} ${funcName}(${params})`;
        
        functions.push({
            name: funcName,
            signature: signature,
            line: content.substring(0, match.index).split('\n').length
        });
    }
    
    return functions;
}

// Function to update manifest functions
function updateManifestFunctions(manifest) {
    console.log('Updating MANIFEST.json with current function signatures...\n');
    
    for (const [filePath, fileInfo] of Object.entries(manifest.files)) {
        const fullPath = path.join(__dirname, '..', filePath);
        
        if (!fs.existsSync(fullPath)) {
            console.warn(`Warning: File ${filePath} not found, skipping...`);
            continue;
        }
        
        console.log(`Processing ${filePath}...`);
        const extractedFunctions = extractFunctions(fullPath);
        
        // Update existing functions with correct signatures
        if (fileInfo.functions) {
            for (const func of fileInfo.functions) {
                const found = extractedFunctions.find(f => f.name === func.name);
                if (found) {
                    const oldSig = func.signature;
                    func.signature = found.signature;
                    if (oldSig !== found.signature) {
                        console.log(`  Updated ${func.name}: ${oldSig} → ${found.signature}`);
                    }
                } else {
                    console.warn(`  Warning: Function ${func.name} not found in ${filePath}`);
                }
            }
            
            // Check for new functions not in manifest
            for (const extracted of extractedFunctions) {
                if (!fileInfo.functions.find(f => f.name === extracted.name)) {
                    console.log(`  New function found: ${extracted.name} at line ${extracted.line}`);
                    console.log(`    Consider adding to manifest: ${extracted.signature}`);
                }
            }
        }
    }
    
    return manifest;
}

// Main execution
try {
    const updatedManifest = updateManifestFunctions(manifest);
    
    // Write updated manifest
    fs.writeFileSync(manifestPath, JSON.stringify(updatedManifest, null, 2) + '\n');
    console.log('\nMANIFEST.json has been updated successfully!');
    
    // Run enforcement check
    console.log('\nRunning enforcement check...');
    try {
        execSync('node scripts/enforce_manifest.js', { stdio: 'inherit' });
    } catch (e) {
        console.error('Enforcement check failed. Please review the changes.');
    }
} catch (error) {
    console.error('Error updating manifest:', error.message);
    process.exit(1);
}