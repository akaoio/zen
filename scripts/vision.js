#!/usr/bin/env node

/**
 * ZEN Project Vision
 * 
 * Visualizes the project structure from MANIFEST.json with color-coded
 * implementation status for both files and functions
 */

const fs = require('fs');
const path = require('path');

// ANSI color codes
const colors = {
    red: '\x1b[31m',      // Missing/stub/fake
    yellow: '\x1b[33m',   // Incomplete
    green: '\x1b[32m',    // Complete
    blue: '\x1b[34m',     // Directory
    gray: '\x1b[90m',     // Metadata
    magenta: '\x1b[35m',  // Desired (not yet created)
    cyan: '\x1b[36m',     // Future feature
    orange: '\x1b[38;5;208m', // Unauthorized (bright orange)
    reset: '\x1b[0m',
    bold: '\x1b[1m',
    dim: '\x1b[2m',
    italic: '\x1b[3m'
};

// Tree drawing characters
const tree = {
    branch: '├── ',
    lastBranch: '└── ',
    vertical: '│   ',
    empty: '    '
};

class ProjectVision {
    constructor(projectRoot) {
        this.projectRoot = projectRoot;
        this.manifestPath = path.join(projectRoot, 'MANIFEST.json');
        this.fileStatuses = new Map();
        this.functionStatuses = new Map();
        this.activeTasks = new Map(); // Map of file -> agent working on it
    }

    loadManifest() {
        if (!fs.existsSync(this.manifestPath)) {
            console.error('❌ MANIFEST.json not found!');
            process.exit(1);
        }
        return JSON.parse(fs.readFileSync(this.manifestPath, 'utf8'));
    }

    /**
     * Check if a function implementation looks like a stub/fake
     */
    isStubImplementation(content, functionName) {
        // Common patterns for stub implementations
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
            /^\s*\{\s*\}\s*$/,  // Empty function body
            /^\s*\{\s*return\s+NULL\s*;\s*\}\s*$/,  // Only returns NULL (entire function)
            /^\s*\{\s*return\s+0\s*;\s*\}\s*$/,     // Only returns 0 (entire function)
            /^\s*\{\s*return\s+false\s*;\s*\}\s*$/  // Only returns false (entire function)
        ];

        // Find function definition more accurately using proper brace matching
        const functionBody = this.extractFunctionBody(content, functionName);
        if (!functionBody) return false;  // Function not found
        
        // Extract just the body content (without function signature)
        const bodyContent = functionBody.substring(functionBody.indexOf('{') + 1, functionBody.lastIndexOf('}'));
        
        // Check if it's explicitly a stub - apply patterns to body content only
        for (const pattern of stubPatterns) {
            if (pattern.test(bodyContent.trim())) return true;
        }

        // Enhanced meaningful content analysis (bodyContent already extracted above)
        
        // Count actual meaningful lines of code (not just lines)
        const meaningfulLines = bodyContent.split('\n').filter(line => {
            const trimmed = line.trim();
            // Filter out comments, empty lines, and braces
            return trimmed && 
                   !trimmed.startsWith('//') && 
                   !trimmed.startsWith('/*') &&
                   !trimmed.endsWith('*/') &&
                   trimmed !== '*' &&
                   !trimmed.match(/^\s*\*/) &&  // doxygen comment lines
                   trimmed !== '{' &&
                   trimmed !== '}';
        });

