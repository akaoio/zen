# GitHub Workflow Integration Component

## SEAMLESS WORKFLOW INTEGRATION

**Purpose**: Ensure GitHub issue tracking integrates seamlessly with existing swarm workflows without disrupting productivity or coordination mechanisms.

### Integration Points with Existing Systems

#### 1. Task.js Integration

**Bidirectional Sync between Tasks and Issues:**

```bash
# Enhanced task creation with GitHub issue linking
create_task_with_github_integration() {
    local agent_id="$1"
    local task_description="$2"
    local files=("${@:3}")
    
    # Create traditional task file
    local task_file=$(node task.js create "$agent_id" "$task_description" "${files[@]}" | grep "Created task:" | cut -d' ' -f3)
    
    # Check if task should create a GitHub issue
    if should_create_github_issue "$task_description"; then
        local issue_url=$(create_agent_issue "$task_description" \
            "Task created by $agent_id for implementation work" \
            "$agent_id" "task" "${files[@]}")
        
        # Link issue to task
        if [ -n "$issue_url" ]; then
            node task.js activity "$task_file" "Linked to GitHub issue: $issue_url"
            echo "GitHub Issue: $issue_url" >> "tasks/$(basename $task_file .yaml)_github.link"
        fi
    fi
    
    echo "$task_file"
}

# Determine if task needs GitHub issue
should_create_github_issue() {
    local task_description="$1"
    
    # Create issues for:
    if [[ "$task_description" =~ (implement|fix|bug|error|design|architecture) ]]; then
        return 0  # Create issue
    fi
    
    # Skip issues for routine coordination
    if [[ "$task_description" =~ (update status|daily check|routine) ]]; then
        return 1  # Skip issue
    fi
    
    return 0  # Default: create issue for visibility
}

# Enhanced task completion with GitHub issue resolution
complete_task_with_github_resolution() {
    local task_file="$1"
    local success_msg="$2"
    local agent_id="$3"
    
    # Complete traditional task
    node task.js complete "$task_file" --success "$success_msg"
    
    # Resolve linked GitHub issue
    local github_link="tasks/$(basename $task_file .yaml)_github.link"
    if [ -f "$github_link" ]; then
        local issue_url=$(grep "GitHub Issue:" "$github_link" | cut -d' ' -f3)
        local issue_number=$(echo "$issue_url" | grep -o '#[0-9]*' | cut -d'#' -f2)
        
        if [ -n "$issue_number" ]; then
            handle_agent_success "$success_msg" "$agent_id" "$issue_number"
            rm -f "$github_link"  # Clean up link file
        fi
    fi
}
```

#### 2. Make Vision Integration

**GitHub Status in Vision Output:**

```bash
# Enhanced vision command with GitHub integration
make_vision_with_github() {
    # Run standard vision
    make vision
    
    echo ""
    echo "🐙 GitHub Integration Status:"
    echo "=============================="
    
    # Show active GitHub issues by swarm
    for swarm in swarm-1 swarm-2 swarm-3 swarm-4; do
        local issue_count=$(gh issue list --label "$swarm" --state open --json number | jq length)
        if [ $issue_count -gt 0 ]; then
            echo "📋 $swarm: $issue_count active issues"
            
            # Show top 3 issues for this swarm
            gh issue list --label "$swarm" --state open --limit 3 \
                --json number,title,assignees | jq -r '.[] | 
                "  #\(.number): \(.title) (\(.assignees | length) assigned)"'
        fi
    done
    
    # Show critical system-wide issues
    local critical_count=$(gh issue list --label "critical" --state open --json number | jq length)
    if [ $critical_count -gt 0 ]; then
        echo ""
        echo "🚨 Critical Issues Needing Immediate Attention:"
        gh issue list --label "critical" --state open --limit 5 \
            --json number,title | jq -r '.[] | "  #\(.number): \(.title)"'
    fi
    
    # Show blocking issues
    local blocking_count=$(gh issue list --label "blocking" --state open --json number | jq length)
    if [ $blocking_count -gt 0 ]; then
        echo ""
        echo "🚫 Blocking Issues:"
        gh issue list --label "blocking" --state open --limit 3 \
            --json number,title | jq -r '.[] | "  #\(.number): \(.title)"'
    fi
}
```

