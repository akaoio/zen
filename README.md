# ZEN Programming Language

A lightweight, human-friendly scripting language with natural syntax, minimal punctuation, and powerful file operations.

## 🚀 Quick Start

```bash
# Build from source
make clean && make

# Install system-wide
sudo make install PREFIX=/usr/local

# Run a ZEN script (ALWAYS use timeout to prevent hangs)
timeout 2 ./zen script.zen

# Interactive REPL
timeout 2 ./zen
```

## 📝 Language Overview

ZEN is designed for simplicity and readability with:
- **Natural syntax** - reads like plain English
- **No semicolons** - newline terminated
- **No parentheses** in function calls
- **Indentation-based** blocks
- **Database-like file operations** with GET/PUT keywords

### Core Syntax Rules

```zen
# Assignment uses 'set'
set x 42

# Comparison uses '=' (not '==')
if x = 42
    print "The answer"

# Functions without parentheses
print "Hello World"
set result calculate 10 20
```

## 🎯 Key Features

### Variables and Data Types

```zen
# Basic types
set name "Alice"
set age 30
set pi 3.14159
set active true
set empty null

# Arrays (comma-separated)
set numbers 1, 2, 3, 4, 5
print numbers[2]  # Output: 3

# Objects (key-value pairs)
set user name "Bob", age 25, active true
print user.name   # Output: Bob

# Deep nesting (5+ levels supported!)
set company 
    name "TechCorp",
    departments
        engineering
            backend
                team "Infrastructure",
                size 10
```

### Database-like File Operations (NEW!)

```zen
# GET - Read from JSON/YAML files with streaming
set data get "large_file.json"                    # Entire file
set name get "users.json".alice.profile.name      # Nested property
set config get "settings.yaml".server.port        # Works with YAML too

# PUT - Write to files with auto-detection
put "config.json" debug false, port 3000          # Create/overwrite file
put "data.json".users.alice.age 31                # Modify specific property
put "nested.json" a b c d e "deep value"          # Deep nesting (5+ levels)

# Handles GB-sized files efficiently with streaming!
```

### Control Flow

```zen
# If/else statements
if age >= 18
    print "Adult"
else if age >= 13
    print "Teen"
else
    print "Child"

# While loops
set i 0
while i < 5
    print "Count: " + i
    set i i + 1

# For loops (arrays)
set fruits "apple", "banana", "orange"
for fruit in fruits
    print "I like " + fruit
```

### Functions

```zen
# Define functions
function greet name
    return "Hello, " + name

function calculate x y operation
    if operation = "add"
        return x + y
    else if operation = "multiply"
        return x * y
    return 0

# Call functions (no parentheses!)
set message greet "World"
set result calculate 10 5 "add"
```

### Classes (Basic Support)

```zen
class Person
    function init name age
        set self.name name
        set self.age age
    
    function greet
        return "Hi, I'm " + self.name

set alice new Person "Alice" 30
print alice.greet
```

## 📦 Built-in Functions

### I/O Operations
```zen
print "Hello World"                    # Output to console
set input readLine "Enter name: "      # Read user input
set content readFile "data.txt"        # Read file
writeFile "output.txt" content         # Write file
```

### JSON/YAML Operations
```zen
# JSON
set data jsonParse '{"key": "value"}'
set json jsonStringify data

# YAML
set config yamlParse "key: value\nlist:\n  - item1"
set yaml yamlStringify config
```

### String Manipulation
```zen
set upper toUpperCase "hello"          # HELLO
set lower toLowerCase "WORLD"          # world
set parts split "a,b,c" ","           # ["a", "b", "c"]
set joined join parts "-"              # "a-b-c"
set trimmed trim "  space  "          # "space"
set replaced replace "hello" "l" "w"   # "hewwo"
```

### Type Conversion
```zen
set num toNumber "42"                  # String to number
set str toString 42                    # Number to string
set int toInteger 3.14                 # Float to integer
```

### Math Functions
```zen
set r random                           # Random 0-1
set rounded round 3.14159 2           # 3.14
set minimum min 10 5                   # 5
set maximum max 10 5                   # 10
set absolute abs -42                   # 42
```

## ✅ Working Features

