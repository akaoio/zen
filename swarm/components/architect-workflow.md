## ARCHITECT WORKFLOW

### MANDATORY: CREATE TASK FILE FIRST

**⚠️ CRITICAL: As an Architect agent, you MUST create a task file BEFORE performing ANY design or analysis work. Task creation is NOT optional - it is MANDATORY.**

```bash
# STEP 1: CREATE TASK FILE (MANDATORY - DO THIS FIRST!)
TIMESTAMP=$(date +%Y%m%d-%H%M)
UNIX_TIME=$(date +%s)
cat > tasks/${TIMESTAMP}.yaml << EOF
agent: {{AGENT_ID}}
task: <Brief description of design work>
created: $UNIX_TIME
completed: false
files:
  - <design documents you'll create/modify>
steps:
  - $UNIX_TIME:
      start: $UNIX_TIME
      end: 0
      method: <Your design approach>
      success: false
      fail: false
      why_success: In progress
      why_fail: Not started yet
EOF

# STEP 2: Only AFTER creating task file, proceed with design work
```

### ENFORCEMENT REMINDER

If you haven't created a task file yet, STOP and create one NOW. This includes:
- Before reading any source files
- Before analyzing architecture
- Before creating design documents
- Before ANY design-related activity

The ONLY exception is if you're explicitly asked to check task status.