#### 3. Workspace Isolation Integration

**GitHub Integration Respects Workspace Boundaries:**

```bash
# Workspace-aware GitHub integration
workspace_github_integration() {
    local workspace_dir="$1"
    local agent_id="$2"
    
    # Create workspace-specific GitHub tracking
    mkdir -p "$workspace_dir/.github"
    
    # Track issues being worked on in this workspace
    create_workspace_issue_tracker() {
        local tracker_file="$workspace_dir/.github/active_issues.json"
        
        # Get issues currently assigned to this agent
        gh issue list --assignee @me --state open --json number,title,labels > "$tracker_file"
        
        echo "📋 Workspace tracking $(jq length < "$tracker_file") active issues"
    }
    
    # Update workspace issue progress
    update_workspace_issue_progress() {
        local issue_number="$1"
        local progress_update="$2"
        
        comment_on_issue "$issue_number" \
            "**Workspace Progress Update** from $agent_id:

$progress_update

**Workspace**: $workspace_dir
**Timestamp**: $(date -Iseconds)

---
*Workspace progress update*" "$agent_id"
    }
    
    create_workspace_issue_tracker
}
```

#### 4. Swarm Coordination Integration

**GitHub Issues Enhance Cross-Swarm Coordination:**

```bash
# Cross-swarm coordination via GitHub issues
coordinate_across_swarms_via_github() {
    local initiating_swarm="$1"
    local coordination_topic="$2"
    local affected_swarms=("${@:3}")
    
    echo "🌐 Initiating cross-swarm coordination via GitHub"
    
    # Create cross-swarm coordination issue
    local coord_title="Cross-Swarm Coordination: $coordination_topic"
    local coord_body="## Cross-Swarm Coordination Request

**Initiating Swarm**: $initiating_swarm
**Topic**: $coordination_topic
**Affected Swarms**: ${affected_swarms[*]}
**Initiated**: $(date -Iseconds)

## Coordination Objectives
[To be filled by initiating swarm]

## Action Items by Swarm

$(for swarm in "${affected_swarms[@]}"; do
    echo "### $swarm Tasks:"
    echo "- [ ] [To be assigned by $swarm queen]"
    echo ""
done)

## Status Updates
[Swarms will update progress here]

---
*Cross-swarm coordination initiated by $initiating_swarm*"
    
    # Create with all affected swarm labels
    local all_labels="$initiating_swarm,coordination"
    for swarm in "${affected_swarms[@]}"; do
        all_labels="$all_labels,$swarm"
    done
    
    local coord_issue_url=$(gh issue create \
        --title "$coord_title" \
        --body "$coord_body" \
        --label "$all_labels" \
        --assignee @me 2>/dev/null || echo "")
    
    echo "✅ Cross-swarm coordination issue created: $coord_issue_url"
    
    # Notify other swarm queens
    for swarm in "${affected_swarms[@]}"; do
        if [ "$swarm" != "$initiating_swarm" ]; then
            echo "📢 Notifying $swarm of coordination request"
            # This would trigger other swarm queens to check the issue
        fi
    done
}

# Enhanced file conflict resolution via GitHub
resolve_file_conflict_via_github() {
    local conflicted_file="$1"
    local agent1="$2"
    local agent2="$3"
    
    echo "⚖️ Resolving file conflict via GitHub coordination"
    
    # Create conflict resolution issue
    local conflict_title="File Conflict Resolution: $(basename "$conflicted_file")"
    local conflict_body="## File Conflict Detected

**File**: $conflicted_file
**Conflicting Agents**: $agent1, $agent2
**Detection Time**: $(date -Iseconds)

## Conflict Analysis
- Both agents are working on the same file
- Requires coordination to avoid work duplication
- May indicate need for component interface clarification

## Resolution Process

1. **Immediate Actions**:
   - [ ] Pause conflicting implementations
   - [ ] Assess work overlap
   - [ ] Determine coordination approach

2. **Queen Coordination**:
   - [ ] Queens mediate work assignment
   - [ ] Sequence implementations if needed
   - [ ] Update task assignments

3. **Architect Review** (if needed):
   - [ ] Review component interfaces
   - [ ] Consider design improvements
   - [ ] Update MANIFEST.json if necessary

## Resolution Status
[To be updated by coordinating queens]

---
*Auto-generated conflict resolution*"
    
    create_agent_issue "$conflict_title" "$conflict_body" "system" "conflict" "$conflicted_file"
    
    # Pause conflicting tasks via traditional task system
    node task.js list --active | grep -E "($agent1|$agent2)" | while read -r task_line; do
        local task_id=$(echo "$task_line" | awk '{print $2}')
        if [[ "$task_line" == *"$(basename "$conflicted_file")"* ]]; then
            node task.js activity "$task_id" "Paused due to file conflict - see GitHub issue"
        fi
    done
}
```

