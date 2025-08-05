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
            /\{\s*\}/  // Empty function body
        ];

        // Extract function body
        const funcRegex = new RegExp(`${functionName}\\s*\\([^)]*\\)\\s*\\{([^}]*\\{[^}]*\\})*[^}]*\\}`, 'gs');
        const match = content.match(funcRegex);
        if (!match) return false;

        const functionBody = match[0];
        
        // Check if it's a stub
        for (const pattern of stubPatterns) {
            if (pattern.test(functionBody)) return true;
        }

        // Check if function body is too short (likely stub)
        const bodyContent = functionBody.substring(functionBody.indexOf('{') + 1, functionBody.lastIndexOf('}'));
        const meaningfulLines = bodyContent.split('\n').filter(line => {
            const trimmed = line.trim();
            return trimmed && !trimmed.startsWith('//') && !trimmed.startsWith('/*');
        });

        // Check if function body is too short (likely stub)
        // A function with just a single return statement calling another function is still minimal
        return meaningfulLines.length <= 1;
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

            // Check for doxygen documentation
            const lines = content.split('\n');
            let hasValidDoc = false;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes(func.name + '(')) {
                    // Look for doxygen comment above
                    for (let j = i - 1; j >= 0; j--) {
                        if (lines[j].trim().endsWith('*/')) {
                            const docBlock = [];
                            for (let k = j; k >= 0; k--) {
                                docBlock.unshift(lines[k]);
                                if (lines[k].trim().startsWith('/**')) break;
                            }
                            const docText = docBlock.join('\n');
                            hasValidDoc = docText.includes('@brief') && 
                                        (docText.includes('@param') || func.signature.includes('(void)')) &&
                                        (docText.includes('@return') || func.signature.startsWith('void '));
                            break;
                        }
                        if (!lines[j].trim().startsWith('*') && lines[j].trim()) break;
                    }
                    break;
                }
            }

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

        return { status: fileStatus, functions: functionStatuses };
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
                _functions: analysis.functions
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
                const progress = this.calculateProgress(node);
                const progressColor = this.getProgressColor(progress);
                console.log(prefix + connector + colors.blue + colors.bold + name + '/' + colors.reset + 
                          ' ' + progressColor + `[${progress}%]` + colors.reset);
            } else {
                const color = this.getStatusColor(node._status);
                const symbol = this.getStatusSymbol(node._status);
                const progress = this.calculateProgress(node);
                const progressColor = this.getProgressColor(progress);
                const desc = node._spec?.description ? colors.gray + ' # ' + node._spec.description + colors.reset : '';
                
                // Check if an agent is working on this file
                const activeTask = this.activeTasks.get(node._path);
                const agentIndicator = activeTask 
                    ? ` ${colors.cyan}[${activeTask.agent}]${colors.reset}` 
                    : '';
                
                console.log(prefix + connector + color + symbol + ' ' + name + colors.reset + 
                          ' ' + progressColor + `[${progress}%]` + colors.reset + 
                          agentIndicator + desc);
                
                // Print functions if it's a file with functions
                if (node._type === 'file' && node._functions && Object.keys(node._functions).length > 0) {
                    const funcPrefix = prefix + (isLast ? tree.empty : tree.vertical) + tree.empty;
                    const funcs = Object.entries(node._functions);
                    
                    funcs.forEach(([funcName, funcStatus], index) => {
                        const isLastFunc = index === funcs.length - 1;
                        const funcConnector = isLastFunc ? tree.lastBranch : tree.branch;
                        const funcColor = this.getStatusColor(funcStatus);
                        const funcSymbol = this.getStatusSymbol(funcStatus);
                        const funcDesc = node._spec.functions.find(f => f.name === funcName)?.description || '';
                        
                        console.log(funcPrefix + funcConnector + funcColor + funcSymbol + ' ' + funcName + '()' + 
                                  colors.reset + colors.gray + ' # ' + funcDesc + colors.reset);
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