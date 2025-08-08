#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load manifest
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

console.log('Fixing all TODO descriptions in MANIFEST.json...\n');

let todoCount = 0;
let fixedCount = 0;

// Function to generate description based on function name
function generateDescription(funcName, signature) {
    // Common patterns
    if (funcName.endsWith('_new')) {
        const type = funcName.slice(0, -4);
        return `Create new ${type} instance`;
    }
    if (funcName.endsWith('_free')) {
        const type = funcName.slice(0, -5);
        return `Free ${type} instance and cleanup resources`;
    }
    if (funcName.endsWith('_init')) {
        const type = funcName.slice(0, -5);
        return `Initialize ${type} system`;
    }
    if (funcName.endsWith('_cleanup')) {
        const type = funcName.slice(0, -8);
        return `Clean up ${type} system`;
    }
    if (funcName.startsWith('parse_')) {
        const what = funcName.slice(6).replace(/_/g, ' ');
        return `Parse ${what}`;
    }
    if (funcName.startsWith('visit_')) {
        const what = funcName.slice(6).replace(/_/g, ' ');
        return `Visit ${what} node`;
    }
    if (funcName.startsWith('ast_new_')) {
        const what = funcName.slice(8).replace(/_/g, ' ');
        return `Create new ${what} AST node`;
    }
    if (funcName.startsWith('array_')) {
        const what = funcName.slice(6).replace(/_/g, ' ');
        return `Array ${what} operation`;
    }
    if (funcName.startsWith('object_')) {
        const what = funcName.slice(7).replace(/_/g, ' ');
        return `Object ${what} operation`;
    }
    if (funcName.startsWith('set_')) {
        const what = funcName.slice(4).replace(/_/g, ' ');
        return `Set ${what} operation`;
    }
    if (funcName.startsWith('priority_queue_')) {
        const what = funcName.slice(15).replace(/_/g, ' ');
        return `Priority queue ${what} operation`;
    }
    if (funcName.includes('_stdlib')) {
        const base = funcName.replace('_stdlib', '');
        return `Standard library wrapper for ${base}`;
    }
    if (funcName.includes('_internal')) {
        const base = funcName.replace('_internal', '');
        return `Internal implementation of ${base}`;
    }
    
    // Default: Convert snake_case to readable description
    return funcName.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
}

// Process files
for (const [filePath, fileInfo] of Object.entries(manifest.files)) {
    if (fileInfo.description && fileInfo.description.includes("TODO")) {
        todoCount++;
        // Generate description based on file path
        const fileName = path.basename(filePath, '.c');
        const dirName = path.basename(path.dirname(filePath));
        
        if (dirName === 'core') {
            fileInfo.description = `Core ${fileName} implementation`;
        } else if (dirName === 'types') {
            fileInfo.description = `${fileName.charAt(0).toUpperCase() + fileName.slice(1)} type implementation`;
        } else if (dirName === 'stdlib') {
            fileInfo.description = `Standard library ${fileName} functions`;
        } else if (dirName === 'runtime') {
            fileInfo.description = `Runtime ${fileName} implementation`;
        } else {
            fileInfo.description = `${fileName.charAt(0).toUpperCase() + fileName.slice(1)} implementation`;
        }
        
        console.log(`Fixed file description: ${filePath}`);
        fixedCount++;
    }
    
    // Process functions in file
    if (fileInfo.functions) {
        for (const func of fileInfo.functions) {
            if (func.description && func.description.includes("TODO")) {
                todoCount++;
                func.description = generateDescription(func.name, func.signature);
                console.log(`  Fixed function: ${func.name} -> "${func.description}"`);
                fixedCount++;
            }
        }
    }
}

// Process headers
for (const [headerPath, headerInfo] of Object.entries(manifest.headers)) {
    if (headerInfo.description && headerInfo.description.includes("TODO")) {
        todoCount++;
        // Generate description based on header path
        const fileName = path.basename(headerPath, '.h');
        const dirName = path.basename(path.dirname(headerPath));
        
        if (dirName === 'core') {
            headerInfo.description = `Core ${fileName} declarations`;
        } else if (dirName === 'types') {
            headerInfo.description = `${fileName.charAt(0).toUpperCase() + fileName.slice(1)} type declarations`;
        } else if (dirName === 'stdlib') {
            headerInfo.description = `Standard library ${fileName} declarations`;
        } else if (dirName === 'runtime') {
            headerInfo.description = `Runtime ${fileName} declarations`;
        } else {
            headerInfo.description = `${fileName.charAt(0).toUpperCase() + fileName.slice(1)} declarations`;
        }
        
        console.log(`Fixed header description: ${headerPath}`);
        fixedCount++;
    }
}

// Write updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log(`\nFixed ${fixedCount} out of ${todoCount} TODO descriptions.`);
console.log('MANIFEST.json updated successfully!');