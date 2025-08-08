## SAFETY CONSTRAINTS & ROLLBACK SYSTEM

### HUMAN APPROVAL GATES

**Major Changes Requiring Human Approval:**
```bash
# Define what constitutes a "major change"
is-major-change() {
    local change_type=$1
    local scope=$2
    
    # Major changes requiring human approval
    case "$change_type" in
        "component-creation"|"component-deletion"|"architecture-change"|"role-creation")
            return 0  # Requires approval
            ;;
        "multi-file-edit")
            if [ $(echo "$scope" | wc -w) -gt 5 ]; then
                return 0  # More than 5 files = major change
            fi
            ;;
        "swarm-modification")
            return 0  # All swarm structure changes
            ;;
    esac
    
    return 1  # Not a major change
}

# Request human approval for major changes
request-human-approval() {
    local change_description=$1
    local impact_assessment=$2
    local rollback_plan=$3
    
    local approval_file="approval-request-$(date +%Y%m%d-%H%M%S).md"
    
    cat > "$approval_file" << EOF
# HUMAN APPROVAL REQUIRED

## Proposed Change
$change_description

## Impact Assessment
$impact_assessment

## Rollback Plan
$rollback_plan

## Current System State
- Health Score: $(get-system-health)%
- Coordination Efficiency: $(get-coordination-efficiency)%
- Active Tasks: $(node task.js list --active | wc -l)

## Evidence Supporting This Change
(Please provide quantitative evidence before approval)

## Approval Status
[ ] APPROVED - Human has reviewed and approved this change
[ ] REJECTED - Change rejected, maintain current system
[ ] NEEDS_MORE_INFO - Require additional evidence or clarification

Approved by: _______________
Date: _______________
EOF
    
    echo "APPROVAL REQUIRED: Created $approval_file"
    echo "System will wait for human approval before proceeding."
    return 1  # Block execution until approved
}
```

### AUTONOMOUS ACTION LIMITS

**Strict Boundaries for Autonomous Behavior:**
```bash
# Define what agents CAN do autonomously
can-do-autonomously() {
    local action=$1
    local agent_role=$2
    
    case "$action" in
        "fix-single-bug"|"implement-function"|"update-documentation"|"run-tests")
            return 0  # Allowed
            ;;
        "create-file")
            if [ "$agent_role" = "architect" ]; then
                return 0  # Only architects can create files (with path validation)
            fi
            ;;
        "modify-manifest")
            if [ "$agent_role" = "architect" ]; then
                return 0  # Only architects can modify manifest
            fi
            ;;
        "create-temp-file")
            return 0  # All agents can create files in tmp/ folder
            ;;
    esac
    
    return 1  # Not allowed autonomously
}

# Define what agents CANNOT do autonomously
cannot-do-autonomously() {
    local action=$1
    
    case "$action" in
        "delete-files"|"change-architecture"|"modify-swarm-structure"|"create-new-roles"|"bulk-refactor")
            return 0  # Blocked
            ;;
        "create-root-test-file"|"create-root-debug-file"|"create-root-temp-file")
            return 0  # Blocked - must use tmp/ folder
            ;;
    esac
    
    return 1  # Action is allowed
}

# Enforce autonomous action limits
enforce-autonomous-limits() {
    local action=$1
    local agent_role=$2
    
    if cannot-do-autonomously "$action"; then
        echo "BLOCKED: Action '$action' requires human approval"
        request-human-approval "$action" "High impact system change" "See rollback procedures below"
        return 1
    fi
    
    if ! can-do-autonomously "$action" "$agent_role"; then
        echo "BLOCKED: Agent role '$agent_role' cannot perform '$action' autonomously"
        return 1
    fi
    
    return 0
}
```

### ROLLBACK CAPABILITIES

**Comprehensive Rollback System:**
```bash
# Create system snapshot before major changes
create-system-snapshot() {
    local snapshot_name="snapshot-$(date +%Y%m%d-%H%M%S)"
    local snapshot_dir="snapshots/$snapshot_name"
    
    mkdir -p "$snapshot_dir"
    
    # Backup critical system files
    cp -r swarm/components "$snapshot_dir/"
    cp -r .claude/agents "$snapshot_dir/"
    cp -r tasks "$snapshot_dir/"
    cp MANIFEST.json "$snapshot_dir/"
    cp task.js "$snapshot_dir/"
    
    # Record system state
    cat > "$snapshot_dir/system-state.md" << EOF
# System Snapshot: $snapshot_name

## System Health
- Health Score: $(get-system-health)%
- Coordination Efficiency: $(get-coordination-efficiency)%
- Test Pass Rate: $(make test 2>/dev/null | grep -o '[0-9]*% passed' | grep -o '[0-9]*' || echo "unknown")

## Active Tasks
$(node task.js list --active | wc -l) active tasks

## Git Status
$(git status --porcelain | head -10)
EOF
    
    echo "$snapshot_name" > .latest-snapshot
    echo "System snapshot created: $snapshot_name"
}

# Rollback to previous system state
rollback-to-snapshot() {
    local snapshot_name=$1
    local snapshot_dir="snapshots/$snapshot_name"
    
    if [ ! -d "$snapshot_dir" ]; then
        echo "ERROR: Snapshot $snapshot_name not found"
        return 1
    fi
    
    echo "Rolling back to snapshot: $snapshot_name"
    
    # Create emergency backup of current state
    local emergency_backup="emergency-backup-$(date +%Y%m%d-%H%M%S)"
    mkdir -p "snapshots/$emergency_backup"
    cp -r swarm/components "snapshots/$emergency_backup/"
    cp -r .claude/agents "snapshots/$emergency_backup/"
    
    # Restore from snapshot
    cp -r "$snapshot_dir/components" swarm/
    cp -r "$snapshot_dir/agents" .claude/
    cp "$snapshot_dir/MANIFEST.json" ./
    cp "$snapshot_dir/task.js" ./
    
    # Clean up active tasks that may be invalid after rollback
    find tasks/ -name "*.yaml" -exec grep -l "completed: false" {} \; | xargs rm -f
    
    # Regenerate all agents from restored components
    for swarm in 1 2 3 4; do
        ./swarm.sh clean
        ./swarm.sh create-swarm "swarm-$swarm" >/dev/null 2>&1
    done
    
    echo "Rollback completed. Emergency backup saved as: $emergency_backup"
}

# Auto-rollback on system failure
auto-rollback-on-failure() {
    local health=$(get-system-health)
    
    if [ $health -lt 30 ]; then
        echo "CRITICAL: System health at ${health}%. Initiating auto-rollback."
        
        if [ -f .latest-snapshot ]; then
            local latest_snapshot=$(cat .latest-snapshot)
            echo "Rolling back to latest snapshot: $latest_snapshot"
            rollback-to-snapshot "$latest_snapshot"
        else
            echo "No snapshot available for rollback. Manual intervention required."
        fi
    fi
}
```

