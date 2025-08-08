# NAMING ENFORCEMENT COMPONENT

## MANDATORY NAMING STANDARDS

**⚠️ CRITICAL: All agents MUST follow the `module_action_target` naming convention for ALL functions.**

### Universal Naming Pattern

Every function MUST follow this exact pattern:
```c
ReturnType module_action_target(parameters)
```

**Examples of CORRECT naming:**
- `lexer_scan_token()` - lexer module, scan action, token target
- `value_new_string()` - value module, new action, string target
- `io_read_file()` - io module, read action, file target
- `array_push_element()` - array module, push action, element target

**Examples of INCORRECT naming:**
- `zen_stdlib_print()` ❌ (use `io_print_line()`)
- `init_lexer()` ❌ (use `lexer_new()`)
- `json_parse()` ❌ (use `json_parse_string()`)

### Module Assignments

**Core Engine Modules:**
- `lexer_*` - Tokenization and scanning operations
- `parser_*` - Syntax analysis and AST construction
- `ast_*` - Abstract syntax tree operations
- `visitor_*` - Runtime evaluation and execution
- `scope_*` - Variable and function scope management
- `memory_*` - Memory management and reference counting
- `error_*` - Error handling and reporting

**Type System Modules:**
- `value_*` - Core value type operations
- `array_*` - Dynamic array operations
- `object_*` - Hash-map object operations

**Standard Library Modules:**
- `io_*` - File operations, printing, input (replaces zen_stdlib_*)
- `json_*` - JSON parsing and serialization
- `string_*` - String manipulation operations
- `math_*` - Mathematical operations and functions
- `convert_*` - Type conversion utilities
- `datetime_*` - Date and time operations
- `system_*` - System-level operations

### Action Categories

**Common Actions:**
- `new` - Create new instance (e.g., `lexer_new()`)
- `free` - Free/destroy instance (e.g., `value_free()`)
- `get` - Retrieve/access data (e.g., `array_get_element()`)
- `set` - Assign/modify data (e.g., `object_set_property()`)
- `scan` - Tokenize/parse input (e.g., `lexer_scan_number()`)
- `parse` - Analyze syntax/structure (e.g., `parser_parse_expr()`)
- `eval` - Execute/evaluate (e.g., `visitor_eval_node()`)
- `print` - Output to console/file (e.g., `io_print_line()`)
- `read` - Input from file/user (e.g., `io_read_file()`)
- `convert` - Transform data (e.g., `convert_to_string()`)

### Target Specifications

**Common Targets:**
- `token`, `string`, `number`, `file`, `line`, `element`, `property`
- `expr`, `statement`, `program`, `node`, `scope`, `context`
- `value`, `array`, `object`, `reference`, `memory`

## AGENT RESPONSIBILITIES BY ROLE

### Architect Agents
**YOU MUST:**
- Design ALL new functions using `module_action_target` pattern
- Update MANIFEST.json with proper naming standards
- Validate all function signatures follow the pattern
- Reject any function names that don't follow the pattern

**VALIDATION COMMANDS:**
```bash
# Check naming compliance before any architectural decision
validate_function_name() {
    local func_name=$1
    
    if ! echo "$func_name" | grep -E "^[a-z]+_[a-z]+_[a-z]+$" > /dev/null; then
        echo "❌ NAMING VIOLATION: '$func_name' doesn't follow module_action_target"
        echo "   Example: 'lexer_scan_token' not 'scan_token' or 'lexer_token'"
        return 1
    fi
    
    echo "✅ Valid naming: $func_name"
    return 0
}

# Validate module exists
validate_module_name() {
    local func_name=$1
    local module=$(echo "$func_name" | cut -d'_' -f1)
    
    local valid_modules="lexer parser ast visitor scope memory error value array object io json string math convert datetime system"
    
    if ! echo "$valid_modules" | grep -q "$module"; then
        echo "❌ INVALID MODULE: '$module' not in approved list"
        echo "   Valid modules: $valid_modules"
        return 1
    fi
    
    echo "✅ Valid module: $module"
    return 0
}
```

### Worker Agents
**YOU MUST:**
- Implement ALL functions exactly as specified in MANIFEST.json
- Use ONLY approved function names following module_action_target
- Report naming violations immediately to architect agents
- NEVER create functions with non-standard names

**IMPLEMENTATION VALIDATION:**
```bash
# Before implementing any function
before_implement_function() {
    local func_name=$1
    
    # Check MANIFEST.json for exact signature
    local signature=$(jq -r ".files[].functions[] | select(.name==\"$func_name\") | .signature" MANIFEST.json)
    
    if [ -z "$signature" ]; then
        echo "❌ FUNCTION NOT IN MANIFEST: $func_name"
        echo "   Request architect to add it with proper naming"
        return 1
    fi
    
    # Validate naming pattern
    if ! validate_function_name "$func_name"; then
        echo "❌ MANIFEST NAMING VIOLATION: $func_name"
        echo "   Report to architect for correction"
        return 1
    fi
    
    echo "✅ Ready to implement: $func_name"
    echo "   Signature: $signature"
    return 0
}
```

### Queen Agents
**YOU MUST:**
- Coordinate naming consistency across your swarm
- Monitor for naming violations in task assignments
- Ensure all agents follow naming standards
- Report cross-swarm naming conflicts

