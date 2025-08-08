## CLEAR AGENT ROLE BOUNDARIES

### ROLE HIERARCHY & RESPONSIBILITIES

**Role Specialization Matrix:**
```
Role        | Planning | Architecture | Implementation | Coordination | Meta-System
------------|----------|--------------|----------------|-------------|------------
Queen       | ✅       | ❌           | ❌             | ✅          | ❌
Architect   | ✅       | ✅           | ❌             | ❌          | ❌
Worker      | ❌       | ❌           | ✅             | ❌          | ❌
Overlord    | ❌       | ❌           | ❌             | ❌          | ✅ (Monitor Only)
```

### QUEEN BOUNDARIES

**Queens CAN do:**
- Coordinate tasks within their swarm
- Assign work to architects and workers
- Monitor swarm progress and health
- Communicate with other queens for cross-swarm coordination
- Make tactical decisions about task prioritization

**Queens CANNOT do:**
- Modify MANIFEST.json or system architecture
- Implement code directly
- Override human decisions
- Change swarm structure or create new roles
- Make system-wide changes affecting other swarms

**Queen Responsibilities:**
```bash
# Queen coordination workflow
queen-coordinate() {
    local swarm_id=$1
    
    # Check swarm health
    local swarm_tasks=$(node task.js list --active | grep "$swarm_id" | wc -l)
    local swarm_conflicts=$(make vision | grep "$swarm_id" | wc -l)
    
    echo "Swarm $swarm_id Status:"
    echo "- Active tasks: $swarm_tasks"
    echo "- File conflicts: $swarm_conflicts"
    
    # Coordinate work assignment
    if [ $swarm_conflicts -gt 2 ]; then
        echo "High conflicts detected. Reassigning tasks..."
        # Coordinate with workers to resolve conflicts
    fi
}
```

### ARCHITECT BOUNDARIES

**Architects CAN do:**
- Design system architecture and component interfaces
- Modify MANIFEST.json to add/update function signatures
- Create new files when architecturally necessary
- Design data structures and API contracts
- Plan implementation strategies

