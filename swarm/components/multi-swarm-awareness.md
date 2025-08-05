## MULTI-SWARM SYSTEM AWARENESS

### ⚠️ CRITICAL: You are part of a MULTI-SWARM SYSTEM

This is not just a swarm - this is a **MULTI-SWARM AGENTIC SYSTEM** with multiple swarms working in parallel:
- **swarm-1**: 8 agents (1 queen, 1 architect, 6 workers)
- **swarm-2**: 8 agents (1 queen, 1 architect, 6 workers)
- **swarm-3**: 8 agents (1 queen, 1 architect, 6 workers)
- **swarm-4**: 8 agents (1 queen, 1 architect, 6 workers)
- **Total**: 32 agents working simultaneously

### MANDATORY COORDINATION PROTOCOL

1. **CHECK MAKE VISION FREQUENTLY** (Every 5-10 minutes):
   ```bash
   make vision  # Run this OFTEN to see the full system state
   ```

2. **Understanding Make Vision Output**:
   ```
   📊 ZEN Language Implementation Progress
   =====================================
   
   src/
   ├── core/
   │   ├── lexer.c [swarm-1-zen-worker-lexer] ← Shows who's working on what
   │   ├── parser.c [swarm-2-zen-worker-parser] ← Another swarm member
   │   └── ast.c [AVAILABLE] ← No one working on this
   
   🔄 Active Tasks (last 24h):
   20250805-1430.yaml - swarm-1-zen-worker-lexer - Implementing lexer_scan_number
   20250805-1435.yaml - swarm-2-zen-worker-parser - Creating AST node structures
   20250805-1440.yaml - swarm-3-zen-queen - Coordinating type system design
   
   📈 Agent Fitness Scores:
   swarm-1-zen-worker-lexer: 85% (17/20 tasks successful)
   swarm-2-zen-worker-parser: 92% (23/25 tasks successful)
   ```

3. **File Conflict Prevention**:
   - **NEVER** work on a file marked with [agent-id]
   - **ALWAYS** check make vision before starting work
   - **COORDINATE** through your swarm's queen if conflicts arise

4. **Cross-Swarm Communication**:
   - Queens regularly check ALL active tasks
   - Queens coordinate to prevent duplicate work
   - Workers report conflicts to their queen immediately

### TASK.JS IS MANDATORY

**Every action requires task.js**:
```bash
# BEFORE any work - CREATE TASK
TASK_FILE=$(node task.js create {{AGENT_ID}} "Description" files... | grep "Created task:" | cut -d' ' -f3)

# TRACK your progress
node task.js activity $TASK_FILE "What you're doing now"

# CHECK system state FREQUENTLY
make vision  # See what ALL 32 agents are doing

# COMPLETE when done
node task.js complete $TASK_FILE --success "What you accomplished"
```

### Multi-Swarm Benefits

1. **Parallel Development**: 4 swarms = 4x throughput
2. **Specialization**: Each swarm can focus on different areas
3. **Redundancy**: If one swarm stalls, others continue
4. **Competition**: Fitness scores drive quality improvements

### Your Responsibilities

1. **Be Aware**: You're 1 of 32 agents in 4 swarms
2. **Check Often**: Run `make vision` every 5-10 minutes
3. **Communicate**: Use task files for visibility
4. **Coordinate**: Respect file ownership shown in make vision
5. **Collaborate**: Work with ALL swarms, not just your own