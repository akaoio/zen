## PRACTICAL MULTI-SWARM METRICS

### KEY PERFORMANCE INDICATORS

**System Effectiveness Metrics:**
```bash
# 1. Development Velocity - Lines of quality code per day
get-development-velocity() {
    local commits_today=$(git log --since="24 hours ago" --oneline | wc -l)
    local lines_added=$(git log --since="24 hours ago" --numstat | awk '{added+=$1} END {print added+0}')
    local lines_removed=$(git log --since="24 hours ago" --numstat | awk '{removed+=$2} END {print removed+0}')
    local net_lines=$(($lines_added - $lines_removed))
    
    echo "Development Velocity:"
    echo "- Commits today: $commits_today"
    echo "- Net lines added: $net_lines"
    echo "- Velocity score: $(echo "scale=1; $net_lines / 10" | bc)"
}

# 2. Quality Score - Test pass rate and build success
get-quality-score() {
    local test_output=$(make test 2>&1)
    local tests_passed=$(echo "$test_output" | grep -c "PASS\|passed" || echo "0")
    local tests_failed=$(echo "$test_output" | grep -c "FAIL\|failed" || echo "0")
    local total_tests=$(($tests_passed + $tests_failed))
    
    local build_success=$(make clean && make >/dev/null 2>&1 && echo "1" || echo "0")
    
    if [ $total_tests -eq 0 ]; then
        local pass_rate=0
    else
        local pass_rate=$(echo "scale=1; ($tests_passed * 100) / $total_tests" | bc)
    fi
    
    local quality=$(echo "scale=1; ($pass_rate + ($build_success * 100)) / 2" | bc)
    
    echo "Quality Score:"
    echo "- Tests passed: $tests_passed/$total_tests (${pass_rate}%)"
    echo "- Build success: $([ $build_success -eq 1 ] && echo "Yes" || echo "No")"
    echo "- Overall quality: ${quality}%"
}

# 3. Coordination Efficiency - File conflicts and task completion rate
get-coordination-efficiency() {
    local active_conflicts=$(make vision | grep -E "\[(swarm-[1-4])" | wc -l)
    local total_active_tasks=$(node task.js list --active | wc -l)
    local completed_today=$(find tasks/ -name "*.yaml" -newermt "24 hours ago" -exec grep -l "completed: true" {} \; | wc -l)
    local failed_today=$(find tasks/ -name "*.yaml" -newermt "24 hours ago" -exec grep -l "completed: false" {} \; | grep -v "$(date +%Y%m%d)" | wc -l)
    
    local conflict_ratio=$(echo "scale=1; ($active_conflicts * 100) / ($total_active_tasks + 1)" | bc)
    local completion_ratio=$(echo "scale=1; ($completed_today * 100) / ($completed_today + $failed_today + 1)" | bc)
    local coordination_eff=$(echo "scale=1; (100 - $conflict_ratio + $completion_ratio) / 2" | bc)
    
    echo "Coordination Efficiency:"
    echo "- Active file conflicts: $active_conflicts"
    echo "- Task completion rate: ${completion_ratio}%"
    echo "- Coordination efficiency: ${coordination_eff}%"
}
```

### AGENT PRODUCTIVITY METRICS

**Individual Agent Performance:**
```bash
# Track individual agent effectiveness over time
get-agent-productivity() {
    local agent_id=$1
    local period=${2:-"7 days"}
    
    local tasks_completed=$(find tasks/ -name "*.yaml" -newermt "$period ago" -exec grep -l "$agent_id" {} \; | xargs grep -l "completed: true" | wc -l)
    local tasks_failed=$(find tasks/ -name "*.yaml" -newermt "$period ago" -exec grep -l "$agent_id" {} \; | xargs grep -l "completed: false" | wc -l)
    local total_tasks=$(($tasks_completed + $tasks_failed))
    
    if [ $total_tasks -eq 0 ]; then
        local success_rate=0
    else
        local success_rate=$(echo "scale=1; ($tasks_completed * 100) / $total_tasks" | bc)
    fi
    
    echo "$agent_id Productivity (last $period):"
    echo "- Tasks completed: $tasks_completed"
    echo "- Tasks failed: $tasks_failed" 
    echo "- Success rate: ${success_rate}%"
}

# Swarm-wide productivity comparison
compare-swarm-productivity() {
    echo "Swarm Productivity Comparison:"
    for swarm in 1 2 3 4; do
        local swarm_tasks=$(find tasks/ -name "*.yaml" -newermt "24 hours ago" -exec grep -l "swarm-$swarm" {} \; | wc -l)
        local swarm_completed=$(find tasks/ -name "*.yaml" -newermt "24 hours ago" -exec grep -l "swarm-$swarm" {} \; | xargs grep -l "completed: true" | wc -l)
        local swarm_rate=$(echo "scale=1; ($swarm_completed * 100) / ($swarm_tasks + 1)" | bc)
        echo "- Swarm-$swarm: $swarm_completed/$swarm_tasks tasks (${swarm_rate}%)"
    done
}
```

### SYSTEM HEALTH MONITORING

