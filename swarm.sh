#!/bin/bash

# ZEN Swarm Agent Generator
# Creates Claude Code sub-agents from assembled DNA files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
CLAUDE_DIR="$PROJECT_ROOT/.claude/agents"
DNA_DIR="$PROJECT_ROOT/swarm/dna"
SWARM_DIR="$PROJECT_ROOT/swarm"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}🧬 ZEN Swarm Agent Generator${NC}"
echo "================================"

# Function to check if DNA files exist
check_dna_files() {
    local swarm_id=$1
    local dna_subdir=""
    
    if [ -n "$swarm_id" ]; then
        dna_subdir="/$swarm_id"
    fi
    
    local target_dna_dir="$DNA_DIR$dna_subdir"
    
    if [ ! -d "$target_dna_dir" ] || [ -z "$(ls -A "$target_dna_dir" 2>/dev/null)" ]; then
        echo -e "${YELLOW}⚠️  No DNA files found for ${swarm_id:-default swarm}. Assembling from components...${NC}"
        
        # Check if assemble script exists
        if [ ! -f "$SWARM_DIR/assemble-dna.js" ]; then
            echo -e "${RED}❌ assemble-dna.js not found${NC}"
            return 1
        fi
        
        # Check if js-yaml is installed
        if ! npm list js-yaml >/dev/null 2>&1; then
            echo -e "${YELLOW}Installing js-yaml...${NC}"
            npm install js-yaml
        fi
        
        # Run assembly with swarm ID if provided
        if [ -n "$swarm_id" ]; then
            node "$SWARM_DIR/assemble-dna.js" --swarm-id "$swarm_id"
        else
            node "$SWARM_DIR/assemble-dna.js"
        fi
        echo ""
    fi
}

# Function to create agent from DNA file
create_agent_from_dna() {
    local dna_file=$1
    local agent_id=$2
    local agent_file="$CLAUDE_DIR/${agent_id}.md"
    
    if [ ! -f "$dna_file" ]; then
        echo -e "${RED}❌ DNA file not found: $dna_file${NC}"
        return 1
    fi
    
    # Create agents directory if it doesn't exist
    mkdir -p "$CLAUDE_DIR"
    
    # Copy DNA as agent file
    cp "$dna_file" "$agent_file"
    
    echo -e "${GREEN}✓ Created agent: ${agent_id}.md${NC}"
}

# Function to show usage
usage() {
    echo "Usage: $0 [init|add-worker|status|clean|create-swarm|create-overlord|populate]"
    echo ""
    echo "Commands:"
    echo "  init              - Create initial swarm (1 queen, 1 architect, 2 workers)"
    echo "  add-worker        - Add a new worker with specialization"
    echo "  status            - Show current swarm agents"
    echo "  clean             - Remove all generated agents"
    echo "  create-swarm      - Create a new swarm with specific ID"
    echo "  create-overlord   - Create the Overlord meta-agent (system evolution)"
    echo "  populate          - 🧬 Full ecosystem setup: assemble DNA, create all 4 swarms + overlord"
    echo ""
    echo "Examples:"
    echo "  $0 init"
    echo "  $0 add-worker lexer"
    echo "  $0 add-worker parser"
    echo "  $0 create-swarm swarm-1"
    echo "  $0 create-swarm swarm-2 --workers lexer,parser,types"
    echo "  $0 create-overlord"
    echo "  $0 populate         # Complete multi-swarm ecosystem setup"
}

# Initialize swarm with basic agents
init_swarm() {
    echo -e "${YELLOW}Initializing ZEN swarm...${NC}"
    
    # Ensure DNA files exist
    check_dna_files
    
    # Create queen
    create_agent_from_dna "$DNA_DIR/queen.md" "zen-queen"
    
    # Create architect
    create_agent_from_dna "$DNA_DIR/architect.md" "zen-architect"
    
    # Create initial workers
    create_agent_from_dna "$DNA_DIR/worker-lexer.md" "zen-worker-lexer"
    create_agent_from_dna "$DNA_DIR/worker-parser.md" "zen-worker-parser"
    
    echo ""
    echo -e "${GREEN}✨ Swarm initialized successfully!${NC}"
    echo ""
    echo "To use these agents in Claude Code:"
    echo "1. Run: /agents"
    echo "2. Your agents will appear with prefix 'zen-'"
    echo "3. Use explicit invocation: \"Use the zen-queen agent to analyze progress\""
    echo ""
}

