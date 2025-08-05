# ZEN Language Architecture

## Complete Codebase Structure

```
zen/
├── src/                              # Core interpreter implementation
│   ├── core/                         # Core language runtime
│   │   ├── lexer.c                   # Tokenization with indent/dedent
│   │   ├── parser.c                  # Recursive descent parser
│   │   ├── ast.c                     # AST node definitions
│   │   ├── visitor.c                 # AST visitor/evaluator
│   │   ├── scope.c                   # Variable scope management
│   │   ├── error.c                   # Error handling and reporting
│   │   └── memory.c                  # Memory management/GC
│   │
│   ├── types/                        # Type system
│   │   ├── value.c                   # Value representation
│   │   ├── string.c                  # String operations
│   │   ├── number.c                  # Number operations
│   │   ├── boolean.c                 # Boolean operations
│   │   ├── array.c                   # Array implementation
│   │   ├── object.c                  # Object/hash implementation
│   │   ├── function.c                # Function values
│   │   └── null.c                    # Null type
│   │
│   ├── runtime/                      # Runtime features
│   │   ├── operators.c               # Operator implementations
│   │   ├── control_flow.c            # if/else, loops
│   │   ├── exceptions.c              # try/catch/throw
│   │   ├── classes.c                 # OOP support
│   │   ├── modules.c                 # Import/export system
│   │   ├── async.c                   # Async/await support
│   │   ├── generators.c              # Generator functions
│   │   └── logic.c                   # Formal logic system
│   │
│   ├── stdlib/                       # Standard library
│   │   ├── io.c                      # File I/O operations
│   │   ├── json.c                    # JSON parse/stringify
│   │   ├── yaml.c                    # YAML parse/stringify
│   │   ├── regex.c                   # Regular expressions
│   │   ├── date.c                    # Date/time operations
│   │   ├── math.c                    # Math functions
│   │   ├── string_lib.c              # String methods
│   │   ├── array_lib.c               # Array methods
│   │   ├── http.c                    # HTTP client
│   │   └── database.c                # get/put operations
│   │
│   ├── include/                      # Header files
│   │   ├── core/                     # Core headers
│   │   ├── types/                    # Type headers
│   │   ├── runtime/                  # Runtime headers
│   │   └── stdlib/                   # Stdlib headers
│   │
│   └── main.c                        # Entry point, REPL
│
├── lib/                              # External dependencies
│   ├── libyaml/                      # YAML parsing
│   ├── pcre2/                        # Regex engine
│   ├── libcurl/                      # HTTP client
│   └── sqlite3/                      # Optional: persistent storage
│
├── tools/                            # Development tools
│   ├── zenfmt/                       # Code formatter
│   │   ├── formatter.c               # Format engine
│   │   └── main.c                    # CLI interface
│   │
│   ├── zenlint/                      # Linter
│   │   ├── rules.c                   # Lint rules
│   │   └── main.c                    # CLI interface
│   │
│   └── zendoc/                       # Documentation generator
│       ├── parser.c                  # Doc parser
│       └── generator.c               # Doc generator
│
├── tests/                            # Test suite
│   ├── unit/                         # Unit tests
│   │   ├── lexer_test.c              # Lexer tests
│   │   ├── parser_test.c             # Parser tests
│   │   ├── types_test.c              # Type tests
│   │   └── runtime_test.c            # Runtime tests
│   │
│   ├── integration/                  # Integration tests
│   │   ├── features/                 # Feature tests
│   │   └── stdlib/                   # Stdlib tests
│   │
│   ├── fixtures/                     # Test fixtures
│   │   └── *.zen                     # Test scripts
│   │
│   └── framework/                    # Test framework
│       ├── assert.c                  # Assertions
│       └── runner.c                  # Test runner
│
├── examples/                         # Example programs
│   ├── hello.zen                     # Basic examples
│   ├── oop.zen                       # OOP examples
│   ├── async.zen                     # Async examples
│   ├── logic.zen                     # Formal logic
│   └── web.zen                       # Web examples
│
├── docs/                             # Documentation
│   ├── idea.md                       # Language specification
│   ├── api/                          # API documentation
│   ├── guides/                       # User guides
│   └── internals/                    # Implementation docs
│
├── scripts/                          # Build scripts
│   ├── setup.sh                      # Setup dependencies
│   ├── test.sh                       # Run tests
│   └── release.sh                    # Release process
│
├── .github/                          # GitHub workflows
│   ├── workflows/
│   │   ├── ci.yml                    # CI/CD pipeline
│   │   └── release.yml               # Release automation
│   └── CODEOWNERS                    # Code ownership
│
├── CMakeLists.txt                    # CMake build
├── Makefile                          # Make build
├── CLAUDE.md                         # Claude guidance
├── ARCHITECTURE.md                   # This file
├── README.md                         # User documentation
├── LICENSE                           # MIT License
└── .clang-format                     # Code style
```

## Core Components

### 1. Lexer (src/core/lexer.c)
- Token types: ID, STRING, NUMBER, BOOLEAN, NULL, NEWLINE, INDENT, DEDENT
- Operators: =, !=, !==, <=, >=, +=, -=, *=, /=, &, |, !, +, -, *, /, %, ?, :
- Keywords: set, function, return, if, elif, else, then, while, for, in, loop, times, break, continue, class, extends, constructor, super, private, public, protected, try, catch, throw, async, await, yield, import, export, from, as, all, exists, prove, axiom, theorem, true, false, null

