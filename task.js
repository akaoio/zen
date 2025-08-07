#!/usr/bin/env node

/**
 * Task Management Utility for ZEN Swarm Agents
 * 
 * This utility helps agents create and update task files with proper format
 * and activity tracking. It simplifies task management for all agent types.
 * 
 * Usage:
 *   node task.js create <agent-id> <description> [files...]
 *   node task.js activity <task-file> <method> [--success|--fail <reason>]
 *   node task.js complete <task-file> [--success|--fail] <reason>
 *   node task.js status <task-file>
 *   node task.js list [--active|--completed|--failed]
 */

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

class TaskManager {
    constructor() {
        this.tasksDir = path.join(__dirname, 'tasks');
        this.ensureTasksDir();
    }

    ensureTasksDir() {
        if (!fs.existsSync(this.tasksDir)) {
            fs.mkdirSync(this.tasksDir, { recursive: true });
        }
    }

    generateTimestamp() {
        const now = new Date();
        const year = now.getFullYear();
        const month = String(now.getMonth() + 1).padStart(2, '0');
        const day = String(now.getDate()).padStart(2, '0');
        const hour = String(now.getHours()).padStart(2, '0');
        const minute = String(now.getMinutes()).padStart(2, '0');
        return `${year}${month}${day}-${hour}${minute}`;
    }

    getUnixTime() {
        return Math.floor(Date.now() / 1000);
    }

    createTask(agentId, description, files = []) {
        const timestamp = this.generateTimestamp();
        const unixTime = this.getUnixTime();
        const filename = `${timestamp}.yaml`;
        const filepath = path.join(this.tasksDir, filename);

        // Check if file already exists (same minute)
        if (fs.existsSync(filepath)) {
            // Add seconds to make it unique
            const seconds = String(new Date().getSeconds()).padStart(2, '0');
            const newFilename = `${timestamp}-${seconds}.yaml`;
            const newFilepath = path.join(this.tasksDir, newFilename);
            return this.createTaskFile(newFilepath, agentId, description, files, unixTime);
        }

        return this.createTaskFile(filepath, agentId, description, files, unixTime);
    }

    createTaskFile(filepath, agentId, description, files, unixTime) {
        const task = {
            agent: agentId,
            task: description,
            created: unixTime,
            completed: false,
            files: files.length > 0 ? files : ['TBD'],
            activities: [{
                timestamp: unixTime,
                start: unixTime,
                end: 0,
                method: 'Task initialized',
                success: false,
                fail: false,
                why_success: 'In progress',
                why_fail: 'Not completed yet'
            }]
        };

        const yamlContent = yaml.dump(task, { 
            lineWidth: -1,
            noRefs: true,
            sortKeys: false 
        });

        fs.writeFileSync(filepath, yamlContent);
        return filepath;
    }

    loadTask(taskFile) {
        const filepath = path.join(this.tasksDir, path.basename(taskFile));
        if (!fs.existsSync(filepath)) {
            throw new Error(`Task file not found: ${taskFile}`);
        }
        const content = fs.readFileSync(filepath, 'utf8');
        return { data: yaml.load(content), filepath };
    }

    saveTask(filepath, taskData) {
        // Clean the task data to prevent YAML corruption
        const cleanData = this.cleanTaskData(taskData);
        
        const yamlContent = yaml.dump(cleanData, { 
            lineWidth: -1,
            noRefs: true,
            sortKeys: false,
            flowLevel: -1,
            indent: 2
        });
        
        // Ensure clean YAML without shell artifacts
        const cleanYaml = yamlContent
            .replace(/EOF.*$/gm, '')  // Remove EOF markers
            .replace(/^\s*<.*$/gm, '')  // Remove shell redirects
            .replace(/\n\s*\n\s*\n/g, '\n\n')  // Remove excessive newlines
            .trim();
            
        fs.writeFileSync(filepath, cleanYaml + '\n');
    }

    cleanTaskData(data) {
        // Deep clean the data structure
        const cleanData = JSON.parse(JSON.stringify(data));
        
        // Clean strings that might contain shell artifacts
        const cleanString = (str) => {
            if (typeof str !== 'string') return str;
            return str
                .replace(/EOF.*$/g, '')
                .replace(/<.*$/g, '')
                .replace(/\s+/g, ' ')
                .trim();
        };
        
        // Recursively clean all strings in the object
        const deepClean = (obj) => {
            if (typeof obj === 'string') {
                return cleanString(obj);
            } else if (Array.isArray(obj)) {
                return obj.map(deepClean);
            } else if (obj && typeof obj === 'object') {
                const result = {};
                for (const [key, value] of Object.entries(obj)) {
                    result[key] = deepClean(value);
                }
                return result;
            }
            return obj;
        };
        
        return deepClean(cleanData);
    }

