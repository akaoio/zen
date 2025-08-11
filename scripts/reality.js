#!/usr/bin/env node

/**
 * reality.js - Real Project Structure Analyzer
 * Shows actual files and functions in the ZEN codebase without relying on MANIFEST.json
 * Displays a tree-like structure similar to vision.js but based on actual file content
 */

const fs = require('fs');
const path = require('path');

// ANSI Color codes
const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    
    // Regular colors
    black: '\x1b[30m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    magenta: '\x1b[35m',
    cyan: '\x1b[36m',
    white: '\x1b[37m',
    
    // Background colors
    bgBlack: '\x1b[40m',
    bgRed: '\x1b[41m',
    bgGreen: '\x1b[42m',
    bgYellow: '\x1b[43m',
    bgBlue: '\x1b[44m',
    bgMagenta: '\x1b[45m',
    bgCyan: '\x1b[46m',
    bgWhite: '\x1b[47m'
};

/**
 * Extract function definitions from C source file
 */
function extractFunctions(filePath) {
    if (!fs.existsSync(filePath)) {
        return [];
    }
    
    const content = fs.readFileSync(filePath, 'utf8');
    const functions = [];
    const lines = content.split('\n');
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i].trim();
        
        // Skip empty lines, comments, preprocessor directives, and control flow
        if (!line || line.startsWith('//') || line.startsWith('/*') || line.startsWith('*') || 
            line.startsWith('#') || line.startsWith('}') || line.startsWith('if (') ||
            line.startsWith('else if') || line.startsWith('else ') || line.startsWith('while (') ||
            line.startsWith('for (') || line.startsWith('switch (') || line.startsWith('case ') ||
            line.startsWith('default:') || line.startsWith('break;') || line.startsWith('continue;') ||
            line.startsWith('return ')) {
            continue;
        }
        
        // Check if this looks like a function definition
        // Pattern: [static] ReturnType [*] function_name(...
        const funcMatch = line.match(/^(static\s+)?(\w+(?:\s*\*)*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/) || 
                         line.match(/^(static\s+)?(\w+)\s+\*\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/);
        
        if (funcMatch) {
            const isStatic = !!funcMatch[1];
            let returnType = funcMatch[2].trim();
            const funcName = funcMatch[3];
            
            // Handle cases where pointer is separate (like "RuntimeValue *")
            if (line.includes(' *' + funcName)) {
                returnType += ' *';
            }
            
            // Skip common non-function patterns and keywords
            if (returnType === 'if' || returnType === 'while' || returnType === 'for' || 
                returnType === 'switch' || returnType === 'sizeof' || returnType === 'return' ||
                returnType === 'else' || funcName === 'main' || funcName === 'if' || 
                line.includes('else if') || line.includes('} else') || line.includes('else {')) {
                continue;
            }
            
            // Look ahead to find the opening brace (within a few lines)
            let foundBrace = false;
            for (let j = i; j < Math.min(i + 5, lines.length); j++) {
                const checkLine = lines[j].trim();
                if (checkLine.includes('{')) {
                    foundBrace = true;
                    break;
                }
                // If we hit another function-like pattern or certain keywords, stop looking
                if (j > i && (checkLine.match(/^\w+\s+\w+\s*\(/) || checkLine.startsWith('typedef') || 
                    checkLine.startsWith('struct') || checkLine.startsWith('enum'))) {
                    break;
                }
            }
            
            if (foundBrace) {
                functions.push({
                    name: funcName,
                    returnType: returnType,
                    line: i + 1,
                    isStatic: isStatic
                });
            }
        }
    }
    
    return functions.sort((a, b) => a.line - b.line);
}

/**
 * Get file size and lines of code
 */
function getFileStats(filePath) {
    if (!fs.existsSync(filePath)) {
        return { size: 0, lines: 0 };
    }
    
    const stats = fs.statSync(filePath);
    const content = fs.readFileSync(filePath, 'utf8');
    const lines = content.split('\n').length;
    
    return {
        size: stats.size,
        lines: lines
    };
}

/**
 * Scan directory structure
 */
function scanDirectory(dirPath, basePath = '') {
    const items = [];
    
    if (!fs.existsSync(dirPath)) {
        return items;
    }
    
    const entries = fs.readdirSync(dirPath, { withFileTypes: true });
    
    for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);
        const relativePath = path.join(basePath, entry.name);
        
        if (entry.isDirectory()) {
            const subItems = scanDirectory(fullPath, relativePath);
            items.push({
                name: entry.name,
                type: 'directory',
                path: relativePath,
                children: subItems,
                count: subItems.length
            });
        } else if (entry.isFile() && (entry.name.endsWith('.c') || entry.name.endsWith('.h'))) {
            const functions = extractFunctions(fullPath);
            const stats = getFileStats(fullPath);
            
            items.push({
                name: entry.name,
                type: 'file',
                path: relativePath,
                functions: functions,
                stats: stats,
                extension: path.extname(entry.name)
            });
        }
    }
    
    return items.sort((a, b) => {
        if (a.type !== b.type) {
            return a.type === 'directory' ? -1 : 1;
        }
        return a.name.localeCompare(b.name);
    });
}

/**
 * Format file size
 */
function formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 10) / 10 + ' ' + sizes[i];
}

/**
 * Print tree structure
 */
