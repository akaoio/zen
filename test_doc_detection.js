#!/usr/bin/env node

const fs = require('fs');

function checkDocumentation(content, funcName, signature) {
    const lines = content.split('\n');
    let hasValidDoc = false;
    for (let i = 0; i < lines.length; i++) {
        if (lines[i].includes(funcName + '(')) {
            console.log(`Found function declaration at line ${i+1}: ${lines[i].trim()}`);
            // Look for doxygen comment above
            for (let j = i - 1; j >= 0; j--) {
                if (lines[j].trim().endsWith('*/')) {
                    const docBlock = [];
                    for (let k = j; k >= 0; k--) {
                        docBlock.unshift(lines[k]);
                        if (lines[k].trim().startsWith('/**')) break;
                    }
                    const docText = docBlock.join('\n');
                    console.log(`Found doc block:\n${docText}`);
                    
                    const hasBrief = docText.includes('@brief');
                    const hasParam = docText.includes('@param') || signature.includes('(void)');
                    const hasReturn = docText.includes('@return') || signature.startsWith('void ');
                    
                    console.log(`Brief: ${hasBrief}, Param: ${hasParam}, Return: ${hasReturn}`);
                    hasValidDoc = hasBrief && hasParam && hasReturn;
                    break;
                }
                if (!lines[j].trim().startsWith('*') && lines[j].trim()) break;
            }
            break;
        }
    }
    return hasValidDoc;
}

const content = fs.readFileSync('src/core/memory.c', 'utf8');
const result = checkDocumentation(content, 'memory_debug_alloc', 'void* memory_debug_alloc(size_t size, const char* file, int line)');
console.log(`Has valid doc: ${result}`);