    addActivity(taskFile, method, options = {}) {
        const { data, filepath } = this.loadTask(taskFile);
        const unixTime = this.getUnixTime();

        // Ensure activities array exists (for compatibility with old format)
        if (!data.activities) {
            data.activities = [];
            // Convert old steps format if exists
            if (data.steps) {
                data.activities = data.steps;
                delete data.steps;
            }
        }

        const activity = {
            timestamp: unixTime,
            start: unixTime,
            end: options.end || 0,
            method: method,
            success: options.success || false,
            fail: options.fail || false,
            why_success: options.why_success || 'In progress',
            why_fail: options.why_fail || 'Not completed yet'
        };

        data.activities.push(activity);
        this.saveTask(filepath, data);
        return filepath;
    }

    completeTask(taskFile, success = true, reason = '') {
        const { data, filepath } = this.loadTask(taskFile);
        const unixTime = this.getUnixTime();

        data.completed = true;

        // Update the last activity
        if (data.activities && data.activities.length > 0) {
            const lastActivity = data.activities[data.activities.length - 1];
            lastActivity.end = unixTime;
            lastActivity.success = success;
            lastActivity.fail = !success;
            if (success) {
                lastActivity.why_success = reason || 'Task completed successfully';
                lastActivity.why_fail = '';
            } else {
                lastActivity.why_fail = reason || 'Task failed';
                lastActivity.why_success = '';
            }
        }

        this.saveTask(filepath, data);
        return filepath;
    }

    getTaskStatus(taskFile) {
        const { data } = this.loadTask(taskFile);
        const activities = data.activities || data.steps || [];
        
        return {
            agent: data.agent,
            task: data.task,
            created: new Date(data.created * 1000).toISOString(),
            completed: data.completed,
            files: data.files,
            activityCount: activities.length,
            lastActivity: activities.length > 0 ? activities[activities.length - 1] : null,
            duration: this.calculateDuration(data)
        };
    }

    calculateDuration(taskData) {
        if (!taskData.activities || taskData.activities.length === 0) return 0;
        
        const firstActivity = taskData.activities[0];
        const lastActivity = taskData.activities[taskData.activities.length - 1];
        
        const endTime = lastActivity.end || this.getUnixTime();
        const duration = endTime - firstActivity.start;
        
        return this.formatDuration(duration);
    }

    formatDuration(seconds) {
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;

        if (hours > 0) {
            return `${hours}h ${minutes}m`;
        } else if (minutes > 0) {
            return `${minutes}m ${secs}s`;
        } else {
            return `${secs}s`;
        }
    }

    listTasks(filter = 'all') {
        const files = fs.readdirSync(this.tasksDir)
            .filter(f => f.endsWith('.yaml'))
            .sort();

        const tasks = [];
        const errors = [];
        
        for (const file of files) {
            try {
                const { data } = this.loadTask(file);
                const include = 
                    filter === 'all' ||
                    (filter === 'active' && !data.completed) ||
                    (filter === 'completed' && data.completed && this.isTaskSuccessful(data)) ||
                    (filter === 'failed' && data.completed && !this.isTaskSuccessful(data));

                if (include) {
                    tasks.push({
                        file,
                        agent: data.agent,
                        task: data.task,
                        created: new Date(data.created * 1000),
                        completed: data.completed,
                        success: this.isTaskSuccessful(data)
                    });
                }
            } catch (e) {
                errors.push({ file, error: e.message });
            }
        }

        if (filter === 'all' && errors.length > 0) {
            console.warn(`⚠️  ${errors.length} corrupted task files found. Use 'node task.js cleanup' to fix.`);
        }

        return tasks;
    }