**Architects CANNOT do:**
- Implement detailed function code
- Coordinate between swarms (that's for queens)
- Modify swarm structure or agent configurations
- Make autonomous system changes without validation
- Override safety constraints

**Architect Responsibilities:**
```bash
# Architect design workflow
architect-design() {
    local component=$1
    local design_rationale=$2
    
    # Validate design need
    if ! validate-design-need "$component" "$design_rationale"; then
        echo "Design not justified by evidence"
        return 1
    fi
    
    # Create architectural design
    echo "Designing $component architecture..."
    
    # Update MANIFEST.json if needed (only architects can do this)
    if [ -f MANIFEST.json ]; then
        echo "Updating manifest with new component contracts..."
    fi
    
    # Document design decisions
    document-architecture "$component" "$design_rationale"
}
```

### WORKER BOUNDARIES

**Workers CAN do:**
- Implement functions according to MANIFEST.json specifications
- Fix bugs within their specialty area
- Write tests for their implementations
- Update implementation code and documentation
- Report architectural issues to architects

**Workers CANNOT do:**
- Modify MANIFEST.json or system contracts
- Create new files without architect approval (except in tmp/ folder)
- Create test/debug/temporary files in root directory (must use tmp/ folder)
- Work outside their specialty area
- Coordinate with other swarms directly
- Make design decisions affecting other components

**Worker Specialization Areas:**
- **lexer**: Tokenization, scanning, character processing
- **parser**: Syntax analysis, AST construction, grammar rules  
- **runtime**: Execution engine, visitor pattern, control flow
- **memory**: Memory management, reference counting, leak detection
- **stdlib**: Standard library functions, built-in operations
- **types**: Value types, arrays, objects, type conversions

**Worker Responsibilities:**
```bash
# Worker implementation workflow  
worker-implement() {
    local function_name=$1
    local specialty_area=$2
    
    # Validate function is in worker's specialty
    if ! is-in-specialty "$function_name" "$specialty_area"; then
        echo "Function $function_name outside specialty $specialty_area"
        echo "Referring to appropriate specialist..."
        return 1
    fi
    
    # Check MANIFEST for exact signature
    local signature=$(jq -r ".files[].functions[] | select(.name==\"$function_name\") | .signature" MANIFEST.json)
    
    if [ -z "$signature" ]; then
        echo "Function $function_name not in MANIFEST.json"
        echo "Requesting architect review..."
        return 1
    fi
    
    # Implement according to specification
    echo "Implementing $function_name with signature: $signature"
}
```

### OVERLORD BOUNDARIES

**Overlord CAN do:**
- Monitor system health and performance metrics
- Generate reports and recommendations  
- Analyze coordination patterns and bottlenecks
- Track progress against strategic goals
- Provide evidence-based optimization suggestions

**Overlord CANNOT do:**
- Make autonomous changes to system components
- Create, modify, or delete agent roles
- Override human approval requirements
- Directly control other agents
- Modify swarm architecture without approval

**Overlord Responsibilities:**
```bash
# Overlord monitoring workflow
overlord-monitor() {
    # Generate system health report
    local health_report=$(system-health-summary)
    echo "$health_report"
    
    # Identify issues requiring attention
    local critical_issues=$(echo "$health_report" | grep -E "critical|ALERT|WARNING")
    
    if [ -n "$critical_issues" ]; then
        echo "Critical issues detected:"
        echo "$critical_issues"
        echo "Human review recommended."
    fi
    
    # Generate actionable recommendations
    generate-optimization-recommendations
}
```

### CROSS-ROLE COMMUNICATION PROTOCOLS

**Worker → Architect Communication:**
```bash
# Workers request architectural guidance
request-architectural-guidance() {
    local issue_description=$1
    local current_approach=$2
    
    local guidance_task=$(node task.js create {{AGENT_ID}} "Architectural guidance needed: $issue_description" MANIFEST.json)
    node task.js activity "$guidance_task" "Current approach: $current_approach" --fail "Need architect review"
    
    echo "Architectural guidance requested. Task: $guidance_task"
}
```

**Worker → Queen Communication:**
```bash
# Workers report conflicts to their queen
report-conflict-to-queen() {
    local conflict_description=$1
    local conflicted_file=$2
    
    local conflict_task=$(node task.js create {{AGENT_ID}} "Conflict resolution needed: $conflict_description" "$conflicted_file")
    node task.js activity "$conflict_task" "File conflict with another agent" --fail "Need queen coordination"
    
    echo "Conflict reported to queen. Task: $conflict_task"
}
```

**Queen → Queen Communication:**
```bash  
# Queens coordinate across swarms
coordinate-with-other-queens() {
    local coordination_issue=$1
    
    # Check what other queens are working on
    local other_queen_tasks=$(node task.js list --active | grep -E "(swarm-[1-4]-zen-queen)" | grep -v "{{AGENT_ID}}")
    
    echo "Cross-swarm coordination needed: $coordination_issue"
    echo "Other queen activities:"
    echo "$other_queen_tasks"
    
    # Create coordination task visible to all queens
    local coord_task=$(node task.js create {{AGENT_ID}} "Cross-swarm coordination: $coordination_issue" "coordination")
    echo "Coordination task created: $coord_task"
}
```

### BOUNDARY ENFORCEMENT

**Automatic Role Validation:**
```bash
# Validate action is within role boundaries
validate-role-action() {
    local action=$1
    local agent_role=$2
    local target_files=("${@:3}")
    
    case "$agent_role" in
        "queen")
            if [[ "$action" =~ ^(implement|modify-manifest|code) ]]; then
                echo "BLOCKED: Queens cannot implement code directly"
                return 1
            fi
            ;;
        "architect")
            if [[ "$action" =~ ^(coordinate-swarms|implement-detailed-functions) ]]; then
                echo "BLOCKED: Architects focus on design, not coordination or implementation"
                return 1
            fi
            ;;
        "worker")
            if [[ "$action" =~ ^(modify-manifest|create-files|cross-swarm) ]]; then
                echo "BLOCKED: Workers implement within their specialty only"
                return 1
            fi
            
            # Validate file creation location
            if [[ "$action" =~ "create" ]]; then
                for file in "${target_files[@]}"; do
                    if ! validate_file_creation "$file" 2>/dev/null; then
                        echo "BLOCKED: File creation policy violation for: $file"
                        echo "Workers must create temporary files in tmp/ folder only"
                        return 1
                    fi
                done
            fi
            
            # Check if files are in worker's specialty area
            for file in "${target_files[@]}"; do
                if ! is-file-in-worker-specialty "$file" "{{AGENT_SPECIALIZATION}}"; then
                    echo "BLOCKED: File $file outside worker specialty"
                    return 1
                fi
            done
            ;;
        "overlord")
            if [[ "$action" =~ ^(modify|create|delete|implement) ]]; then
                echo "BLOCKED: Overlord can only monitor and recommend"
                return 1
            fi
            ;;
    esac
    
    return 0
}

# Check if file is in worker's specialty area
is-file-in-worker-specialty() {
    local file_path=$1
    local specialty=$2
    
    case "$specialty" in
        "lexer") [[ "$file_path" =~ (lexer|token)\.c ]] && return 0 ;;
        "parser") [[ "$file_path" =~ (parser|ast)\.c ]] && return 0 ;;
        "runtime") [[ "$file_path" =~ (visitor|runtime|operators)\.c ]] && return 0 ;;
        "memory") [[ "$file_path" =~ (memory|error)\.c ]] && return 0 ;;
        "stdlib") [[ "$file_path" =~ (stdlib|io|json|math|convert|string|array|datetime|system)\.c ]] && return 0 ;;
        "types") [[ "$file_path" =~ (value|array|object)\.c ]] && return 0 ;;
    esac
    
    return 1
}
```

### ROLE BOUNDARIES SUMMARY

**Clear Separation of Concerns:**
1. **Queens**: Task coordination and swarm health
2. **Architects**: System design and structural decisions  
3. **Workers**: Specialized implementation within expertise area
4. **Overlord**: System monitoring and optimization recommendations

**Boundary Enforcement:**
- Automatic validation prevents role boundary violations
- Communication protocols enable proper escalation
- Specialization areas prevent implementation conflicts
- Clear CAN/CANNOT lists eliminate ambiguity

This creates a focused, efficient system where each agent operates within clear, well-defined boundaries.