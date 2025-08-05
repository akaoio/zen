# ZEN Language Test Suite

This directory contains a comprehensive test suite that verifies 100% ZEN language functionality.

## Test Categories

### 1. Unit Tests (`unit/`)
Tests for individual components:
- **Lexer Tests** (`unit/lexer/`) - Token recognition, lexical analysis
- **Parser Tests** (`unit/parser/`) - AST generation, syntax parsing
- **Value System Tests** (`unit/value/`) - Data types, value operations
- **Operator Tests** (`unit/operators/`) - Arithmetic, logical, comparison operators
- **Memory Tests** (`unit/memory/`) - Memory management, reference counting
- **Scope Tests** (`unit/scope/`) - Variable scoping, symbol table

### 2. Integration Tests (`integration/`)
Tests for complete ZEN programs and language features:
- **Basic Programs** (`integration/basic/`) - Simple ZEN programs
- **Control Flow** (`integration/control/`) - If/while/for statements
- **Functions** (`integration/functions/`) - Function definitions and calls
- **Data Structures** (`integration/data/`) - Arrays, objects, nested structures
- **Error Handling** (`integration/errors/`) - Error conditions and recovery

### 3. Language Tests (`language/`)
Tests for all ZEN syntax constructs and semantics:
- **Variables** (`language/variables/`) - Variable declarations, assignments
- **Operators** (`language/operators/`) - All operator types and precedence
- **Types** (`language/types/`) - String, number, boolean, null, array, object
- **Control Flow** (`language/control/`) - If/elif/else, loops, break/continue
- **Functions** (`language/functions/`) - Parameter passing, return values, scope
- **Objects** (`language/objects/`) - Object creation, property access

### 4. Standard Library Tests (`stdlib/`)
Tests for all stdlib functions:
- **I/O Operations** (`stdlib/io/`) - File operations, print/input functions
- **String Functions** (`stdlib/string/`) - String manipulation, formatting
- **Math Functions** (`stdlib/math/`) - Mathematical operations and constants
- **Conversion Functions** (`stdlib/convert/`) - Type conversions
- **JSON Support** (`stdlib/json/`) - JSON parsing and generation

### 5. Memory Tests (`memory/`)
Valgrind-based memory leak and corruption testing:
- **Leak Detection** - Tests for memory leaks
- **Corruption Detection** - Tests for buffer overflows, use-after-free
- **Reference Counting** - Tests for proper reference counting

## Test Framework

The test suite uses a lightweight custom test framework (`framework/test.h`) that provides:
- Test assertions (ASSERT_EQ, ASSERT_TRUE, etc.)
- Test fixtures and setup/teardown
- Memory leak detection integration
- Automated test discovery and execution

## Running Tests

```bash
# Run all tests
make test

# Run specific test categories  
make test-unit
make test-integration
make test-language
make test-stdlib
make test-memory

# Run with Valgrind memory checking
make test-valgrind

# Generate coverage report
make test-coverage
```

## Test Structure

Each test file follows this pattern:

```c
#include "framework/test.h"
#include "zen/core/lexer.h"

TEST_SUITE(lexer_tests)

TEST(test_lexer_basic_tokens) {
    lexer_T* lexer = init_lexer("set x 42");
    
    token_T* token = lexer_get_next_token(lexer);
    ASSERT_STR_EQ(token->value, "set");
    ASSERT_EQ(token->type, TOKEN_ID);
    
    // ... more assertions
    
    // Cleanup handled by framework
}

TEST(test_lexer_string_literals) {
    // Test string tokenization
}

END_TEST_SUITE
```

## Success Criteria

All tests must pass with:
- ✅ Zero memory leaks (Valgrind clean)
- ✅ 100% feature coverage
- ✅ All language constructs working
- ✅ All stdlib functions operational
- ✅ Error conditions properly handled

## Adding New Tests

1. Choose appropriate category directory
2. Create test file following naming convention
3. Include framework/test.h
4. Define TEST_SUITE and individual TESTs
5. Add to relevant Makefile target
6. Ensure tests are self-contained and clean up resources