### 2. Parser (src/core/parser.c)
- Recursive descent with operator precedence
- Indentation-based block parsing
- Expression parser for complex expressions
- Statement parser for all statement types

### 3. Type System (src/types/)
- Value struct with type tag and union data
- Reference counting for memory management
- Type coercion rules
- Method dispatch tables

### 4. Runtime (src/runtime/)
- Stack-based execution model
- Call frames with local scopes
- Exception handling with stack unwinding
- Class inheritance and method resolution
- Module loading and caching
- Coroutine support for async/generators

### 5. Standard Library (src/stdlib/)
- Built-in functions and objects
- File I/O with text/binary modes
- JSON/YAML serialization
- HTTP client with async support
- Database-like get/put operations

## External Dependencies

```c
// Required external libraries:
1. cJSON       - JSON parsing/generation (lightweight, single-file)
2. libyaml     - YAML parsing/generation
3. pcre2       - Regular expression support
4. libcurl     - HTTP client functionality
5. sqlite3     - Optional: persistent storage backend

// Development dependencies:
6. cmocka      - Unit testing framework
7. valgrind    - Memory debugging
8. gcov/lcov   - Code coverage
```

## Key Functions by Module

### Core Functions
```c
// lexer.c
Token* lexer_scan(Lexer* lexer);
void lexer_handle_indentation(Lexer* lexer);

// parser.c
AST_Node* parser_parse(Parser* parser);
AST_Node* parser_expression(Parser* parser);
AST_Node* parser_statement(Parser* parser);

// visitor.c
Value* visitor_evaluate(Visitor* visitor, AST_Node* node);
Value* visitor_call_function(Visitor* visitor, Value* func, Value** args);

// scope.c
void scope_define(Scope* scope, char* name, Value* value);
Value* scope_lookup(Scope* scope, char* name);
```

### Type Functions
```c
// value.c
Value* value_new(ValueType type);
Value* value_copy(Value* value);
void value_free(Value* value);
bool value_equals(Value* a, Value* b);
char* value_to_string(Value* value);

// array.c
Value* array_new(size_t capacity);
void array_push(Value* array, Value* item);
Value* array_get(Value* array, size_t index);

// object.c
Value* object_new(void);
void object_set(Value* object, char* key, Value* value);
Value* object_get(Value* object, char* key);
```

### Runtime Functions
```c
// operators.c
Value* op_add(Value* a, Value* b);
Value* op_compare(Value* a, Value* b);
Value* op_logical_and(Value* a, Value* b);

// control_flow.c
Value* eval_if_statement(Visitor* v, AST_Node* node);
Value* eval_while_loop(Visitor* v, AST_Node* node);
Value* eval_for_loop(Visitor* v, AST_Node* node);

// exceptions.c
void exception_throw(Runtime* rt, Value* error);
void exception_catch(Runtime* rt, ExceptionHandler* handler);

// classes.c
Class* class_new(char* name, Class* parent);
void class_add_method(Class* cls, char* name, Function* method);
Value* class_instantiate(Class* cls, Value** args);
```

### Standard Library Functions
```c
// io.c
Value* file_read(char* path);
Value* file_write(char* path, Value* content);

// json.c (using cJSON)
Value* json_parse(char* json_string);
char* json_stringify(Value* value);
cJSON* value_to_cjson(Value* value);
Value* cjson_to_value(cJSON* json);

// yaml.c (using libyaml)
Value* yaml_parse(char* yaml_string);
char* yaml_stringify(Value* value);

// database.c
Value* db_get(char* path);
void db_put(char* path, Value* value);
```

## Build Configuration

### CMakeLists.txt structure:
```cmake
cmake_minimum_required(VERSION 3.10)
project(zen VERSION 1.0.0)

# Core library
add_library(zencore STATIC
    src/core/*.c
    src/types/*.c
    src/runtime/*.c
    src/stdlib/*.c
)

# Main executable
add_executable(zen src/main.c)
target_link_libraries(zen zencore yaml pcre2 curl sqlite3)

# Tools
add_executable(zenfmt tools/zenfmt/*.c)
add_executable(zenlint tools/zenlint/*.c)

# Tests
enable_testing()
add_executable(zen_tests tests/framework/*.c tests/unit/*.c)
add_test(NAME zen_tests COMMAND zen_tests)
```

## Enforcement Mechanism

Create `.zenproject` configuration file:
```yaml
version: 1.0
structure:
  enforce: true
  allowed_directories:
    - src/core
    - src/types
    - src/runtime
    - src/stdlib
    - tests
    - tools
    - examples
    - docs
  
  required_files:
    - CLAUDE.md
    - ARCHITECTURE.md
    - CMakeLists.txt
    - .clang-format

code_style:
  formatter: clang-format
  linter: zenlint
  
testing:
  framework: cmocka
  coverage_threshold: 80
  
ci:
  pre_commit:
    - make format
    - make lint
    - make test
```

This architectural vision provides a complete roadmap for implementing the full ZEN language specification with professional tooling and practices.