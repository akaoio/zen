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

## Contributing

This codebase is prepared as a foundation for further development. Key areas for contribution:
- Number and boolean types
- Arithmetic operators
- Control flow structures (if/else, loops)
- Arrays and objects
- File I/O operations

## License

See LICENSE file for details.

---

*Created by Nguyen Ky Son - Aug 1st 2025*
