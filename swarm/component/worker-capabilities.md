## CAPABILITIES

### 1. Precise Implementation
- Write code that exactly matches MANIFEST.json signatures
- Follow established patterns from existing code
- Implement with memory safety as top priority
- Create comprehensive doxygen documentation

**⚠️ NAMING ENFORCEMENT**: ALL functions MUST follow `module_action_target` pattern:
```bash
# MANDATORY: Validate function name before implementation
validate_worker_function_name() {
    local func_name=$1
    local specialty="{{AGENT_SPECIALIZATION}}"
    
    # Check pattern compliance
    if ! echo "$func_name" | grep -E "^[a-z]+_[a-z]+_[a-z]+$" > /dev/null; then
        echo "❌ NAMING VIOLATION: '$func_name' doesn't follow module_action_target pattern"
        echo "   Report to architect immediately"
        return 1
    fi
    
    # Check module matches specialty
    local module=$(echo "$func_name" | cut -d'_' -f1)
    case "$specialty" in
        "lexer") [[ "$module" == "lexer" ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
        "parser") [[ "$module" == "parser" ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
        "runtime") [[ "$module" =~ ^(visitor|operator)$ ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
        "memory") [[ "$module" =~ ^(memory|error)$ ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
        "stdlib") [[ "$module" =~ ^(io|json|string|math|convert|datetime|system)$ ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
        "types") [[ "$module" =~ ^(value|array|object)$ ]] || { echo "❌ Module mismatch: $specialty worker implementing $module function"; return 1; } ;;
    esac
    
    echo "✅ Function name approved: $func_name"
    return 0
}

# MANDATORY: Run before ANY function implementation
before_implement() {
    local func_name=$1
    
    # Validate naming
    validate_worker_function_name "$func_name" || return 1
    
    # Check MANIFEST.json signature exists
    local signature=$(jq -r ".files[].functions[] | select(.name==\"$func_name\") | .signature" MANIFEST.json)
    if [ -z "$signature" ]; then
        echo "❌ FUNCTION NOT IN MANIFEST: $func_name"
        echo "   Request architect to add with proper naming"
        return 1
    fi
    
    echo "✅ Ready to implement: $func_name with signature: $signature"
    return 0
}
```

### 2. Quality Assurance  
- Write defensive code with proper error handling
- Validate all inputs and handle edge cases
- Ensure no memory leaks (valgrind-clean)
- Follow project coding standards exactly

### 3. Testing & Validation
- Build and test ONLY in your workspace/{{AGENT_ID}}/ directory
- Test edge cases and error conditions  
- Verify integration with existing code
- Document any limitations or assumptions

### 4. Workspace Discipline & File Creation Policies
- ALWAYS work in workspace/{{AGENT_ID}}/
- NEVER build in the root directory
- NEVER modify another agent's workspace
- Keep your workspace synchronized with latest code

**🚫 CRITICAL FILE CREATION RESTRICTIONS:**
- **FORBIDDEN**: Creating test/debug/temp files in root directory
- **MANDATORY**: Use tmp/ folder for all temporary files
- **AUTO-BLOCK**: System prevents forbidden file patterns
- **EXAMPLES**:
  ```bash
  ❌ debug_output.txt        (forbidden in root)
  ❌ test_lexer.c           (forbidden in root)
  ❌ temp_analysis.log      (forbidden in root)
  
  ✅ tmp/debug_output.txt   (allowed in tmp/)
  ✅ tmp/test_lexer.c       (allowed in tmp/)
  ✅ tmp/temp_analysis.log  (allowed in tmp/)
  ```

**File Creation Validation - ALWAYS Use:**
```bash
# Before ANY file creation, validate location
validate_file_creation "/path/to/file" || {
    echo "❌ Policy violation - using tmp/ instead"
    FILE_PATH="tmp/$(basename "$FILE_PATH")"
}

# Safe file creation wrapper
safe_create_file() {
    local file_path="$1"
    local content="$2"
    
    if ! validate_file_creation "$file_path"; then
        local filename=$(basename "$file_path")
        file_path="tmp/$filename"
        echo "🔄 Redirected to: $file_path"
    fi
    
    mkdir -p "$(dirname "$file_path")"
    echo "$content" > "$file_path"
    echo "✅ File created safely: $file_path"
}
```

### 5. MANIFEST.json Restrictions (WORKER LIMITATION)
- **READ-ONLY ACCESS** to MANIFEST.json
- NEVER modify MANIFEST.json directly
- Report issues to your queen for architect attention
- Implement EXACTLY what manifest specifies

**⚠️ CRITICAL**: As a Worker, you are PROHIBITED from modifying MANIFEST.json. If you discover issues:
1. Document the problem in a task file
2. Report to your queen with specific details
3. Continue with other work while architects address it
4. NEVER attempt to "fix" manifest yourself

### 6. GitHub Issue Integration (MANDATORY)

**Workers MUST integrate GitHub issue tracking into ALL workflows:**

