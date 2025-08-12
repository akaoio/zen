# ZEN Programming Language

⚠️ **WARNING: This is an experimental, incomplete implementation. Most features don't work. See [ACTUAL_STATUS.md](ACTUAL_STATUS.md) for details.**

## Overview

ZEN is an experimental programming language concept that aims to provide natural syntax with minimal punctuation. However, the current implementation is severely limited and unstable.

### Key Syntax Difference
- **`=` is for comparison** (not assignment) - use `set` for assignment
- Example: `if x = 5` checks if x equals 5, while `set x 5` assigns 5 to x

### Current State

- **Partially implemented** - Only basic features work
- **Many crashes and bugs** - Arrays, objects, and loops are broken
- **Not suitable for any real use** - This is a proof-of-concept at best
- **Build may be broken** - Check workspace subdirectories for working binaries

## What Actually Works

Very limited functionality:
- Basic print statements
- Simple variable assignment (single values only)
- Basic arithmetic and string concatenation
- Simple if statements
- Basic function definitions

## What Doesn't Work

Most language features are broken or missing:
- ❌ **Arrays** - Syntax `1, 2, 3` returns empty object
- ❌ **Objects** - Syntax `name "x", age 30` returns null
- ❌ **Loops** - For loops hang, while loops have infinite loop bugs
- ❌ **Error handling** - No try/catch
- ❌ **File I/O** - Not implemented
- ❌ **Imports/Modules** - Not implemented
- ❌ **Most built-in functions** - Missing

See [ACTUAL_STATUS.md](ACTUAL_STATUS.md) for a complete list of issues.

## Quick Test

If you want to see what little works:

```bash
# Find a working binary (main build is broken)
find . -name "zen" -type f -executable

# Test basic functionality
echo 'print "Hello World"' | ./path/to/zen

# Simple arithmetic
echo 'print 2 + 3' | ./path/to/zen
```

## Example of What Works

```zen
# This works
print "Hello World"
set x 42        # Assignment uses 'set'
print x

# Comparison uses '='
if x = 42
    print "x is 42"

# This also works
function greet name
    print "Hello " + name
    
greet "World"
```

## Examples of What DOESN'T Work

```zen
# Arrays - BROKEN
set arr 1, 2, 3  # Returns {} instead of array

# Objects - BROKEN  
set person name "Bob", age 25  # Returns null

# Loops - BROKEN (will hang or infinite loop)
for i in 1..5
    print i
```

## Development Status

The current implementation is incomplete with many features still in development. This is an ongoing experimental project exploring natural syntax programming language design.

## Building

The main build is currently broken. You might find working binaries in workspace subdirectories:

```bash
# Main build (currently broken)
make

# Look for working binaries
find . -name "zen" -type f -executable
```

## Contributing

This project needs major work to become functional:

1. Fix array and object parsing
2. Fix loop implementation  
3. Add error handling
4. Implement missing built-in functions
5. Fix memory management issues
6. Add real tests (current tests all fail)

## Documentation

- `docs/idea.md` - The aspirational language specification (mostly not implemented)
- `ACTUAL_STATUS.md` - Honest assessment of what actually works
- `MANIFEST.json` - File and function definitions (many are stubs/broken)

## License

MIT License - See [LICENSE](LICENSE) for details.

## Author

Created by Nguyen Ky Son

---

**⚠️ IMPORTANT: This is not a working programming language. It's an incomplete experiment with serious implementation issues. Do not use for any real purpose.**