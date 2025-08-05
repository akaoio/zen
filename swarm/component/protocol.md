## SWARM PROTOCOL

This protocol ensures coordinated development across all swarm agents working on the ZEN language project.

### MANDATORY PREREQUISITES
1. **Read `docs/idea.md` completely** - Do not skip any line. This contains the ZEN language specification.
2. **Read `MANIFEST.json`** - Understand the desired architecture and function signatures.
3. **Read `ARCHITECTURE.md`** - Understand the complete codebase vision.

### SWARM COORDINATION COMMANDS
1. **`make vision`** - View project status and active swarm tasks
   - Shows file implementation status (complete/incomplete/stub/desired)
   - Displays agents actively working on files with `[agent-id]` indicators
   - Lists active and recently completed tasks

2. **`make enforce`** - Check code compliance
   - Validates function signatures match MANIFEST.json
   - Ensures all functions have doxygen documentation
   - Detects stub implementations

### TASK MANAGEMENT PROTOCOL

#### Creating a Task
When starting any work, create a task file in `tasks/` with format: `YYYYMMDD-HHMM.yaml`

```yaml
agent: <your-agent-id>  # e.g., worker-01, queen-01
task: <clear-description>  # What problem you're solving
created: <unix-timestamp>  # Use: date +%s
completed: false  # Set to true when done
files:  # List ALL files you'll modify
  - src/core/lexer.c
  - src/include/zen/core/lexer.h
steps: []  # Will be populated as you work
```

#### Working on a Task
Add a step entry for each work session:

```yaml
steps:
  - <unix-timestamp>:  # Step timestamp
      start: <unix-timestamp>
      end: <unix-timestamp>  # 0 if still in progress
      method: |
        Detailed explanation of your approach, design decisions,
        algorithms chosen, and rationale for the implementation
      success: false  # true when step succeeds
      fail: false     # true if step fails
      why_success: |
        What worked well, patterns that can be reused by swarm
      why_fail: |
        What went wrong, lessons learned, what to avoid
```

#### Completing a Task
1. Set `completed: true` in the task file
2. Set final step's `end` timestamp
3. Ensure either `success: true` or `fail: true` is set

### AGENT WORKSPACE ISOLATION

**CRITICAL**: Each agent MUST work in their own isolated workspace to prevent conflicts.

1. **Workspace Structure**
   ```
   workspace/
   ├── worker-01/     # Agent's dedicated folder
   │   ├── build/     # Agent's build output
   │   ├── src/       # Working copy of source
   │   └── tests/     # Agent's test runs
   ├── worker-02/
   ├── queen-01/
   └── architect-01/
   ```

2. **Workspace Rules**
   - **NEVER** build in the root `build/` folder
   - **NEVER** build in another agent's workspace
   - **ALWAYS** use `workspace/<your-agent-id>/build/`
   - **ALWAYS** copy files to your workspace before modifying

3. **Build Commands**
   ```bash
   # Wrong - DO NOT DO THIS
   make
   ./zen
   
   # Correct - Build in your workspace
   cd workspace/<your-agent-id>
   make -C ../.. BUILD_DIR=workspace/<your-agent-id>/build
   workspace/<your-agent-id>/build/zen
   ```

### SWARM COLLABORATION RULES

1. **Check Active Tasks First**
   - Run `make vision` before starting work
   - Don't work on files with active `[agent-id]` indicators
   - Coordinate in task descriptions to avoid conflicts

2. **Follow the Manifest**
   - All function signatures must match MANIFEST.json exactly
   - New functions require manifest update first
   - Use `make enforce` to verify compliance

3. **Document Everything**
   - All functions need doxygen comments (@brief, @param, @return)
   - Task files must detail your methodology
   - Share learnings through why_success/why_fail

4. **Update Task Files Continuously**
   - Update immediately when starting work
   - Add steps as you progress
   - Mark completed before moving to next task

5. **Handle Failures Gracefully**
   - Document why approaches failed
   - Leave code in working state
   - Create new task for retry with lessons learned

6. **Workspace Hygiene**
   - Keep your workspace clean
   - Don't leave temporary files in root
   - Clean failed builds before retrying

### WORKSPACE SETUP PROCESS

1. **Initial Setup** (do this once)
   ```bash
   mkdir -p workspace/<your-agent-id>
   cd workspace/<your-agent-id>
   cp -r ../../src .  # Copy source to your workspace
   mkdir -p build
   ```

2. **Before Each Task**
   ```bash
   # Sync your workspace with latest changes
   cd workspace/<your-agent-id>
   rsync -av --delete ../../src/ ./src/
   ```

3. **Testing Your Changes**
   ```bash
   # Build in your workspace
   make -C ../.. BUILD_DIR=workspace/<your-agent-id>/build clean
   make -C ../.. BUILD_DIR=workspace/<your-agent-id>/build
   
   # Test your build
   workspace/<your-agent-id>/build/zen test.zen
   ```

4. **Submitting Changes**
   - Only copy back files you modified
   - Run `make enforce` from root before copying
   - Document all changes in your task file

### TASK LIFECYCLE
1. **Claim**: Create task file with `completed: false`
2. **Setup**: Prepare your workspace
3. **Work**: Add steps, update progress
4. **Test**: Verify in your isolated environment
5. **Submit**: Copy validated changes back
6. **Complete**: Set `completed: true`, document outcomes
7. **Abandon**: Tasks older than 24 hours are considered abandoned

### AGENT NAMING CONVENTION
- Format: `<role>-<number>`
- Examples: `worker-01`, `queen-01`, `architect-01`
- Increment number for each instance of same role

### QUALITY STANDARDS
- No commits without passing `make enforce`
- No stub functions in completed tasks
- All code must follow existing patterns
- Test your implementation before marking complete

