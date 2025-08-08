## MANIFEST ACCESS CONTROL

### ⚠️ CRITICAL: MANIFEST.json Access Rules

The `MANIFEST.json` file is the SOURCE OF TRUTH for the entire ZEN project. It defines:
- All function signatures that MUST be implemented
- The exact contract between components
- The authoritative project structure

### Access Control Matrix

| Agent Type | Read Access | Write Access | Rationale |
|------------|-------------|--------------|-----------|
| **Architect** | ✅ YES | ✅ YES | Architects design the system and update contracts |
| **Queen** | ✅ YES | ❌ NO | Queens coordinate but don't change architecture |
| **Worker** | ✅ YES | ❌ NO | Workers implement but don't modify contracts |

### For Architects ONLY

```bash
# ONLY ARCHITECTS can modify MANIFEST.json
if [[ "{{AGENT_TYPE}}" == "Architect" ]]; then
    # You have authority to update manifest when:
    # 1. Adding new functions after design phase
    # 2. Fixing signature errors discovered during implementation
    # 3. Adding missing components identified by workers
    
    # ALWAYS create a task before modifying
    TASK_FILE=$(node task.js create {{AGENT_ID}} "Update MANIFEST.json for [reason]" MANIFEST.json | grep "Created task:" | cut -d' ' -f3)
    
    # Document the change
    node task.js activity $TASK_FILE "Adding function X to support feature Y"
    
    # Make the edit
    # ... edit MANIFEST.json ...
    
    # Verify it's valid
    make enforce
    
    # Complete task
    node task.js complete $TASK_FILE --success "MANIFEST.json updated with new functions"
fi
```

### For Queens and Workers

```bash
# READ-ONLY ACCESS TO MANIFEST.json
# You can read to understand contracts
cat MANIFEST.json | jq '.files[].functions[] | select(.name == "function_name")'

# BUT YOU CANNOT MODIFY IT
# If you discover issues with MANIFEST.json:
# 1. Create a task documenting the issue
ISSUE_TASK=$(node task.js create {{AGENT_ID}} "MANIFEST ISSUE: [describe problem]" MANIFEST.json | grep "Created task:" | cut -d' ' -f3)

# 2. Document the specific problem
node task.js activity $ISSUE_TASK "Function X signature doesn't match implementation needs because..."

# 3. Complete with recommendation for architect
node task.js complete $ISSUE_TASK --fail "Need architect to update MANIFEST.json - suggested change: [details]"

# 4. Your queen will coordinate with an architect to fix it
```

### Enforcement Rules

1. **Git hooks** will REJECT commits that modify MANIFEST.json from non-architect agents
2. **Make enforce** will flag unauthorized changes
3. **Vision** will show warnings if non-architects modify MANIFEST.json
4. **Queens** monitor and report violations

### Why This Matters

- **Contract Stability**: Workers need stable contracts to implement against
- **Architectural Integrity**: Only architects should make structural decisions  
- **Conflict Prevention**: Prevents multiple agents changing contracts simultaneously
- **Quality Control**: Ensures all contract changes are properly designed

### Exception Protocol

In EMERGENCY situations where a worker discovers a CRITICAL issue:
1. Document extensively in task file WHY immediate change is needed
2. Create a branch: `git checkout -b manifest-emergency-{{AGENT_ID}}`
3. Make the minimal change needed
4. Create PR with detailed explanation
5. Tag ALL architects and queens for review

But this should be EXTREMELY RARE. The proper flow is:
**Worker finds issue → Reports to Queen → Queen coordinates with Architect → Architect updates MANIFEST.json**