**COORDINATION COMMANDS:**
```bash
# Check swarm naming compliance
check_swarm_naming_compliance() {
    local swarm_id=$1
    local violations=0
    
    echo "🔍 Checking naming compliance for $swarm_id"
    
    # Check active tasks for naming violations
    node task.js list --active | grep "$swarm_id" | while read task; do
        if echo "$task" | grep -E "(zen_stdlib_|init_|json_parse[^_])" > /dev/null; then
            echo "⚠️  Naming violation in task: $task"
            violations=$((violations + 1))
        fi
    done
    
    if [ $violations -gt 0 ]; then
        echo "❌ Found $violations naming violations in $swarm_id"
        echo "   Coordinate with architects to fix"
    else
        echo "✅ $swarm_id naming compliance verified"
    fi
    
    return $violations
}
```

## ENFORCEMENT MECHANISMS

### Pre-Implementation Validation
```bash
# MANDATORY: Run before any function implementation
enforce_naming_standards() {
    local source_file=$1
    local violations=0
    
    echo "🔍 Enforcing naming standards in $source_file"
    
    # Extract all function definitions
    grep -E "^[a-zA-Z_][a-zA-Z0-9_]*\*?\s+[a-zA-Z_][a-zA-Z0-9_]*\(" "$source_file" | while read func_def; do
        local func_name=$(echo "$func_def" | grep -o "[a-zA-Z_][a-zA-Z0-9_]*(" | sed 's/($//')
        
        # Skip static/internal functions (start with _)
        if [[ "$func_name" =~ ^_ ]]; then
            continue
        fi
        
        # Validate naming pattern
        if ! validate_function_name "$func_name"; then
            echo "❌ VIOLATION in $source_file: $func_name"
            violations=$((violations + 1))
        fi
        
        # Validate module assignment
        if ! validate_module_name "$func_name"; then
            echo "❌ MODULE VIOLATION in $source_file: $func_name"
            violations=$((violations + 1))
        fi
    done
    
    if [ $violations -eq 0 ]; then
        echo "✅ Naming standards enforced: $source_file"
    else
        echo "❌ $violations violations found in $source_file"
        echo "   BLOCKED: Fix naming before proceeding"
        return 1
    fi
    
    return 0
}
```

### Git Hook Integration
```bash
# Pre-commit hook to enforce naming
pre_commit_naming_check() {
    echo "🔍 Pre-commit naming standards check"
    
    local violations=0
    
    # Check all modified C files
    git diff --cached --name-only --diff-filter=AM | grep '\.c$' | while read file; do
        if ! enforce_naming_standards "$file"; then
            violations=$((violations + 1))
        fi
    done
    
    if [ $violations -gt 0 ]; then
        echo "❌ COMMIT BLOCKED: Fix $violations naming violations"
        echo "   All functions must follow module_action_target pattern"
        return 1
    fi
    
    echo "✅ Naming standards compliance verified"
    return 0
}
```

### MANIFEST.json Validation
```bash
# Validate MANIFEST.json naming compliance
validate_manifest_naming() {
    echo "🔍 Validating MANIFEST.json naming standards"
    
    local violations=0
    
    # Extract all function names from manifest
    jq -r '.files[].functions[].name' MANIFEST.json | while read func_name; do
        # Skip internal functions
        if [[ "$func_name" =~ ^_ ]]; then
            continue
        fi
        
        if ! validate_function_name "$func_name"; then
            echo "❌ MANIFEST VIOLATION: $func_name"
            violations=$((violations + 1))
        fi
    done
    
    if [ $violations -eq 0 ]; then
        echo "✅ MANIFEST.json naming compliance verified"
    else
        echo "❌ $violations violations in MANIFEST.json"
        echo "   Architect must fix before proceeding"
        return 1
    fi
    
    return 0
}
```

## VIOLATION HANDLING

### Immediate Response to Violations
1. **STOP** all work on violating functions
2. **REPORT** to appropriate architect agent
3. **COORDINATE** with queen agent for resolution
4. **WAIT** for proper naming before continuing

### Automated Correction Suggestions
```bash
# Suggest correct names for common violations
suggest_correct_naming() {
    local incorrect_name=$1
    
    case "$incorrect_name" in
        "zen_stdlib_print") echo "io_print_line" ;;
        "zen_stdlib_read_file") echo "io_read_file" ;;
        "zen_stdlib_write_file") echo "io_write_file" ;;
        "init_lexer") echo "lexer_new" ;;
        "json_parse") echo "json_parse_string" ;;
        "json_stringify") echo "json_stringify_value" ;;
        "zen_to_string") echo "convert_to_string" ;;
        "zen_to_number") echo "convert_to_number" ;;
        *) echo "Follow module_action_target pattern" ;;
    esac
}
```

## SUCCESS METRICS

**Naming Compliance Targets:**
- 100% of core/ functions follow pattern ✅
- 100% of types/ functions follow pattern ✅  
- 100% of stdlib/ functions follow pattern ❌ (needs fixing)
- 0 violations in new code
- MANIFEST.json 100% compliant with standards

**This component ensures consistent, predictable function naming across all 32 swarm agents working on the ZEN project.**