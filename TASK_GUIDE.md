# Task Management Guide

The `task.js` utility provides a standardized way for all swarm agents to create and manage tasks. This ensures proper tracking, coordination, and visibility across all development activities.

## Quick Start

### Creating a Task (MANDATORY First Step)

```bash
# Create task and capture filename
TASK_FILE=$(node task.js create <agent-id> "Task description" file1 file2 | grep "Created task:" | cut -d' ' -f3)

# Example:
TASK_FILE=$(node task.js create swarm-1-zen-worker-lexer "Implement number tokenization" src/core/lexer.c | grep "Created task:" | cut -d' ' -f3)
```

### Tracking Activities

```bash
# Simple activity
node task.js activity $TASK_FILE "Starting implementation"

# Successful progress
node task.js activity $TASK_FILE "Integer parsing complete" --success "All tests pass"

# Failed attempt
node task.js activity $TASK_FILE "Float parsing attempt" --fail "Precision issues"
```

### Completing Tasks

```bash
# Success
node task.js complete $TASK_FILE --success "Feature fully implemented"

# Failure
node task.js complete $TASK_FILE --fail "Blocked by dependencies"
```

## File Format

Tasks are stored in `/tasks/` as YAML files with timestamp names (YYYYMMDD-HHMM.yaml):

```yaml
agent: swarm-1-zen-worker-lexer
task: Implement lexer_scan_number function
created: 1738771200
completed: false
files:
  - src/core/lexer.c
  - src/include/zen/core/lexer.h
activities:
  - timestamp: 1738771200
    start: 1738771200
    end: 0
    method: Task initialized
    success: false
    fail: false
    why_success: In progress
    why_fail: Not completed yet
```

## Integration with Make Vision

The `make vision` command reads task files to:
- Show active tasks with [agent-id] indicators
- Calculate agent fitness scores
- Identify stalled tasks (>24 hours)
- Track implementation progress

## Agent Workflow

1. **MANDATORY**: Create task before ANY work
2. Add activities as you progress
3. Complete task when done (success or fail)
4. Check status and list tasks as needed

## Commands Reference

- `create` - Create new task (returns filename)
- `activity` - Add progress activity
- `complete` - Mark task as done
- `status` - View task details
- `list` - List tasks with filters (--active, --completed, --failed)

## Best Practices

1. **Always create task first** - No exceptions
2. **Add activities frequently** - Track your progress
3. **Be specific** - Clear descriptions help coordination
4. **Complete tasks** - Mark success or failure with reasons
5. **Check for conflicts** - Use `list --active` to see what others are working on

## Troubleshooting

If task.js is unavailable, agents can fall back to manual task creation as documented in their DNA files, but task.js is the preferred method for consistency and ease of use.