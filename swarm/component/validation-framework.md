## VALIDATION FRAMEWORK

### MEASURABLE EFFECTIVENESS METRICS

**Agent Performance Tracking:**
```bash
# Track individual agent effectiveness
get-agent-effectiveness() {
    local agent_id=$1
    local completed_tasks=$(node task.js list --completed | grep -c "$agent_id" || echo "0")
    local failed_tasks=$(node task.js list --failed | grep -c "$agent_id" || echo "0")
    local total_tasks=$(($completed_tasks + $failed_tasks))
    
    if [ $total_tasks -eq 0 ]; then
        echo "0"
    else
        local success_rate=$(echo "scale=2; ($completed_tasks * 100) / $total_tasks" | bc)
        echo "$success_rate"
    fi
}

# Measure coordination efficiency
get-coordination-efficiency() {
    local conflicts=$(make vision | grep -E "\\[(swarm-[1-4])" | wc -l)
    local active_agents=$(make vision | grep -E "\\[(swarm-[1-4])" | cut -d'[' -f2 | cut -d']' -f1 | sort -u | wc -l)
    
    if [ $active_agents -eq 0 ]; then
        echo "100"
    else
        local conflict_ratio=$(echo "scale=2; ($conflicts * 100) / $active_agents" | bc)
        local efficiency=$(echo "scale=2; 100 - $conflict_ratio" | bc)
        echo "$efficiency"
    fi
}
```

**System Health Indicators:**
```bash
# Overall system health score (0-100)
get-system-health() {
    local test_pass_rate=$(make test 2>/dev/null | grep -o '[0-9]*% passed' | grep -o '[0-9]*' || echo "0")
    local build_success=$(make clean && make >/dev/null 2>&1 && echo "100" || echo "0")
    local coordination_eff=$(get-coordination-efficiency)
    
    local health=$(echo "scale=0; ($test_pass_rate + $build_success + $coordination_eff) / 3" | bc)
    echo "$health"
}

# Productivity measurement
get-productivity-score() {
    local recent_commits=$(git log --since="24 hours ago" --oneline | wc -l)
    local recent_completed=$(find tasks/ -name "*.yaml" -newer tasks/baseline.marker 2>/dev/null | xargs grep -l "completed: true" | wc -l)
    local productivity=$(echo "scale=0; ($recent_commits * 10) + ($recent_completed * 5)" | bc)
    echo "$productivity"
}
```

### VALIDATION GATES

**Pre-Action Validation:**
```bash
# Validate action before execution
validate-action() {
    local action_type=$1
    local agent_id=$2
    
    # Check system health threshold
    local health=$(get-system-health)
    if [ $health -lt 50 ]; then
        echo "BLOCKED: System health too low ($health%). Focus on fixing existing issues."
        return 1
    fi
    
    # Check agent effectiveness
    local agent_eff=$(get-agent-effectiveness "$agent_id")
    if [ $(echo "$agent_eff < 60" | bc) -eq 1 ]; then
        echo "WARNING: Agent effectiveness low ($agent_eff%). Consider role reassignment."
    fi
    
    # Check coordination load
    local coord_eff=$(get-coordination-efficiency)
    if [ $coord_eff -lt 70 ]; then
        echo "WARNING: High coordination overhead (${coord_eff}% efficiency). Limit concurrent work."
    fi
    
    return 0
}

# Validate before component modifications
validate-component-change() {
    local component=$1
    local change_type=$2
    
    echo "Validating component change: $component ($change_type)"
    
    # Check if change is evidence-based
    if [ "$change_type" = "theoretical" ]; then
        echo "BLOCKED: Only evidence-based changes allowed. Provide metrics justification."
        return 1
    fi
    
    # Backup before change
    cp "swarm/components/$component.md" "swarm/components/$component.md.backup-$(date +%s)"
    
    return 0
}
```

### EVIDENCE-BASED DECISION FRAMEWORK

