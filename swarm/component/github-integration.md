# GitHub Integration Component

## SYSTEMATIC ISSUE TRACKING INTEGRATION

**Purpose**: Integrate GitHub issue tracking as a core workflow component for all 32 swarm agents. Every error, bug, or problem encountered during development automatically creates, updates, and resolves GitHub issues.

### Core Integration Principles

1. **Automatic Issue Creation**: All agent errors trigger GitHub issues
2. **Work Visibility**: Issues provide public visibility into swarm progress  
3. **Priority Coordination**: Issues enable cross-swarm priority alignment
4. **Resolution Tracking**: Issues close automatically when problems are resolved
5. **Seamless Workflow**: Integration doesn't disrupt existing task.js coordination

## GitHub Issue Workflow Functions

### Issue Creation & Management

```bash
# Create issue with agent context
create_agent_issue() {
    local issue_title="$1"
    local issue_body="$2" 
    local agent_id="$3"
    local issue_type="${4:-bug}"  # bug, enhancement, task
    local files_involved=("${@:5}")
    
    # Prepare comprehensive issue body
    local full_body="## Agent Context
- **Agent**: $agent_id
- **Timestamp**: $(date -Iseconds)
- **Files**: ${files_involved[*]}
- **Issue Type**: $issue_type

## Description
$issue_body

## System Context
\`\`\`bash
# System health at time of issue
Health Score: $(get-system-health 2>/dev/null || echo "unknown")%
Coordination Efficiency: $(get-coordination-efficiency 2>/dev/null || echo "unknown")%
Active Tasks: $(node task.js list --active | wc -l)
\`\`\`

## Reproduction Steps
1. Agent: $agent_id
2. Working on: ${files_involved[*]}
3. Encountered: $issue_title

---
*Auto-created by $agent_id via swarm GitHub integration*"
    
    # Create issue with labels
    local labels="swarm,$issue_type,${agent_id%%zen-*}"  # Include swarm ID as label
    
    local issue_url=$(gh issue create \
        --title "$issue_title" \
        --body "$full_body" \
        --label "$labels" \
        --assignee @me 2>/dev/null || echo "")
    
    if [ -n "$issue_url" ]; then
        echo "✅ GitHub issue created: $issue_url"
        
        # Store issue URL for agent tracking
        echo "$(date -Iseconds)|$agent_id|$issue_url|$issue_title" >> tmp/agent_issues.log
        
        # Link issue to task if task file exists
        if [ -n "$TASK_FILE" ]; then
            node task.js activity "$TASK_FILE" "GitHub issue created: $issue_url"
        fi
        
        return 0
    else
        echo "❌ Failed to create GitHub issue"
        return 1
    fi
}

# Comment on existing issue
comment_on_issue() {
    local issue_number="$1"
    local comment_body="$2"
    local agent_id="$3"
    
    local full_comment="**$agent_id Update** ($(date -Iseconds)):

$comment_body

---
*Auto-updated by $agent_id*"
    
    gh issue comment "$issue_number" --body "$full_comment" >/dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo "✅ Commented on issue #$issue_number"
    else
        echo "❌ Failed to comment on issue #$issue_number"
    fi
}

# Close issue when resolved
close_agent_issue() {
    local issue_number="$1"
    local resolution_comment="$2"
    local agent_id="$3"
    
    local close_comment="**Resolution by $agent_id** ($(date -Iseconds)):

$resolution_comment

Issue resolved successfully. ✅

---
*Auto-closed by $agent_id*"
    
    # Add final comment
    gh issue comment "$issue_number" --body "$close_comment" >/dev/null 2>&1
    
    # Close the issue
    gh issue close "$issue_number" >/dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo "✅ Closed issue #$issue_number"
        
        # Remove from agent tracking
        sed -i "/|$issue_number|/d" tmp/agent_issues.log 2>/dev/null || true
        
        return 0
    else
        echo "❌ Failed to close issue #$issue_number"
        return 1
    fi
}
```

### Issue Discovery & Work Assignment

