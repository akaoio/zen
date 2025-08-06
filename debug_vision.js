const fs = require('fs');
const content = fs.readFileSync('src/core/lexer.c', 'utf8');
const lines = content.split('\n');

// Find function line
for (let i = 0; i < lines.length; i++) {
    if (lines[i].includes('lexer_collect_string(')) {
        console.log('Function found at line:', i + 1);
        console.log('Line content:', lines[i]);
        
        // Look for doxygen comment above
        for (let j = i - 1; j >= 0; j--) {
            if (lines[j].trim().endsWith('*/')) {
                const docBlock = [];
                for (let k = j; k >= 0; k--) {
                    docBlock.unshift(lines[k]);
                    if (lines[k].trim().startsWith('/**')) break;
                }
                const docText = docBlock.join('\n');
                console.log('Doc block found:');
                console.log(docText);
                console.log('Has @brief:', docText.includes('@brief'));
                console.log('Has @param:', docText.includes('@param'));
                console.log('Has @return:', docText.includes('@return'));
                break;
            }
            if (!lines[j].trim().startsWith('*') && lines[j].trim()) break;
        }
        break;
    }
}