#### 5. Error Handling Integration

**GitHub Issues Complement Existing Error Handling:**

```bash
# Enhanced error handling with GitHub integration
handle_error_with_github_and_task_integration() {
    local error_msg="$1"
    local agent_id="$2"
    local context="$3"
    local files=("${@:4}")
    
    # Traditional error logging
    echo "$(date -Iseconds)|$agent_id|ERROR|$error_msg|${files[*]}" >> tmp/agent_errors.log
    
    # Update task file if available
    if [ -n "$TASK_FILE" ]; then
        node task.js activity "$TASK_FILE" "Error encountered: $error_msg" --fail "GitHub issue created for resolution"
    fi
    
    # Create GitHub issue (from github-integration.md)
    handle_agent_error "$error_msg" "$agent_id" "$context" "${files[@]}"
    
    # Notify relevant queen for coordination
    local agent_swarm=$(echo "$agent_id" | cut -d'-' -f1-2)  # Extract swarm-N
    if [ -f "tasks/coordination.lock" ]; then
        echo "$agent_id error: $error_msg" >> "tasks/coordination.lock"
    fi
}

# Success handling with complete integration
handle_success_with_github_and_task_integration() {
    local success_msg="$1"
    local agent_id="$2"
    local issue_number="$3"
    
    # Traditional success logging
    echo "$(date -Iseconds)|$agent_id|SUCCESS|$success_msg" >> tmp/agent_successes.log
    
    # Update task file if available
    if [ -n "$TASK_FILE" ]; then
        node task.js activity "$TASK_FILE" "Success: $success_msg" --success "Implementation complete"
    fi
    
    # Close GitHub issue (from github-integration.md)
    if [ -n "$issue_number" ]; then
        handle_agent_success "$success_msg" "$agent_id" "$issue_number"
    fi
    
    # Update fitness scores and coordination metrics
    # (This would integrate with existing fitness tracking)
}
```

### Non-Disruptive Integration Principles

#### 1. Optional GitHub Features

```bash
# GitHub integration is additive, not required for basic functionality
github_integration_enabled() {
    # Check if GitHub integration should be active
    if [ -f ".github_integration_disabled" ]; then
        return 1  # Disabled
    fi
    
    if ! command -v gh >/dev/null 2>&1; then
        return 1  # GitHub CLI not available
    fi
    
    if ! gh auth status >/dev/null 2>&1; then
        return 1  # Not authenticated
    fi
    
    return 0  # Enabled
}

# Wrapper functions that gracefully degrade
safe_github_operation() {
    local operation="$1"
    shift
    
    if github_integration_enabled; then
        # Execute GitHub operation
        "$operation" "$@"
    else
        # Fall back to traditional workflow
        echo "ℹ️ GitHub integration not available - using traditional workflow"
        return 0
    fi
}
```

#### 2. Performance Optimization

