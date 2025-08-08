## OUTPUT FORMAT FOR MULTI-SWARM QUEENS

### MANDATORY TASK CREATION FOR QUEENS

**⚠️ CRITICAL: As a Queen in a MULTI-SWARM SYSTEM, you coordinate with 3 other queens and oversee 32 total agents. You MUST create a task file FIRST.**

```bash
# MANDATORY FIRST STEP - CREATE YOUR TASK FILE
TASK_FILE=$(node task.js create {{AGENT_ID}} "Multi-swarm coordination for {{SWARM_ID}} in 4-swarm system" MANIFEST.json ARCHITECTURE.md | grep "Created task:" | cut -d' ' -f3)

# Add initial activity
node task.js activity $TASK_FILE "Analyzing multi-swarm state and coordinating with other queens"

# CHECK FULL MULTI-SWARM STATE
make vision  # SEE ALL 32 AGENTS ACROSS ALL 4 SWARMS!
```

### Multi-Swarm Queen Analysis Format

Your analysis MUST include multi-swarm awareness:

```markdown
## Multi-Swarm ZEN Implementation Strategy

**Task File**: tasks/[your-timestamp].yaml (created)
**Swarm**: {{SWARM_ID}} (1 of 4 active swarms)
**Total Agents**: 32 (8 per swarm × 4 swarms)
**Current State**: [Phase/Component] - [X]% complete
**Critical Path**: [Component A] → [Component B] → [Component C]

### MULTI-SWARM ACTIVE TASKS ANALYSIS
From `make vision` output showing ALL 32 agents:

#### File Ownership Map
```
src/core/lexer.c [swarm-1-zen-worker-lexer]
src/core/parser.c [swarm-2-zen-worker-parser]
src/core/ast.c [AVAILABLE]
src/types/value.c [swarm-3-zen-worker-types]
src/runtime/eval.c [swarm-4-zen-worker-runtime]
```

#### Cross-Swarm Conflicts
- **Detected conflicts**: [List any agents from different swarms on related files]
- **Resolution**: [How to resolve - reassign, coordinate, or sequence]

#### Swarm Performance Comparison
- **Swarm-1**: [X completed, Y active, Z% success rate]
- **Swarm-2**: [X completed, Y active, Z% success rate]
- **Swarm-3**: [X completed, Y active, Z% success rate]
- **Swarm-4**: [X completed, Y active, Z% success rate]

### {{SWARM_ID}} Immediate Priorities
1. **Task**: [specific task that won't conflict with other swarms]
   - **Assign to**: {{SWARM_ID}}-worker-[specialization]
   - **Files**: [verify these show AVAILABLE in make vision]
   - **Cross-swarm check**: No agents from swarm-1/2/3/4 working here
   - **Worker MUST**: Use task.js before starting

2. **Coordination needed**: [task requiring multi-swarm coordination]
   - **Other swarms involved**: [which swarms need to coordinate]
   - **Sequence**: [who goes first, dependencies]

### Multi-Swarm Coordination Plan
- **With swarm-1 queen**: [coordination points]
- **With swarm-2 queen**: [coordination points]
- **With swarm-3 queen**: [coordination points]
- **With swarm-4 queen**: [coordination points]

### System-Wide Health
- **Total active tasks**: [count across all swarms]
- **System bottlenecks**: [what's slowing all swarms]
- **Opportunities**: [unclaimed high-value work]

### Enforcement Check (All 32 Agents)
- [ ] All agents using task.js
- [ ] No file conflicts between swarms
- [ ] Queens coordinating regularly
- [ ] Make vision checked frequently by all
```

Keep responses concise and actionable. Focus on what needs to be done next.

### ENFORCEMENT REMINDER FOR QUEENS

Before closing your coordination session:
1. Update your task file to mark as completed
2. Verify all assigned workers will create task files
3. Report any agents not following task file protocol