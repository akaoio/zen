## FILE CREATION POLICIES - STRICT ENFORCEMENT

### 🚫 CRITICAL: Root Folder Protection

**FORBIDDEN**: Creating test, debug, or temporary files in the root project directory.

**MANDATORY**: All temporary, test, debug, and scratch files MUST be created in the `tmp/` folder.

### Forbidden File Patterns in Root Directory

**Filename Patterns** (case-insensitive):
- `test_*`, `debug_*`, `temp_*`, `tmp_*`, `scratch_*`, `draft_*`
- Files ending in `.test`, `.debug`, `.tmp`, `.temp`, `.bak`, `.old`
- Files containing "test", "debug", "temp", "scratch" anywhere in the name
- Files starting with underscores (except standard config files like `.gitignore`, `.clang-format`)
- Files starting with dots (except standard config files)

**Example Forbidden Files**:
```
❌ test_file.c
❌ debug_output.txt
❌ temp_analysis.md
❌ scratch_work.json
❌ analysis_temp.txt
❌ _debug.log
❌ .temp_config
```

### Mandatory tmp/ Folder Usage

**Required Location**: `/home/x/Projects/zen/tmp/`

**Allowed in tmp/**:
```
✅ tmp/test_lexer_parsing.c
✅ tmp/debug_swarm_coordination.log
✅ tmp/temp_analysis_20250807.md
✅ tmp/scratch_component_evolution.txt
✅ tmp/draft_implementation.c
```

### File Creation Validation Functions

**ALWAYS use these validation functions before creating ANY file:**

```bash
# Validation function - MANDATORY before file creation
validate_file_creation() {
    local file_path="$1"
    local filename=$(basename "$file_path")
    local dirname=$(dirname "$file_path")
    
    # Check if file is in root directory
    if [[ "$dirname" == "." || "$dirname" == "/home/x/Projects/zen" ]]; then
        # Check for forbidden patterns
        if [[ "$filename" =~ ^(test_|debug_|temp_|tmp_|scratch_|draft_) ]] ||
           [[ "$filename" =~ \.(test|debug|tmp|temp|bak|old)$ ]] ||
           [[ "$filename" =~ (test|debug|temp|scratch) ]] ||
           [[ "$filename" =~ ^[_\.] && ! "$filename" =~ ^(\.(gitignore|clang-format|git|github|claude))$ ]]; then
            echo "❌ FORBIDDEN: File '$filename' violates creation policy"
            echo "   Must be created in tmp/ folder instead"
            return 1
        fi
    fi
    
    return 0
}

# Safe file creation function
safe_create_file() {
    local file_path="$1"
    local content="$2"
    
    if ! validate_file_creation "$file_path"; then
        # Auto-redirect to tmp/ folder
        local filename=$(basename "$file_path")
        local tmp_path="tmp/$filename"
        echo "🔄 Redirecting to: $tmp_path"
        file_path="$tmp_path"
    fi
    
    # Ensure tmp directory exists
    mkdir -p tmp/
    
    # Create the file
    echo "$content" > "$file_path"
    echo "✅ File created: $file_path"
}
```

### Integration with Tool Usage

**Before ANY file creation, ALWAYS validate**:

```bash
# Example: Creating a test file
FILE_PATH="debug_analysis.txt"

# MANDATORY validation
if ! validate_file_creation "$FILE_PATH"; then
    FILE_PATH="tmp/debug_analysis.txt"
fi

# Then create file
echo "Debug content..." > "$FILE_PATH"
```

### Automatic Cleanup Policies

**tmp/ folder cleanup rules**:
- Files older than 7 days are automatically removable
- Agents should clean up their own temporary files when tasks complete
- System cleanup runs weekly to remove stale temporary files
- Emergency cleanup triggers if tmp/ exceeds 100MB

**Enhanced Cleanup Functions:**
```bash
# Cleanup function for completed tasks
cleanup_temp_files() {
    local agent_id="$1"
    echo "🧹 Cleaning up temporary files for $agent_id"
    
    # Remove files older than 7 days
    find tmp/ -name "*$agent_id*" -mtime +7 -delete 2>/dev/null || true
    
    # Remove empty directories
    find tmp/ -type d -empty -delete 2>/dev/null || true
    
    echo "✅ Cleanup completed for $agent_id"
}

# Emergency cleanup when tmp/ gets too large
emergency_tmp_cleanup() {
    local tmp_size=$(du -sm tmp/ | cut -f1)
    
    if [ $tmp_size -gt 100 ]; then
        echo "🚨 EMERGENCY: tmp/ folder is ${tmp_size}MB - cleaning up"
        
        # Remove files older than 3 days (emergency threshold)
        find tmp/ -type f -mtime +3 -delete 2>/dev/null || true
        
        # Remove files larger than 10MB
        find tmp/ -type f -size +10M -delete 2>/dev/null || true
        
        # Remove empty directories
        find tmp/ -type d -empty -delete 2>/dev/null || true
        
        local new_size=$(du -sm tmp/ | cut -f1)
        echo "✅ Emergency cleanup completed: ${tmp_size}MB → ${new_size}MB"
    fi
}

# System-wide cleanup policy
system_cleanup_policy() {
    echo "🧹 System-wide tmp/ cleanup policy enforcement"
    
    # Daily cleanup for files older than 7 days
    find tmp/ -type f -mtime +7 -delete 2>/dev/null || true
    
    # Weekly cleanup for files older than 3 days
    if [ "$(date +%u)" -eq 1 ]; then  # Monday cleanup
        find tmp/ -type f -mtime +3 -delete 2>/dev/null || true
    fi
    
    # Emergency cleanup check
    emergency_tmp_cleanup
    
    # Audit and report
    local file_count=$(find tmp/ -type f | wc -l)
    local total_size=$(du -sh tmp/ | cut -f1)
    echo "📊 tmp/ status: $file_count files, $total_size total"
}

# Task completion cleanup hook
task_completion_cleanup() {
    local task_file="$1"
    local agent_id="$2"
    
    # Extract task-specific temporary files created
    local task_temps=$(grep -o "tmp/[^[:space:]]*" "$task_file" 2>/dev/null || true)
    
    for temp_file in $task_temps; do
        if [ -f "$temp_file" ]; then
            echo "🧹 Cleaning up task-specific temp file: $temp_file"
            rm -f "$temp_file" 2>/dev/null || true
        fi
    done
    
    # Agent-specific cleanup
    cleanup_temp_files "$agent_id"
}
```

### Enforcement Through All Tools

**Write Tool Enhancement**:
- ALWAYS validate file paths before using Write tool
- Redirect forbidden patterns to tmp/ automatically
- Log policy violations for monitoring

**Edit Tool Enhancement**:
- Creating new files through Edit tool must follow same policies
- Multi-edit operations must validate ALL file paths

**Agent Workflow Integration**:
- Add validation to all file creation workflows
- Include policy check in task creation process
- Report violations in task completion summaries

### Policy Violation Handling

**If agent attempts forbidden file creation**:
1. **Block the action** - Do not create the file
2. **Auto-redirect** - Move to tmp/ folder with warning
3. **Log violation** - Record in task activity
4. **Educate agent** - Explain why it was blocked

**Example violation handling**:
```bash
# Agent attempts: echo "test" > debug_file.txt
# System response:
echo "❌ POLICY VIOLATION: debug_file.txt forbidden in root"
echo "🔄 Auto-redirecting to: tmp/debug_file.txt"
echo "test" > tmp/debug_file.txt
echo "✅ File created in proper location"
```

### Standard Config File Exceptions

**Allowed in root directory** (configuration files only):
- `.gitignore`, `.clang-format`, `.github/`, `.claude/`
- `Makefile`, `MANIFEST.json`, `ARCHITECTURE.md`
- Standard project files defined in MANIFEST.json

**NOT allowed** (even if they look like config):
- `.temp_config`, `._backup`, `.debug_settings`

### Agent-Specific Validation Functions

**Role-based validation for all agent types:**
```bash
# Queen file creation validation
queen_validate_file_creation() {
    local file_path="$1"
    local agent_id="$2"
    
    # Queens cannot create files directly, only coordinate
    if [[ "$file_path" != tmp/* ]]; then
        echo "❌ QUEEN RESTRICTION: Queens cannot create files outside tmp/"
        echo "   Queens coordinate - workers implement"
        return 1
    fi
    
    # Allow coordination files in tmp/
    if [[ "$file_path" =~ tmp/coordination_ || "$file_path" =~ tmp/planning_ ]]; then
        echo "✅ Queen coordination file allowed: $file_path"
        return 0
    fi
    
    # Standard validation for other tmp/ files
    return $(validate_file_creation "$file_path")
}

# Architect file creation validation  
architect_validate_file_creation() {
    local file_path="$1"
    local agent_id="$2"
    
    # Architects can create architectural files in src/
    if [[ "$file_path" =~ ^src/ ]]; then
        echo "✅ Architect authorized: $file_path"
        return 0
    fi
    
    # Standard validation for all other files
    return $(validate_file_creation "$file_path")
}

# Worker file creation validation
worker_validate_file_creation() {
    local file_path="$1" 
    local agent_id="$2"
    local worker_specialty="$3"
    
    # Workers can only create files in tmp/ or their workspace
    if [[ "$file_path" =~ ^tmp/ || "$file_path" =~ ^workspace/ ]]; then
        echo "✅ Worker file location approved: $file_path"
        return 0
    fi
    
    echo "❌ WORKER RESTRICTION: Workers can only create files in tmp/ or workspace/"
    echo "   Use tmp/ for temporary files"
    return 1
}

# Overlord file creation validation
overlord_validate_file_creation() {
    local file_path="$1"
    local agent_id="$2"
    
    # Overlords can only create analysis files in tmp/
    if [[ "$file_path" =~ ^tmp/overlord_ || "$file_path" =~ ^tmp/analysis_ ]]; then
        echo "✅ Overlord analysis file allowed: $file_path"
        return 0
    fi
    
    echo "❌ OVERLORD RESTRICTION: Can only create analysis files in tmp/"
    return 1
}

# Universal validation dispatcher
dispatch_agent_file_validation() {
    local file_path="$1"
    local agent_id="$2"
    
    # Extract agent role from ID
    local agent_role=""
    if [[ "$agent_id" =~ queen ]]; then
        agent_role="queen"
    elif [[ "$agent_id" =~ architect ]]; then
        agent_role="architect"
    elif [[ "$agent_id" =~ worker ]]; then
        agent_role="worker"
    elif [[ "$agent_id" =~ overlord ]]; then
        agent_role="overlord"
    else
        echo "❌ UNKNOWN AGENT TYPE: $agent_id"
        return 1
    fi
    
    # Dispatch to role-specific validation
    case "$agent_role" in
        "queen")
            queen_validate_file_creation "$file_path" "$agent_id"
            ;;
        "architect") 
            architect_validate_file_creation "$file_path" "$agent_id"
            ;;
        "worker")
            local specialty=$(echo "$agent_id" | sed 's/.*worker-\([^-]*\).*/\1/')
            worker_validate_file_creation "$file_path" "$agent_id" "$specialty"
            ;;
        "overlord")
            overlord_validate_file_creation "$file_path" "$agent_id"
            ;;
    esac
}
```

### Integration Points

This policy MUST be integrated into:
1. **All swarm components** - Every agent follows these rules
2. **Task validation** - Check before starting any file creation task
3. **Tool wrappers** - Validate in all file creation utilities
4. **Safety constraints** - Core safety requirement
5. **Role boundaries** - Applies to all agent types with role-specific rules
6. **Agent DNA** - Validation functions embedded in all agent types

### Monitoring and Metrics

**Track policy compliance**:
- Count policy violations per agent
- Monitor tmp/ folder usage patterns
- Report cleanup effectiveness
- Identify agents needing additional training

```bash
# Enhanced policy compliance check with agent tracking
check_policy_compliance() {
    echo "📊 File Creation Policy Compliance Report"
    echo "========================================"
    echo "Generated: $(date)"
    echo ""
    
    # Check for violations in root directory
    local violations=0
    local violation_files=()
    
    # Forbidden patterns in root
    for pattern in "test_*" "debug_*" "temp_*" "scratch_*" "*_test.*" "*_debug.*" "*_temp.*" "*_scratch.*"; do
        while IFS= read -r -d '' file; do
            if [[ ! "$file" =~ ^tmp/ ]]; then
                violations=$((violations + 1))
                violation_files+=("$file")
            fi
        done < <(find . -maxdepth 1 -name "$pattern" -print0 2>/dev/null)
    done
    
    echo "Root directory violations: $violations"
    if [ $violations -gt 0 ]; then
        echo "❌ VIOLATIONS DETECTED:"
        for file in "${violation_files[@]}"; do
            echo "  - $file"
        done
    fi
    
    # Check tmp/ usage and organization
    local tmp_files=$(find tmp/ -type f 2>/dev/null | wc -l)
    local tmp_size=$(du -sh tmp/ 2>/dev/null | cut -f1 || echo "0")
    echo ""
    echo "tmp/ folder status:"
    echo "  - Files: $tmp_files"
    echo "  - Total size: $tmp_size"
    
    # Agent-specific tmp/ usage analysis
    echo ""
    echo "Agent tmp/ usage breakdown:"
    for agent_pattern in "overlord" "queen" "architect" "worker-lexer" "worker-parser" "worker-runtime" "worker-memory" "worker-stdlib" "worker-types"; do
        local agent_files=$(find tmp/ -name "*${agent_pattern}*" 2>/dev/null | wc -l)
        if [ $agent_files -gt 0 ]; then
            echo "  - $agent_pattern: $agent_files files"
        fi
    done
    
    # Check for orphaned files (no agent identifier)
    local orphaned_files=$(find tmp/ -type f ! -name "*overlord*" ! -name "*queen*" ! -name "*architect*" ! -name "*worker*" ! -name "*swarm*" 2>/dev/null | wc -l)
    if [ $orphaned_files -gt 0 ]; then
        echo "  - ⚠️  Orphaned files (no agent ID): $orphaned_files"
    fi
    
    # Overall compliance status
    echo ""
    echo "=== COMPLIANCE STATUS ==="
    if [ $violations -eq 0 ]; then
        echo "✅ FULL COMPLIANCE: No policy violations detected"
        echo "📈 Compliance Score: 100%"
    else
        local compliance_score=$(echo "scale=1; ((1000 - $violations * 100) / 10)" | bc)
        if [ $(echo "$compliance_score < 0" | bc) -eq 1 ]; then
            compliance_score="0.0"
        fi
        echo "❌ VIOLATIONS DETECTED: $violations policy violations"
        echo "📉 Compliance Score: ${compliance_score}%"
        echo ""
        echo "🔧 REMEDIATION REQUIRED:"
        echo "   1. Move violating files to tmp/ folder"
        echo "   2. Update agent workflows to use proper validation"
        echo "   3. Review agent DNA for policy integration"
    fi
    
    # Cleanup recommendations
    if [ $tmp_files -gt 100 ]; then
        echo ""
        echo "🧹 CLEANUP RECOMMENDATION: tmp/ has $tmp_files files"
        echo "   Run: system_cleanup_policy"
    fi
    
    echo ""
    echo "Report saved to: tmp/policy_compliance_$(date +%Y%m%d_%H%M).log"
}

