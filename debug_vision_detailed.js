const fs = require('fs');
const path = require('path');

// Load manifest and function specs
const manifest = JSON.parse(fs.readFileSync('MANIFEST.json', 'utf8'));
const lexerFile = manifest.files['src/core/lexer.c'];
const func = lexerFile.functions.find(f => f.name === 'lexer_collect_string');

console.log('Function spec from manifest:', func);

// Analyze the file
const content = fs.readFileSync('src/core/lexer.c', 'utf8');
const lines = content.split('\n');

// Find the function
for (let i = 0; i < lines.length; i++) {
    if (lines[i].includes(func.name + '(')) {
        console.log(`\nFound function '${func.name}' at line ${i + 1}:`);
        console.log('Line:', lines[i]);
        
        // Look for doxygen comment above
        let hasValidDoc = false;
        for (let j = i - 1; j >= 0; j--) {
            if (lines[j].trim().endsWith('*/')) {
                const docBlock = [];
                for (let k = j; k >= 0; k--) {
                    docBlock.unshift(lines[k]);
                    if (lines[k].trim().startsWith('/**')) break;
                }
                const docText = docBlock.join('\n');
                console.log('\nFound documentation block:');
                console.log(docText);
                
                const hasBrief = docText.includes('@brief');
                const hasParam = docText.includes('@param') || func.signature.includes('(void)');
                const hasReturn = docText.includes('@return') || func.signature.startsWith('void ');
                
                console.log('\nDocumentation analysis:');
                console.log('- Has @brief:', hasBrief);
                console.log('- Has @param (or void):', hasParam, '(signature has void?', func.signature.includes('(void)'), ')');
                console.log('- Has @return (or void return):', hasReturn, '(signature starts with void?', func.signature.startsWith('void '), ')');
                console.log('- Function signature:', func.signature);
                
                hasValidDoc = hasBrief && hasParam && hasReturn;
                console.log('- Overall hasValidDoc:', hasValidDoc);
                break;
            }
            if (!lines[j].trim().startsWith('*') && lines[j].trim()) {
                console.log('Stopped looking for docs at line', j + 1, ':', lines[j].trim());
                break;
            }
        }
        
        // Check if it's a stub (simplified version)
        const funcRegex = new RegExp(`${func.name}\\s*\\([^)]*\\)\\s*\\{([^}]*\\{[^}]*\\})*[^}]*\\}`, 'gs');
        const match = content.match(funcRegex);
        let isStub = false;
        if (match) {
            const functionBody = match[0];
            const bodyContent = functionBody.substring(functionBody.indexOf('{') + 1, functionBody.lastIndexOf('}'));
            const meaningfulLines = bodyContent.split('\n').filter(line => {
                const trimmed = line.trim();
                return trimmed && !trimmed.startsWith('//') && !trimmed.startsWith('/*');
            });
            isStub = meaningfulLines.length <= 1;
            console.log('\nStub analysis:');
            console.log('- Function body meaningful lines:', meaningfulLines.length);
            console.log('- Is stub?', isStub);
        }
        
        // Final status determination
        let status;
        if (isStub) {
            status = 'stub';
        } else if (!hasValidDoc) {
            status = 'incomplete';
        } else {
            status = 'complete';
        }
        
        console.log('\nFinal status:', status);
        break;
    }
}