```bash
# Find open issues to work on
find_issues_to_work_on() {
    local agent_id="$1"
    local specialty="${2:-all}"
    
    echo "🔍 $agent_id checking for GitHub issues to resolve..."
    
    # Get issues with appropriate labels
    local issue_query="is:open is:issue"
    
    # Add specialty-specific filters
    case "$specialty" in
        "lexer") issue_query="$issue_query label:lexer" ;;
        "parser") issue_query="$issue_query label:parser" ;; 
        "runtime") issue_query="$issue_query label:runtime" ;;
        "memory") issue_query="$issue_query label:memory" ;;
        "stdlib") issue_query="$issue_query label:stdlib" ;;
        "types") issue_query="$issue_query label:types" ;;
        "architecture") issue_query="$issue_query label:enhancement" ;;
        "coordination") issue_query="$issue_query label:coordination" ;;
    esac
    
    # Get up to 5 relevant issues
    gh issue list --search "$issue_query" --limit 5 --json number,title,labels 2>/dev/null | \
        jq -r '.[] | "\(.number)|\(.title)"' | while IFS='|' read -r issue_num issue_title; do
        echo "📋 Available: Issue #$issue_num - $issue_title"
    done
}

# Claim an issue for work
claim_issue_for_work() {
    local issue_number="$1"
    local agent_id="$2"
    
    local claim_comment="**$agent_id claiming this issue** ($(date -Iseconds)):

🔧 Starting work on this issue.

**Work Plan:**
- Analyzing the problem
- Implementing solution
- Testing and validation
- Updating issue with progress

---
*Auto-assigned by $agent_id*"
    
    gh issue comment "$issue_number" --body "$claim_comment" >/dev/null 2>&1
    
    # Add agent as assignee if possible
    gh issue edit "$issue_number" --add-assignee @me >/dev/null 2>&1 || true
    
    echo "✅ Claimed issue #$issue_number for $agent_id"
    
    # Track in agent issues
    echo "$(date -Iseconds)|$agent_id|#$issue_number|WORKING" >> tmp/agent_issues.log
}
```

### Error Handling Integration

```bash
# Handle errors with automatic issue creation
handle_agent_error() {
    local error_description="$1"
    local agent_id="$2"
    local error_context="$3"
    local files_involved=("${@:4}")
    
    echo "❌ $agent_id encountered error: $error_description"
    
    # Check if similar issue already exists
    local existing_issue=$(gh issue list --search "is:open \"$error_description\"" --json number --jq '.[0].number' 2>/dev/null || echo "")
    
    if [ -n "$existing_issue" ] && [ "$existing_issue" != "null" ]; then
        echo "📋 Adding to existing issue #$existing_issue"
        comment_on_issue "$existing_issue" "Encountered same error in $error_context. Files: ${files_involved[*]}" "$agent_id"
    else
        echo "🆕 Creating new issue for error"
        create_agent_issue "$error_description" "$error_context" "$agent_id" "bug" "${files_involved[@]}"
    fi
    
    # Always log error locally too
    echo "$(date -Iseconds)|$agent_id|ERROR|$error_description|${files_involved[*]}" >> tmp/agent_errors.log
}

# Handle successful problem resolution
handle_agent_success() {
    local success_description="$1"
    local agent_id="$2"
    local issue_number="$3"
    
    if [ -n "$issue_number" ]; then
        close_agent_issue "$issue_number" "$success_description" "$agent_id"
    fi
    
    # Log success
    echo "$(date -Iseconds)|$agent_id|SUCCESS|$success_description" >> tmp/agent_successes.log
}
```

### Issue Priority & Coordination

```bash
# Prioritize issues based on swarm needs
prioritize_issues_for_agent() {
    local agent_role="$1"
    local agent_specialty="$2"
    
    echo "📊 Prioritizing issues for $agent_role ($agent_specialty)"
    
    # Get system health to determine priority focus
    local health=$(get-system-health 2>/dev/null || echo "50")
    
    if [ $health -lt 60 ]; then
        echo "🚨 Low system health - prioritizing bug fixes"
        gh issue list --label bug --state open --limit 10 --json number,title | \
            jq -r '.[] | "#\(.number): \(.title)"'
    elif [ "$agent_role" = "queen" ]; then
        echo "👑 Queen prioritizing coordination issues"
        gh issue list --label coordination --state open --limit 5 --json number,title | \
            jq -r '.[] | "#\(.number): \(.title)"'
    elif [ "$agent_role" = "architect" ]; then
        echo "🏗️ Architect prioritizing design issues"
        gh issue list --label enhancement --state open --limit 5 --json number,title | \
            jq -r '.[] | "#\(.number): \(.title)"'
    elif [ "$agent_role" = "worker" ]; then
        echo "⚙️ Worker prioritizing implementation issues"
        gh issue list --label "$agent_specialty" --state open --limit 3 --json number,title | \
            jq -r '.[] | "#\(.number): \(.title)"'
    fi
}

# Cross-swarm issue coordination
coordinate_cross_swarm_issues() {
    local current_swarm="$1"
    
    echo "🌐 Coordinating cross-swarm issues"
    
    # Find issues affecting multiple swarms
    gh issue list --label "swarm-1,swarm-2" --state open --json number,title 2>/dev/null | \
        jq -r '.[] | "Multi-swarm issue #\(.number): \(.title)"' | while read -r issue_info; do
        echo "⚠️ $issue_info"
    done
    
    # Find blocking issues that affect coordination
    gh issue list --label "blocking" --state open --json number,title,labels 2>/dev/null | \
        jq -r '.[] | "#\(.number): \(.title)"' | while read -r blocking_issue; do
        echo "🚫 Blocking issue: $blocking_issue"
    done
}
```