**Require Evidence for Changes:**
```bash
# Check if change is justified by data
require-evidence() {
    local change_description=$1
    local evidence_file="$2"
    
    if [ ! -f "$evidence_file" ]; then
        echo "BLOCKED: Evidence file required for change: $change_description"
        echo "Create evidence file with metrics, measurements, or specific problem examples."
        return 1
    fi
    
    # Validate evidence contains quantitative data
    if ! grep -q -E '[0-9]+%|[0-9]+ (tasks|conflicts|failures)' "$evidence_file"; then
        echo "BLOCKED: Evidence must contain quantitative metrics"
        return 1
    fi
    
    echo "Evidence validated for: $change_description"
    return 0
}
```

### VALIDATION REPORTING

**Daily System Validation Report:**
```bash
# Generate comprehensive system validation report
generate-validation-report() {
    local report_file="validation-report-$(date +%Y%m%d).md"
    
    cat > "$report_file" << EOF
# System Validation Report - $(date)

## Health Metrics
- System Health: $(get-system-health)%
- Coordination Efficiency: $(get-coordination-efficiency)%
- Productivity Score: $(get-productivity-score)

## Agent Effectiveness
EOF
    
    for swarm in 1 2 3 4; do
        echo "### Swarm-$swarm Agents" >> "$report_file"
        for role in queen architect worker-lexer worker-parser worker-runtime worker-memory worker-stdlib worker-types; do
            local agent_id="swarm-$swarm-zen-$role"
            local effectiveness=$(get-agent-effectiveness "$agent_id")
            echo "- $agent_id: ${effectiveness}%" >> "$report_file"
        done
        echo "" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## System Issues Detected
$(make test 2>&1 | grep -E "(FAIL|ERROR)" | head -10)

## Recommendations
$(get-system-recommendations)
EOF
    
    echo "Validation report generated: $report_file"
}

# Get actionable recommendations based on metrics
get-system-recommendations() {
    local health=$(get-system-health)
    local coord_eff=$(get-coordination-efficiency)
    
    if [ $health -lt 70 ]; then
        echo "- CRITICAL: Fix failing tests before adding new features"
    fi
    
    if [ $coord_eff -lt 80 ]; then
        echo "- Reduce concurrent file modifications to improve coordination"
    fi
    
    # Check for consistently underperforming agents
    for swarm in 1 2 3 4; do
        for role in queen architect worker-lexer worker-parser worker-runtime worker-memory worker-stdlib worker-types; do
            local agent_id="swarm-$swarm-zen-$role"
            local effectiveness=$(get-agent-effectiveness "$agent_id")
            if [ $(echo "$effectiveness < 50" | bc) -eq 1 ]; then
                echo "- Consider reassigning or retraining $agent_id (${effectiveness}% effective)"
            fi
        done
    done
}
```

### VALIDATION ENFORCEMENT

**Automatic Validation Hooks:**
```bash
# Hook into task creation
validate-task-creation() {
    local agent_id=$1
    local task_description=$2
    
    # Validate system readiness
    if ! validate-action "task-creation" "$agent_id"; then
        return 1
    fi
    
    # Validate task is not duplicate
    if node task.js list --active | grep -q "$task_description"; then
        echo "WARNING: Similar active task exists. Consider coordination."
    fi
    
    return 0
}

# Hook into file modifications
validate-file-modification() {
    local file_path=$1
    local agent_id=$2
    
    # Check if file is already being modified
    if make vision | grep -q "$file_path.*\\[.*\\]"; then
        echo "BLOCKED: File $file_path already being modified by another agent"
        return 1
    fi
    
    return 0
}

# Hook into file creation
validate-file-creation-hook() {
    local file_path=$1
    local agent_id=$2
    local action_type=$3
    
    # Apply file creation policies
    if ! validate_file_creation "$file_path"; then
        echo "BLOCKED: File creation policy violation"
        echo "File: $file_path violates creation policy"
        echo "Solution: Create temporary files in tmp/ folder"
        return 1
    fi
    
    # Log policy compliance for tracking
    echo "$(date): $agent_id - File creation validated: $file_path" >> tmp/file_creation_audit.log
    
    return 0
}
```

This validation framework provides:
1. **Quantitative metrics** for measuring system effectiveness
2. **Evidence-based decisions** requiring data justification
3. **Automatic validation gates** preventing harmful actions
4. **Regular health monitoring** with actionable recommendations
5. **Performance tracking** for individual agents and overall system

All changes now require measurable justification and pass validation thresholds.