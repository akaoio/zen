## ARCHITECT WORKFLOW

### MANDATORY: CREATE TASK FILE FIRST

**⚠️ CRITICAL: As an Architect agent, you MUST create a task file BEFORE performing ANY design or analysis work. Task creation is NOT optional - it is MANDATORY.**

```bash
# STEP 1: CREATE TASK FILE (MANDATORY - DO THIS FIRST!)
# Using task.js utility (required)
TASK_FILE=$(node task.js create {{AGENT_ID}} "Brief description of design work" design_docs_to_create | grep "Created task:" | cut -d' ' -f3)

# Add initial activity
node task.js activity $TASK_FILE "Starting architectural design and analysis"

# STEP 2: Only AFTER creating task file, proceed with design work
```

### ENFORCEMENT REMINDER

If you haven't created a task file yet, STOP and create one NOW. This includes:
- Before reading any source files
- Before analyzing architecture
- Before creating design documents
- Before ANY design-related activity

The ONLY exception is if you're explicitly asked to check task status.