### INCREMENTAL CHANGE ENFORCEMENT

**Force Small, Testable Changes:**
```bash
# Validate change size and impact
validate-change-size() {
    local files_changed=($1)
    local lines_changed=$2
    
    # Limit concurrent file modifications
    if [ ${#files_changed[@]} -gt 3 ]; then
        echo "BLOCKED: Change affects ${#files_changed[@]} files. Maximum 3 files per change."
        return 1
    fi
    
    # Limit lines changed per modification
    if [ $lines_changed -gt 100 ]; then
        echo "BLOCKED: Change affects $lines_changed lines. Maximum 100 lines per change."
        return 1
    fi
    
    return 0
}

# Enforce incremental development
enforce-incremental-changes() {
    local change_description=$1
    
    # Require test after each change
    if ! make test >/dev/null 2>&1; then
        echo "BLOCKED: Tests must pass before making additional changes"
        return 1
    fi
    
    # Create snapshot before each significant change
    create-system-snapshot
    
    return 0
}
```

### SAFETY MONITORING

**Continuous Safety Monitoring:**
```bash
# Monitor system safety continuously
monitor-system-safety() {
    while true; do
        local health=$(get-system-health)
        local coord_eff=$(get-coordination-efficiency)
        
        # Check for critical issues
        if [ $health -lt 40 ]; then
            echo "ALERT: System health critical ($health%)"
            auto-rollback-on-failure
        fi
        
        if [ $coord_eff -lt 50 ]; then
            echo "ALERT: Coordination breakdown ($coord_eff% efficiency)"
            # Temporarily halt new task creation
            touch .coordination-pause
        fi
        
        # Check for runaway agents
        local runaway_count=$(ps aux | grep -c "[z]en-.*" | head -1)
        if [ $runaway_count -gt 50 ]; then
            echo "ALERT: Too many agent processes ($runaway_count). Possible runaway condition."
            # Kill excessive processes
            killall zen-worker zen-queen zen-architect 2>/dev/null
        fi
        
        sleep 60  # Check every minute
    done
}
```

### SAFETY COMMAND INTEGRATION

**Safety Commands Available to All Agents:**
```bash
# Emergency stop command
emergency-stop() {
    echo "EMERGENCY STOP ACTIVATED"
    
    # Stop all active agents
    touch .emergency-stop
    
    # Create emergency snapshot
    create-system-snapshot
    
    # Kill all swarm processes
    killall -TERM zen-worker zen-queen zen-architect 2>/dev/null
    
    echo "All agents stopped. System state preserved."
}

# Safety check before any action
safety-check() {
    local action=$1
    local agent_role=$2
    local file_path=$3
    
    # Check emergency stop
    if [ -f .emergency-stop ]; then
        echo "BLOCKED: Emergency stop active. Manual intervention required."
        return 1
    fi
    
    # Check coordination pause
    if [ -f .coordination-pause ]; then
        echo "BLOCKED: Coordination paused due to efficiency issues."
        return 1
    fi
    
    # Enforce file creation policies
    if [[ "$action" =~ "create-file" && -n "$file_path" ]]; then
        if ! validate_file_creation "$file_path"; then
            echo "BLOCKED: File creation policy violation for: $file_path"
            echo "Use tmp/ folder for temporary, test, or debug files."
            return 1
        fi
    fi
    
    # Enforce autonomous limits
    if ! enforce-autonomous-limits "$action" "$agent_role"; then
        return 1
    fi
    
    return 0
}
```

### SAFETY INTEGRATION

Every agent action now requires:
1. **Safety check** - Verify action is allowed and safe
2. **Validation** - Confirm system health and readiness
3. **Approval gate** - Human approval for major changes
4. **Rollback preparation** - Snapshot before significant changes
5. **Continuous monitoring** - Auto-rollback on failure

This creates a robust safety net preventing autonomous overreach while maintaining system productivity.