    cleanupCorruptedTasks() {
        const files = fs.readdirSync(this.tasksDir)
            .filter(f => f.endsWith('.yaml'));

        const corrupted = [];
        const fixed = [];

        for (const file of files) {
            const filepath = path.join(this.tasksDir, file);
            try {
                // Try to load the file
                this.loadTask(file);
            } catch (e) {
                corrupted.push(file);
                
                try {
                    // Try to fix common YAML issues
                    let content = fs.readFileSync(filepath, 'utf8');
                    
                    // Remove shell artifacts
                    content = content
                        .replace(/EOF.*$/gm, '')
                        .replace(/^\s*<.*$/gm, '')
                        .replace(/\n\s*\n\s*\n/g, '\n\n')
                        .trim();
                    
                    // Fix indentation issues
                    const lines = content.split('\n');
                    const fixedLines = [];
                    let inActivities = false;
                    
                    for (const line of lines) {
                        if (line.trim() === 'activities:') {
                            inActivities = true;
                            fixedLines.push(line);
                        } else if (inActivities && line.match(/^\s*- timestamp:/)) {
                            fixedLines.push('  ' + line.trim());
                        } else if (inActivities && line.match(/^\s*(start|end|method|success|fail|why_success|why_fail):/)) {
                            fixedLines.push('    ' + line.trim());
                        } else {
                            fixedLines.push(line);
                        }
                    }
                    
                    const fixedContent = fixedLines.join('\n');
                    fs.writeFileSync(filepath, fixedContent);
                    
                    // Verify the fix worked
                    yaml.load(fixedContent);
                    fixed.push(file);
                } catch (fixError) {
                    // If we can't fix it, rename it so it doesn't break the system
                    const backupPath = filepath + '.corrupted';
                    fs.renameSync(filepath, backupPath);
                    console.warn(`⚠️  Moved corrupted file ${file} to ${file}.corrupted`);
                }
            }
        }

        return { corrupted: corrupted.length, fixed: fixed.length };
    }

    archiveOldTasks(daysCutoff) {
        const cutoffTime = Date.now() - (daysCutoff * 24 * 60 * 60 * 1000);
        const files = fs.readdirSync(this.tasksDir)
            .filter(f => f.endsWith('.yaml'));

        let archived = 0;
        let remaining = 0;

        for (const file of files) {
            try {
                const { data, filepath } = this.loadTask(file);
                const taskAge = data.created * 1000; // Convert to milliseconds
                
                if (taskAge < cutoffTime && !data.completed) {
                    // Mark old incomplete tasks as completed with archived status
                    data.completed = true;
                    
                    // Add completion activity
                    if (!data.activities) data.activities = [];
                    data.activities.push({
                        timestamp: Math.floor(Date.now() / 1000),
                        start: Math.floor(Date.now() / 1000),
                        end: Math.floor(Date.now() / 1000),
                        method: `Auto-archived after ${daysCutoff} days`,
                        success: false,
                        fail: true,
                        why_success: '',
                        why_fail: `Task automatically archived due to age (>${daysCutoff} days old)`
                    });
                    
                    this.saveTask(filepath, data);
                    archived++;
                } else if (!data.completed) {
                    remaining++;
                }
            } catch (e) {
                // Skip corrupted files
            }
        }

        return { archived, remaining };
    }

    isTaskSuccessful(taskData) {
        if (!taskData.activities || taskData.activities.length === 0) return false;
        const lastActivity = taskData.activities[taskData.activities.length - 1];
        return lastActivity.success === true;
    }
}

