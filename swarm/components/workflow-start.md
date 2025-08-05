## WORKFLOW IN A MULTI-SWARM SYSTEM

### MANDATORY: CREATE TASK FILE FIRST

**⚠️ CRITICAL: You MUST create a task file BEFORE doing ANY work. You are part of a MULTI-SWARM SYSTEM with 32 agents across 4 swarms working simultaneously!**

```bash
# STEP 1: CREATE TASK FILE (MANDATORY - DO THIS FIRST!)
TASK_FILE=$(node task.js create {{AGENT_ID}} "Brief description of what you're about to do" file1 file2 | grep "Created task:" | cut -d' ' -f3)
echo "Working on task: $TASK_FILE"

# STEP 2: CHECK MULTI-SWARM STATE (CRITICAL!)
make vision          # SEE ALL 32 AGENTS' ACTIVITIES - CHECK FOR CONFLICTS!
# EXAMINE CAREFULLY:
# - Which files show [agent-id] (being worked on)
# - Active tasks from ALL 4 swarms
# - Your fitness score vs other agents
# - Potential conflicts or duplicated efforts

make enforce         # Verify manifest compliance

# STEP 3: VERIFY NO CONFLICTS
# If make vision shows your target files with [other-agent-id]:
# - STOP! Do not proceed
# - Choose different files or tasks
# - Coordinate with your queen

# STEP 4: Setup your workspace (if not exists)
mkdir -p workspace/{{AGENT_ID}}/{src,build,tests}

# STEP 5: Sync latest code to your workspace
rsync -av --delete src/ workspace/{{AGENT_ID}}/src/

# STEP 6: FREQUENT CHECKS (Every 5-10 minutes)
# Add this to your workflow:
while working; do
    make vision      # Check for new conflicts
    node task.js activity $TASK_FILE "Current progress description"
    # Work for 5-10 minutes
done
```

### MULTI-SWARM COORDINATION RULES

1. **NEVER** work on files marked with [other-agent-id] in make vision
2. **ALWAYS** run make vision before AND during work
3. **UPDATE** your task file frequently with activities
4. **RESPECT** the 32-agent ecosystem - you're not alone!
5. **COORDINATE** through queens when conflicts arise

### CONTINUOUS WORK LOOP - NEVER STOP

After initial setup, enter your PERSISTENT WORK LOOP:

```bash
# THE INFINITE PRODUCTIVITY LOOP
while true; do
    # Check overall progress
    PROGRESS=$(make vision | grep -E "Overall|Complete" | grep -o "[0-9]*%" | head -1)
    echo "Project Progress: ${PROGRESS:-0%}"
    
    # If 100% complete, verify tests
    if [ "$PROGRESS" = "100%" ]; then
        if make test 2>&1 | grep -q "All tests passed"; then
            echo "🎉 PROJECT COMPLETE! All features implemented and tests passing!"
            break
        fi
    fi
    
    # Find next work item
    if [ -z "$CURRENT_TASK" ]; then
        # Check for TODOs, unimplemented features, or failing tests
        NEXT_WORK=$(make vision | grep -E "TODO|UNIMPLEMENTED|FAILING|AVAILABLE" | head -1)
        if [ -n "$NEXT_WORK" ]; then
            CURRENT_TASK=$(node task.js create {{AGENT_ID}} "$NEXT_WORK" | grep "Created task:" | cut -d' ' -f3)
        fi
    fi
    
    # Do the work
    if [ -n "$CURRENT_TASK" ]; then
        node task.js activity $CURRENT_TASK "Implementing feature"
        # ... your implementation work ...
        # Complete when done
        node task.js complete $CURRENT_TASK --success "Feature implemented"
        unset CURRENT_TASK
    fi
    
    # Brief system check
    sleep 30
done
```

### ENFORCEMENT REMINDER

**YOU ARE A PERSISTENT AGENT**: Your mission is to work continuously until:
- ✅ 100% of features implemented
- ✅ 100% of tests passing
- ✅ 0 memory leaks
- ✅ Full ZEN language working

If you haven't created a task file yet, STOP NOW. In this multi-swarm system:
- 32 agents are watching the task list
- Conflicts waste everyone's time
- Your fitness score depends on successful coordination
- Task files are your communication channel
- **YOU NEVER STOP UNTIL 100% COMPLETE**