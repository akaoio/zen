#!/usr/bin/env node

/**
 * ZEN Project Structure Validator
 * 
 * This script validates that the codebase follows the structure defined in .zenproject
 * Run this as a pre-commit hook or CI step to enforce project standards.
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

class ProjectValidator {
    constructor(projectRoot) {
        this.projectRoot = projectRoot;
        this.configFile = path.join(projectRoot, '.zenproject');
        this.errors = [];
        this.warnings = [];
    }

    loadConfig() {
        if (!fs.existsSync(this.configFile)) {
            this.errors.push('Missing .zenproject configuration file');
            return null;
        }

        try {
            const content = fs.readFileSync(this.configFile, 'utf8');
            // Simple YAML parser for our specific format
            return this.parseYAML(content);
        } catch (error) {
            this.errors.push(`Failed to load config: ${error.message}`);
            return null;
        }
    }

    parseYAML(content) {
        // Basic YAML parser for our configuration structure
        const config = {
            structure: {
                required_directories: [],
                required_files: [],
                file_patterns: {}
            },
            development: {
                required_tools: []
            }
        };

        const lines = content.split('\n');
        let currentSection = null;
        let currentSubsection = null;
        let indent = 0;

        for (const line of lines) {
            // Skip comments and empty lines
            if (line.trim().startsWith('#') || line.trim() === '') continue;

            const leadingSpaces = line.length - line.trimStart().length;
            const trimmed = line.trim();

            // Parse structure section
            if (trimmed === 'required_directories:') {
                currentSection = 'required_directories';
                continue;
            }
            if (trimmed === 'required_files:') {
                currentSection = 'required_files';
                continue;
            }
            if (trimmed === 'file_patterns:') {
                currentSection = 'file_patterns';
                continue;
            }
            if (trimmed === 'required_tools:') {
                currentSection = 'required_tools';
                continue;
            }

            // Parse list items
            if (trimmed.startsWith('- ')) {
                const value = trimmed.substring(2).replace(/"/g, '');
                if (currentSection === 'required_directories') {
                    config.structure.required_directories.push(value);
                } else if (currentSection === 'required_files') {
                    config.structure.required_files.push(value);
                } else if (currentSection === 'required_tools') {
                    config.development.required_tools.push(value);
                }
            }

            // Parse file patterns
            if (currentSection === 'file_patterns' && trimmed.includes(':')) {
                const [key, value] = trimmed.split(':').map(s => s.trim().replace(/"/g, ''));
                config.structure.file_patterns[key] = value;
            }
        }

        return config;
    }

    validateDirectories(config) {
        const requiredDirs = config.structure.required_directories;
        
        for (const dirPath of requiredDirs) {
            const fullPath = path.join(this.projectRoot, dirPath);
            if (!fs.existsSync(fullPath)) {
                this.errors.push(`Missing required directory: ${dirPath}`);
            } else if (!fs.statSync(fullPath).isDirectory()) {
                this.errors.push(`Path exists but is not a directory: ${dirPath}`);
            }
        }
    }

    validateFiles(config) {
        const requiredFiles = config.structure.required_files;
        
        for (const filePath of requiredFiles) {
            const fullPath = path.join(this.projectRoot, filePath);
            if (!fs.existsSync(fullPath)) {
                this.errors.push(`Missing required file: ${filePath}`);
            } else if (!fs.statSync(fullPath).isFile()) {
                this.errors.push(`Path exists but is not a file: ${filePath}`);
            }
        }
    }

    validateNamingPatterns(config) {
        const patterns = config.structure.file_patterns;
        
        // Check source files
        if (patterns.source_files) {
            const srcPattern = new RegExp(patterns.source_files);
            this.walkDirectory(path.join(this.projectRoot, 'src'), '.c', (file) => {
                if (!srcPattern.test(path.basename(file))) {
                    this.warnings.push(`Source file doesn't follow naming convention: ${path.basename(file)}`);
                }
            });
        }

        // Check header files
        if (patterns.header_files) {
            const headerPattern = new RegExp(patterns.header_files);
            this.walkDirectory(path.join(this.projectRoot, 'src'), '.h', (file) => {
                if (!headerPattern.test(path.basename(file))) {
                    this.warnings.push(`Header file doesn't follow naming convention: ${path.basename(file)}`);
                }
            });
        }

        // Check test files
        if (patterns.test_files) {
            const testPattern = new RegExp(patterns.test_files);
            const testDir = path.join(this.projectRoot, 'tests', 'unit');
            if (fs.existsSync(testDir)) {
                fs.readdirSync(testDir).forEach(file => {
                    if (file.endsWith('.c') && !testPattern.test(file)) {
                        this.warnings.push(`Test file doesn't follow naming convention: ${file}`);
                    }
                });
            }
        }
    }

    walkDirectory(dir, ext, callback) {
        if (!fs.existsSync(dir)) return;
        
        const files = fs.readdirSync(dir);
        for (const file of files) {
            const fullPath = path.join(dir, file);
            const stat = fs.statSync(fullPath);
            
            if (stat.isDirectory()) {
                this.walkDirectory(fullPath, ext, callback);
            } else if (file.endsWith(ext)) {
                callback(fullPath);
            }
        }
    }

    validateTestCoverage() {
        const srcFiles = new Set();
        const srcDir = path.join(this.projectRoot, 'src');
        
        this.walkDirectory(srcDir, '.c', (file) => {
            const basename = path.basename(file, '.c');
            if (basename !== 'main') {
                srcFiles.add(basename);
            }
        });

        const testFiles = new Set();
        const testDir = path.join(this.projectRoot, 'tests', 'unit');
        if (fs.existsSync(testDir)) {
            fs.readdirSync(testDir).forEach(file => {
                if (file.endsWith('_test.c')) {
                    testFiles.add(file.replace('_test.c', ''));
                }
            });
        }

        for (const srcFile of srcFiles) {
            if (!testFiles.has(srcFile)) {
                this.warnings.push(`Missing unit test for: ${srcFile}.c`);
            }
        }
    }

    validateTools(config) {
        const requiredTools = config.development.required_tools;
        
        for (const tool of requiredTools) {
            try {
                execSync(`which ${tool}`, { stdio: 'ignore' });
            } catch (e) {
                this.warnings.push(`Required tool not found in PATH: ${tool}`);
            }
        }
    }

    checkFileHeaders() {
        this.walkDirectory(path.join(this.projectRoot, 'src'), '.c', (file) => {
            const content = fs.readFileSync(file, 'utf8');
            if (!content.startsWith('/*')) {
                const relativePath = path.relative(this.projectRoot, file);
                this.warnings.push(`Missing file header comment: ${relativePath}`);
            }
        });
    }

    validate() {
        const config = this.loadConfig();
        if (!config) {
            return false;
        }

        console.log('🔍 Validating ZEN project structure...');

        // Run validations
        this.validateDirectories(config);
        this.validateFiles(config);
        this.validateNamingPatterns(config);
        this.validateTestCoverage();
        this.validateTools(config);
        this.checkFileHeaders();

        // Report results
        if (this.errors.length > 0) {
            console.log('\n❌ ERRORS (must fix):');
            for (const error of this.errors) {
                console.log(`   - ${error}`);
            }
        }

        if (this.warnings.length > 0) {
            console.log('\n⚠️  WARNINGS (should fix):');
            for (const warning of this.warnings) {
                console.log(`   - ${warning}`);
            }
        }

        if (this.errors.length === 0 && this.warnings.length === 0) {
            console.log('\n✅ Project structure is valid!');
        }

        return this.errors.length === 0;
    }
}

function findProjectRoot() {
    let currentDir = process.cwd();
    
    while (currentDir !== path.dirname(currentDir)) {
        if (fs.existsSync(path.join(currentDir, '.zenproject'))) {
            return currentDir;
        }
        currentDir = path.dirname(currentDir);
    }
    
    console.error('❌ Error: Could not find .zenproject file in current or parent directories');
    process.exit(1);
}

// Main
const projectRoot = findProjectRoot();
const validator = new ProjectValidator(projectRoot);

if (!validator.validate()) {
    process.exit(1);
}