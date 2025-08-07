#!/bin/bash

# System validation functions for improved swarm system

get-system-health() {
    local test_pass_rate=$(make test 2>/dev/null | grep -o '[0-9]*% passed' | grep -o '[0-9]*' || echo "0")
    local build_success=$(make clean >/dev/null 2>&1 && make >/dev/null 2>&1 && echo "100" || echo "0")
    local active_tasks=$(node task.js list --active | wc -l)
    
    # Simple health calculation
    if [ $active_tasks -eq 0 ]; then
        local task_health=100
    elif [ $active_tasks -lt 10 ]; then
        local task_health=80
    elif [ $active_tasks -lt 20 ]; then
        local task_health=60
    else
        local task_health=40
    fi
    
    local health=$(echo "scale=0; ($test_pass_rate + $build_success + $task_health) / 3" | bc -l 2>/dev/null || echo "50")
    echo "$health"
}

get-coordination-efficiency() {
    local conflicts=$(make vision 2>/dev/null | grep -E "\[(swarm-[1-4])" | wc -l)
    local active_agents=$(make vision 2>/dev/null | grep -E "\[(swarm-[1-4])" | cut -d'[' -f2 | cut -d']' -f1 | sort -u | wc -l)
    
    if [ $active_agents -eq 0 ]; then
        echo "100"
    else
        local conflict_ratio=$(echo "scale=2; ($conflicts * 100) / $active_agents" | bc -l 2>/dev/null || echo "0")
        local efficiency=$(echo "scale=0; 100 - $conflict_ratio" | bc -l 2>/dev/null || echo "80")
        echo "$efficiency"
    fi
}

system-health-check() {
    echo "=== SYSTEM HEALTH CHECK ==="
    echo "Timestamp: $(date)"
    
    local health=$(get-system-health)
    local coordination=$(get-coordination-efficiency)
    local conflicts=$(make vision 2>/dev/null | grep -E "\[(swarm-[1-4])" | wc -l)
    local active_tasks=$(node task.js list --active 2>/dev/null | wc -l)
    
    echo "- System Health: ${health}%"
    echo "- Coordination Efficiency: ${coordination}%"
    echo "- File Conflicts: $conflicts"
    echo "- Active Tasks: $active_tasks"
    
    if [ $health -lt 70 ]; then
        echo "⚠️  System health below 70% - attention needed"
    elif [ $health -lt 50 ]; then
        echo "🚨 Critical system health - immediate action required"
    else
        echo "✅ System health acceptable"
    fi
    
    if [ $conflicts -gt 10 ]; then
        echo "⚠️  High coordination conflicts detected"
    fi
}

quick-validation() {
    local health=$(get-system-health)
    local coordination=$(get-coordination-efficiency)
    
    echo "Quick Validation: Health ${health}%, Coordination ${coordination}%"
    
    if [ $health -gt 70 ] && [ $coordination -gt 70 ]; then
        echo "✅ System operating within acceptable parameters"
        return 0
    else
        echo "⚠️  System requires attention"
        return 1
    fi
}

# Make functions available when script is sourced
if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    echo "Validation functions loaded successfully"
fi
