# Swarm Protocol

## WORKSPACE ISOLATION

Each agent in the swarm MUST work in their own isolated workspace to prevent conflicts. This protocol ensures clean separation of work and prevents interference between agents.

### Workspace Structure

Each agent has a dedicated workspace at:
```
workspace/<swarm-id>-<agent-id>/
  ├── src/      # Source files for modification
  ├── build/    # Build artifacts
  └── tests/    # Test files
```

### Workspace Setup Protocol

Before starting any implementation task:

```bash
# 1. Create isolated workspace structure
mkdir -p workspace/<swarm-id>-<agent-id>/{src,build,tests}

# 2. Sync source files to workspace
rsync -av --delete src/ workspace/<swarm-id>-<agent-id>/src/

# 3. Change to workspace directory
cd workspace/<swarm-id>-<agent-id>
```

### Build Protocol

**Critical Rule**: NEVER build in the root directory. Always use your isolated workspace:

```bash
# WRONG - Never do this!
make
./zen

# CORRECT - Always build in your workspace
make -C ../.. BUILD_DIR=workspace/<swarm-id>-<agent-id>/build
workspace/<swarm-id>-<agent-id>/build/zen

# Or from your workspace directory:
cd workspace/<swarm-id>-<agent-id>
make -C ../.. BUILD_DIR=$(pwd)/build
```

### Testing Protocol

```bash
# Build in your workspace
make -C ../.. BUILD_DIR=workspace/<swarm-id>-<agent-id>/build clean
make -C ../.. BUILD_DIR=workspace/<swarm-id>-<agent-id>/build

# Test your build
workspace/<swarm-id>-<agent-id>/build/zen test.zen

# Run valgrind from your workspace
valgrind --leak-check=full workspace/<swarm-id>-<agent-id>/build/zen
```

### Change Submission Protocol

Only submit changes after successful testing in your workspace:

```bash
# From project root
make enforce  # Verify compliance

# Copy only modified files back
cp workspace/<swarm-id>-<agent-id>/src/modified_file.c src/
cp workspace/<swarm-id>-<agent-id>/src/include/zen/modified_header.h src/include/zen/
```

## TASK MANAGEMENT

### Task Creation (MANDATORY)

Every piece of work MUST have a corresponding task file:

```bash
# Create task file first
TASK_FILE=$(node task.js create <swarm-id>-<agent-id> "Description of work" file1 file2)

# Add activities as you progress
node task.js activity $TASK_FILE "Current progress description"

# Complete when done
node task.js complete $TASK_FILE --success "Work completed successfully"
```

### Task Monitoring

All agents must regularly monitor task status:

```bash
# View your active tasks
node task.js list --active | grep <swarm-id>-<agent-id>

# Check all active tasks in your swarm
node task.js list --active | grep <swarm-id>

# View completed tasks
node task.js list --completed | grep <swarm-id>-<agent-id>
```

## COORDINATION RULES

1. **Check Before Working**:
   - Always run `make vision` before starting work
   - Never modify files marked with [other-agent-id]
   - Coordinate through queens if conflicts detected

2. **Regular System Checks**:
   - Run `make vision` every 5-10 minutes
   - Update task files with your current progress
   - Monitor for new conflicts or dependencies

3. **File Management**:
   - Never create files in root directory
   - Use tmp/ folder for temporary files
   - Follow file creation policies strictly

4. **Cross-Swarm Communication**:
   - Queens coordinate between swarms
   - Workers only communicate through queens
   - Use task.js for all coordination

## FITNESS METRICS

Your agent fitness score depends on:
1. Task completion rate
2. Coordination efficiency
3. Conflict avoidance
4. Protocol compliance

Remember:
- All 32 agents are watching the task list
- Conflicts waste everyone's time
- Your fitness score depends on successful coordination
- Task files are your communication channel
- NEVER STOP UNTIL 100% COMPLETE