**Real-Time Health Dashboard:**
```bash
# Generate system health summary
system-health-summary() {
    echo "=== MULTI-SWARM SYSTEM HEALTH ==="
    echo "Timestamp: $(date)"
    echo ""
    
    get-development-velocity
    echo ""
    get-quality-score
    echo ""
    get-coordination-efficiency
    echo ""
    compare-swarm-productivity
    echo ""
    
    # Overall system score
    local velocity=$(get-development-velocity | grep "Velocity score:" | awk '{print $3}')
    local quality=$(get-quality-score | grep "Overall quality:" | awk '{print $3}' | tr -d '%')
    local coordination=$(get-coordination-efficiency | grep "Coordination efficiency:" | awk '{print $3}' | tr -d '%')
    
    local overall=$(echo "scale=1; ($velocity * 10 + $quality + $coordination) / 3" | bc)
    echo "OVERALL SYSTEM HEALTH: ${overall}%"
    
    # Health recommendations
    if [ $(echo "$overall < 70" | bc) -eq 1 ]; then
        echo ""
        echo "RECOMMENDATIONS:"
        if [ $(echo "$quality < 80" | bc) -eq 1 ]; then
            echo "- Focus on fixing failing tests and build issues"
        fi
        if [ $(echo "$coordination < 70" | bc) -eq 1 ]; then
            echo "- Reduce file conflicts through better task coordination"
        fi
        if [ $(echo "$velocity < 5" | bc) -eq 1 ]; then
            echo "- Increase development velocity with focused work"
        fi
    fi
}
```

### TREND ANALYSIS

**Performance Trend Tracking:**
```bash
# Track metrics over time for trend analysis
record-daily-metrics() {
    local metrics_file="metrics/daily-$(date +%Y%m%d).json"
    local velocity=$(get-development-velocity | grep "Velocity score:" | awk '{print $3}')
    local quality=$(get-quality-score | grep "Overall quality:" | awk '{print $3}' | tr -d '%')
    local coordination=$(get-coordination-efficiency | grep "Coordination efficiency:" | awk '{print $3}' | tr -d '%')
    local overall=$(echo "scale=1; ($velocity * 10 + $quality + $coordination) / 3" | bc)
    
    mkdir -p metrics
    cat > "$metrics_file" << EOF
{
  "date": "$(date +%Y-%m-%d)",
  "velocity": $velocity,
  "quality": $quality,
  "coordination": $coordination,
  "overall_health": $overall,
  "active_agents": $(make vision | grep -E "\[(swarm-[1-4])" | cut -d'[' -f2 | cut -d']' -f1 | sort -u | wc -l),
  "total_tasks": $(node task.js list | wc -l),
  "completed_tasks": $(node task.js list --completed | wc -l)
}
EOF
    
    echo "Daily metrics recorded: $metrics_file"
}

# Generate weekly trend report
generate-trend-report() {
    echo "=== WEEKLY TREND REPORT ==="
    echo "Generated: $(date)"
    echo ""
    
    if [ -d metrics ]; then
        local latest_files=$(ls -t metrics/daily-*.json | head -7)
        
        echo "Last 7 days performance:"
        for file in $latest_files; do
            local date=$(jq -r '.date' "$file")
            local health=$(jq -r '.overall_health' "$file")
            echo "- $date: ${health}% health"
        done
        
        # Calculate trend
        local first_health=$(echo "$latest_files" | tail -1 | xargs jq -r '.overall_health' 2>/dev/null || echo "0")
        local last_health=$(echo "$latest_files" | head -1 | xargs jq -r '.overall_health' 2>/dev/null || echo "0")
        local trend=$(echo "scale=1; $last_health - $first_health" | bc)
        
        echo ""
        if [ $(echo "$trend > 0" | bc) -eq 1 ]; then
            echo "TREND: Improving (+${trend}%)"
        elif [ $(echo "$trend < 0" | bc) -eq 1 ]; then
            echo "TREND: Declining (${trend}%)"
        else
            echo "TREND: Stable"
        fi
    else
        echo "No historical data available. Run record-daily-metrics to start tracking."
    fi
}
```

### AUTOMATED MONITORING

**Continuous Metrics Collection:**
```bash
# Start automated metrics collection
start-metrics-monitoring() {
    echo "Starting automated metrics monitoring..."
    
    while true; do
        # Record metrics every hour
        record-daily-metrics
        
        # Check for health alerts
        local health=$(system-health-summary | grep "OVERALL SYSTEM HEALTH:" | awk '{print $4}' | tr -d '%')
        
        if [ $(echo "$health < 60" | bc) -eq 1 ]; then
            echo "ALERT: System health critical (${health}%)" | tee -a metrics/alerts.log
            system-health-summary >> metrics/alerts.log
            echo "---" >> metrics/alerts.log
        fi
        
        # Wait 1 hour
        sleep 3600
    done
}

# Quick metrics check
metrics-check() {
    echo "Quick System Check:"
    local quality=$(get-quality-score | grep "Overall quality:" | awk '{print $3}' | tr -d '%')
    local coordination=$(get-coordination-efficiency | grep "Coordination efficiency:" | awk '{print $3}' | tr -d '%')
    local conflicts=$(make vision | grep -E "\[(swarm-[1-4])" | wc -l)
    local active_tasks=$(node task.js list --active | wc -l)
    
    echo "- Quality: ${quality}%"
    echo "- Coordination: ${coordination}%"
    echo "- File conflicts: $conflicts"
    echo "- Active tasks: $active_tasks"
    
    if [ $(echo "$quality < 80" | bc) -eq 1 ] || [ $(echo "$coordination < 70" | bc) -eq 1 ]; then
        echo "⚠️  System needs attention"
        return 1
    else
        echo "✅ System healthy"
        return 0
    fi
}
```

### METRICS INTEGRATION

These metrics are designed to be:
1. **Actionable** - Each metric suggests specific improvements
2. **Automated** - Can be collected without human intervention  
3. **Trend-aware** - Track performance over time
4. **Alert-capable** - Automatically warn of system degradation
5. **Simple** - Easy to understand and act upon

Use `metrics-check` for quick status, `system-health-summary` for detailed analysis, and `start-metrics-monitoring` for continuous tracking.