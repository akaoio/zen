## TASK MANAGEMENT IN MULTI-SWARM SYSTEM

### Task Management with task.js (MANDATORY)

**⚠️ CRITICAL: In this MULTI-SWARM SYSTEM with 32 agents, task.js is your PRIMARY communication channel. Every agent MUST use it.**

### Creating Tasks

```bash
# Create a new task (returns task filename)
TASK_FILE=$(node task.js create {{AGENT_ID}} "Brief description of your task" file1.c file2.h | grep "Created task:" | cut -d' ' -f3)

# Example:
TASK_FILE=$(node task.js create {{AGENT_ID}} "Implement lexer_scan_number function" src/core/lexer.c src/include/zen/core/lexer.h | grep "Created task:" | cut -d' ' -f3)
```

### Adding Activities

Track your progress by adding activities as you work:

```bash
# Add a simple activity
node task.js activity $TASK_FILE "Starting implementation of integer parsing"

# Add activity when making progress
node task.js activity $TASK_FILE "Completed integer parsing logic" --success "All integer tests pass"

# Add activity when encountering issues
node task.js activity $TASK_FILE "Attempting float parsing" --fail "Need to handle scientific notation"
```

### Completing Tasks

```bash
# Complete task successfully
node task.js complete $TASK_FILE --success "Implemented number scanning with full float support"

# Complete task with failure
node task.js complete $TASK_FILE --fail "Blocked by missing AST node definitions"
```

### Checking Status

```bash
# View task status
node task.js status $TASK_FILE

# List all your active tasks
node task.js list --active | grep {{AGENT_ID}}

# List completed tasks
node task.js list --completed | grep {{AGENT_ID}}
```

### Complete Workflow Example

```bash
# 1. Create task when starting work
TASK_FILE=$(node task.js create {{AGENT_ID}} "Implement lexer_scan_string function" src/core/lexer.c | grep "Created task:" | cut -d' ' -f3)

# 2. Add activity when starting
node task.js activity $TASK_FILE "Analyzing string token requirements"

# 3. Add activities as you progress
node task.js activity $TASK_FILE "Implementing escape sequence handling"
node task.js activity $TASK_FILE "Added support for unicode escapes" --success "Tests passing"

# 4. Complete the task
node task.js complete $TASK_FILE --success "String scanning fully implemented with escape sequences"
```

### Manual Task Creation (Fallback)

If task.js is unavailable, use this manual method:
```bash
TIMESTAMP=$(date +%Y%m%d-%H%M)
UNIX_TIME=$(date +%s)
cat > tasks/${TIMESTAMP}.yaml << EOF
agent: {{AGENT_ID}}
task: <Your task description>
created: $UNIX_TIME
completed: false
files:
  - <files you'll work on>
activities:
  - timestamp: $UNIX_TIME
    start: $UNIX_TIME
    end: 0
    method: Task initialized
    success: false
    fail: false
    why_success: In progress
    why_fail: Not completed yet
EOF
```

### Task Analysis with Vision in Multi-Swarm System

Your tasks are visible to ALL 32 AGENTS via `make vision`:

```
📊 Make Vision Output Shows:
- Files with [agent-id] markers - who's working on what
- Active tasks from ALL 4 swarms in real-time
- Fitness scores comparing you to 31 other agents
- Conflicts and bottlenecks across the entire system
```

**CRITICAL RULES**:
1. **Check make vision BEFORE creating tasks** - avoid conflicts
2. **Check make vision DURING work** - every 5-10 minutes
3. **Update task files FREQUENTLY** - other agents are watching
4. **NEVER work on files with [other-agent-id]** - respect ownership
5. **Coordinate through queens** - they see the big picture

### Multi-Swarm Task Visibility

When you run `node task.js list --active`, you see tasks from ALL swarms:
```
○ 20250805-1430.yaml - swarm-1-zen-worker-lexer - 2025-08-05
  Implement lexer_scan_number function

○ 20250805-1435.yaml - swarm-2-zen-worker-parser - 2025-08-05
  Create AST node structures

○ 20250805-1440.yaml - swarm-3-zen-queen - 2025-08-05
  Multi-swarm coordination for swarm-3

○ 20250805-1445.yaml - swarm-4-zen-worker-types - 2025-08-05
  Implement value reference counting
```

This transparency enables:
- Cross-swarm coordination
- Conflict prevention
- Workload balancing
- System-wide progress tracking

### Example Task Creation

When assigned "Implement lexer_scan_number function":
```bash
TIMESTAMP=$(date +%Y%m%d-%H%M)
UNIX_TIME=$(date +%s)
cat > tasks/${TIMESTAMP}.yaml << EOF
agent: {{AGENT_ID}}
task: Implement lexer_scan_number function for NUMBER tokens
created: $UNIX_TIME
completed: false
files:
  - src/core/lexer.c
  - src/include/zen/core/lexer.h
steps:
  - $UNIX_TIME:
      start: $UNIX_TIME
      end: 0
      method: Implementing number scanning with support for integers and floats
      success: false
      fail: false
      why_success: In progress
      why_fail: Not completed yet
EOF
```