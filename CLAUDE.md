# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ZEN is a lightweight, mobile-friendly, markdown-compatible scripting language with minimal syntax. The project implements a comprehensive interpreter for a natural language-inspired programming language.

**Key Resources:**
- Language Specification: `docs/idea.md` (complete 564-line spec)
- Architecture Document: `ARCHITECTURE.md` (complete codebase vision)
- Current Implementation: Minimal subset in C

## Development Commands

### Build & Run
```bash
# Build the interpreter
make clean && make

# Run REPL
./zen

# Run a ZEN file
./zen filename.zen

# Run tests (when implemented)
make test

# Format code (when implemented)
make format

# Install
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

## Development Workflow

1. **Before implementing**: Review `docs/idea.md` for exact syntax
2. **During implementation**: Follow existing patterns in codebase
3. **After implementation**: Run tests, update docs, format code
4. **Before committing**: Ensure all tests pass, no memory leaks

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