# FixBot - Automated Issue Fixer

FixBot is an automated tool that continuously monitors and fixes GitHub issues for the ZEN language project.

## Features

- 🔍 **Automatic Issue Detection**: Polls GitHub for open issues every 60 seconds
- 🤖 **AI-Powered Fixing**: Uses Claude to analyze and fix issues
- 🧪 **Comprehensive Testing**: Runs test suites every 5 minutes to find new issues
- 💾 **Memory Leak Detection**: Automatically checks for memory leaks using valgrind
- 📝 **Issue Creation**: Creates new issues when tests fail
- 💬 **Progress Updates**: Comments on issues with fix status
- 🔄 **Continuous Operation**: Runs in an infinite loop, constantly improving the codebase

## How It Works

1. **Issue Discovery Phase**:
   - Fetches open issues from GitHub using `gh issue list`
   - Prioritizes unassigned issues or those assigned to fixbot
   - Skips already processed issues

2. **Fixing Phase**:
   - Comments on the issue to indicate work has started
   - Assigns the issue to fixbot (if permissions allow)
   - Spawns Claude with the issue context using `claude --dangerously-skip-permissions`
   - Claude analyzes and implements fixes

3. **Verification Phase**:
   - Runs `make test` to verify fixes
   - Analyzes git diff to see what changed
   - Determines if issue is fully fixed, partially fixed, or failed

4. **Resolution Phase**:
   - **Fully Fixed**: Comments with success, closes the issue
   - **Partially Fixed**: Comments with progress, continues working
   - **Failed**: Comments with error details, marks for human intervention

5. **Test Discovery Phase** (every 5 minutes):
   - Runs various test suites (unit, integration, language, memory)
   - Creates GitHub issues for any test failures
   - Checks for memory leaks with valgrind

## Usage

### Start FixBot

```bash
# Using Node directly
node fixbot.js

# Using npm script
npm start

# Run in background
nohup node fixbot.js > fixbot.log 2>&1 &
```

### Requirements

- Node.js >= 14.0.0
- `gh` CLI tool (authenticated)
- `claude` CLI tool
- `make` and project build tools
- `valgrind` for memory leak detection

### Configuration

Edit the constants at the top of `fixbot.js`:

```javascript
const POLL_INTERVAL = 60000;  // Check issues every 60 seconds
const TEST_INTERVAL = 300000;  // Run tests every 5 minutes
const BOT_NAME = 'fixbot';     // GitHub username for the bot
```

## Safety Features

- Only processes one issue at a time to avoid conflicts
- Tracks processed issues to avoid infinite loops
- Includes timeout protection for long-running operations
- Graceful shutdown on SIGINT/SIGTERM

## Logs

FixBot outputs detailed logs including:
- 🔍 Issue discovery
- 🎯 Issue selection
- 🔧 Fix attempts
- ✅ Success notifications
- ❌ Error reports
- 🧪 Test results

## Example Session

```
🤖 FixBot starting...
📁 Project path: /home/x/Projects/zen
🔍 Checking for open issues...
🎯 Found issue #30: Nested object syntax not working properly
🤖 FixBot is analyzing and attempting to fix this issue...
🔧 Attempting to fix with Claude...
🧪 Running tests to verify fix...
✅ Issue #30 appears to be fixed!
💬 Commented on issue #30
✅ Closed issue #30
🔄 FixBot is now running in continuous mode...
```

## Monitoring

To monitor FixBot's activity:

```bash
# View real-time logs
tail -f fixbot.log

# Check GitHub issues
gh issue list --assignee fixbot

# View bot comments
gh issue view [ISSUE_NUMBER] --comments
```

## Stopping FixBot

```bash
# Find the process
ps aux | grep fixbot

# Kill gracefully
kill [PID]

# Or kill by name
pkill -f fixbot.js
```

## Limitations

- Requires appropriate GitHub permissions to assign/close issues
- Claude must be properly configured and authenticated
- Some complex issues may require human intervention
- Test creation is limited to detected failures

## Contributing

FixBot is part of the ZEN language project. To improve FixBot:
1. Enhance issue detection logic
2. Improve fix verification
3. Add more test suite integrations
4. Enhance error handling and recovery

## License

MIT License - Part of the ZEN language project