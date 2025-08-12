# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

🔴 **CRITICAL REMINDER - ALWAYS USE TIMEOUT**: 
- **NEVER run `./zen` without timeout** - it can hang forever!
- **ALWAYS use: `timeout 2 ./zen file.zen`** (2 seconds max)
- **For longer operations: `timeout 5 ./zen file.zen`**
- **This is MANDATORY - the interpreter WILL hang on certain inputs**

⚠️ **CRITICAL**: This project enforces strict compliance through `MANIFEST.json`. You MUST follow the manifest exactly or your code will be rejected by git hooks.

## Project Overview

ZEN is a lightweight, mobile-friendly, markdown-compatible scripting language with minimal syntax. The project implements a comprehensive interpreter for a natural language-inspired programming language.

**Key Resources:**
- Language Specification: `docs/idea.md` (complete 564-line spec)
- **Manifest**: `MANIFEST.json` (🔒 ENFORCED specification of ALL files/functions)
- Current Implementation: Full interpreter in development

## Development Commands

### Essential Commands
```bash
# 🔒 ENFORCEMENT (Always use these!)
make enforce              # Check manifest compliance
make enforce-generate     # Generate code stubs from manifest
make setup-dev           # Setup git hooks (one-time)

# Build & Run
make clean && make       # Build the interpreter
timeout 2 ./zen          # Run REPL (WITH TIMEOUT!)
timeout 2 ./zen file.zen # Run a ZEN file (ALWAYS USE TIMEOUT!)
# ⚠️ NEVER run ./zen without timeout - it WILL hang!

# Development
make format              # Format code
make test               # Run comprehensive test suite
make test-unit          # Run unit tests only
make test-integration   # Run integration tests
make test-valgrind      # Check for memory leaks
make lint              # Static analysis (cppcheck)


# Installation
sudo make install PREFIX=/usr/local
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
- **`=` is comparison** (not assignment) - use `set` for assignment
- No `==` operator - just use `=` for equality checks
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
- JSON parsing and stringification with full round-trip support
- YAML parsing and stringification (with libyaml)
- Nested object syntax with indentation (via `parser_parse_indented_object`)

✅ **Working Programs:**
```zen
set x 5
set y 10
set result x + y * 2
print result          # Outputs: 25

function greet name
    print "Hello, " + name

greet "World"         # Outputs: Hello, World

# Nested object with indentation
set user
    name "Alice",
    profile
        age 30,
        location
            city "NYC"
print user.profile.location.city  # Outputs: NYC

# JSON/YAML round-trip
set data jsonParse '{"key":"value"}'
print yamlStringify data          # Outputs YAML format
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

# Check manifest compliance before committing
make enforce
```


## Important Development Principles

### String Interpolation
- In ZEN, for variable interpolation in strings, use `{{var}}` instead of `${var}`

### Root Cause Analysis
**CRITICAL**: When encountering bugs or issues, ALWAYS identify and fix the root cause rather than implementing workarounds.
- Do NOT apply band-aid solutions or fix symptoms
- Invest time to understand the fundamental problem
- Proper root cause fixes prevent future issues and save time long-term
- Workarounds create technical debt and compound problems
- When a bug/error/issue happen -> find and fix the root cause, do not use workaround.
- All functions must follow this naming format: <filename>_<functionname>
- Do not stub, mock, todos etc... We'd rather go slowly but precisely functional, than go fast but not functional.
- When working without any specific objectives, try to use `gh` to look for open issues and try to fix the open issues -> update/close them when fixed.
- in zen class, zen uses self instead of this