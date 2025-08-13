# ZEN Programming Language

A lightweight scripting language with natural syntax and minimal punctuation.

## Installation

```bash
# Build from source
make clean && make

# Run tests
make test

# Install system-wide
sudo make install PREFIX=/usr/local
```

## Quick Start

```bash
# Run REPL (ALWAYS use timeout to prevent hangs)
timeout 2 ./zen

# Run a ZEN file
timeout 2 ./zen script.zen

# One-liner
echo 'print "Hello World"' | timeout 2 ./zen
```

## Language Basics

### Key Syntax

- **`set` for assignment**: `set x 42`
- **`=` for comparison**: `if x = 42`
- **No semicolons** - newline terminated
- **No parentheses** in function calls
- **Indentation-based** blocks

### Variables and Types

```zen
# Basic types
set name "Alice"
set age 30
set pi 3.14159
set flag true
set nothing null

# Arrays
set numbers 1, 2, 3, 4, 5
print numbers[0]  # Access element: 1

# Objects
set user name "Bob", age 25, active true
print user.name   # Access property: Bob

# Nested structures
set config
    server "localhost",
    port 8080,
    database
        host "db.example.com",
        credentials
            user "admin",
            pass "secret"
```

### Control Flow

```zen
# If statements
if age >= 18
    print "Adult"
else if age >= 13
    print "Teen"
else
    print "Child"

# While loops
set count 0
while count < 5
    print count
    set count count + 1

# For loops (limited support)
set items "apple", "banana", "orange"
for item in items
    print item
```

### Functions

```zen
# Define function
function greet name
    return "Hello, " + name

# Call function
set message greet "World"
print message

# Functions with multiple parameters
function calculate x y operation
    if operation = "add"
        return x + y
    else if operation = "multiply"
        return x * y
    else
        return 0

set result calculate 10 5 "add"
print result  # 15
```

### Built-in Functions

```zen
# I/O
print "Output text"
set input readLine "Enter name: "

# Type conversion
set num toNumber "42"
set str toString 42

# JSON operations
set data jsonParse "{\"key\":\"value\"}"
set json jsonStringify data

# YAML operations
set config yamlParse "key: value"
set yaml yamlStringify config

# String operations
set upper toUpperCase "hello"
set lower toLowerCase "WORLD"
set parts split "a,b,c" ","
set joined join parts "-"

# Math
set r random
set rounded round 3.14159 2
```

## Working Features

✅ **Core Language**
- Variables and assignments
- Basic data types (null, boolean, number, string)
- Arrays and objects with access syntax
- Arithmetic and logical operations
- String concatenation
- If/else conditionals
- While loops
- Function definitions and calls
- Comments

✅ **Standard Library**
- print, readLine
- JSON parsing/stringification
- YAML parsing/stringification (requires libyaml)
- Type conversions
- Basic string operations
- Math functions

## Known Limitations

⚠️ **Important Notes**
- Always use `timeout` when running - some inputs can cause hangs
- For loops have limited functionality
- Complex nested operations may fail
- Error messages need improvement
- No module/import system yet
- No try/catch error handling

## Development

```bash
# Essential commands
make enforce              # Check manifest compliance
make format              # Format code
make test               # Run test suite
make test-valgrind      # Memory leak check
make lint               # Static analysis

# Debug flags
./zen --debug-lexer file.zen    # Show tokens
./zen --debug-ast file.zen      # Show AST
```

### Project Structure

```
src/
├── core/       # Lexer, parser, AST, runtime
├── types/      # Value system
├── runtime/    # Operators, execution
└── stdlib/     # Built-in functions

tests/          # Test suite
docs/           # Documentation
MANIFEST.json   # Function specifications (enforced)
CLAUDE.md      # AI assistant instructions
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. Key points:
- Follow `MANIFEST.json` specifications exactly
- Run `make enforce` before committing
- Add tests for new features
- Fix root causes, not symptoms

## License

MIT License - See [LICENSE](LICENSE)

## Author

Created by Nguyen Ky Son