### Core Language
- ✅ Variables with dynamic typing
- ✅ All primitive types (null, boolean, number, string)
- ✅ Arrays with indexing
- ✅ Objects with property access
- ✅ Deep object nesting (5+ levels)
- ✅ Arithmetic operators (+, -, *, /, %)
- ✅ Comparison operators (=, !=, <, >, <=, >=)
- ✅ Logical operators (and, or, not)
- ✅ String concatenation
- ✅ If/else conditionals
- ✅ While loops
- ✅ For-in loops (arrays)
- ✅ Functions with return values
- ✅ Basic classes with methods
- ✅ Comments (# single line)

### File Operations (NEW!)
- ✅ GET keyword with streaming for large files
- ✅ PUT keyword with deep nesting support
- ✅ Property path access (file.path.to.property)
- ✅ JSON/YAML auto-detection
- ✅ Memory-mapped files (<100MB)
- ✅ Buffered streaming (>100MB)
- ✅ GB-scale file handling

### Standard Library
- ✅ I/O functions (print, readLine, readFile, writeFile)
- ✅ JSON operations (parse, stringify)
- ✅ YAML operations (parse, stringify)
- ✅ String operations (upper, lower, split, join, trim, replace)
- ✅ Type conversions (toNumber, toString, toInteger)
- ✅ Math functions (random, round, min, max, abs)
- ✅ Array operations (length, push, pop)
- ✅ Object operations (keys, values)

## ⚠️ Known Limitations

- **Always use `timeout`** - Some operations may hang without it
- For loops have limited functionality
- No module/import system yet
- No try/catch error handling
- No async/await
- Method chaining limited to property access

## 🛠️ Development

### Build and Test

```bash
# Build
make clean && make

# Run tests
make test                    # All tests
make test-unit              # Unit tests only
make test-valgrind          # Memory leak check

# Code quality
make format                 # Auto-format code
make lint                   # Static analysis
make enforce                # Check manifest compliance
```

### Debug Options

```bash
./zen --debug file.zen      # General debug output
./zen --debug-lexer file.zen   # Show tokens
./zen --debug-ast file.zen     # Show AST tree
./zen --debug-visitor file.zen # Show evaluation
```

### Project Structure

```
zen/
├── src/
│   ├── core/           # Lexer, parser, AST, visitor
│   ├── types/          # Value system
│   ├── runtime/        # Operators, execution
│   └── stdlib/         # Built-in functions
├── tests/              # Comprehensive test suite
├── docs/               # Documentation
│   └── ZEN.md         # Language specification
├── examples/           # Example scripts
├── MANIFEST.json       # Function specifications (enforced)
└── CLAUDE.md          # AI assistant guidelines
```

## 📊 Performance

- **Memory efficient**: Reference counting with leak detection
- **Large file support**: Streaming for GET/PUT operations
- **Optimized parsing**: Single-pass recursive descent
- **Fast execution**: Direct AST evaluation

## 🤝 Contributing

We welcome contributions! Please:

1. Read `CONTRIBUTING.md` for guidelines
2. Follow `MANIFEST.json` specifications exactly
3. Run `make enforce` before committing
4. Add tests for new features
5. Fix root causes, not symptoms

## 📚 Examples

### Simple Script
```zen
# hello.zen
set name readLine "What's your name? "
print "Hello, " + name + "!"

set age toNumber readLine "How old are you? "
if age >= 18
    print "You can vote!"
```

### Working with Files
```zen
# config.zen
# Read configuration
set config get "settings.json"
print "Current port: " + config.server.port

# Update configuration
put "settings.json".server.port 8080
put "settings.json".debug true

# Verify changes
set updated get "settings.json"
print "New port: " + updated.server.port
```

### Data Processing
```zen
# process.zen
# Load large dataset (streaming!)
set data get "bigdata.json"

# Process records
for record in data.records
    if record.status = "active"
        print "Processing: " + record.name
        # Update record
        put "results.json".processed record.id true
```

## 📄 License

MIT License - See [LICENSE](LICENSE)

## 👥 Credits

Created by **Nguyen Ky Son**

Special thanks to all contributors and the open-source community.

---

**Current Version**: 0.2.0 (with GET/PUT streaming support)

For more information, see the [full language specification](docs/ZEN.md).