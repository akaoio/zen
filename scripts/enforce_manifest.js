#!/usr/bin/env node

/**
 * ZEN Manifest Enforcer
 * 
 * This tool enforces the codebase structure defined in MANIFEST.json
 * It validates files, functions, signatures, and prevents unauthorized changes
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

class ManifestEnforcer {
    constructor(projectRoot) {
        this.projectRoot = projectRoot;
        this.manifestPath = path.join(projectRoot, 'MANIFEST.json');
        this.violations = [];
        this.warnings = [];
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
    isStubImplementation(lines, funcLineIndex, functionName) {
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
            /abort\s*\(\s*\)/
        ];

        // Find function body
        let braceCount = 0;
        let funcStart = -1;
        let funcEnd = -1;
        
        for (let i = funcLineIndex; i < lines.length; i++) {
            const line = lines[i];
            if (funcStart === -1 && line.includes('{')) {
                funcStart = i;
            }
            
            for (const ch of line) {
                if (ch === '{') braceCount++;
                if (ch === '}') braceCount--;
            }
            
            if (braceCount === 0 && funcStart !== -1) {
                funcEnd = i;
                break;
            }
        }
        
        if (funcStart === -1 || funcEnd === -1) return false;
        
        // Extract function body
        const bodyLines = lines.slice(funcStart, funcEnd + 1);
        const bodyText = bodyLines.join('\n');
        
        // Check for stub patterns
        for (const pattern of stubPatterns) {
            if (pattern.test(bodyText)) return true;
        }
        
        // Check if function body is too short (likely stub)
        const meaningfulLines = bodyLines.filter((line, idx) => {
            if (idx === 0 || idx === bodyLines.length - 1) return false; // Skip braces
            const trimmed = line.trim();
            return trimmed && !trimmed.startsWith('//') && !trimmed.startsWith('/*');
        });
        
        // Empty body or single return statement is likely a stub
        return meaningfulLines.length <= 1;
    }

    /**
     * Check if a function has proper doxygen documentation
     */
    checkDoxygenDoc(lines, funcLineIndex) {
        // Look backwards from function line to find documentation
        let docStartIndex = -1;
        let docEndIndex = -1;
        
        // Find the comment block immediately before the function
        for (let i = funcLineIndex - 1; i >= 0; i--) {
            const line = lines[i].trim();
            
            // Skip empty lines
            if (!line) continue;
            
            // If we hit */ it's the end of a block comment
            if (line.endsWith('*/')) {
                docEndIndex = i;
                // Find the start of this comment block
                for (let j = i; j >= 0; j--) {
                    if (lines[j].trim().startsWith('/**')) {
                        docStartIndex = j;
                        break;
                    }
                }
                break;
            }
            
            // If we hit something that's not a comment or empty line, stop
            if (!line.startsWith('*') && !line.startsWith('/**') && !line.startsWith('//')) {
                break;
            }
        }
        
        if (docStartIndex === -1 || docEndIndex === -1) {
            return { hasDoc: false, error: 'Missing doxygen documentation' };
        }
        
        // Extract the documentation block
        const docLines = lines.slice(docStartIndex, docEndIndex + 1);
        const docText = docLines.join('\n');
        
        // Check for required doxygen tags
        const hasBrief = /@brief\s+.+/.test(docText);
        const hasParam = /@param\s+\w+\s+.+/.test(docText) || /\(void\)/.test(lines[funcLineIndex]);
        const hasReturn = /@return\s+.+/.test(docText) || /^void\s+/.test(lines[funcLineIndex]);
        
        const errors = [];
        if (!hasBrief) errors.push('Missing @brief');
        if (!hasParam) errors.push('Missing @param for parameters');
        if (!hasReturn) errors.push('Missing @return');
        
        return {
            hasDoc: true,
            isValid: errors.length === 0,
            errors: errors
        };
    }

    /**
     * Extract function signatures from C file
     */
    extractFunctions(filePath) {
        if (!fs.existsSync(filePath)) return [];
        
        const content = fs.readFileSync(filePath, 'utf8');
        const functions = [];
        
        // Split into lines and process each potential function
        const lines = content.split('\n');
        
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
            
            // Check if this is followed by a { (might be on next line)
            let hasOpenBrace = false;
            for (let j = i; j < Math.min(lineIdx + 3, lines.length); j++) {
                if (lines[j].includes('{')) {
                    hasOpenBrace = true;
                    break;
                }
                // If we hit a semicolon, it's a declaration not definition
                if (lines[j].includes(';')) break;
            }
            
            if (!hasOpenBrace) continue;
            
            // Skip static functions
            if (line.trim().startsWith('static ')) continue;
            
            // Build the full signature
            const cleanReturnType = returnType.trim();
            const cleanParams = params.trim();
            const signature = `${cleanReturnType} ${name}(${cleanParams})`;
            
            // Check doxygen documentation
            const docCheck = this.checkDoxygenDoc(lines, i);
            
            // Check if it's a stub
            const isStub = this.isStubImplementation(lines, i, name);
            
            functions.push({ 
                name, 
                signature,
                lineNumber: i + 1,
                docCheck,
                isStub
            });
        }
        
        return functions;
    }

    /**
     * Validate a single file against manifest
     */
    validateFile(manifestPath, manifestFile) {
        const actualPath = path.join(this.projectRoot, manifestPath);
        
        // Check if file should be renamed
        if (manifestFile.rename_from) {
            const oldPath = path.join(this.projectRoot, manifestFile.rename_from);
            if (fs.existsSync(oldPath) && !fs.existsSync(actualPath)) {
                this.violations.push(`File needs renaming: ${manifestFile.rename_from} → ${manifestPath}`);
                return;
            }
        }
        
        // Check if file exists
        if (!fs.existsSync(actualPath)) {
            // Skip enforcement for files that don't exist yet (desired future files)
            return;
        }
        
        // Extract and validate functions
        const actualFunctions = this.extractFunctions(actualPath);
        const actualFuncMap = new Map(actualFunctions.map(f => [f.name, f]));
        
        // Check required functions
        for (const func of manifestFile.functions || []) {
            const actual = actualFuncMap.get(func.name);
            
            if (!actual) {
                this.violations.push(`Missing function in ${manifestPath}: ${func.name}`);
                continue;
            }
            
            // Normalize signatures for comparison
            // Remove __attribute__ annotations for comparison
            const normalizeSignature = (sig) => {
                return sig
                    .replace(/__attribute__\(\([^)]+\)\)/g, '') // Remove attributes
                    .replace(/\s+/g, ' ') // Normalize whitespace
                    .trim();
            };
            
            const expectedSig = normalizeSignature(func.signature);
            const actualSig = normalizeSignature(actual.signature);
            
            if (expectedSig !== actualSig) {
                this.violations.push(
                    `Function signature mismatch in ${manifestPath}:\n` +
                    `  Expected: ${func.signature}\n` +
                    `  Actual:   ${actual.signature}`
                );
            }
            
            // Check doxygen documentation
            if (!actual.docCheck.hasDoc) {
                this.violations.push(
                    `Missing doxygen documentation for function ${func.name} in ${manifestPath}:${actual.lineNumber}`
                );
            } else if (!actual.docCheck.isValid) {
                this.violations.push(
                    `Invalid doxygen documentation for function ${func.name} in ${manifestPath}:${actual.lineNumber}\n` +
                    `  Issues: ${actual.docCheck.errors.join(', ')}`
                );
            }
            
            // Check for stub implementation
            if (actual.isStub) {
                this.warnings.push(
                    `Stub implementation detected for function ${func.name} in ${manifestPath}:${actual.lineNumber}`
                );
            }
            
            actualFuncMap.delete(func.name);
        }
        
        // Check for unauthorized functions
        for (const [name, func] of actualFuncMap) {
            // Skip private functions if allowed
            if (manifestFile.private_functions?.includes(name)) continue;
            
            this.violations.push(
                `Unauthorized function in ${manifestPath}: ${name}\n` +
                `  Signature: ${func.signature}`
            );
        }
    }

    /**
     * Validate all files in the codebase
     */
    validateAllFiles(manifest) {
        // Check for unauthorized files
        const manifestFiles = new Set(Object.keys(manifest.files));
        const manifestHeaders = new Set(Object.keys(manifest.headers || {}));
        
        // Walk through src directory
        this.walkDirectory(path.join(this.projectRoot, 'src'), (file) => {
            const relativePath = path.relative(this.projectRoot, file);
            const isHeader = relativePath.endsWith('.h');
            const fileSet = isHeader ? manifestHeaders : manifestFiles;
            
            if (!fileSet.has(relativePath)) {
                this.violations.push(`Unauthorized file: ${relativePath}`);
            }
        });
        
        // Validate each manifest file
        for (const [filePath, fileSpec] of Object.entries(manifest.files)) {
            this.validateFile(filePath, fileSpec);
        }
        
        // Validate headers for doxygen documentation
        for (const [headerPath, headerSpec] of Object.entries(manifest.headers || {})) {
            this.validateHeader(headerPath, headerSpec);
        }
    }

    /**
     * Validate header file for doxygen documentation
     */
    validateHeader(headerPath, headerSpec) {
        const fullPath = path.join(this.projectRoot, headerPath);
        
        if (!fs.existsSync(fullPath)) {
            // Skip enforcement for headers that don't exist yet (desired future files)
            return;
        }
        
        const content = fs.readFileSync(fullPath, 'utf8');
        const lines = content.split('\n');
        
        // Find all function declarations
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i].trim();
            
            // Look for function declarations (ending with ;)
            if (/^\w[\w\s*]*\s+(\w+)\s*\([^)]*\)\s*;/.test(line)) {
                const funcMatch = line.match(/(\w+)\s*\(/);
                const funcName = funcMatch ? funcMatch[1] : 'unknown';
                
                // Check for doxygen comment above
                let hasDoc = false;
                for (let j = i - 1; j >= 0; j--) {
                    const prevLine = lines[j].trim();
                    if (prevLine.endsWith('*/')) {
                        // Check if it's a doxygen block
                        for (let k = j; k >= 0; k--) {
                            if (lines[k].trim().startsWith('/**')) {
                                hasDoc = true;
                                break;
                            }
                        }
                        break;
                    }
                    if (prevLine && !prevLine.startsWith('*') && !prevLine.startsWith('//')) {
                        break;
                    }
                }
                
                if (!hasDoc) {
                    this.violations.push(
                        `Missing doxygen documentation for function ${funcName} in ${headerPath}:${i + 1}`
                    );
                }
            }
        }
    }

    /**
     * Walk directory recursively
     */
    walkDirectory(dir, callback) {
        if (!fs.existsSync(dir)) return;
        
        const files = fs.readdirSync(dir);
        for (const file of files) {
            const fullPath = path.join(dir, file);
            const stat = fs.statSync(fullPath);
            
            if (stat.isDirectory()) {
                this.walkDirectory(fullPath, callback);
            } else if (file.endsWith('.c') || file.endsWith('.h')) {
                callback(fullPath);
            }
        }
    }

    /**
     * Generate stub files from manifest
     */
    generateStubs(manifest) {
        console.log('🔧 Generating code stubs from manifest...\n');
        
        for (const [filePath, fileSpec] of Object.entries(manifest.files)) {
            const fullPath = path.join(this.projectRoot, filePath);
            
            // Skip if file already exists
            if (fs.existsSync(fullPath)) continue;
            
            // Create directory if needed
            const dir = path.dirname(fullPath);
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
            }
            
            // Generate stub content
            const content = this.generateFileStub(filePath, fileSpec);
            fs.writeFileSync(fullPath, content);
            console.log(`✅ Generated: ${filePath}`);
        }
    }

    /**
     * Generate stub content for a file
     */
    generateFileStub(filePath, fileSpec) {
        const fileName = path.basename(filePath);
        const headerGuard = `ZEN_${path.basename(filePath, '.c').toUpperCase()}_H`;
        
        let content = `/*
 * ${fileName}
 * ${fileSpec.description}
 * 
 * This file is auto-generated from MANIFEST.json
 * DO NOT modify function signatures without updating the manifest
 */

`;

        // Add includes
        if (fileSpec.includes) {
            for (const inc of fileSpec.includes) {
                content += `#include "${inc}"\n`;
            }
            content += '\n';
        }

        // Add function stubs
        for (const func of fileSpec.functions || []) {
            // Generate proper doxygen documentation
            content += '/**\n';
            content += ` * @brief ${func.description}\n`;
            
            // Parse parameters from signature
            const paramMatch = func.signature.match(/\((.*)\)/);
            if (paramMatch) {
                const params = paramMatch[1].trim();
                if (params && params !== 'void') {
                    // Split parameters and extract names
                    const paramList = params.split(',').map(p => p.trim());
                    for (const param of paramList) {
                        // Extract parameter name (last word before any array brackets)
                        const paramName = param.replace(/\[.*\]/, '').trim().split(/\s+/).pop();
                        if (paramName && paramName !== '*') {
                            content += ` * @param ${paramName} TODO: Describe parameter\n`;
                        }
                    }
                }
            }
            
            // Add return documentation
            const returnMatch = func.signature.match(/^(\w[\w\s*]*)\s+\w+\s*\(/);
            if (returnMatch && returnMatch[1].trim() !== 'void') {
                content += ` * @return TODO: Describe return value\n`;
            }
            
            content += ' */\n';
            content += `${func.signature} {\n`;
            
            // Add return statement if needed
            if (func.returns) {
                if (func.returns.includes('*')) {
                    content += '    return NULL;  // TODO: Implement\n';
                } else if (func.returns === 'bool') {
                    content += '    return false;  // TODO: Implement\n';
                } else if (func.returns === 'void') {
                    content += '    // TODO: Implement\n';
                } else {
                    content += '    return 0;  // TODO: Implement\n';
                }
            }
            
            content += '}\n\n';
        }

        return content;
    }

    /**
     * Run enforcement check
     */
    enforce(options = {}) {
        const manifest = this.loadManifest();
        
        console.log('🔒 ZEN Manifest Enforcer');
        console.log('========================');
        console.log('Checking: Signatures, Files, and Doxygen Documentation\n');
        
        if (options.generate) {
            this.generateStubs(manifest);
            return true;
        }
        
        // Validate all files
        this.validateAllFiles(manifest);
        
        // Report results
        if (this.violations.length > 0) {
            console.log('❌ VIOLATIONS FOUND:\n');
            for (const violation of this.violations) {
                console.log(`  ⚠️  ${violation}`);
            }
        }
        
        if (this.warnings.length > 0) {
            console.log('\n⚠️  WARNINGS:\n');
            for (const warning of this.warnings) {
                console.log(`  ⚡ ${warning}`);
            }
        }
        
        if (this.violations.length > 0) {
            console.log('\n❌ Enforcement check FAILED!');
            console.log('\nTo fix:');
            console.log('1. Update code to match MANIFEST.json');
            console.log('2. OR update MANIFEST.json to match code');
            console.log('3. Run with --generate to create missing stubs');
            return false;
        }
        
        console.log('✅ All files and functions match manifest!');
        if (this.warnings.length > 0) {
            console.log(`   (${this.warnings.length} stub implementations detected)`);
        }
        return true;
    }
}

// Parse command line arguments
const args = process.argv.slice(2);
const options = {
    generate: args.includes('--generate'),
    fix: args.includes('--fix')
};

// Run enforcer
const projectRoot = process.cwd();
const enforcer = new ManifestEnforcer(projectRoot);

if (!enforcer.enforce(options)) {
    process.exit(1);
}