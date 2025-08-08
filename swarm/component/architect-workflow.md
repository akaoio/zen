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

### NAMING STANDARDS ENFORCEMENT (MANDATORY)

**⚠️ CRITICAL: ALL function names MUST follow `module_action_target` pattern.**

Before designing ANY function signature:
```bash
# STEP 1: Validate function name follows pattern
validate_function_name() {
    local func_name=$1
    
    if ! echo "$func_name" | grep -E "^[a-z]+_[a-z]+_[a-z]+$" > /dev/null; then
        echo "❌ NAMING VIOLATION: '$func_name' doesn't follow module_action_target"
        echo "   Must be: module_action_target (e.g., 'lexer_scan_token')"
        return 1
    fi
    
    echo "✅ Valid naming: $func_name"
    return 0
}

# STEP 2: Validate module exists in approved list
validate_module_assignment() {
    local func_name=$1
    local module=$(echo "$func_name" | cut -d'_' -f1)
    
    local valid_modules="lexer parser ast visitor scope memory error value array object io json string math convert datetime system"
    
    if ! echo "$valid_modules" | grep -q "$module"; then
        echo "❌ INVALID MODULE: '$module' not in approved module list"
        echo "   Valid modules: $valid_modules"
        return 1
    fi
    
    echo "✅ Valid module assignment: $module"
    return 0
}

# MANDATORY: Run before adding ANY function to MANIFEST.json
before_manifest_update() {
    local func_name=$1
    
    validate_function_name "$func_name" || return 1
    validate_module_assignment "$func_name" || return 1
    
    echo "✅ Function approved for MANIFEST.json: $func_name"
    return 0
}
```