```bash
# Batch GitHub operations to minimize API calls
batch_github_operations() {
    local operations=("$@")
    
    # Queue operations instead of executing immediately
    local batch_file="tmp/github_batch_$(date +%s).json"
    
    for op in "${operations[@]}"; do
        echo "$op" >> "$batch_file"
    done
    
    # Execute batch every 5 minutes or when 10 operations queued
    if [ $(wc -l < "$batch_file") -ge 10 ] || [ ! -f "tmp/last_github_batch" ] || 
       [ $(($(date +%s) - $(cat tmp/last_github_batch))) -ge 300 ]; then
        
        echo "🔄 Executing batched GitHub operations"
        execute_github_batch "$batch_file"
        date +%s > tmp/last_github_batch
        rm -f "$batch_file"
    fi
}

# Background GitHub sync to avoid blocking main workflow
background_github_sync() {
    # Run GitHub operations in background to not block agent work
    (
        # Set up background logging
        exec > tmp/github_background.log 2>&1
        
        # Sync issues, comments, and status updates
        sync_pending_github_operations
        
        # Clean up old GitHub tracking files
        find tmp/ -name "*github*" -mtime +7 -delete
        
    ) &
    
    # Don't wait for background operations
    echo "🔄 GitHub sync running in background (PID: $!)"
}
```

#### 3. Fallback Mechanisms

```bash
# Graceful degradation when GitHub is unavailable
github_fallback_handler() {
    local github_operation="$1"
    local fallback_action="$2"
    
    if ! safe_github_operation "$github_operation"; then
        echo "⚠️ GitHub operation failed, executing fallback"
        eval "$fallback_action"
    fi
}

# Example fallback for issue creation
create_issue_with_fallback() {
    local issue_title="$1"
    local issue_body="$2"
    local agent_id="$3"
    
    # Try GitHub first
    if github_integration_enabled; then
        if create_agent_issue "$issue_title" "$issue_body" "$agent_id"; then
            return 0
        fi
    fi
    
    # Fallback to local issue tracking
    local local_issue_file="tmp/issues/$(date +%Y%m%d-%H%M%S)-$agent_id.md"
    mkdir -p tmp/issues
    
    cat > "$local_issue_file" << EOF
# $issue_title

**Agent**: $agent_id
**Created**: $(date -Iseconds)
**Status**: Open

## Description
$issue_body

## Local Tracking
This issue was created locally due to GitHub unavailability.
Will sync to GitHub when connection is restored.
EOF
    
    echo "📝 Created local issue: $local_issue_file"
    
    # Queue for later GitHub sync
    echo "$local_issue_file" >> tmp/pending_github_sync.list
}
```

### Integration Testing & Validation

```bash
# Test GitHub integration without disrupting existing workflows
test_github_integration() {
    echo "🧪 Testing GitHub integration with existing workflows"
    
    # Test 1: Task creation with GitHub linking
    echo "Test 1: Task + GitHub integration"
    local test_task=$(create_task_with_github_integration "test-agent" "Test implementation task" "test.c")
    echo "✅ Task created: $test_task"
    
    # Test 2: Error handling integration
    echo "Test 2: Error handling integration"
    handle_error_with_github_and_task_integration "Test error" "test-agent" "Integration testing" "test.c"
    echo "✅ Error handling integrated"
    
    # Test 3: Success handling integration
    echo "Test 3: Success handling integration"
    handle_success_with_github_and_task_integration "Test success" "test-agent" ""
    echo "✅ Success handling integrated"
    
    # Test 4: Coordination integration
    echo "Test 4: Cross-swarm coordination"
    coordinate_across_swarms_via_github "swarm-1" "Test coordination" "swarm-2" "swarm-3"
    echo "✅ Cross-swarm coordination integrated"
    
    # Clean up test artifacts
    cleanup_test_github_integration
    
    echo "🎉 GitHub integration tests completed successfully"
}

cleanup_test_github_integration() {
    # Remove test tasks
    find tasks/ -name "*test-agent*" -delete 2>/dev/null
    
    # Close test GitHub issues
    gh issue list --search "Test" --state open --json number | \
        jq -r '.[].number' | while read -r issue_num; do
        gh issue close "$issue_num" >/dev/null 2>&1
    done
    
    # Clean up test files
    rm -f tmp/test_* 2>/dev/null
}
```

This integration ensures that GitHub issue tracking becomes a seamless part of the swarm workflow while maintaining all existing functionality and providing graceful fallbacks when GitHub is unavailable.