        // Count control structures, function calls, assignments etc.
        const hasControlFlow = /\b(if|else|while|for|switch|case)\b/.test(bodyContent);
        const hasFunctionCalls = /\w+\s*\([^)]*\)\s*;/.test(bodyContent);
        const hasAssignments = /\w+\s*[=+\-*\/]\s*/.test(bodyContent);
        const hasMemoryOps = /\b(malloc|calloc|realloc|free|memory_alloc|memory_free)\b/.test(bodyContent);
        const hasComplexLogic = hasControlFlow || hasFunctionCalls || hasAssignments || hasMemoryOps;

        // A function is a stub if:
        // 1. It has no meaningful lines, OR
        // 2. It has very few lines (≤ 2) AND no complex logic
        return meaningfulLines.length === 0 || 
               (meaningfulLines.length <= 2 && !hasComplexLogic);
    }
    
    /**
     * Extract function body with proper brace matching - finds DEFINITIONS not calls
     */
    extractFunctionBody(content, functionName) {
        // Escape special regex characters in function name
        const escapedName = functionName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        
        // Find function definition - prioritize definitions over calls
        // Look for: [optional_return_type] functionName([params]) {
        const funcDefRegex = new RegExp(
            `(?:^|\\n)\\s*(?:\\w+\\s*\\*?\\s+)?${escapedName}\\s*\\([^)]*\\)\\s*\\{`, 
            'gm'
        );
        
        let match = funcDefRegex.exec(content);
        
        // If no definition found, try broader pattern but filter out obvious calls
        if (!match) {
            const broadRegex = new RegExp(`\\b${escapedName}\\s*\\([^)]*\\)\\s*\\{`, 'g');
            let candidate;
            while ((candidate = broadRegex.exec(content)) !== null) {
                // Get context around the match to filter out calls
                const startLine = content.lastIndexOf('\n', candidate.index) + 1;
                const lineContent = content.substring(startLine, candidate.index + candidate[0].length);
                
                // Skip if this looks like a function call
                const isCall = /(?:return\s+|=\s*|if\s*\(|while\s*\(|for\s*\(|printf\s*\(|fprintf\s*\()/.test(lineContent);
                
                if (!isCall) {
                    match = candidate;
                    break;
                }
            }
        }
        
        if (!match) return null;

        // Find matching closing brace using proper brace counting
        let braceCount = 1;
        let pos = match.index + match[0].length;
        let inString = false;
        let inComment = false;
        let inLineComment = false;
        
        while (pos < content.length && braceCount > 0) {
            const char = content[pos];
            const nextChar = pos + 1 < content.length ? content[pos + 1] : '';
            
            // Handle string literals
            if (char === '"' && !inComment && !inLineComment && content[pos - 1] !== '\\') {
                inString = !inString;
            }
            // Handle block comments
            else if (char === '/' && nextChar === '*' && !inString && !inLineComment) {
                inComment = true;
                pos++; // skip next char
            }
            else if (char === '*' && nextChar === '/' && inComment) {
                inComment = false;
                pos++; // skip next char
            }
            // Handle line comments
            else if (char === '/' && nextChar === '/' && !inString && !inComment) {
                inLineComment = true;
            }
            else if (char === '\n' && inLineComment) {
                inLineComment = false;
            }
            // Count braces only when not in strings or comments
            else if (!inString && !inComment && !inLineComment) {
                if (char === '{') {
                    braceCount++;
                } else if (char === '}') {
                    braceCount--;
                }
            }
            
            pos++;
        }
        
        if (braceCount === 0) {
            return content.substring(match.index, pos);
        }
        
        return null; // Unmatched braces
    }
    
    /**
     * Check if a function has adequate documentation
     */
    hasDocumentation(content, functionName) {
        const lines = content.split('\n');
        
        // Find the function definition line (not just any call)
        // Look for pattern: [return_type] functionName([params]) {
        let functionLineIndex = -1;
        const escapedName = functionName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        const functionDefRegex = new RegExp(`\\b${escapedName}\\s*\\([^)]*\\)\\s*\\{`);
        
        for (let i = 0; i < lines.length; i++) {
            // First check if this line contains function definition pattern
            if (functionDefRegex.test(lines[i])) {
                functionLineIndex = i;
                break;
            }
            
            // Better approach: look for function definition by checking for return type prefix
            const line = lines[i].trim();
            if (line.match(new RegExp(`^(int|void|char\\*|AST_T\\*|parser_T\\*|Value\\*|bool|size_t|double|float)\\s+${escapedName}\\s*\\(`))) {
                functionLineIndex = i;
                break;
            }
            
            // Fallback: check if line contains function name with opening brace on same or next line
            if (lines[i].includes(functionName + '(')) {
                // Check if this looks like a definition (has opening brace nearby)
                if (lines[i].includes('{') || 
                    (i + 1 < lines.length && lines[i + 1].includes('{'))) {
                    // Enhanced check: make sure this isn't a function call
                    const lineContent = lines[i].trim();
                    const isCall = lineContent.startsWith('return ') ||
                                  lineContent.includes(' = ') ||
                                  lineContent.includes('if (') ||
                                  lineContent.includes('while (') ||
                                  lineContent.includes('for (') ||
                                  lineContent.includes('printf(') ||
                                  lineContent.includes('fprintf(') ||
                                  lineContent.startsWith('!') ||
                                  lineContent.includes('&&') ||
                                  lineContent.includes('||') ||
                                  lineContent.includes('->') ||
                                  lineContent.includes('::') ||
                                  lineContent.includes('++') ||
                                  lineContent.includes('--') ||
                                  lineContent.includes(');') ||
                                  /^\s*(if|while|for|switch|case|default|else)\s*/.test(lineContent);
                    
                    if (!isCall) {
                        functionLineIndex = i;
                        break;
                    }
                }
            }
        }
        
        if (functionLineIndex === -1) {
            return false; // Function not found
        }
        
        // Look for comments above the function (within 10 lines)
        let hasDocumentation = false;
        let docContent = '';
        
        for (let j = Math.max(0, functionLineIndex - 10); j < functionLineIndex; j++) {
            const line = lines[j].trim();
            
            // Check for various comment styles
            if (line.startsWith('/**') || line.startsWith('/*') || 
                line.startsWith('*') || line.startsWith('//')) {
                docContent += line + ' ';
                hasDocumentation = true;
            }
        }
        
        // If we found any comments, do a basic quality check
        if (hasDocumentation) {
            // Accept documentation if it:
            // 1. Has @brief or a description, OR  
            // 2. Has meaningful content (more than just the function name), OR
            // 3. Explains parameters or return values
            const hasDescription = docContent.includes('@brief') || 
                                  docContent.includes('@param') || 
                                  docContent.includes('@return') ||
                                  docContent.includes('Create') ||
                                  docContent.includes('Parse') ||
                                  docContent.includes('Get') ||
                                  docContent.includes('Set') ||
                                  docContent.includes('Free') ||
                                  docContent.includes('Initialize') ||
                                  docContent.includes('Check') ||
                                  docContent.includes('Handle');
            
            // Additional check: meaningful content beyond just function name
            const cleanDoc = docContent.toLowerCase().replace(/[*\/\s]/g, '');
            const cleanFunctionName = functionName.toLowerCase().replace(/_/g, '');
            const hasMeaningfulContent = cleanDoc.length > cleanFunctionName.length + 10;
            
            return hasDescription || hasMeaningfulContent;
        }
        
        return false;
    }

    /**
     * Extract all functions from a C/H file
     */
    extractAllFunctions(filePath) {
        if (!fs.existsSync(filePath)) {
            return [];
        }

        const content = fs.readFileSync(filePath, 'utf8');
        const functions = [];
        
        // Remove comments
        const cleanContent = content.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '');
        
        // Split into lines and process each potential function
        const lines = cleanContent.split('\n');
        
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            
            // Skip empty lines and preprocessor directives
            if (!line.trim() || line.trim().startsWith('#')) continue;
            
            // Look for function-like patterns: return_type name(params...)
            const funcMatch = line.match(/^(\w[\w\s*]*)\s+(\w+)\s*\((.*)$/);
            if (!funcMatch) continue;
            
            let [, returnType, name, paramsStart] = funcMatch;
            
            // Skip control structures
            if (['if', 'for', 'while', 'switch', 'return'].includes(returnType.trim())) continue;
            if (['if', 'for', 'while', 'switch'].includes(name)) continue;
            
            // Collect full parameter list (might span multiple lines)
            let fullParams = paramsStart;
            let parenCount = 1; // We already have opening paren
            let lineIdx = i;
            
            // Count parens in first line
            for (const ch of paramsStart) {
                if (ch === '(') parenCount++;
                if (ch === ')') parenCount--;
            }
            
            // If not closed, continue to next lines
            while (parenCount > 0 && lineIdx < lines.length - 1) {
                lineIdx++;
                const nextLine = lines[lineIdx];
                fullParams += ' ' + nextLine.trim();
                for (const ch of nextLine) {
                    if (ch === '(') parenCount++;
                    if (ch === ')') parenCount--;
                }
            }
            
            // Extract just the parameters (remove trailing )
            const paramsMatch = fullParams.match(/^([^)]*)\)/);
            if (!paramsMatch) continue;
            const params = paramsMatch[1];
            
            // Check if this is followed by a { (might be on next line) or ; (for headers)
            let hasOpenBrace = false;
            let hasSemicolon = false;
            for (let j = i; j < Math.min(lineIdx + 3, lines.length); j++) {
                if (lines[j].includes('{')) {
                    hasOpenBrace = true;
                    break;
                }
                if (lines[j].includes(';')) {
                    hasSemicolon = true;
                    break;
                }
            }
            
            // Must have either { (implementation) or ; (declaration)
            if (!hasOpenBrace && !hasSemicolon) continue;
            
            // Skip static functions
            if (line.trim().startsWith('static ')) continue;
            
            functions.push({
                name,
                signature: `${returnType.trim()} ${name}(${params.trim()})`,
                lineNumber: i + 1
            });
        }
        
        return functions;
    }

    /**
     * Check if header file has doxygen documentation for its declarations
     */
    checkHeaderDoxygen(content) {
        const lines = content.split('\n');
        
        // Find all function declarations (ending with ;)
        const funcDeclarations = [];
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            // Look for function declarations
            if (/^\w[\w\s*]*\s+\w+\s*\([^)]*\)\s*;/.test(line.trim())) {
                funcDeclarations.push(i);
            }
        }
        
        // If no function declarations, check for struct/typedef docs
        if (funcDeclarations.length === 0) {
            // Check if structs/typedefs have documentation
            const hasStructDocs = /\/\*\*[\s\S]*?\*\/\s*(typedef\s+)?struct/.test(content);
            const hasTypedefDocs = /\/\*\*[\s\S]*?\*\/\s*typedef/.test(content);
            return hasStructDocs || hasTypedefDocs;
        }
        
        // Check if all function declarations have doxygen comments
        for (const lineIdx of funcDeclarations) {
            let hasDoc = false;
            
            // Look backwards for doxygen comment
            for (let j = lineIdx - 1; j >= 0; j--) {
                const line = lines[j].trim();
                if (line.endsWith('*/')) {
                    // Check if this is a doxygen comment block
                    for (let k = j; k >= 0; k--) {
                        if (lines[k].trim().startsWith('/**')) {
                            hasDoc = true;
                            break;
                        }
                    }
                    break;
                }
                // Stop if we hit something that's not a comment or empty
                if (line && !line.startsWith('*') && !line.startsWith('//')) {
                    break;
                }
            }
            
            if (!hasDoc) return false;
        }
        
        return true;
    }

    /**
     * Analyze a file to determine implementation status
     */
    analyzeFile(filePath, fileSpec) {
        if (!fs.existsSync(filePath)) {
            // Check if this is a desired future file
            return { status: 'desired', functions: {} };
        }

        const content = fs.readFileSync(filePath, 'utf8');
        const functionStatuses = {};

        // Check each function
        for (const func of fileSpec.functions || []) {
            // Check if function exists
            const funcRegex = new RegExp(`\\b${func.name}\\s*\\(`, 'g');
            if (!funcRegex.test(content)) {
                functionStatuses[func.name] = 'desired';
                continue;
            }

            // Check for documentation (more flexible than strict doxygen)
            let hasValidDoc = this.hasDocumentation(content, func.name);

            // Check if it's a stub
            if (this.isStubImplementation(content, func.name)) {
                functionStatuses[func.name] = 'stub';
            } else if (!hasValidDoc) {
                functionStatuses[func.name] = 'incomplete';
            } else {
                functionStatuses[func.name] = 'complete';
            }
        }

        // Determine overall file status
        const funcValues = Object.values(functionStatuses);
        let fileStatus = 'complete';
        
        if (funcValues.length === 0 && fileSpec.functions?.length > 0) {
            fileStatus = 'missing';
        } else if (funcValues.some(s => s === 'missing')) {
            fileStatus = 'missing';
        } else if (funcValues.every(s => s === 'desired')) {
            // If ALL functions are desired (not implemented), the file is a stub
            fileStatus = 'stub';
        } else if (funcValues.every(s => s === 'stub' || s === 'incomplete' || s === 'desired')) {
            // If ALL functions are stubs, incomplete, or desired, the file is a stub
            fileStatus = 'stub';
        } else if (funcValues.some(s => s === 'stub')) {
            fileStatus = 'stub';
        } else if (funcValues.some(s => s === 'incomplete')) {
            fileStatus = 'incomplete';
        }

        // Special case for stub files with no functions
        if (fileSpec.functions?.length === 0 && content.length < 200) {
            fileStatus = 'stub';
        }

        // Extract all actual functions from the file
        const allFunctions = this.extractAllFunctions(filePath);
        const manifestFunctionNames = new Set((fileSpec.functions || []).map(f => f.name));
        
        // Find unauthorized functions (functions not in manifest)
        const unauthorizedFunctions = [];
        for (const actualFunc of allFunctions) {
            if (!manifestFunctionNames.has(actualFunc.name)) {
                unauthorizedFunctions.push(actualFunc);
            }
        }

        return { status: fileStatus, functions: functionStatuses, unauthorized: unauthorizedFunctions };
    }

    /**
     * Get color for status
     */
    getStatusColor(status) {
        switch (status) {
            case 'missing': return colors.red;
            case 'stub': return colors.red;
            case 'incomplete': return colors.yellow;
            case 'complete': return colors.green;
            case 'desired': return colors.magenta;
            case 'future': return colors.cyan;
            case 'unauthorized': return colors.orange;
            default: return colors.reset;
        }
    }

    /**
     * Get color for progress percentage
     */
    getProgressColor(percentage) {
        if (percentage === 100) return colors.green;
        if (percentage >= 50) return colors.yellow;
        return colors.red;
    }

    /**
     * Calculate progress percentage
     */
    calculateProgress(node) {
        if (node._type === 'file') {
            const funcs = Object.values(node._functions || {});
            if (funcs.length === 0) return node._status === 'complete' ? 100 : 0;
            const complete = funcs.filter(s => s === 'complete').length;
            return Math.round((complete / funcs.length) * 100);
        } else if (node._type === 'header') {
            // Headers progress based on their implementation status
            switch (node._status) {
                case 'complete': return 100;
                case 'incomplete': return 50;
                case 'stub': return 10;
                case 'missing': return 0;
                case 'desired': return 0;
                case 'future': return 0;
                default: return 0;
            }
        } else if (node._type === 'directory') {
            let totalFunctions = 0;
            let completeFunctions = 0;
            
            const countFunctions = (n) => {
                if (n._type === 'file' && n._functions) {
                    const funcs = Object.values(n._functions);
                    totalFunctions += funcs.length;
                    completeFunctions += funcs.filter(s => s === 'complete').length;
                } else if (n._type === 'header') {
                    // Skip headers in directory progress calculation
                } else if (n._type === 'directory' || !n._type) {
                    const children = n._children || n;
                    Object.values(children).forEach(child => {
                        if (typeof child === 'object' && child !== null) {
                            countFunctions(child);
                        }
                    });
                }
            };
            
            countFunctions(node);
            return totalFunctions > 0 ? Math.round((completeFunctions / totalFunctions) * 100) : 0;
        }
        return 0;
    }

    /**
     * Get status symbol
     */
    getStatusSymbol(status) {
        switch (status) {
            case 'missing': return '✗';
            case 'stub': return '○';
            case 'incomplete': return '◐';
            case 'complete': return '●';
            case 'desired': return '◇';  // Empty diamond for desired files
            case 'future': return '⟡';   // Special symbol for future features
            case 'unauthorized': return '⚠';  // Warning symbol for unauthorized functions
            default: return '?';
        }
    }

    /**
     * Build directory tree structure
     */
    buildDirectoryTree(manifest) {
        const tree = {};
        
        // Add files from manifest
        for (const filePath of Object.keys(manifest.files)) {
            const parts = filePath.split('/');
            let current = tree;
            
            for (let i = 0; i < parts.length - 1; i++) {
                if (!current[parts[i]]) {
                    current[parts[i]] = { _type: 'directory', _children: {} };
                }
                current = current[parts[i]]._children;
            }
            
            const fileName = parts[parts.length - 1];
            const fullPath = path.join(this.projectRoot, filePath);
            const analysis = this.analyzeFile(fullPath, manifest.files[filePath]);
            
            current[fileName] = {
                _type: 'file',
                _path: filePath,
                _spec: manifest.files[filePath],
                _status: analysis.status,
                _functions: analysis.functions,
                _unauthorized: analysis.unauthorized || []
            };
        }
        
        // Add headers
        for (const headerPath of Object.keys(manifest.headers || {})) {
            const parts = headerPath.split('/');
            let current = tree;
            
            for (let i = 0; i < parts.length - 1; i++) {
                if (!current[parts[i]]) {
                    current[parts[i]] = { _type: 'directory', _children: {} };
                }
                current = current[parts[i]]._children;
            }
            
            const fileName = parts[parts.length - 1];
            const fullPath = path.join(this.projectRoot, headerPath);
            
            // Check header implementation status
            let headerStatus = 'desired';
            if (fs.existsSync(fullPath)) {
                const content = fs.readFileSync(fullPath, 'utf8');
                // Check if it's just a stub header (too small or has TODO comments)
                if (content.length < 100 || /\/\/\s*TODO|\/\*\s*TODO/.test(content)) {
                    headerStatus = 'stub';
                } else {
                    // Check for doxygen documentation in header
                    const headerHasDocs = this.checkHeaderDoxygen(content);
                    
                    // Check if corresponding .c file has implementations
                    const implPath = headerPath.replace('/include/zen/', '/').replace('.h', '.c');
                    const implFile = manifest.files[implPath];
                    
                    if (implFile) {
                        const implFullPath = path.join(this.projectRoot, implPath);
                        const analysis = this.analyzeFile(implFullPath, implFile);
                        
                        // Header is only complete if both have docs AND implementation is complete
                        if (headerHasDocs && analysis.status === 'complete') {
                            headerStatus = 'complete';
                        } else if (!headerHasDocs || analysis.status === 'stub' || analysis.status === 'missing') {
                            headerStatus = 'stub';
                        } else {
                            headerStatus = 'incomplete';
                        }
                    } else {
                        // No implementation file found
                        headerStatus = headerHasDocs ? 'incomplete' : 'stub';
                    }
                }
            }
            
            current[fileName] = {
                _type: 'header',
                _path: headerPath,
                _spec: manifest.headers[headerPath],
                _status: headerStatus
            };
        }
        
        // Scan for unauthorized files in tracked directories
        const trackedDirs = new Set();
        
        // Always scan the root directory
        trackedDirs.add('.');
        
        // Collect all directories that have manifest files
        for (const filePath of Object.keys(manifest.files)) {
            const parts = filePath.split('/');
            let dirPath = '';
            for (let i = 0; i < parts.length - 1; i++) {
                dirPath = dirPath ? dirPath + '/' + parts[i] : parts[i];
                trackedDirs.add(dirPath);
            }
        }
        
        // Also check header directories
        for (const headerPath of Object.keys(manifest.headers || {})) {
            const parts = headerPath.split('/');
            let dirPath = '';
            for (let i = 0; i < parts.length - 1; i++) {
                dirPath = dirPath ? dirPath + '/' + parts[i] : parts[i];
                trackedDirs.add(dirPath);
            }
        }
        
        // Scan each tracked directory for unauthorized files
        const manifestFiles = new Set([
            ...Object.keys(manifest.files),
            ...Object.keys(manifest.headers || {})
        ]);
        
        for (const dir of trackedDirs) {
            const fullDirPath = path.join(this.projectRoot, dir);
            if (!fs.existsSync(fullDirPath)) continue;
            
            try {
                const files = fs.readdirSync(fullDirPath);
                for (const file of files) {
                    const relPath = dir === '.' ? file : path.join(dir, file);
                    const fullPath = path.join(fullDirPath, file);
                    
                    // Skip if already in manifest
                    if (manifestFiles.has(relPath)) continue;
                    
                    const stat = fs.statSync(fullPath);
                    
                    if (stat.isFile() && (file.endsWith('.c') || file.endsWith('.h'))) {
                        // This is an unauthorized C/H file
                        const parts = relPath.split('/');
                        let current = tree;
                        
                        // Navigate to the directory
                        for (let i = 0; i < parts.length - 1; i++) {
                            if (!current[parts[i]]) {
                                current[parts[i]] = { _type: 'directory', _children: {} };
                            }
                            current = current[parts[i]]._children;
                        }
                        
                        // Add unauthorized file
                        const fileName = parts[parts.length - 1];
                        current[fileName] = {
                            _type: 'file',
                            _path: relPath,
                            _spec: null,
                            _status: 'unauthorized',
                            _functions: {},
                            _unauthorized: []
                        };
                    } else if (stat.isDirectory()) {
                        // Check if this directory is completely unauthorized (not in any manifest path)
                        const subDirPath = relPath;
                        let isAuthorized = false;
                        
                        // Whitelist of authorized project directories
                        const authorizedDirectories = [
                            '.claude',          // Claude Code configuration
                            '.git',             // Git repository data  
                            '.githooks',        // Git hooks
                            'node_modules',     // Node.js dependencies
                            'scripts',          // Project automation scripts
                            'swarm',            // Multi-swarm system architecture
                            'tasks',            // Task management files
                            'tests',            // Test suite
                            'tools',            // Development tools
                            'workspace',        // Multi-swarm workspaces
                            'docs',             // Documentation
                            'examples',         // Example files
                            'build',            // Build artifacts
                            'bin',              // Binary output
                            'lib',              // Libraries
                            'include',          // Header files
                            'target',           // Rust build directory
                            'dist',             // Distribution files
                            '.vscode',          // VS Code settings
                            '.github',          // GitHub workflows
                            'cmake',            // CMake files
                            'config'            // Configuration files
                        ];
                        
                        // Check if this is a whitelisted directory or subdirectory
                        const isWhitelisted = authorizedDirectories.some(authDir => {
                            return subDirPath === authDir || subDirPath.startsWith(authDir + '/');
                        });
                        
                        // Check if any manifest file is in this directory or subdirectories
                        for (const manifestPath of manifestFiles) {
                            if (manifestPath.startsWith(subDirPath + '/')) {
                                isAuthorized = true;
                                break;
                            }
                        }
                        
                        // Directory is authorized if it's whitelisted, has manifest files, or is a tracked directory
                        isAuthorized = isAuthorized || isWhitelisted || trackedDirs.has(subDirPath);
                        
                        if (!isAuthorized) {
                            // This is an unauthorized directory
                            const parts = relPath.split('/');
                            let current = tree;
                            
                            // Navigate to parent directory
                            for (let i = 0; i < parts.length - 1; i++) {
                                if (!current[parts[i]]) {
                                    current[parts[i]] = { _type: 'directory', _children: {} };
                                }
                                current = current[parts[i]]._children;
                            }
                            
                            // Add unauthorized directory
                            current[file] = {
                                _type: 'directory',
                                _path: subDirPath,
                                _spec: null,
                                _status: 'unauthorized',
                                _children: {}
                            };
                        }
                    }
                }
            } catch (err) {
                // Ignore errors reading directories
            }
        }
        
        return tree;
    }

    /**
     * Print the directory tree
     */
    printTree(node, name = '.', prefix = '', isLast = true) {
        // Print current node
        if (name !== '.') {
            const connector = isLast ? tree.lastBranch : tree.branch;
            
            if (node._type === 'directory') {
                if (node._status === 'unauthorized') {
                    // Unauthorized directory
                    console.log(prefix + connector + colors.orange + colors.bold + '⚠ ' + name + '/' + colors.reset + 
                              ' ' + colors.orange + '[UNAUTHORIZED]' + colors.reset);
                } else {
                    // Normal directory
                    const progress = this.calculateProgress(node);
                    const progressColor = this.getProgressColor(progress);
                    console.log(prefix + connector + colors.blue + colors.bold + name + '/' + colors.reset + 
                              ' ' + progressColor + `[${progress}%]` + colors.reset);
                }
            } else {
                const color = this.getStatusColor(node._status);
                const symbol = this.getStatusSymbol(node._status);
                const progress = this.calculateProgress(node);
                const progressColor = this.getProgressColor(progress);
                const desc = node._status === 'unauthorized' 
                    ? colors.gray + ' # UNAUTHORIZED FILE (not in manifest)' + colors.reset
                    : (node._spec?.description ? colors.gray + ' # ' + node._spec.description + colors.reset : '');
                
                // Check if an agent is working on this file
                const activeTask = this.activeTasks.get(node._path);
                const agentIndicator = activeTask 
                    ? ` ${colors.cyan}[${activeTask.agent}]${colors.reset}` 
                    : '';
                
                const progressDisplay = node._status === 'unauthorized' 
                    ? '' 
                    : ' ' + progressColor + `[${progress}%]` + colors.reset;
                
                console.log(prefix + connector + color + symbol + ' ' + name + colors.reset + 
                          progressDisplay + 
                          agentIndicator + desc);
                
                // Print functions if it's a file with functions
                if (node._type === 'file' && (node._functions && Object.keys(node._functions).length > 0 || 
                    node._unauthorized && node._unauthorized.length > 0)) {
                    const funcPrefix = prefix + (isLast ? tree.empty : tree.vertical) + tree.empty;
                    const funcs = Object.entries(node._functions || {});
                    const unauthorizedFuncs = node._unauthorized || [];
                    const totalFuncs = funcs.length + unauthorizedFuncs.length;
                    let funcIndex = 0;
                    
                    // Print manifest functions first
                    funcs.forEach(([funcName, funcStatus], index) => {
                        const isLastFunc = funcIndex === totalFuncs - 1;
                        const funcConnector = isLastFunc ? tree.lastBranch : tree.branch;
                        const funcColor = this.getStatusColor(funcStatus);
                        const funcSymbol = this.getStatusSymbol(funcStatus);
                        const funcDesc = node._spec.functions.find(f => f.name === funcName)?.description || '';
                        
                        console.log(funcPrefix + funcConnector + funcColor + funcSymbol + ' ' + funcName + '()' + 
                                  colors.reset + colors.gray + ' # ' + funcDesc + colors.reset);
                        funcIndex++;
                    });
                    
                    // Print unauthorized functions
                    unauthorizedFuncs.forEach((func, index) => {
                        const isLastFunc = funcIndex === totalFuncs - 1;
                        const funcConnector = isLastFunc ? tree.lastBranch : tree.branch;
                        const funcColor = colors.orange;  // Orange for unauthorized
                        const funcSymbol = this.getStatusSymbol('unauthorized');
                        
                        console.log(funcPrefix + funcConnector + funcColor + funcSymbol + ' ' + func.name + '()' + 
                                  colors.reset + colors.gray + ' # UNAUTHORIZED (not in manifest)' + colors.reset);
                        funcIndex++;
                    });
                }
            }
        }
        
        // Print children for directories
        if (node._type === 'directory' || name === '.') {
            const children = node._children || node;
            const entries = Object.entries(children).filter(([key]) => !key.startsWith('_'));
            
            entries.forEach(([childName, childNode], index) => {
                const isLastChild = index === entries.length - 1;
                const childPrefix = name === '.' ? '' : prefix + (isLast ? tree.empty : tree.vertical);
                this.printTree(childNode, childName, childPrefix, isLastChild);
            });
        }
    }

    /**
     * Generate statistics
     */
    generateStats(tree) {
        let stats = {
            files: { total: 0, complete: 0, incomplete: 0, stub: 0, missing: 0, desired: 0 },
            functions: { total: 0, complete: 0, incomplete: 0, stub: 0, missing: 0, desired: 0 }
        };

        const processNode = (node) => {
            if (node._type === 'file') {
                stats.files.total++;
                stats.files[node._status]++;
                
                if (node._functions) {
                    for (const [, status] of Object.entries(node._functions)) {
                        stats.functions.total++;
                        if (stats.functions[status] !== undefined) {
                            stats.functions[status]++;
                        }
                    }
                }
            } else if (node._type === 'directory' || !node._type) {
                const children = node._children || node;
                for (const child of Object.values(children)) {
                    if (!child._type && child.constructor === Object) continue;
                    processNode(child);
                }
            }
        };

        processNode(tree);
        return stats;
    }

    /**
     * Print statistics
     */
    printStats(stats) {
        console.log('\n' + colors.bold + 'Implementation Status:' + colors.reset);
        console.log('─'.repeat(50));
        
        // Files
        console.log(colors.bold + 'Files:' + colors.reset);
        console.log(`  ${colors.green}● Complete:    ${stats.files.complete}/${stats.files.total}${colors.reset}`);
        console.log(`  ${colors.yellow}◐ Incomplete:  ${stats.files.incomplete}/${stats.files.total}${colors.reset}`);
        console.log(`  ${colors.red}○ Stubs:       ${stats.files.stub}/${stats.files.total}${colors.reset}`);
        console.log(`  ${colors.red}✗ Missing:     ${stats.files.missing}/${stats.files.total}${colors.reset}`);
        console.log(`  ${colors.magenta}◇ Desired:     ${stats.files.desired}/${stats.files.total}${colors.reset}`);
        
        // Functions
        console.log('\n' + colors.bold + 'Functions:' + colors.reset);
        console.log(`  ${colors.green}● Complete:    ${stats.functions.complete}/${stats.functions.total}${colors.reset}`);
        console.log(`  ${colors.yellow}◐ Incomplete:  ${stats.functions.incomplete}/${stats.functions.total}${colors.reset}`);
        console.log(`  ${colors.red}○ Stubs:       ${stats.functions.stub}/${stats.functions.total}${colors.reset}`);
        console.log(`  ${colors.red}✗ Missing:     ${stats.functions.missing}/${stats.functions.total}${colors.reset}`);
        
        // Progress bar
        const progress = stats.functions.total > 0 
            ? Math.round((stats.functions.complete / stats.functions.total) * 100)
            : 0;
        const barLength = 30;
        const filled = Math.round(barLength * progress / 100);
        const bar = '█'.repeat(filled) + '░'.repeat(barLength - filled);
        
        console.log('\n' + colors.bold + 'Overall Progress:' + colors.reset);
        console.log(`[${colors.green}${bar}${colors.reset}] ${progress}%`);
    }

    /**
     * Load active tasks from tasks folder
     */
    loadActiveTasks() {
        const tasksDir = path.join(this.projectRoot, 'tasks');
        if (!fs.existsSync(tasksDir)) return [];
        
        const now = Date.now() / 1000; // Current unix timestamp
        const maxAge = 3600 * 24; // 24 hours - tasks older than this are considered abandoned
        const tasks = [];
        
        const taskFiles = fs.readdirSync(tasksDir)
            .filter(f => f.endsWith('.yaml') || f.endsWith('.yml'))
            .sort((a, b) => b.localeCompare(a));
            
        for (const file of taskFiles) {
            try {
                const filePath = path.join(tasksDir, file);
                const content = fs.readFileSync(filePath, 'utf8');
                
                // Parse task
                const agent = content.match(/agent:\s*(.+)/)?.[1]?.trim() || 'unknown';
                const task = content.match(/task:\s*(.+)/)?.[1]?.trim() || 'No description';
                const created = parseInt(content.match(/created:\s*(\d+)/)?.[1] || '0');
                const completed = /completed:\s*true/i.test(content);
                const filesMatch = content.match(/files:\s*\n((?:\s*-\s*.+\n)*)/);
                const files = [];
                
                if (filesMatch) {
                    const fileLines = filesMatch[1].trim().split('\n');
                    for (const line of fileLines) {
                        const file = line.match(/^\s*-\s*(.+)/)?.[1]?.trim();
                        if (file) files.push(file);
                    }
                }
                
                // Check latest step for success/fail status
                const success = /success:\s*true/i.test(content);
                const fail = /fail:\s*true/i.test(content);
                const isComplete = completed;
                const isOld = (now - created) > maxAge;
                
                if (!isComplete && !isOld && files.length > 0) {
                    // Active task - map files to agent
                    for (const file of files) {
                        this.activeTasks.set(file, { agent, task, created });
                    }
                }
                
                tasks.push({
                    file: file,
                    agent,
                    task,
                    created,
                    files,
                    success,
                    fail,
                    isComplete,
                    isOld
                });
            } catch (e) {
                // Skip invalid task files
            }
        }
        
        return tasks;
    }

    /**
     * Run vision
     */
    run() {
        const manifest = this.loadManifest();
        
        // Load active tasks before building tree
        const tasks = this.loadActiveTasks();
        
        console.log(colors.bold + '🔮 ZEN Project Vision' + colors.reset);
        console.log('=' .repeat(50));
        console.log(colors.dim + 'Visualizing desired architecture from MANIFEST.json' + colors.reset);
        console.log();
        
        console.log(colors.bold + 'Legend:' + colors.reset);
        console.log(colors.dim + 'Reality:' + colors.reset);
        console.log(`  ${colors.green}● Complete${colors.reset}     - Fully implemented with documentation`);
        console.log(`  ${colors.yellow}◐ Incomplete${colors.reset}   - Partially implemented`);
        console.log(`  ${colors.red}○ Stub${colors.reset}         - Stub/TODO implementation`);
        console.log(`  ${colors.red}✗ Missing${colors.reset}      - Required but not created`);
        console.log(colors.dim + 'Desire:' + colors.reset);
        console.log(`  ${colors.magenta}◇ Desired${colors.reset}      - Planned future file/feature`);
        console.log(`  ${colors.cyan}⟡ Future${colors.reset}       - Long-term vision`);
        console.log(colors.dim + 'Compliance:' + colors.reset);
        console.log(`  ${colors.orange}⚠ Unauthorized${colors.reset} - File/folder not in MANIFEST.json`);
        console.log(colors.dim + 'Agents:' + colors.reset);
        console.log(`  ${colors.cyan}[agent-id]${colors.reset}     - Agent actively working on file`);
        console.log();
        
        const treeStructure = this.buildDirectoryTree(manifest);
        
        // Calculate and show root progress
        const rootProgress = this.calculateProgress(treeStructure);
        const rootProgressColor = this.getProgressColor(rootProgress);
        console.log(`. ${rootProgressColor}[${rootProgress}%]${colors.reset}`);
        
        this.printTree(treeStructure);
        
        const stats = this.generateStats(treeStructure);
        this.printStats(stats);
        
        // Show latest tasks from swarm
        this.showLatestTasks(tasks);
    }
    
    /**
     * Calculate agent fitness scores based on task history
     */
    calculateAgentFitness(allTasks) {
        const agentStats = new Map();
        
        // Process all tasks to build agent statistics
        allTasks.forEach(task => {
            if (!agentStats.has(task.agent)) {
                agentStats.set(task.agent, {
                    totalTasks: 0,
                    successTasks: 0,
                    failedTasks: 0,
                    activeTasks: 0,
                    completionRate: 0,
                    successRate: 0,
                    fitnessScore: 0,
                    role: this.detectAgentRole(task.agent)
                });
            }
            
            const stats = agentStats.get(task.agent);
            stats.totalTasks++;
            
            if (!task.isComplete && !task.isOld) {
                stats.activeTasks++;
            } else if (task.success) {
                stats.successTasks++;
            } else if (task.fail) {
                stats.failedTasks++;
            }
        });
        
        // Calculate metrics for each agent
        agentStats.forEach((stats, agent) => {
            const completedTasks = stats.successTasks + stats.failedTasks;
            stats.completionRate = completedTasks / stats.totalTasks;
            stats.successRate = completedTasks > 0 ? stats.successTasks / completedTasks : 0;
            
            // Fitness score: weighted combination of completion rate and success rate
            // Penalize agents with only active tasks (no completions)
            if (completedTasks === 0) {
                stats.fitnessScore = 0;
            } else {
                stats.fitnessScore = (stats.completionRate * 0.3 + stats.successRate * 0.7) * 100;
            }
        });
        
        return agentStats;
    }
    
    /**
     * Detect agent role from agent ID
     */
    detectAgentRole(agentId) {
        if (agentId.includes('queen')) return 'queen';
        if (agentId.includes('architect')) return 'architect';
        if (agentId.includes('worker')) return 'worker';
        return 'unknown';
    }
    
    /**
     * Get fitness color based on score
     */
    getFitnessColor(score) {
        if (score >= 80) return colors.green;
        if (score >= 60) return colors.yellow;
        if (score >= 40) return colors.gray;
        return colors.red;
    }

    /**
     * Show latest tasks from tasks/ folder
     */
    showLatestTasks(allTasks) {
        console.log('\n' + colors.bold + '📋 Swarm Activity:' + colors.reset);
        console.log('─'.repeat(80));
        
        if (!allTasks || allTasks.length === 0) {
            console.log(colors.dim + '  No tasks found' + colors.reset);
            return;
        }
        
        // Calculate agent fitness
        const agentFitness = this.calculateAgentFitness(allTasks);
        
        // Separate active and completed tasks
        const activeTasks = allTasks.filter(t => !t.isComplete && !t.isOld);
        const recentCompleted = allTasks.filter(t => t.isComplete).slice(0, 10);
        
        // Show active tasks
        if (activeTasks.length > 0) {
            console.log('\n' + colors.bold + 'Active Tasks:' + colors.reset);
            activeTasks.forEach(task => {
                const timeStr = this.formatTimestamp(task.file);
                const taskDesc = task.task.length > 50 
                    ? task.task.substring(0, 47) + '...'
                    : task.task;
                
                console.log(`  ${colors.yellow}◐${colors.reset} ${colors.cyan}[${task.agent}]${colors.reset} ${taskDesc}`);
                console.log(`     ${colors.dim}Started: ${timeStr}${colors.reset}`);
                if (task.files.length > 0) {
                    console.log(`     ${colors.dim}Files: ${task.files.join(', ')}${colors.reset}`);
                }
            });
        }
        
        // Show recent completed tasks
        if (recentCompleted.length > 0) {
            console.log('\n' + colors.bold + 'Recent Completed:' + colors.reset);
            recentCompleted.forEach(task => {
                const timeStr = this.formatTimestamp(task.file);
                const statusSymbol = task.success ? colors.green + '✓' : colors.red + '✗';
                const statusText = task.success ? 'Success' : 'Failed';
                const taskDesc = task.task.length > 50 
                    ? task.task.substring(0, 47) + '...'
                    : task.task;
                
                console.log(`  ${statusSymbol}${colors.reset} ${colors.cyan}[${task.agent}]${colors.reset} ${taskDesc}`);
                console.log(`     ${colors.dim}${statusText} at ${timeStr}${colors.reset}`);
                if (task.files.length > 0) {
                    console.log(`     ${colors.dim}Files: ${task.files.join(', ')}${colors.reset}`);
                }
            });
        }
        
        // Show agent fitness scores
        console.log('\n' + colors.bold + '🏆 Agent Fitness Scores:' + colors.reset);
        console.log('─'.repeat(80));
        
        // Group agents by role
        const agentsByRole = new Map();
        agentFitness.forEach((stats, agent) => {
            if (!agentsByRole.has(stats.role)) {
                agentsByRole.set(stats.role, []);
            }
            agentsByRole.get(stats.role).push({ agent, stats });
        });
        
        // Show best agent for each role
        ['queen', 'architect', 'worker'].forEach(role => {
            const agents = agentsByRole.get(role) || [];
            if (agents.length > 0) {
                // Sort by fitness score
                agents.sort((a, b) => b.stats.fitnessScore - a.stats.fitnessScore);
                
                console.log('\n' + colors.bold + `${role.charAt(0).toUpperCase() + role.slice(1)} Agents:` + colors.reset);
                
                agents.forEach(({ agent, stats }, index) => {
                    const fitnessColor = this.getFitnessColor(stats.fitnessScore);
                    const crown = index === 0 && stats.fitnessScore > 0 ? ' 👑' : '';
                    
                    console.log(`  ${colors.cyan}[${agent}]${colors.reset}${crown}`);
                    console.log(`    Fitness: ${fitnessColor}${stats.fitnessScore.toFixed(1)}%${colors.reset} | ` +
                               `Tasks: ${stats.totalTasks} | ` +
                               `Success: ${colors.green}${stats.successTasks}${colors.reset} | ` +
                               `Failed: ${colors.red}${stats.failedTasks}${colors.reset} | ` +
                               `Active: ${colors.yellow}${stats.activeTasks}${colors.reset}`);
                    console.log(`    Success Rate: ${stats.successRate > 0 ? colors.green : colors.gray}${(stats.successRate * 100).toFixed(1)}%${colors.reset} | ` +
                               `Completion Rate: ${stats.completionRate > 0 ? colors.green : colors.gray}${(stats.completionRate * 100).toFixed(1)}%${colors.reset}`);
                });
            }
        });
        
        // Summary
        const totalActive = activeTasks.length;
        const totalSuccess = allTasks.filter(t => t.success).length;
        const totalFailed = allTasks.filter(t => t.fail).length;
        
        console.log('\n' + colors.bold + 'Overall Summary:' + colors.reset);
        console.log(`  Total Tasks: ${allTasks.length} | ` +
                    `Active: ${colors.yellow}${totalActive}${colors.reset} | ` +
                    `Success: ${colors.green}${totalSuccess}${colors.reset} | ` +
                    `Failed: ${colors.red}${totalFailed}${colors.reset}`);
    }
    
    /**
     * Format timestamp from filename
     */
    formatTimestamp(filename) {
        const dateMatch = filename.match(/(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})/);
        if (dateMatch) {
            return `${dateMatch[1]}-${dateMatch[2]}-${dateMatch[3]} ${dateMatch[4]}:${dateMatch[5]}`;
        }
        return filename.replace('.yaml', '').replace('.yml', '');
    }
}

// Run the vision
const projectRoot = process.cwd();
const vision = new ProjectVision(projectRoot);
vision.run();