function printTree(items, prefix = '', isLast = true, depth = 0) {
    for (let i = 0; i < items.length; i++) {
        const item = items[i];
        const isLastItem = i === items.length - 1;
        const connector = isLastItem ? '└── ' : '├── ';
        const nextPrefix = prefix + (isLastItem ? '    ' : '│   ');
        
        if (item.type === 'directory') {
            console.log(`${prefix}${connector}${colors.blue}${colors.bright}${item.name}/${colors.reset} ${colors.dim}(${item.count} items)${colors.reset}`);
            if (item.children && item.children.length > 0) {
                printTree(item.children, nextPrefix, isLastItem, depth + 1);
            }
        } else if (item.type === 'file') {
            const fileIcon = item.extension === '.h' ? '📋' : '📝';
            const sizeInfo = `${colors.dim}${formatSize(item.stats.size)} ${item.stats.lines}L${colors.reset}`;
            
            console.log(`${prefix}${connector}${fileIcon} ${colors.cyan}${item.name}${colors.reset} ${sizeInfo}`);
            
            // Show functions
            if (item.functions && item.functions.length > 0) {
                for (let j = 0; j < item.functions.length; j++) {
                    const func = item.functions[j];
                    const isFuncLast = j === item.functions.length - 1;
                    const funcConnector = isFuncLast ? '└── ' : '├── ';
                    const funcType = func.isStatic ? '🔒' : '🟢';
                    const returnTypeColor = func.returnType.includes('*') ? colors.yellow : colors.green;
                    
                    console.log(`${nextPrefix}${funcConnector}${funcType} ${returnTypeColor}${func.returnType}${colors.reset} ${colors.bright}${func.name}${colors.reset}() ${colors.dim}:${func.line}${colors.reset}`);
                }
            } else if (item.extension === '.c') {
                console.log(`${nextPrefix}└── ${colors.dim}no functions found${colors.reset}`);
            }
        }
    }
}

/**
 * Calculate project statistics
 */
function calculateStats(items) {
    let stats = {
        totalFiles: 0,
        totalFunctions: 0,
        totalLines: 0,
        totalSize: 0,
        cFiles: 0,
        hFiles: 0,
        staticFunctions: 0,
        publicFunctions: 0
    };
    
    function processItem(item) {
        if (item.type === 'file') {
            stats.totalFiles++;
            stats.totalLines += item.stats.lines;
            stats.totalSize += item.stats.size;
            
            if (item.extension === '.c') {
                stats.cFiles++;
            } else if (item.extension === '.h') {
                stats.hFiles++;
            }
            
            if (item.functions) {
                stats.totalFunctions += item.functions.length;
                item.functions.forEach(func => {
                    if (func.isStatic) {
                        stats.staticFunctions++;
                    } else {
                        stats.publicFunctions++;
                    }
                });
            }
        } else if (item.type === 'directory' && item.children) {
            item.children.forEach(processItem);
        }
    }
    
    items.forEach(processItem);
    return stats;
}

/**
 * Main function
 */
function main() {
    console.log(`${colors.bright}${colors.cyan}══════════════════════════════════════════════════════════${colors.reset}`);
    console.log(`${colors.bright}${colors.cyan}                    ZEN PROJECT REALITY                    ${colors.reset}`);
    console.log(`${colors.bright}${colors.cyan}══════════════════════════════════════════════════════════${colors.reset}`);
    console.log();
    
    const projectRoot = path.resolve(__dirname, '..');
    const srcPath = path.join(projectRoot, 'src');
    
    console.log(`${colors.bright}📂 Project Root:${colors.reset} ${projectRoot}`);
    console.log(`${colors.dim}Generated: ${new Date().toLocaleString()}${colors.reset}`);
    console.log();
    
    console.log(`${colors.bright}${colors.yellow}🌳 SOURCE STRUCTURE${colors.reset}`);
    console.log('─'.repeat(60));
    
    const srcItems = scanDirectory(srcPath, 'src');
    printTree(srcItems);
    
    console.log();
    console.log(`${colors.bright}${colors.yellow}📊 PROJECT STATISTICS${colors.reset}`);
    console.log('─'.repeat(60));
    
    const stats = calculateStats(srcItems);
    
    console.log(`${colors.bright}Files:${colors.reset}`);
    console.log(`  📝 C Files: ${colors.green}${stats.cFiles}${colors.reset}`);
    console.log(`  📋 Header Files: ${colors.cyan}${stats.hFiles}${colors.reset}`);
    console.log(`  📁 Total Files: ${colors.bright}${stats.totalFiles}${colors.reset}`);
    console.log();
    
    console.log(`${colors.bright}Functions:${colors.reset}`);
    console.log(`  🟢 Public Functions: ${colors.green}${stats.publicFunctions}${colors.reset}`);
    console.log(`  🔒 Static Functions: ${colors.yellow}${stats.staticFunctions}${colors.reset}`);
    console.log(`  🎯 Total Functions: ${colors.bright}${stats.totalFunctions}${colors.reset}`);
    console.log();
    
    console.log(`${colors.bright}Code Metrics:${colors.reset}`);
    console.log(`  📏 Total Lines: ${colors.bright}${stats.totalLines.toLocaleString()}${colors.reset}`);
    console.log(`  💾 Total Size: ${colors.bright}${formatSize(stats.totalSize)}${colors.reset}`);
    console.log(`  📊 Avg Functions/File: ${colors.bright}${(stats.totalFunctions / stats.cFiles).toFixed(1)}${colors.reset}`);
    
    console.log();
    console.log(`${colors.bright}${colors.green}✅ REALITY CHECK COMPLETE${colors.reset}`);
    console.log(`${colors.dim}This shows the ACTUAL codebase structure and functions${colors.reset}`);
    console.log(`${colors.dim}No reliance on MANIFEST.json - pure filesystem analysis${colors.reset}`);
    console.log();
}

// Run the main function
if (require.main === module) {
    main();
}

module.exports = { extractFunctions, scanDirectory, calculateStats };