# Real-time policy violation detection
monitor_policy_violations() {
    echo "🔍 Starting real-time policy violation monitoring..."
    
    # Monitor file creation in real-time using inotify
    inotifywait -m -r -e create --exclude 'tmp/.*' . 2>/dev/null | while read path action file; do
        # Skip if it's in tmp/ folder
        if [[ "$path" =~ ^tmp/ ]]; then
            continue
        fi
        
        # Check if new file violates policy
        local full_path="${path}${file}"
        if ! validate_file_creation "$full_path" 2>/dev/null; then
            echo "🚨 REAL-TIME VIOLATION: $full_path"
            echo "   $(date): Policy violation detected"
            echo "   Action: File created in root with forbidden pattern"
            echo "   Recommendation: Move to tmp/ folder immediately"
            
            # Log the violation
            echo "$(date): VIOLATION: $full_path" >> tmp/policy_violations.log
            
            # Optional: Auto-move to tmp/ (if safe to do so)
            if [[ "$file" =~ ^(test_|debug_|temp_) ]]; then
                echo "   🔄 AUTO-FIXING: Moving to tmp/$file"
                mv "$full_path" "tmp/$file" 2>/dev/null || true
            fi
        fi
    done
}

# Agent compliance scoring
get_agent_compliance_score() {
    local agent_id="$1"
    local violations=$(grep -c "$agent_id" tmp/policy_violations.log 2>/dev/null || echo "0")
    local total_files=$(find tmp/ -name "*${agent_id}*" | wc -l)
    
    if [ $total_files -eq 0 ]; then
        echo "100"  # No files created = perfect compliance
    else
        local compliance=$(echo "scale=1; ((${total_files} - ${violations}) * 100) / ${total_files}" | bc)
        echo "$compliance"
    fi
}
```

This policy ensures clean project organization and prevents root directory clutter across all 32 swarm agents.