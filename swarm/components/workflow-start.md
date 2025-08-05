## WORKFLOW

### MANDATORY: CREATE TASK FILE FIRST

**⚠️ CRITICAL: You MUST create a task file BEFORE doing ANY work, including reading files, running commands, or making changes. Task creation is NOT optional - it is MANDATORY for ALL agents including queens.**

```bash
# STEP 1: CREATE TASK FILE (MANDATORY - DO THIS FIRST!)
TIMESTAMP=$(date +%Y%m%d-%H%M)
UNIX_TIME=$(date +%s)
cat > tasks/${TIMESTAMP}.yaml << EOF
agent: {{AGENT_ID}}
task: <Brief description of what you're about to do>
created: $UNIX_TIME
completed: false
files:
  - <files you plan to work on>
steps:
  - $UNIX_TIME:
      start: $UNIX_TIME
      end: 0
      method: <Your planned approach>
      success: false
      fail: false
      why_success: In progress
      why_fail: Not started yet
EOF

# STEP 2: Only AFTER creating task file, check project state
make vision          # Check current state, active tasks, and agent fitness
make enforce         # Verify manifest compliance

# Review active tasks to avoid conflicts
# Look for:
# - Files with [agent-id] indicators (someone is working on them)
# - Your fitness score and performance metrics
# - Stalled tasks that might need help

# STEP 3: Setup your workspace (if not exists)
mkdir -p workspace/{{AGENT_ID}}/{src,build,tests}

# STEP 4: Sync latest code to your workspace
rsync -av --delete src/ workspace/{{AGENT_ID}}/src/
```

### ENFORCEMENT REMINDER

If you haven't created a task file yet, STOP and create one NOW. No exceptions. This includes:
- Reading any project files
- Running any analysis commands
- Making any code changes
- Running make vision or make enforce
- ANY action related to the project

The ONLY exception is if you're explicitly asked to check task status or clean up old tasks.