// CLI Interface
function main() {
    const args = process.argv.slice(2);
    const command = args[0];
    const manager = new TaskManager();

    try {
        switch (command) {
            case 'create': {
                const [, agentId, description, ...files] = args;
                if (!agentId || !description) {
                    console.error('Usage: node task.js create <agent-id> <description> [files...]');
                    process.exit(1);
                }
                const filepath = manager.createTask(agentId, description, files);
                console.log(`Created task: ${path.basename(filepath)}`);
                break;
            }

            case 'activity': {
                const [, taskFile, method, ...options] = args;
                if (!taskFile || !method) {
                    console.error('Usage: node task.js activity <task-file> <method> [--success|--fail <reason>]');
                    process.exit(1);
                }

                const opts = {};
                for (let i = 0; i < options.length; i++) {
                    if (options[i] === '--success') {
                        opts.success = true;
                        opts.why_success = options[i + 1] || 'Activity successful';
                        i++;
                    } else if (options[i] === '--fail') {
                        opts.fail = true;
                        opts.why_fail = options[i + 1] || 'Activity failed';
                        i++;
                    }
                }

                manager.addActivity(taskFile, method, opts);
                console.log(`Added activity to: ${taskFile}`);
                break;
            }

            case 'complete': {
                const [, taskFile, status, ...reason] = args;
                if (!taskFile) {
                    console.error('Usage: node task.js complete <task-file> [--success|--fail] <reason>');
                    process.exit(1);
                }

                const success = status !== '--fail';
                const reasonText = reason.join(' ');
                manager.completeTask(taskFile, success, reasonText);
                console.log(`Task completed: ${taskFile} (${success ? 'SUCCESS' : 'FAILED'})`);
                break;
            }

            case 'status': {
                const [, taskFile] = args;
                if (!taskFile) {
                    console.error('Usage: node task.js status <task-file>');
                    process.exit(1);
                }

                const status = manager.getTaskStatus(taskFile);
                console.log('\nTask Status:');
                console.log(`  Agent: ${status.agent}`);
                console.log(`  Task: ${status.task}`);
                console.log(`  Created: ${status.created}`);
                console.log(`  Completed: ${status.completed}`);
                console.log(`  Files: ${status.files.join(', ')}`);
                console.log(`  Activities: ${status.activityCount}`);
                console.log(`  Duration: ${status.duration}`);
                
                if (status.lastActivity) {
                    console.log('\nLast Activity:');
                    console.log(`  Method: ${status.lastActivity.method}`);
                    console.log(`  Status: ${status.lastActivity.success ? 'SUCCESS' : status.lastActivity.fail ? 'FAILED' : 'IN PROGRESS'}`);
                }
                break;
            }

            case 'list': {
                const filter = args[1]?.replace('--', '') || 'all';
                const tasks = manager.listTasks(filter);
                
                console.log(`\n${filter.toUpperCase()} TASKS (${tasks.length}):\n`);
                
                for (const task of tasks) {
                    const status = task.completed ? (task.success ? '✓' : '✗') : '○';
                    const date = task.created.toISOString().split('T')[0];
                    console.log(`${status} ${task.file} - ${task.agent} - ${date}`);
                    console.log(`  ${task.task}\n`);
                }
                break;
            }

            case 'cleanup': {
                console.log('🔧 Cleaning up corrupted task files...');
                const result = manager.cleanupCorruptedTasks();
                console.log(`✅ Cleanup complete:`);
                console.log(`   - Corrupted files found: ${result.corrupted}`);
                console.log(`   - Files fixed: ${result.fixed}`);
                break;
            }

            case 'archive-old': {
                const daysCutoff = parseInt(args[1]) || 2;
                console.log(`📦 Archiving tasks older than ${daysCutoff} days...`);
                const result = manager.archiveOldTasks(daysCutoff);
                console.log(`✅ Archive complete:`);
                console.log(`   - Tasks archived: ${result.archived}`);
                console.log(`   - Active tasks remaining: ${result.remaining}`);
                break;
            }

            case 'help':
            default: {
                console.log(`
Task Management Utility for ZEN Swarm Agents

Usage:
  node task.js create <agent-id> <description> [files...]
    Create a new task file with timestamp
    
  node task.js activity <task-file> <method> [--success|--fail <reason>]
    Add an activity to an existing task
    
  node task.js complete <task-file> [--success|--fail] <reason>
    Mark a task as completed with success or failure
    
  node task.js status <task-file>
    Show detailed status of a task
    
  node task.js list [--active|--completed|--failed]
    List tasks with optional filter

Examples:
  # Create a new task
  node task.js create swarm-1-zen-worker-lexer "Implement lexer_scan_number" src/core/lexer.c

  # Add an activity
  node task.js activity 20250805-1430.yaml "Implementing integer parsing"
  
  # Add a successful activity
  node task.js activity 20250805-1430.yaml "Completed integer parsing" --success "All tests pass"
  
  # Complete a task successfully
  node task.js complete 20250805-1430.yaml --success "Implemented number scanning with float support"
  
  # Complete a task with failure
  node task.js complete 20250805-1430.yaml --fail "Blocked by missing AST node types"
  
  # Check task status
  node task.js status 20250805-1430.yaml
  
  # List all active tasks
  node task.js list --active
`);
                break;
            }
        }
    } catch (error) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

// Check if js-yaml is installed
try {
    require('js-yaml');
} catch (e) {
    console.error('Error: js-yaml is required. Install it with:');
    console.error('npm install js-yaml');
    process.exit(1);
}

// Run CLI if called directly
if (require.main === module) {
    main();
}

// Export for use as module
module.exports = TaskManager;// Integration with validation framework available
