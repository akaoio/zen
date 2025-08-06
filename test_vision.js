#!/usr/bin/env node

const fs = require('fs');

function isStubImplementation(content, functionName) {
    const stubPatterns = [
        /\/\/\s*TODO/i,
        /\/\*\s*TODO/i,
        /return\s+NULL\s*;\s*\/\/\s*TODO/i,
        /return\s+0\s*;\s*\/\/\s*TODO/i,
        /return\s+false\s*;\s*\/\/\s*TODO/i,
        /printf\s*\(\s*["']Not implemented/i,
        /fprintf\s*\([^,]+,\s*["']Not implemented/i,
        /exit\s*\(\s*1\s*\)\s*;.*not implemented/i,
        /assert\s*\(\s*0\s*\)/,
        /abort\s*\(\s*\)/,
        /\{\s*\}/  // Empty function body
    ];

    const funcRegex = new RegExp(`${functionName}\\s*\\([^)]*\\)\\s*\\{([^}]*\\{[^}]*\\})*[^}]*\\}`, 'gs');
    const match = content.match(funcRegex);
    if (!match) return false;

    const functionBody = match[0];
    
    for (const pattern of stubPatterns) {
        if (pattern.test(functionBody)) {
            console.log(`Function ${functionName} matches stub pattern:`, pattern);
            return true;
        }
    }

    const bodyContent = functionBody.substring(functionBody.indexOf('{') + 1, functionBody.lastIndexOf('}'));
    const meaningfulLines = bodyContent.split('\n').filter(line => {
        const trimmed = line.trim();
        return trimmed && !trimmed.startsWith('//') && !trimmed.startsWith('/*');
    });

    console.log(`Function ${functionName} meaningful lines:`, meaningfulLines.length);
    if (meaningfulLines.length <= 1) {
        console.log(`Function ${functionName} is too short (${meaningfulLines.length} lines)`);
        return true;
    }

    return false;
}

const content = fs.readFileSync('src/core/memory.c', 'utf8');
const functions = ['memory_free', 'memory_debug_alloc', 'memory_debug_realloc', 'memory_debug_free', 'memory_debug_strdup'];

functions.forEach(func => {
    const result = isStubImplementation(content, func);
    console.log(`${func}: ${result ? 'STUB' : 'COMPLETE'}`);
});