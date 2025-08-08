# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

⚠️ **CRITICAL**: This project enforces strict compliance through `MANIFEST.json`. You MUST follow the manifest exactly or your code will be rejected by git hooks.

## Project Overview

ZEN is a lightweight, mobile-friendly, markdown-compatible scripting language with minimal syntax. The project implements a comprehensive interpreter for a natural language-inspired programming language.

**Key Resources:**
- Language Specification: `docs/idea.md` (complete 564-line spec)
- **Manifest**: `MANIFEST.json` (🔒 ENFORCED specification of ALL files/functions)
- Current Implementation: Full interpreter with multi-swarm AI development

## Development Commands

### Essential Commands
```bash
# 🔒 ENFORCEMENT (Always use these!)
make enforce              # Check manifest compliance
make enforce-generate     # Generate code stubs from manifest
make setup-dev           # Setup git hooks (one-time)

# Build & Run
make clean && make       # Build the interpreter
./zen                    # Run REPL
./zen filename.zen       # Run a ZEN file

# Development
make format              # Format code
make test               # Run comprehensive test suite
make test-unit          # Run unit tests only
make test-integration   # Run integration tests
make test-valgrind      # Check for memory leaks
make lint              # Static analysis (cppcheck)

# Project Visualization
make vision            # See real-time project status and agent activity

# Installation
sudo make install PREFIX=/usr/local
```

## Multi-Swarm Architecture

This project uses a unique **Multi-Swarm Agentic Development System** with 32 AI agents working in parallel:

- **4 Swarms** (swarm-1, swarm-2, swarm-3, swarm-4) 
- **8 Agents per swarm**: 1 Queen, 1 Architect, 6 Specialized Workers
- **Specialized roles**: lexer, parser, types, runtime, memory, stdlib workers

### Swarm Commands
```bash
# Activate specific swarms
swarm-1 work            # Activate swarm-1 coordination
swarm-2 continue        # Continue swarm-2 tasks
make vision            # Monitor all 32 agents in real-time

# Task management
node task.js list --active    # See active tasks across all swarms
```

## Architectural Overview

The interpreter follows a modular pipeline architecture:

```
Input → Lexer → Parser → AST → Visitor → Output
        ↓       ↓       ↓      ↓
      tokens   AST    Value   Result
```

### Core Components