# Add a specialized worker
add_worker() {
    local specialization=$1
    
    if [ -z "$specialization" ]; then
        echo -e "${RED}❌ Please specify worker specialization${NC}"
        echo "Example: $0 add-worker runtime"
        echo ""
        echo "Available specializations:"
        if [ -d "$SWARM_DIR/specializations" ]; then
            for spec in "$SWARM_DIR/specializations"/*.yaml; do
                if [ -f "$spec" ]; then
                    basename "$spec" .yaml | sed 's/^/  - /'
                fi
            done
        fi
        return 1
    fi
    
    # Ensure DNA files exist
    check_dna_files
    
    local worker_dna="$DNA_DIR/worker-${specialization}.md"
    local agent_id="zen-worker-${specialization}"
    
    if [ ! -f "$worker_dna" ]; then
        echo -e "${RED}❌ No DNA found for specialization: ${specialization}${NC}"
        echo "Available workers:"
        ls "$DNA_DIR"/worker-*.md 2>/dev/null | xargs -n1 basename | sed 's/worker-//;s/\.md$//' | sed 's/^/  - /'
        return 1
    fi
    
    create_agent_from_dna "$worker_dna" "$agent_id"
    echo -e "${GREEN}Added specialized worker: ${agent_id}${NC}"
}

# Show current swarm status
show_status() {
    echo -e "${BLUE}Current ZEN Swarm Agents:${NC}"
    echo "========================="
    
    if [ ! -d "$CLAUDE_DIR" ]; then
        echo -e "${YELLOW}No agents found. Run '$0 init' to create swarm.${NC}"
        return
    fi
    
    # First show default swarm agents
    local default_count=0
    echo -e "\n${BLUE}Default Swarm:${NC}"
    for agent_file in "$CLAUDE_DIR"/zen-*.md; do
        if [ -f "$agent_file" ]; then
            local agent_name=$(basename "$agent_file" .md)
            # Skip if it's part of a named swarm
            if [[ "$agent_name" =~ ^swarm-[^-]+-zen- ]]; then
                continue
            fi
            
            local agent_type="unknown"
            # Determine agent type
            if [[ "$agent_name" == *"queen"* ]]; then
                agent_type="👑 Queen"
            elif [[ "$agent_name" == *"architect"* ]]; then
                agent_type="📐 Architect"
            elif [[ "$agent_name" == *"worker"* ]]; then
                agent_type="🔧 Worker"
            fi
            
            echo -e "  ${GREEN}$agent_type: $agent_name${NC}"
            default_count=$((default_count + 1))
        fi
    done
    
    if [ $default_count -eq 0 ]; then
        echo -e "  ${YELLOW}No agents${NC}"
    fi
    
    # Check for Overlord agent
    if [ -f "$CLAUDE_DIR/zen-overlord.md" ]; then
        echo -e "\n${BLUE}Meta-System:${NC}"
        echo -e "  ${GREEN}🧠 Meta-Agent: zen-overlord${NC}"
    fi
    
    # Now show named swarms
    local swarm_ids=()
    for agent_file in "$CLAUDE_DIR"/*.md; do
        if [ -f "$agent_file" ]; then
            local agent_name=$(basename "$agent_file" .md)
            if [[ "$agent_name" =~ ^(swarm-[^-]+)-zen- ]]; then
                local swarm_id="${BASH_REMATCH[1]}"
                if [[ ! " ${swarm_ids[@]} " =~ " ${swarm_id} " ]]; then
                    swarm_ids+=("$swarm_id")
                fi
            fi
        fi
    done
    
    # Display each swarm
    for swarm_id in "${swarm_ids[@]}"; do
        echo -e "\n${BLUE}$swarm_id:${NC}"
        for agent_file in "$CLAUDE_DIR"/$swarm_id-*.md; do
            if [ -f "$agent_file" ]; then
                local agent_name=$(basename "$agent_file" .md)
                local agent_type="unknown"
                
                # Determine agent type
                if [[ "$agent_name" == *"queen"* ]]; then
                    agent_type="👑 Queen"
                elif [[ "$agent_name" == *"architect"* ]]; then
                    agent_type="📐 Architect"
                elif [[ "$agent_name" == *"worker"* ]]; then
                    agent_type="🔧 Worker"
                fi
                
                echo -e "  ${GREEN}$agent_type: $agent_name${NC}"
            fi
        done
    done
    
    echo ""
    local total_count=$(ls "$CLAUDE_DIR"/*.md 2>/dev/null | wc -l)
    echo "Total agents: $total_count"
}

# Clean all generated agents
clean_agents() {
    echo -e "${YELLOW}Removing all ZEN agents...${NC}"
    
    # Remove both default zen-*.md and swarm-specific swarm-*-zen-*.md agents
    for agent_file in "$CLAUDE_DIR"/zen-*.md "$CLAUDE_DIR"/swarm-*-zen-*.md; do
        if [ -f "$agent_file" ]; then
            rm -f "$agent_file"
            echo -e "${RED}✗ Removed: $(basename "$agent_file")${NC}"
        fi
    done
    
    echo -e "${GREEN}Cleanup complete.${NC}"
}

# Create a new swarm with specific ID
create_swarm() {
    local swarm_id=$1
    shift
    
    if [ -z "$swarm_id" ]; then
        echo -e "${RED}❌ Please specify swarm ID${NC}"
        echo "Example: $0 create-swarm swarm-1"
        return 1
    fi
    
    # Default workers if not specified - all available workers
    local workers="lexer,parser,types,runtime,stdlib,memory"
    
    # Parse additional options
    while [[ $# -gt 0 ]]; do
        case $1 in
            --workers)
                workers="$2"
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done
    
    echo -e "${YELLOW}Creating swarm: ${swarm_id}...${NC}"
    
    # Ensure DNA files exist for this swarm
    check_dna_files "$swarm_id"
    
    local dna_subdir="/$swarm_id"
    local target_dna_dir="$DNA_DIR$dna_subdir"
    
    # Create queen
    create_agent_from_dna "$target_dna_dir/queen.md" "$swarm_id-zen-queen"
    
    # Create architect
    create_agent_from_dna "$target_dna_dir/architect.md" "$swarm_id-zen-architect"
    
    # Create specified workers
    IFS=',' read -ra worker_array <<< "$workers"
    for worker in "${worker_array[@]}"; do
        worker=$(echo "$worker" | xargs) # trim whitespace
        create_agent_from_dna "$target_dna_dir/worker-$worker.md" "$swarm_id-zen-worker-$worker"
    done
    
    echo ""
    echo -e "${GREEN}✨ Swarm ${swarm_id} created successfully!${NC}"
    echo ""
    echo "To activate this swarm:"
    echo "  \"$swarm_id work\"     - Start all agents working"
    echo "  \"$swarm_id status\"   - Check swarm progress"
    echo "  \"$swarm_id continue\" - Continue previous work"
    echo ""
}

# Create the Overlord meta-agent
create_overlord() {
    echo -e "${BLUE}🧠 Creating Overlord Meta-Agent...${NC}"
    echo "========================================"
    
    # Check if assemble-dna.js exists and js-yaml is installed
    if [ ! -f "$SWARM_DIR/assemble-dna.js" ]; then
        echo -e "${RED}❌ assemble-dna.js not found${NC}"
        return 1
    fi
    
    if ! npm list js-yaml >/dev/null 2>&1; then
        echo -e "${YELLOW}Installing js-yaml...${NC}"
        npm install js-yaml
    fi
    
    # Generate Overlord DNA
    echo -e "${YELLOW}🧬 Assembling Overlord DNA...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --role overlord
    
    # Create the Overlord agent
    local overlord_dna="$DNA_DIR/overlord.md"
    if [ ! -f "$overlord_dna" ]; then
        echo -e "${RED}❌ Overlord DNA not generated successfully${NC}"
        return 1
    fi
    
    create_agent_from_dna "$overlord_dna" "zen-overlord"
    
    echo ""
    echo -e "${GREEN}🧠✨ Overlord Meta-Agent created successfully!${NC}"
    echo ""
    echo "The Overlord is the Meta-Architect of your multi-swarm system."
    echo ""
    echo "To activate the Overlord:"
    echo "  \"overlord analyze ecosystem\"     - Comprehensive multi-swarm analysis"
    echo "  \"overlord evolve [component]\"    - Mutate swarm components"
    echo "  \"overlord create [new-role]\"     - Design entirely new agent types"
    echo "  \"overlord optimize [swarm-id]\"   - Improve specific swarm performance"
    echo "  \"overlord coordinate-all\"        - Synchronize all 32+ agents"
    echo "  \"overlord work\"                  - Begin autonomous ecosystem evolution"
    echo ""
    echo -e "${YELLOW}⚠️  The Overlord can modify swarm components and create new agent types.${NC}"
    echo -e "${YELLOW}    Use responsibly for system evolution and meta-architecture tasks.${NC}"
    echo ""
}

# Populate complete multi-swarm ecosystem
populate_ecosystem() {
    echo -e "${BLUE}🧬 POPULATING COMPLETE ZEN MULTI-SWARM ECOSYSTEM${NC}"
    echo "=================================================="
    echo ""
    
    # Step 1: Clean existing agents
    echo -e "${YELLOW}Step 1: Cleaning existing agents...${NC}"
    clean_agents
    echo ""
    
    # Step 2: Check dependencies
    echo -e "${YELLOW}Step 2: Checking dependencies...${NC}"
    if [ ! -f "$SWARM_DIR/assemble-dna.js" ]; then
        echo -e "${RED}❌ assemble-dna.js not found${NC}"
        return 1
    fi
    
    if ! npm list js-yaml >/dev/null 2>&1; then
        echo -e "${YELLOW}Installing js-yaml...${NC}"
        npm install js-yaml
    fi
    echo -e "${GREEN}✓ Dependencies ready${NC}"
    echo ""
    
    # Step 3: Assemble DNA for all swarms
    echo -e "${YELLOW}Step 3: Assembling DNA for all swarms...${NC}"
    echo -e "${BLUE}  🧬 Generating DNA for swarm-1...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --swarm-id swarm-1
    
    echo -e "${BLUE}  🧬 Generating DNA for swarm-2...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --swarm-id swarm-2
    
    echo -e "${BLUE}  🧬 Generating DNA for swarm-3...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --swarm-id swarm-3
    
    echo -e "${BLUE}  🧬 Generating DNA for swarm-4...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --swarm-id swarm-4
    
    echo -e "${BLUE}  🧠 Generating Overlord DNA...${NC}"
    node "$SWARM_DIR/assemble-dna.js" --role overlord
    
    echo -e "${GREEN}✓ All DNA assembled${NC}"
    echo ""
    
    # Step 4: Create all swarms
    echo -e "${YELLOW}Step 4: Creating all swarms (32 agents total)...${NC}"
    
    echo -e "${BLUE}  Creating swarm-1...${NC}"
    create_swarm swarm-1
    echo ""
    
    echo -e "${BLUE}  Creating swarm-2...${NC}"
    create_swarm swarm-2
    echo ""
    
    echo -e "${BLUE}  Creating swarm-3...${NC}"
    create_swarm swarm-3
    echo ""
    
    echo -e "${BLUE}  Creating swarm-4...${NC}"
    create_swarm swarm-4
    echo ""
    
    # Step 5: Create overlord
    echo -e "${YELLOW}Step 5: Creating Overlord meta-agent...${NC}"
    create_overlord
    echo ""
    
    # Step 6: Show final status
    echo -e "${GREEN}🎉 ECOSYSTEM POPULATION COMPLETE!${NC}"
    echo "================================="
    echo ""
    show_status
    echo ""
    echo -e "${GREEN}✨ Complete ZEN Multi-Swarm Ecosystem Ready!${NC}"
    echo ""
    echo -e "${BLUE}Available Commands:${NC}"
    echo "  swarm-1 work         # Activate swarm-1 for parallel development"
    echo "  swarm-2 work         # Activate swarm-2 for parallel development"
    echo "  swarm-3 work         # Activate swarm-3 for parallel development"
    echo "  swarm-4 work         # Activate swarm-4 for parallel development"
    echo "  overlord work        # Activate meta-system evolution"
    echo "  overlord analyze     # Comprehensive multi-swarm analysis"
    echo ""
    echo -e "${YELLOW}Total Agents Created:${NC} $(ls "$CLAUDE_DIR"/*.md 2>/dev/null | wc -l)"
    echo -e "${YELLOW}Swarms Active:${NC} 4 specialized swarms + 1 overlord meta-system"
    echo ""
}

# Main command handling
case "$1" in
    init)
        init_swarm
        ;;
    add-worker)
        add_worker "$2"
        ;;
    status)
        show_status
        ;;
    clean)
        clean_agents
        ;;
    create-swarm)
        create_swarm "$2" "${@:3}"
        ;;
    create-overlord)
        create_overlord
        ;;
    populate)
        populate_ecosystem
        ;;
    *)
        usage
        exit 1
        ;;
esac