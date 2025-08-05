# ZEN Swarm Quick Reference

## 🚀 Quick Start

1. **Create 4 swarms** (already done):
   ```bash
   ./swarm.sh create-swarm swarm-1
   ./swarm.sh create-swarm swarm-2
   ./swarm.sh create-swarm swarm-3
   ./swarm.sh create-swarm swarm-4
   ```

2. **Launch tmux with 4 panes**:
   ```bash
   ./tmux.sh
   ```

3. **In each pane, activate the corresponding swarm**:
   - Pane 1: `"swarm-1 work"`
   - Pane 2: `"swarm-2 work"`
   - Pane 3: `"swarm-3 work"`
   - Pane 4: `"swarm-4 work"`

## 📋 Swarm Commands

### Activation Commands
- `"swarm-N work"` - Start swarm N working on tasks
- `"swarm-N status"` - Check progress of swarm N
- `"swarm-N continue"` - Continue previous work

### Management Commands
```bash
./swarm.sh status              # Show all swarms and agents
./swarm.sh create-swarm NAME   # Create new swarm
./swarm.sh clean               # Remove all agents
```

## 👥 Each Swarm Contains

1. **Queen** (`swarm-N-zen-queen`)
   - Strategic coordination
   - Task assignment
   - Progress monitoring

2. **Architect** (`swarm-N-zen-architect`)
   - Technical design
   - Interface specification
   - Algorithm selection

3. **Workers** (6 specialists):
   - `swarm-N-zen-worker-lexer` - Tokenization
   - `swarm-N-zen-worker-parser` - Syntax analysis
   - `swarm-N-zen-worker-types` - Value system
   - `swarm-N-zen-worker-runtime` - Execution engine
   - `swarm-N-zen-worker-stdlib` - Built-in functions
   - `swarm-N-zen-worker-memory` - Memory management

## 🗂️ Workspace Isolation

Each agent works in its own directory:
```
workspace/
├── swarm-1-zen-queen/
├── swarm-1-zen-worker-lexer/
├── swarm-1-zen-worker-parser/
└── ... (etc)
```

## 🎯 Tmux Navigation

- `Ctrl+b →/←` - Move between panes
- `Ctrl+b z` - Zoom current pane
- `Ctrl+b d` - Detach from session
- `Ctrl+b [` - Enter scroll mode (q to exit)

## 💡 Example Workflow

1. **All swarms analyze the project**:
   ```
   In each pane: "swarm-N work"
   ```

2. **Assign specific tasks**:
   - Swarm 1: "Focus on lexer implementation"
   - Swarm 2: "Work on parser components"
   - Swarm 3: "Implement type system"
   - Swarm 4: "Build runtime engine"

3. **Check progress**:
   ```
   In any pane: "./swarm.sh status"
   ```