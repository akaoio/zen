#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// Load existing manifest
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
        
        // Skip static functions
        if (content.substring(match.index - 100, match.index).includes('static')) {
            continue;
        }
        
        functions.push({
            name: funcName,
            signature: signature,
            line: content.substring(0, match.index).split('\n').length
        });
    }
    
    return functions;
}

// Get all C source files
const sourceFiles = execSync('find src -name "*.c" -type f', { encoding: 'utf8' })
    .trim()
    .split('\n')
    .filter(f => f);

console.log('Updating MANIFEST.json to match current codebase...\n');

// Process each source file
for (const filePath of sourceFiles) {
    const relativePath = filePath;
    
    if (!manifest.files[relativePath]) {
        // Add new file entry
        console.log(`Adding new file: ${relativePath}`);
        manifest.files[relativePath] = {
            description: "TODO: Add description",
            functions: []
        };
    }
    
    const extractedFunctions = extractFunctions(filePath);
    const fileEntry = manifest.files[relativePath];
    
    // Update existing functions and add new ones
    for (const extracted of extractedFunctions) {
        const existingFunc = fileEntry.functions.find(f => f.name === extracted.name);
        
        if (existingFunc) {
            // Update signature if different
            if (existingFunc.signature !== extracted.signature) {
                console.log(`  Updating ${extracted.name}: ${existingFunc.signature} → ${extracted.signature}`);
                existingFunc.signature = extracted.signature;
            }
        } else {
            // Add new function
            console.log(`  Adding new function: ${extracted.name}`);
            fileEntry.functions.push({
                name: extracted.name,
                signature: extracted.signature,
                description: "TODO: Add description",
                returns: extracted.signature.startsWith('void') ? undefined : extracted.signature.split(' ')[0] + '*'
            });
        }
    }
    
    // Remove functions that no longer exist
    const functionNames = extractedFunctions.map(f => f.name);
    fileEntry.functions = fileEntry.functions.filter(f => {
        if (!functionNames.includes(f.name)) {
            console.log(`  Removing deleted function: ${f.name}`);
            return false;
        }
        return true;
    });
}

// Sort files and functions for consistency
const sortedFiles = {};
Object.keys(manifest.files).sort().forEach(key => {
    sortedFiles[key] = manifest.files[key];
    if (sortedFiles[key].functions) {
        sortedFiles[key].functions.sort((a, b) => a.name.localeCompare(b.name));
    }
});
manifest.files = sortedFiles;

// Write updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log('\nMANIFEST.json has been updated successfully!');
console.log('\nNote: Please review and update the "TODO: Add description" placeholders.');