**src/core/**: Foundation systems
- `lexer.c`: Tokenization with indentation tracking, number parsing, keyword recognition
- `parser.c`: Recursive descent parser with operator precedence, expression evaluation  
- `ast.c`: Abstract Syntax Tree with 47 node types, memory management integration
- `visitor.c`: Runtime evaluation engine with control flow, operators, built-ins
- `scope.c`: Variable/function scope management with nested scoping
- `memory.c`: Reference counting, leak detection, atomic operations, debug tracking
- `error.c`: Comprehensive error system with location tracking, source context

**src/types/**: Value system with reference counting
- `value.c`: Core value types (null, boolean, number, string, array, object, function, error)
- `array.c`: Dynamic arrays with automatic resizing
- `object.c`: Hash-map based objects with key-value pairs

**src/runtime/**: Execution engine
- `operators.c`: Arithmetic, logical, comparison operators with type coercion

**src/stdlib/**: Built-in functions
- `io.c`: File operations, print functions, user input
- `json.c`: JSON parsing/stringification (native implementation)
- `string.c`: String manipulation functions
- `math.c`: Mathematical operations and constants
- `convert.c`: Type conversion utilities

## ZEN Language Key Points

Critical language features to remember:
- `=` is comparison (not `==`)
- No semicolons (newline-terminated)
- No parentheses in function calls  
- Indentation-based blocks
- Comma-separated data structures
- Space-separated function arguments

Example showcasing key syntax:
```zen
set scores 85, 92, 78
set student name "Alice", scores scores

function average nums
    set sum 0
    for n in nums
        set sum sum + n
    return sum / nums.length

if average student.scores >= 80
    print "Great job, " + student.name
```

## Current Implementation Status

✅ **Fully Implemented:**
- Complete lexer with number parsing, keyword recognition, indentation handling
- Full parser with expression evaluation, operator precedence, proper ZEN syntax
- Comprehensive AST system with all node types and memory management
- Runtime visitor with arithmetic, logical operations, control flow evaluation
- Complete value system with reference counting and type operations
- Memory management with leak detection and debugging capabilities
- Error handling with location tracking and source context display
- I/O operations and standard library functions
- Multi-swarm coordination system with 32 AI agents

✅ **Working Programs:**
```zen
set x 5
set y 10
set result x + y * 2
print result          # Outputs: 25

function greet name
    print "Hello, " + name

greet "World"         # Outputs: Hello, World
```

## 🔒 CRITICAL: Code Enforcement System

**IMPORTANT**: This project uses a strict enforcement system. ALL code changes MUST comply with `MANIFEST.json`.

### Enforcement Rules
1. **NO new files** without updating `MANIFEST.json` first
2. **NO function signature changes** without manifest update
3. **NO unauthorized functions** - all functions must be in manifest
4. **EXACT signatures required** - must match character-for-character

### Working with the Manifest

Before implementing ANY function:
1. Check `MANIFEST.json` for the exact signature
2. Copy the signature exactly as specified
3. Implement following the signature contract

Example from manifest:
```json
{
  "name": "lexer_new",
  "signature": "Lexer* lexer_new(const char* input)",
  "description": "Create a new lexer instance"
}
```

Your implementation MUST match exactly:
```c
Lexer* lexer_new(const char* input) {  // Exact signature!
    // Implementation here
}
```

### Development Workflow

1. **Check manifest first**: Look up function in `MANIFEST.json`
2. **Run enforcement**: `make enforce` to see current state
3. **Generate stubs**: `make enforce-generate` if needed
4. **Implement exactly**: Follow manifest signatures precisely
5. **Validate**: `make enforce` before committing
6. **Commit**: Git hooks will enforce compliance automatically

## Memory Management

The project uses comprehensive reference counting:
- All heap-allocated values use `memory_alloc()`, `memory_free()`
- Values have `ref_count` managed by `value_ref()`, `value_unref()`
- Enable debugging: `memory_debug_enable(true)`
- Check leaks: `make test-valgrind`

## Testing Architecture

Comprehensive test suite with multiple categories:
```bash
make test-unit          # Core component tests
make test-integration   # System integration tests
make test-language      # ZEN language feature tests
make test-stdlib        # Standard library tests
make test-memory        # Memory management tests
make test-valgrind      # Leak detection
```

Test files are organized by component in `tests/` with framework in `tests/framework/`.

## Code Style Requirements

### C Code Style
- Use `.clang-format` for consistent formatting: `make format`
- Function names: `module_action_target()` (e.g., `lexer_scan_token()`)
- Struct names: `PascalCase` (e.g., `ASTNode`)
- Constants: `UPPER_SNAKE_CASE`
- Include guards: `ZEN_MODULE_H`

### Error Handling
- All functions that can fail should return error codes
- Use comprehensive error system with location tracking
- Provide clear error messages with line/column info

### Integration with Multi-Swarm System
- When making changes, check `make vision` to see active agent work
- Coordinate with swarm queens using `swarm-N work` commands
- Create task files using `node task.js create` for visibility
- Follow file ownership tracking to avoid conflicts

## Performance Considerations

1. **Lexer**: Buffered I/O, minimal allocations, robust number parsing
2. **Parser**: Single-pass recursive descent with expression evaluation
3. **Runtime**: Value-based execution with reference counting
4. **Memory**: Comprehensive tracking with leak detection
5. **Values**: Reference counting with automatic cleanup

## Debugging Tips

```bash
# Debug lexer output
./zen --debug-lexer file.zen

# Debug parser AST  
./zen --debug-ast file.zen

# Check memory leaks with comprehensive tracking
make test-valgrind

# Monitor all 32 AI agents working on the project
make vision

# Check manifest compliance before committing
make enforce
```

## Multi-Swarm Coordination

This project uniquely employs 32 specialized AI agents. When contributing:
1. Check `make vision` to see current agent activity
2. Use `swarm-N work` to coordinate with specific swarms
3. Follow manifest compliance strictly - agents enforce this automatically
4. All work is tracked through task management system for full visibility

The multi-swarm system has achieved 100% implementation of the core ZEN interpreter, making this a fully functional programming language ready for real-world use.