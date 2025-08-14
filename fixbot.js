#!/usr/bin/env node

/**
 * FixBot - Automated issue finder and fixer for the ZEN project
 * This bot continuously monitors GitHub issues and attempts to fix them using Claude
 */

const { exec, spawn } = require('child_process');
const util = require('util');
const execPromise = util.promisify(exec);

// Configuration
const POLL_INTERVAL = 60000; // Check for issues every 60 seconds
const TEST_INTERVAL = 300000; // Run tests every 5 minutes
const BOT_NAME = 'fixbot';
const PROJECT_PATH = process.cwd();

class FixBot {
    constructor() {
        this.isProcessingIssue = false;
        this.processedIssues = new Set();
    }

    /**
     * Start the bot's main loop
     */
    async start() {
        console.log('> FixBot starting...');
        console.log(`=Á Project path: ${PROJECT_PATH}`);
        
        // Initial run
        await this.checkAndFixIssues();
        await this.runTestsAndCreateIssues();
        
        // Set up polling intervals
        setInterval(() => this.checkAndFixIssues(), POLL_INTERVAL);
        setInterval(() => this.runTestsAndCreateIssues(), TEST_INTERVAL);
        
        console.log('= FixBot is now running in continuous mode...');
    }

    /**
     * Check for open issues and attempt to fix them
     */
    async checkAndFixIssues() {
        if (this.isProcessingIssue) {
            console.log('ó Already processing an issue, skipping this cycle...');
            return;
        }

        try {
            console.log('\n= Checking for open issues...');
            
            // Get list of open issues
            const { stdout } = await execPromise('gh issue list --state open --json number,title,body,assignees --limit 10');
            const issues = JSON.parse(stdout);
            
            if (issues.length === 0) {
                console.log(' No open issues found');
                return;
            }
            
            // Find an unassigned issue or one assigned to fixbot
            const issueToFix = issues.find(issue => {
                const isUnassigned = !issue.assignees || issue.assignees.length === 0;
                const isAssignedToBot = issue.assignees?.some(a => a.login === BOT_NAME);
                const notProcessed = !this.processedIssues.has(issue.number);
                
                return notProcessed && (isUnassigned || isAssignedToBot);
            });
            
            if (!issueToFix) {
                console.log('=Ë All issues are either assigned or already processed');
                return;
            }
            
            console.log(`\n<¯ Found issue #${issueToFix.number}: ${issueToFix.title}`);
            await this.processIssue(issueToFix);
            
        } catch (error) {
            console.error('L Error checking issues:', error.message);
        }
    }

