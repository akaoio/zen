#!/bin/bash

# tmux.sh - Create or attach to a tmux session with 4 claude instances
# Usage: ./tmux.sh [--session N] [help]

SESSION_NUM=1
ATTACH_MODE=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --session)
            SESSION_NUM="$2"
            if ! [[ "$SESSION_NUM" =~ ^[1-9][0-9]*$ ]]; then
                echo "Error: --session must be a positive integer greater than 0"
                exit 1
            fi
            shift 2
            ;;
        --attach-session)
            ATTACH_MODE="session"
            ;;
        --new-terminal)
            # Create new terminal window attached to specific tmux window
            ATTACH_MODE="new-terminal"
            ;;
        help|--help|-h)
            SHOW_HELP=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: ./tmux.sh [--session N] [--attach-session] [--new-terminal] [help]"
            exit 1
            ;;
    esac
done

# Show help if requested
if [ "$SHOW_HELP" = "1" ]; then
    cat << 'EOF'
╔══════════════════════════════════════════════════════════════════════╗
║                        TMUX SWARM LAUNCHER                           ║
╚══════════════════════════════════════════════════════════════════════╝

Usage: ./tmux.sh [--session N] [--attach-session] [--new-terminal] [help]

This script creates or attaches to a tmux session with 4 Claude instances
running in horizontal panes for ZEN swarm development.

Options:
  --session N       Use/create session number N (default: 1)
                    Creates 4 horizontal panes with Claude instances in that session
  --attach-session  Attach to specific session only (for use in separate terminal tabs)
  --new-terminal    Open in new terminal window attached to specific tmux session

ESSENTIAL TMUX SHORTCUTS:
══════════════════════════
Navigation:
  Ctrl+b → ←     Move between panes left/right
  Ctrl+b ↑ ↓     Move between panes up/down  
  Ctrl+b o       Cycle through panes
  Ctrl+b q       Show pane numbers (press number to jump)
  
Pane Management:
  Ctrl+b z       Zoom/unzoom current pane (fullscreen toggle)
  Ctrl+b x       Kill current pane (with confirmation)
  Ctrl+b !       Break pane into new window
  Ctrl+b space   Cycle through layouts
  
Session Control:
  Ctrl+b d       Detach from session (keeps running in background)
  Ctrl+b s       List/switch sessions
  Ctrl+b $       Rename current session
  
Scrolling/Copy Mode:
  Ctrl+b [       Enter scroll/copy mode (vim keys to navigate)
    ↑/↓ or j/k   Scroll up/down
    PgUp/PgDn    Page up/down
    g/G          Go to top/bottom
    /            Search forward
    ?            Search backward
    q            Exit scroll mode
    
Pane Resizing:
  Ctrl+b Alt+←   Resize pane left
  Ctrl+b Alt+→   Resize pane right
  Ctrl+b Ctrl+←  Resize pane left (fine)
  Ctrl+b Ctrl+→  Resize pane right (fine)

Window Management:
  Ctrl+b c       Create new window
  Ctrl+b n       Next window
  Ctrl+b p       Previous window
  Ctrl+b 0-9     Switch to window number
  Ctrl+b w       List windows

SWARM AGENT TIPS:
═════════════════
• Each pane runs an independent Claude instance
• Use "swarm-N work" to activate swarm N (where N is 1-4)
• Each swarm has its own queen, architect, and 6 specialized workers
• Agents work in isolated workspaces: workspace/<swarm-id>-<agent-id>/

USAGE EXAMPLES:
══════════════
Open multiple terminal tabs with different sessions:
  Terminal Tab 1: ./tmux.sh --session 1 --attach-session
  Terminal Tab 2: ./tmux.sh --session 2 --attach-session
  
Create and attach to session 3:
  ./tmux.sh --session 3
  
Open new terminal window for session 2:
  ./tmux.sh --session 2 --new-terminal

To kill a specific session: tmux kill-session -t zen-swarm-N
To list sessions: tmux ls
To reattach later: ./tmux.sh --session N or tmux attach -t zen-swarm-N

EOF
    exit 0
fi

# Function to create 4 horizontal panes in a session
create_session_panes() {
    local session_name=$1
    
    # Split horizontally to create second pane
    tmux split-window -h -t "$session_name:0"
    
    # Split the first pane horizontally to create third pane
    tmux split-window -h -t "$session_name:0.0"
    
    # Split the right pane horizontally to create fourth pane
    tmux split-window -h -t "$session_name:0.2"
    
    # Select layout to make all panes equal size horizontally
    tmux select-layout -t "$session_name:0" even-horizontal
    
    # Send claude.sh command to each pane with swarm info
    tmux send-keys -t "$session_name:0.0" 'echo "=== Pane 1: Use \"swarm-1 work\" to activate swarm-1 ===" && ./claude.sh' C-m
    tmux send-keys -t "$session_name:0.1" 'echo "=== Pane 2: Use \"swarm-2 work\" to activate swarm-2 ===" && ./claude.sh' C-m
    tmux send-keys -t "$session_name:0.2" 'echo "=== Pane 3: Use \"swarm-3 work\" to activate swarm-3 ===" && ./claude.sh' C-m
    tmux send-keys -t "$session_name:0.3" 'echo "=== Pane 4: Use \"swarm-4 work\" to activate swarm-4 ===" && ./claude.sh' C-m
}

# Create session name based on session number
SESSION_NAME="zen-swarm-$SESSION_NUM"

# Check if tmux session already exists
tmux has-session -t "$SESSION_NAME" 2>/dev/null

if [ $? != 0 ]; then
    echo "Creating new tmux session: $SESSION_NAME"
    
    # Create new session with initial window
    tmux new-session -d -s "$SESSION_NAME" -n "claude-swarm"
    
    # Create the 4 panes in the session
    create_session_panes "$SESSION_NAME"
else
    echo "Session $SESSION_NAME already exists"
fi

# Handle different attach modes
case "$ATTACH_MODE" in
    "session")
        # Attach to specific session (for separate terminal tabs)
        echo "Attaching to session $SESSION_NAME..."
        tmux attach-session -t "$SESSION_NAME"
        ;;
    "new-terminal")
        # Open new terminal window attached to specific tmux session
        echo "Opening new terminal for session $SESSION_NAME..."
        # Try common terminal emulators
        if command -v gnome-terminal &> /dev/null; then
            gnome-terminal -- bash -c "./tmux.sh --session $SESSION_NUM --attach-session"
        elif command -v xterm &> /dev/null; then
            xterm -e "./tmux.sh --session $SESSION_NUM --attach-session" &
        elif command -v konsole &> /dev/null; then
            konsole -e bash -c "./tmux.sh --session $SESSION_NUM --attach-session" &
        elif command -v alacritty &> /dev/null; then
            alacritty -e bash -c "./tmux.sh --session $SESSION_NUM --attach-session" &
        elif command -v kitty &> /dev/null; then
            kitty bash -c "./tmux.sh --session $SESSION_NUM --attach-session" &
        elif command -v wezterm &> /dev/null; then
            wezterm start -- bash -c "./tmux.sh --session $SESSION_NUM --attach-session" &
        elif command -v terminator &> /dev/null; then
            terminator -e "./tmux.sh --session $SESSION_NUM --attach-session" &
        else
            echo "Error: No supported terminal emulator found"
            echo "Supported: gnome-terminal, xterm, konsole, alacritty, kitty, wezterm, terminator"
            exit 1
        fi
        ;;
    *)
        # Default: attach to session
        echo "Attaching to session $SESSION_NAME..."
        tmux attach-session -t "$SESSION_NAME"
        ;;
esac
