## SIMPLIFIED MULTI-SWARM COORDINATION

### BASIC COORDINATION PROTOCOL

You are part of a 4-swarm development system. Simple coordination rules:

**Before Starting Work:**
```bash
# 1. Check who is working on what
make vision | grep -E "\[(swarm-[1-4])"

# 2. If file is marked with another agent, find different work
# 3. Create your task before starting
TASK_FILE=$(node task.js create {{AGENT_ID}} "Brief description" target-file)
```

### FILE OWNERSHIP RULES

**Simple Rule:** One agent per file at a time.
- File shows `[agent-id]` → Someone else is working on it
- File shows no `[agent-id]` → Available for work
- If conflict detected → Create different task

### COORDINATION WORKFLOW

```bash
# Standard coordination workflow
coordinate-work() {
    local target_files=("$@")
    
    # Check each target file
    for file in "${target_files[@]}"; do
        if make vision | grep "$file" | grep -q "\[swarm-"; then
            echo "File $file is busy. Finding alternative work."
            return 1
        fi
    done
    
    # Files are available - proceed with task
    return 0
}
```

### TASK TRACKING

**Simple task workflow:**
```bash
# Start work
TASK_FILE=$(node task.js create {{AGENT_ID}} "What I'm doing" target-files)

# Update progress occasionally
node task.js activity $TASK_FILE "Progress update"

# Finish work
node task.js complete $TASK_FILE --success "What was accomplished"
```

### COORDINATION RESPONSIBILITIES

**Your responsibilities:**
1. **Check availability**: Run `make vision` before starting work
2. **Avoid conflicts**: Don't work on files marked with other agent IDs
3. **Track work**: Use task.js for all activities
4. **Focus on quality**: One task at a time, done well
5. **Respect boundaries**: Stay within your role's expertise area

### CONFLICT RESOLUTION

If you encounter a conflict:
1. **Stop immediately** - Don't continue work on conflicted file
2. **Find alternative work** - Look for unassigned tasks in your specialty
3. **Report to queen** (if you're a worker) for coordination
4. **Wait for resolution** rather than forcing changes

This simplified coordination reduces overhead while maintaining effective multi-agent collaboration.