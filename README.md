# ZEN Programming Language

> A lightweight, mobile-friendly, markdown-compatible scripting language designed for clarity, minimal syntax, and strong expressiveness.

ZEN is the programming language that is purest and closest to human natural language. It's ideal for embedding in documents, apps, AI workflows, or DSLs.

## Installation

```bash
make
make install
```

## Quick Start

```bash
# Run a ZEN file
zen hello.zen

# Interactive mode
zen
```

## Language Features

### Variables

Variables are declared using `set`:

```zen
set name "Alice"
set age 30
set active true
set pi 3.14
```

### Functions

Functions use space-separated parameters without parentheses:

```zen
function greet name
    return "Hello " + name

function add a b
    return a + b

# Function calls
print "Hello World"
greet "Bob"
```

### Data Structures

**Arrays:**
```zen
set scores 1, 2, 3
set items "pen", "book", "lamp"
```

**Objects:**
```zen
set user name "Alice", age 30, active true
```

### Control Flow

```zen
if score >= 90
    print "Excellent"
elif score >= 70
    print "Good"
else
    print "Try again"

for i in 1 .. 5
    print i
```

### Key Syntax Features

- **No semicolons** - statements are newline-based
- **No parentheses** for function calls
- **No braces** - indentation defines blocks
- **Minimal punctuation** - designed for mobile typing
- **Natural language-like** - `=` for comparison (not `==`)

### File Extensions

- `.zen` - standard ZEN files
- `.zn` - short form

## Current Implementation Status

This is a minimal interpreter that supports:
- ✓ Variable declarations with `set`
- ✓ String literals
- ✓ Function definitions (indentation-based)
- ✓ Function calls (space-separated arguments)
- ✓ Built-in `print` function
- ✓ Interactive REPL

## Building from Source

Requirements:
- GCC compiler
- Make

```bash
git clone https://github.com/yourusername/zen.git
cd zen
make
```

## Development Approach: Multi-Swarm Agentic System

This codebase employs a cutting-edge **Multi-Swarm Agentic Development Strategy** powered by Claude Code. We use 4 parallel swarms with 32 specialized AI agents working continuously to implement the ZEN language:

### 🐝 The Swarm Architecture

- **4 Swarms** running in parallel (swarm-1, swarm-2, swarm-3, swarm-4)
- **8 Agents per swarm**: 1 Queen, 1 Architect, 6 Specialized Workers
- **32 Total Agents** working simultaneously on different components
- **Persistent Development**: Agents work continuously until 100% completion

### 🎯 Agent Specializations

| Agent Type | Role | Authority |
|------------|------|-----------|
| **Queen** | Strategic coordination, task assignment | Read-only manifest access |
| **Architect** | System design, interface specification | Exclusive manifest write access |
| **Worker-Lexer** | Tokenization and lexical analysis | Implementation only |
| **Worker-Parser** | AST construction and syntax analysis | Implementation only |
| **Worker-Types** | Value system and type operations | Implementation only |
| **Worker-Runtime** | Execution engine and control flow | Implementation only |
| **Worker-Memory** | Memory management and garbage collection | Implementation only |
| **Worker-Stdlib** | Built-in functions and standard library | Implementation only |

### 🚀 Running the Swarms

```bash
# Create all 4 swarms
./swarm.sh create-swarm swarm-1 --workers lexer,parser,types,runtime,stdlib,memory
./swarm.sh create-swarm swarm-2 --workers lexer,parser,types,runtime,stdlib,memory
./swarm.sh create-swarm swarm-3 --workers lexer,parser,types,runtime,stdlib,memory
./swarm.sh create-swarm swarm-4 --workers lexer,parser,types,runtime,stdlib,memory

# Run swarms in tmux (4 panes)
./tmux.sh
# Then in each pane: swarm-1 work, swarm-2 work, swarm-3 work, swarm-4 work
```

### 📊 Development Visibility

```bash
# See real-time progress of all 32 agents
make vision

# Check manifest compliance
make enforce

# View active tasks
node task.js list --active
```

### 🔄 How It Works

1. **Continuous Development**: Agents work in infinite loops until the project is 100% complete
2. **Task Management**: Every agent creates task files using `task.js` for full visibility
3. **Conflict Prevention**: File ownership tracking prevents multiple agents from editing the same file
4. **Quality Enforcement**: Only architects can modify MANIFEST.json; all work must comply
5. **Progress Tracking**: Real-time fitness scores and completion percentages for all agents

### 📖 Learn More

- See `docs/SWARM_GUIDE.md` for detailed swarm usage
- See `docs/TASK_GUIDE.md` for task management system
- See `CLAUDE.md` for Claude Code integration details

## Contributing

When contributing to this multi-swarm developed codebase:
1. Check `make vision` to see what agents are working on
2. Coordinate with the swarm queens for task assignment
3. Follow MANIFEST.json specifications exactly
4. Use task.js to track your work alongside the AI agents

Key areas for contribution:
- Number and boolean types
- Arithmetic operators
- Control flow structures (if/else, loops)
- Arrays and objects
- File I/O operations

## License

See LICENSE file for details.

---

*Created by Nguyen Ky Son - Aug 1st 2025*