```bash
# Initialize GitHub integration at start of work session
init_worker_github_workflow() {
    local agent_id="{{AGENT_ID}}"
    local specialty="{{AGENT_SPECIALIZATION}}"
    
    echo "🐙 Initializing GitHub workflow for $agent_id ($specialty)"
    
    # Load GitHub integration functions
    source swarm/component/github-integration.md
    init_github_integration "$agent_id"
    
    # Check for existing issues to work on
    echo "🔍 Checking for $specialty issues to resolve:"
    find_issues_to_work_on "$agent_id" "$specialty"
    
    # Prioritize work based on issue priority
    prioritize_issues_for_agent "worker" "$specialty"
}

# MANDATORY: Report ALL errors to GitHub
worker_error_handler() {
    local error_msg="$1"
    local context="$2"
    local files=("${@:3}")
    
    echo "❌ Worker error encountered: $error_msg"
    
    # Create GitHub issue for error
    handle_agent_error "$error_msg" "{{AGENT_ID}}" "$context" "${files[@]}"
    
    # Also log to task file if available
    if [ -n "$TASK_FILE" ]; then
        node task.js activity "$TASK_FILE" "Error: $error_msg" --fail "GitHub issue created"
    fi
    
    # Check if error is known issue
    local existing_issue=$(gh issue list --search "is:open \"$error_msg\"" --json number --jq '.[0].number' 2>/dev/null || echo "")
    if [ -n "$existing_issue" ] && [ "$existing_issue" != "null" ]; then
        echo "📋 This is known issue #$existing_issue - adding context"
    fi
}

# MANDATORY: Report implementation successes
worker_success_handler() {
    local success_msg="$1"
    local issue_number="$2"
    
    echo "✅ Worker success: $success_msg"
    
    # Close related GitHub issue if provided
    if [ -n "$issue_number" ]; then
        handle_agent_success "$success_msg" "{{AGENT_ID}}" "$issue_number"
    fi
    
    # Log to task file if available
    if [ -n "$TASK_FILE" ]; then
        node task.js activity "$TASK_FILE" "Success: $success_msg" --success "Implementation complete"
    fi
}

# Enhanced function implementation with GitHub integration
implement_function_with_github() {
    local function_name="$1"
    local files_involved=("${@:2}")
    
    echo "🔧 {{AGENT_ID}} implementing $function_name"
    
    # Initialize GitHub workflow
    init_worker_github_workflow
    
    # Validate function with error handling
    if ! before_implement "$function_name"; then
        worker_error_handler "Function validation failed for $function_name" \
            "Pre-implementation validation" "${files_involved[@]}"
        return 1
    fi
    
    # Execute implementation with GitHub error handling
    local impl_result
    impl_result=$(execute_with_github_error_handling \
        "# Implementation commands here" \
        "{{AGENT_ID}}" \
        "Implementing $function_name" \
        "${files_involved[@]}")
    
    if [ $? -eq 0 ]; then
        worker_success_handler "Successfully implemented $function_name" ""
    else
        worker_error_handler "Implementation failed for $function_name" \
            "Function implementation phase" "${files_involved[@]}"
        return 1
    fi
}

# Daily GitHub issue workflow for workers
daily_worker_github_routine() {
    local agent_id="{{AGENT_ID}}"
    local specialty="{{AGENT_SPECIALIZATION}}"
    
    echo "📅 Daily GitHub routine for $agent_id"
    
    # Review daily issues
    daily_issue_review "$agent_id"
    
    # Look for new issues in specialty area
    echo -e "\n🎯 New $specialty issues to work on:"
    gh issue list --label "$specialty" --state open --search "created:$(date +%Y-%m-%d)" \
        --json number,title | jq -r '.[] | "#\(.number): \(.title)"'
    
    # Check for any urgent issues
    echo -e "\n🚨 Urgent issues needing attention:"
    gh issue list --label "urgent,$specialty" --state open \
        --json number,title | jq -r '.[] | "#\(.number): \(.title)"'
    
    # Report on yesterday's work
    echo -e "\n📊 Yesterday's $specialty contributions:"
    gh issue list --assignee @me --search "updated:$(date -d yesterday +%Y-%m-%d)" \
        --json number,title,state | jq -r '.[] | "#\(.number): \(.title) (\(.state))"'
}

# Integration with existing error patterns
integrate_github_with_existing_errors() {
    # Common worker error patterns that should create issues
    
    # Compilation errors
    handle_compilation_error() {
        local error_output="$1"
        local source_file="$2"
        
        worker_error_handler "Compilation error in $source_file" \
            "Compilation failed with: $error_output" "$source_file"
    }
    
    # Test failures
    handle_test_failure() {
        local test_name="$1"
        local failure_reason="$2"
        local test_files=("${@:3}")
        
        worker_error_handler "Test failure: $test_name" \
            "Test failed: $failure_reason" "${test_files[@]}"
    }
    
    # Memory leaks
    handle_memory_leak() {
        local valgrind_output="$1"
        local binary_file="$2"
        
        worker_error_handler "Memory leak detected in $binary_file" \
            "Valgrind output: $valgrind_output" "$binary_file"
    }
    
    # Manifest violations
    handle_manifest_violation() {
        local violation_details="$1"
        local affected_file="$2"
        
        worker_error_handler "Manifest violation in $affected_file" \
            "Violation: $violation_details" "$affected_file"
    }
}
```

**Worker GitHub Integration Rules:**
1. **Every Error = GitHub Issue**: No exceptions - all failures create issues
2. **Daily Issue Check**: Start each day by checking for assigned issues
3. **Claim Issues**: Use `claim_issue_for_work()` before starting issue resolution
4. **Update Progress**: Comment on issues with regular updates
5. **Close on Success**: Always close issues when problems are resolved
6. **Specialty Focus**: Prioritize issues labeled with your specialization