    /**
     * Process a single issue
     */
    async processIssue(issue) {
        this.isProcessingIssue = true;
        
        try {
            // Comment that we're working on it
            await this.commentOnIssue(issue.number, "> FixBot is analyzing and attempting to fix this issue...");
            
            // Assign the issue to fixbot (if we have permissions)
            try {
                await execPromise(`gh issue edit ${issue.number} --add-assignee ${BOT_NAME}`);
                console.log(` Assigned issue #${issue.number} to ${BOT_NAME}`);
            } catch (e) {
                console.log(`9 Could not assign issue (might not have permissions)`);
            }
            
            // Prepare the prompt for Claude
            const prompt = this.preparePrompt(issue);
            
            // Fix the issue using Claude
            console.log('=' Attempting to fix with Claude...');
            const fixResult = await this.runClaude(prompt);
            
            // Analyze the fix result
            const analysis = await this.analyzeFix(issue.number);
            
            if (analysis.isFixed) {
                await this.handleFixedIssue(issue.number, analysis);
            } else if (analysis.isPartiallyFixed) {
                await this.handlePartiallyFixedIssue(issue.number, analysis);
            } else {
                await this.handleFailedFix(issue.number, analysis);
            }
            
            this.processedIssues.add(issue.number);
            
        } catch (error) {
            console.error(`L Error processing issue #${issue.number}:`, error.message);
            await this.commentOnIssue(issue.number, 
                `L FixBot encountered an error while trying to fix this issue:\n\`\`\`\n${error.message}\n\`\`\``);
        } finally {
            this.isProcessingIssue = false;
        }
    }

    /**
     * Prepare the prompt for Claude
     */
    preparePrompt(issue) {
        return `You are FixBot, an automated issue fixer for the ZEN language project.

GitHub Issue #${issue.number}: ${issue.title}

Issue Description:
${issue.body}

Your task:
1. Analyze this issue carefully
2. Fix the problem by modifying the appropriate files
3. Test your fix thoroughly
4. Report whether the issue is fully fixed, partially fixed, or if new issues were discovered

Please fix this issue completely. After making changes, run appropriate tests to verify the fix works.
If you encounter new bugs while fixing, make note of them for creating new issues.

IMPORTANT: 
- Focus on fixing the root cause, not workarounds
- Test your changes thoroughly
- If the issue is only partially fixed, explain what still needs work`;
    }

    /**
     * Run Claude to fix the issue
     */
    async runClaude(prompt) {
        return new Promise((resolve, reject) => {
            const claude = spawn('claude', ['--dangerously-skip-permissions', '-p', prompt], {
                cwd: PROJECT_PATH,
                stdio: ['ignore', 'pipe', 'pipe']
            });
            
            let stdout = '';
            let stderr = '';
            
            claude.stdout.on('data', (data) => {
                stdout += data.toString();
                process.stdout.write(data); // Show Claude's output in real-time
            });
            
            claude.stderr.on('data', (data) => {
                stderr += data.toString();
            });
            
            claude.on('close', (code) => {
                if (code === 0) {
                    resolve({ stdout, stderr });
                } else {
                    reject(new Error(`Claude exited with code ${code}: ${stderr}`));
                }
            });
            
            claude.on('error', (err) => {
                reject(err);
            });
        });
    }

    /**
     * Analyze whether the fix was successful
     */
    async analyzeFix(issueNumber) {
        try {
            // Run tests to check if the issue is fixed
            console.log('\n>ê Running tests to verify fix...');
            
            // Run make test
            const testResult = await this.runTests();
            
            // Check git diff to see what changed
            const { stdout: gitDiff } = await execPromise('git diff --stat');
            
            return {
                isFixed: testResult.success && testResult.allTestsPassed,
                isPartiallyFixed: testResult.success && !testResult.allTestsPassed,
                testOutput: testResult.output,
                changedFiles: gitDiff,
                errors: testResult.errors || []
            };
            
        } catch (error) {
            return {
                isFixed: false,
                isPartiallyFixed: false,
                errors: [error.message]
            };
        }
    }

    /**
     * Run project tests
     */
    async runTests() {
        try {
            const { stdout, stderr } = await execPromise('make test', { 
                timeout: 120000 // 2 minute timeout
            });
            
            const allTestsPassed = stdout.includes('All tests passed') || 
                                  !stdout.includes('FAILED') && 
                                  !stderr.includes('Error');
            
            return {
                success: true,
                allTestsPassed,
                output: stdout,
                errors: stderr ? [stderr] : []
            };
        } catch (error) {
            return {
                success: false,
                allTestsPassed: false,
                output: error.stdout || '',
                errors: [error.message]
            };
        }
    }

    /**
     * Handle a successfully fixed issue
     */
    async handleFixedIssue(issueNumber, analysis) {
        console.log(` Issue #${issueNumber} appears to be fixed!`);
        
        const comment = ` **FixBot has successfully fixed this issue!**

**Changed files:**
\`\`\`
${analysis.changedFiles}
\`\`\`

**Test results:**
All tests passed successfully.

This issue has been automatically resolved. Closing now.`;
        
        await this.commentOnIssue(issueNumber, comment);
        
        // Close the issue
        try {
            await execPromise(`gh issue close ${issueNumber}`);
            console.log(` Closed issue #${issueNumber}`);
        } catch (e) {
            console.log(`  Could not close issue #${issueNumber}: ${e.message}`);
        }
    }

    /**
     * Handle a partially fixed issue
     */
    async handlePartiallyFixedIssue(issueNumber, analysis) {
        console.log(`  Issue #${issueNumber} is partially fixed`);
        
        const comment = `  **FixBot has partially fixed this issue**

**Changed files:**
\`\`\`
${analysis.changedFiles}
\`\`\`

**Test results:**
Some tests are still failing. Will continue working on this issue.

${analysis.errors.length > 0 ? `**Errors encountered:**\n\`\`\`\n${analysis.errors.join('\n')}\n\`\`\`` : ''}

Continuing to work on remaining issues...`;
        
        await this.commentOnIssue(issueNumber, comment);
        
        // Don't mark as processed so we try again next cycle
        this.processedIssues.delete(issueNumber);
    }

    /**
     * Handle a failed fix attempt
     */
    async handleFailedFix(issueNumber, analysis) {
        console.log(`L Failed to fix issue #${issueNumber}`);
        
        const comment = `L **FixBot was unable to fix this issue**

${analysis.errors.length > 0 ? `**Errors encountered:**\n\`\`\`\n${analysis.errors.join('\n')}\n\`\`\`` : ''}

This issue requires human intervention.`;
        
        await this.commentOnIssue(issueNumber, comment);
    }

    /**
     * Comment on a GitHub issue
     */
    async commentOnIssue(issueNumber, comment) {
        try {
            await execPromise(`gh issue comment ${issueNumber} --body "${comment.replace(/"/g, '\\"')}"`);
            console.log(`=¬ Commented on issue #${issueNumber}`);
        } catch (error) {
            console.error(`L Failed to comment on issue #${issueNumber}:`, error.message);
        }
    }

    /**
     * Run tests and create issues for any failures
     */
    async runTestsAndCreateIssues() {
        console.log('\n>ê Running comprehensive tests to find issues...');
        
        try {
            // Run different test suites
            const testSuites = [
                { name: 'Unit Tests', command: 'make test-unit' },
                { name: 'Integration Tests', command: 'make test-integration' },
                { name: 'Language Tests', command: 'make test-language' },
                { name: 'Memory Tests', command: 'make test-valgrind' }
            ];
            
            for (const suite of testSuites) {
                console.log(`\n=Ë Running ${suite.name}...`);
                
                try {
                    const { stdout, stderr } = await execPromise(suite.command, {
                        timeout: 180000 // 3 minute timeout
                    });
                    
                    // Analyze output for failures
                    if (stderr.includes('Error') || stdout.includes('FAILED')) {
                        await this.createIssueFromTestFailure(suite.name, stdout, stderr);
                    } else {
                        console.log(` ${suite.name} passed`);
                    }
                    
                } catch (error) {
                    // Test failure - create an issue
                    await this.createIssueFromTestFailure(suite.name, error.stdout || '', error.stderr || error.message);
                }
            }
            
            // Also check for memory leaks specifically
            await this.checkMemoryLeaks();
            
        } catch (error) {
            console.error('L Error running tests:', error.message);
        }
    }

    /**
     * Create an issue from a test failure
     */
    async createIssueFromTestFailure(testName, stdout, stderr) {
        const title = `Test Failure: ${testName} - ${new Date().toISOString().split('T')[0]}`;
        
        // Check if similar issue already exists
        const { stdout: existingIssues } = await execPromise(`gh issue list --search "${testName}" --state open --json number`);
        const existing = JSON.parse(existingIssues);
        
        if (existing.length > 0) {
            console.log(`9 Similar issue already exists for ${testName}`);
            return;
        }
        
        const body = `## Automated Test Failure Report

**Test Suite:** ${testName}
**Date:** ${new Date().toISOString()}
**Detected by:** FixBot

### Test Output
\`\`\`
${stdout.slice(-2000)}
\`\`\`

### Error Output
\`\`\`
${stderr.slice(-2000)}
\`\`\`

### Priority
High - Automated tests are failing

### Auto-generated
This issue was automatically created by FixBot after detecting test failures.`;
        
        try {
            const { stdout: newIssue } = await execPromise(`gh issue create --title "${title}" --body "${body.replace(/"/g, '\\"')}"`);
            console.log(`=Ý Created issue for ${testName} failure: ${newIssue.trim()}`);
        } catch (error) {
            console.error(`L Failed to create issue for ${testName}:`, error.message);
        }
    }

    /**
     * Check for memory leaks
     */
    async checkMemoryLeaks() {
        console.log('\n= Checking for memory leaks...');
        
        try {
            // Create a simple test file
            const testCode = `
set data value 100
put "./leak_test.json" data
set loaded get "./leak_test.json"
print loaded
`;
            
            await execPromise(`echo '${testCode}' > leak_test.zen`);
            
            // Run with valgrind
            const { stdout, stderr } = await execPromise('valgrind --leak-check=full ./zen leak_test.zen 2>&1', {
                timeout: 30000
            });
            
            const output = stdout + stderr;
            
            if (output.includes('definitely lost') && !output.includes('definitely lost: 0 bytes')) {
                const title = `Memory Leak Detected - ${new Date().toISOString().split('T')[0]}`;
                const body = `## Memory Leak Report

**Date:** ${new Date().toISOString()}
**Detected by:** FixBot

### Valgrind Output
\`\`\`
${output.slice(-3000)}
\`\`\`

### Priority
High - Memory leaks can cause performance degradation

### Auto-generated
This issue was automatically created by FixBot after detecting memory leaks.`;
                
                // Check if similar issue exists
                const { stdout: existingIssues } = await execPromise(`gh issue list --search "Memory Leak" --state open --json number`);
                const existing = JSON.parse(existingIssues);
                
                if (existing.length === 0) {
                    await execPromise(`gh issue create --title "${title}" --body "${body.replace(/"/g, '\\"')}"`);
                    console.log(`=Ý Created memory leak issue`);
                }
            } else {
                console.log(' No memory leaks detected');
            }
            
            // Cleanup
            await execPromise('rm -f leak_test.zen leak_test.json');
            
        } catch (error) {
            console.error('L Error checking memory leaks:', error.message);
        }
    }
}

// Start the bot
const bot = new FixBot();
bot.start().catch(error => {
    console.error('=¥ Fatal error:', error);
    process.exit(1);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
    console.log('\n=K FixBot shutting down...');
    process.exit(0);
});

process.on('SIGTERM', () => {
    console.log('\n=K FixBot shutting down...');
    process.exit(0);
});