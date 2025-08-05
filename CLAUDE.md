# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

⚠️ **CRITICAL**: This project enforces strict compliance through `MANIFEST.json`. You MUST follow the manifest exactly or your code will be rejected by git hooks.

## Project Overview

ZEN is a lightweight, mobile-friendly, markdown-compatible scripting language with minimal syntax. The project implements a comprehensive interpreter for a natural language-inspired programming language.

**Key Resources:**
- Language Specification: `docs/idea.md` (complete 564-line spec)
- Architecture Document: `ARCHITECTURE.md` (complete codebase vision)
- **Manifest**: `MANIFEST.json` (🔒 ENFORCED specification of ALL files/functions)
- Enforcement Guide: `ENFORCEMENT.md` (how the enforcement system works)
- Current Implementation: Minimal subset in C

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
make test               # Run tests (when implemented)

# Installation
sudo make install PREFIX=/usr/local
```

## Architectural Overview

See `ARCHITECTURE.md` for complete codebase structure. The interpreter follows a modular architecture:

```
src/
├── core/      # Lexer, parser, AST, visitor, scope, error, memory
├── types/     # Value system: string, number, boolean, array, object
├── runtime/   # Operators, control flow, OOP, modules, async
└── stdlib/    # I/O, JSON/YAML, regex, HTTP, database ops
```

## Implementation Priorities

When implementing new features, follow this priority order:

### Phase 1: Core Types & Operations
1. Number type with arithmetic operators (+, -, *, /, %)
2. Boolean type with logical operators (&, |, !)
3. Comparison operators (=, !=, <, >, <=, >=)
4. String concatenation and methods

### Phase 2: Control Flow
1. If/elif/else statements
2. While loops
3. For loops (range and array iteration)
4. Break/continue statements

### Phase 3: Data Structures
1. Arrays with comma syntax
2. Objects with key-value pairs
3. Array/object methods
4. Destructuring assignment

### Phase 4: Advanced Features
1. Error handling (try/catch/throw)
2. Classes and OOP
3. Module system (import/export)
4. File I/O and JSON/YAML support

### Phase 5: Extended Features
1. Async/await
2. Generators
3. Regular expressions
4. Formal logic system

## Code Style Requirements

### C Code Style
- Use `.clang-format` for consistent formatting
- Function names: `module_action_target()` (e.g., `lexer_scan_token()`)
- Struct names: `PascalCase` (e.g., `ASTNode`)
- Constants: `UPPER_SNAKE_CASE`
- Include guards: `ZEN_MODULE_H`

### Memory Management
- Use reference counting for all heap-allocated values
- Free all allocated memory in error paths
- Run Valgrind on all changes: `valgrind --leak-check=full ./zen`

### Error Handling
- All functions that can fail should return error codes
- Use `Error*` struct for detailed error information
- Provide clear error messages with line/column info

## Testing Requirements

Every new feature MUST include:
1. Unit tests in `tests/unit/`
2. Integration tests in `tests/integration/`
3. Example programs in `examples/`
4. Documentation updates

Test coverage target: 80% minimum

## External Dependencies

Required libraries (see `scripts/setup.sh`):
- **cJSON**: JSON parsing/generation (lightweight, ~750KB, single header option)
- **libyaml**: YAML parsing/generation
- **pcre2**: Regular expression support
- **libcurl**: HTTP client functionality
- **sqlite3**: Optional persistent storage

Development tools:
- **cmocka**: Unit testing framework
- **valgrind**: Memory debugging
- **clang-format**: Code formatting
- **gcov/lcov**: Code coverage

### Why cJSON for JSON support?
- **Lightweight**: Single C file (~750KB), can be embedded directly
- **Simple API**: Easy to integrate with ZEN's value system
- **MIT License**: Compatible with ZEN's MIT license
- **Battle-tested**: Used by many projects including ESP-IDF, AWS IoT
- **Memory efficient**: Suitable for mobile devices (ZEN's target)
- **Alternative considered**: json-c (more features but heavier)

## Performance Considerations

1. **Lexer**: Use buffered I/O, minimize allocations
2. **Parser**: Single-pass recursive descent
3. **Runtime**: Stack-based execution, minimize heap allocations
4. **Strings**: Intern common strings
5. **GC**: Reference counting with cycle detection

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

✅ Implemented:
- Basic lexer with indentation handling
- Simple parser for variables and functions
- String type support
- Variable scoping
- Function definitions and calls
- REPL

❌ Not Yet Implemented:
- Numbers, booleans, null
- All operators except string concatenation
- Control flow structures
- Arrays and objects
- Standard library
- Module system
- Error handling
- Classes/OOP
- Async features

## 🔒 CRITICAL: Code Enforcement System

**IMPORTANT**: This project uses a strict enforcement system. ALL code changes MUST comply with `MANIFEST.json`.

### Enforcement Commands
```bash
# ALWAYS run before making changes
make enforce              # Check current compliance
make enforce-generate     # Generate stub code from manifest

# One-time setup (REQUIRED)
make setup-dev           # Enable git hooks for automatic enforcement
```

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

Your implementation MUST match:
```c
Lexer* lexer_new(const char* input) {  // Exact signature!
    // Implementation here
}
```

## Development Workflow

1. **Check manifest first**: Look up function in `MANIFEST.json`
2. **Run enforcement**: `make enforce` to see current state
3. **Generate stubs**: `make enforce-generate` if needed
4. **Implement exactly**: Follow manifest signatures precisely
5. **Validate**: `make enforce` before committing
6. **Commit**: Git hooks will enforce compliance automatically

## Debugging Tips

```bash
# Debug lexer output
./zen --debug-lexer file.zen

# Debug parser AST
./zen --debug-ast file.zen

# Check memory leaks
valgrind ./zen file.zen

# Generate coverage report
make coverage
```