### Daily Issue Management

```bash
# Daily issue review for agents
daily_issue_review() {
    local agent_id="$1"
    
    echo "📅 Daily issue review for $agent_id"
    
    # Check assigned issues
    echo "## My Assigned Issues:"
    gh issue list --assignee @me --state open --json number,title | \
        jq -r '.[] | "- #\(.number): \(.title)"'
    
    # Check recently closed issues
    echo -e "\n## Recently Resolved:"
    gh issue list --assignee @me --state closed --limit 5 --json number,title,closedAt | \
        jq -r '.[] | "- #\(.number): \(.title) (closed: \(.closedAt | strptime("%Y-%m-%dT%H:%M:%SZ") | strftime("%Y-%m-%d")))"'
    
    # Show issue creation rate
    echo -e "\n## Issue Activity:"
    local created_today=$(gh issue list --author @me --search "created:$(date +%Y-%m-%d)" --json number | jq length)
    local closed_today=$(gh issue list --assignee @me --search "closed:$(date +%Y-%m-%d)" --json number | jq length)
    echo "- Created today: $created_today"
    echo "- Resolved today: $closed_today"
}

# Weekly issue metrics
weekly_issue_metrics() {
    echo "📊 Weekly Issue Metrics"
    
    local week_ago=$(date -d '7 days ago' +%Y-%m-%d)
    
    echo "## Issues Created This Week:"
    gh issue list --search "created:>=$week_ago" --json number,author | \
        jq -r 'group_by(.author.login) | .[] | "\(.[0].author.login): \(length) issues"'
    
    echo -e "\n## Issues Resolved This Week:" 
    gh issue list --search "closed:>=$week_ago" --state closed --json number,closedAt | \
        jq -r "length" | xargs echo "Total resolved:"
    
    echo -e "\n## Top Issue Labels:"
    gh issue list --state all --json labels | \
        jq -r '.[].labels[].name' | sort | uniq -c | sort -nr | head -5
}
```

## Agent Integration Hooks

### For All Agents

```bash
# Initialize GitHub integration for agent
init_github_integration() {
    local agent_id="$1"
    
    echo "🔧 Initializing GitHub integration for $agent_id"
    
    # Ensure tmp directory exists
    mkdir -p tmp/
    
    # Initialize tracking files
    touch tmp/agent_issues.log
    touch tmp/agent_errors.log  
    touch tmp/agent_successes.log
    
    # Set up agent-specific issue templates
    echo "Agent $agent_id GitHub integration initialized at $(date)" >> tmp/github_integration.log
    
    # Check for existing assigned issues
    find_issues_to_work_on "$agent_id"
}

# Cleanup GitHub integration
cleanup_github_integration() {
    local agent_id="$1"
    
    echo "🧹 Cleaning up GitHub integration for $agent_id"
    
    # Comment on any open issues assigned to this agent
    gh issue list --assignee @me --state open --json number | \
        jq -r '.[].number' | while read -r issue_num; do
        comment_on_issue "$issue_num" "Agent session ending. Issue remains open for other agents." "$agent_id"
    done
    
    # Archive logs older than 30 days
    find tmp/ -name "*_issues.log" -mtime +30 -delete 2>/dev/null || true
    find tmp/ -name "*_errors.log" -mtime +30 -delete 2>/dev/null || true
}
```

### Error Handler Wrapper

```bash
# Wrapper for command execution with GitHub error handling
execute_with_github_error_handling() {
    local command="$1"
    local agent_id="$2"
    local context="$3"
    local files=("${@:4}")
    
    # Execute command and capture output
    local output
    local exit_code
    
    output=$(eval "$command" 2>&1)
    exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        # Command failed - create GitHub issue
        local error_title="Command failed in $agent_id: $(echo "$command" | cut -c1-50)..."
        local error_context="**Command**: \`$command\`
**Context**: $context
**Exit Code**: $exit_code
**Output**: 
\`\`\`
$output
\`\`\`"
        
        handle_agent_error "$error_title" "$agent_id" "$error_context" "${files[@]}"
        
        echo "❌ Command failed (GitHub issue created): $command"
        return $exit_code
    else
        echo "✅ Command succeeded: $command"
        return 0
    fi
}
```

This GitHub integration component provides:

1. **Automatic Issue Creation**: Every error creates a GitHub issue
2. **Work Discovery**: Agents find and claim issues to work on  
3. **Progress Tracking**: Issues updated with agent progress
4. **Resolution Handling**: Issues closed when problems are resolved
5. **Cross-Swarm Coordination**: Issues enable coordination between swarms
6. **Metrics & Reporting**: Daily and weekly issue activity tracking

The integration is designed to be seamless with existing workflows while providing complete visibility into swarm development progress.