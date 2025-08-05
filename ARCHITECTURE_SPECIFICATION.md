# ZEN Language Implementation Specification
## For swarm-4 Workers - Current Stub Implementations

**Created by:** swarm-4-zen-architect  
**Date:** 2025-08-05  
**Status:** MANIFEST.json violations resolved, ready for implementation  
**Purpose:** Provide technical specifications for completing stub implementations

---

## Current Status

### ✅ RESOLVED - Type System Consistency
- MANIFEST.json updated to match current codebase
- Uses consistent `lexer_T`, `parser_T`, `AST_T`, `token_T` types
- Core lexer/parser infrastructure functional

### ⚠️ REMAINING VIOLATIONS
1. **Missing doxygen documentation** - Multiple header functions need docs
2. **Stub implementations** - All types/runtime/stdlib functions are empty stubs
3. **Unauthorized functions** - Some core functions not in MANIFEST (acceptable)

### 🎯 IMPLEMENTATION TARGET
The core ZEN language infrastructure exists. Workers need to implement:
- Complete Value type system with reference counting
- Runtime operators (arithmetic, logical, comparison)  
- Standard library (JSON, I/O operations)
- All currently return empty stubs

---

## CRITICAL: Missing Value Header File

The `src/include/zen/types/value.h` header is **REQUIRED** by MANIFEST.json but **MISSING** from the codebase. This is the first priority for swarm-4 workers.

## IMMEDIATE IMPLEMENTATION PRIORITIES

### Phase 1: Value Type System Foundation (CRITICAL)
1. **Create `src/include/zen/types/value.h`** - Define Value struct and ValueType enum
2. **Implement `src/types/value.c`** - Complete all stub functions with reference counting
3. **Implement `src/types/array.c`** - Dynamic array implementation
4. **Implement `src/types/object.c`** - Hash table implementation

### Phase 2: Runtime Operations (HIGH PRIORITY)  
1. **Complete `src/runtime/operators.c`** - All arithmetic, logical, comparison operators
2. **Integrate with Value system** - Type-safe operations returning proper Values

### Phase 3: Standard Library (MEDIUM PRIORITY)
1. **Complete `src/stdlib/json.c`** - JSON parsing/generation with cJSON
2. **Complete `src/stdlib/io.c`** - File I/O operations

---

## TECHNICAL SPECIFICATIONS FOR WORKERS

### 1. Value System Header - `src/include/zen/types/value.h`

**Status:** MISSING - Must be created first  
**Worker Assignment:** `swarm-4-zen-worker-types-1`

Based on MANIFEST.json specification, create header with:

```c
#ifndef ZEN_TYPES_VALUE_H
#define ZEN_TYPES_VALUE_H

#include <stddef.h>
#include <stdbool.h>

// ValueType enum as defined in MANIFEST
typedef enum {
    VALUE_NULL,
    VALUE_BOOLEAN, 
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_ARRAY,
    VALUE_OBJECT,
    VALUE_FUNCTION,
    VALUE_ERROR
} ValueType;

// Value struct with reference counting
typedef struct Value {
    ValueType type;
    size_t ref_count;
    union {
        bool boolean_val;
        double number_val;
        char* string_val;
        struct {
            struct Value** items;
            size_t length;
            size_t capacity;
        } array_val;
        struct {
            void* hash_table;  // Implementation detail
        } object_val;
    } data;
} Value;

// Exact function signatures from MANIFEST.json
Value* value_new(ValueType type);
Value* value_new_string(const char* str);
Value* value_new_number(double num);
Value* value_new_boolean(bool val);
Value* value_new_null(void);
Value* value_copy(const Value* value);
void value_free(Value* value);
char* value_to_string(const Value* value);
bool value_equals(const Value* a, const Value* b);
const char* value_type_name(ValueType type);
Value* value_ref(Value* value);
void value_unref(Value* value);

#endif
```

### 2. Value Implementation - `src/types/value.c`

**Status:** STUB - Replace stub functions  
**Worker Assignment:** `swarm-4-zen-worker-types-2`

Current file has correct signatures but empty bodies. Implementation requirements:

**Reference Counting Pattern:**
```c
Value* value_new(ValueType type) {
    Value* v = malloc(sizeof(Value));
    if (!v) return NULL;
    v->type = type;
    v->ref_count = 1;  // Start with 1 reference
    // Initialize data union based on type
    return v;
}

Value* value_ref(Value* value) {
    if (!value) return NULL;
    value->ref_count++;
    return value;  // Return same pointer
}

void value_unref(Value* value) {
    if (!value) return;
    if (--value->ref_count == 0) {
        // Free contained data based on type
        value_free(value);
    }
}
```

**Memory Ownership:**
- String values: `strdup()` input, `free()` on cleanup
- Array/Object values: Reference count contained Values
- All constructors return ref_count = 1

### 3. Array Implementation - `src/types/array.c`  

**Status:** STUB - Complete dynamic array implementation  
**Worker Assignment:** `swarm-4-zen-worker-types-3`

Requirements for stub replacement:
- Dynamic resizing (double when full, halve when 25% full)
- Reference counting for stored Values
- Bounds checking (return NULL for invalid access)
- Integration with Value system

Example pattern:
```c
Value* array_new(size_t initial_capacity) {
    Value* array = value_new(VALUE_ARRAY);
    array->data.array_val.items = malloc(sizeof(Value*) * initial_capacity);
    array->data.array_val.capacity = initial_capacity;
    array->data.array_val.length = 0;
    return array;
}
```

### 4. Object Implementation - `src/types/object.c`

**Status:** STUB - Implement hash table for key-value storage  
**Worker Assignment:** `swarm-4-zen-worker-types-4`

Hash table requirements:
- Chained collision resolution
- String keys (djb2 hash algorithm)
- Load factor management (rehash at 75%)
- Reference counting for Values

### 5. Runtime Operators - `src/runtime/operators.c`

**Status:** STUB - Implement all 12 operator functions  
**Worker Assignment:** `swarm-4-zen-worker-runtime-1`

Current stubs need full implementation with type checking:

```c
Value* op_add(Value* a, Value* b) {
    if (!a || !b) return NULL;
    
    if (a->type == VALUE_NUMBER && b->type == VALUE_NUMBER) {
        return value_new_number(a->data.number_val + b->data.number_val);
    }
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        // String concatenation
        char* str_a = value_to_string(a);
        char* str_b = value_to_string(b);
        size_t len = strlen(str_a) + strlen(str_b) + 1;
        char* result = malloc(len);
        strcpy(result, str_a);
        strcat(result, str_b);
        Value* v = value_new_string(result);
        free(result);
        free(str_a);
        free(str_b);
        return v;
    }
    return NULL; // Type error
}
```

**Operator Behavior:**
- `op_add`: Number addition, string concatenation, array merging
- `op_subtract`, `op_multiply`, `op_divide`, `op_modulo`: Numeric only
- `op_equals`, `op_not_equals`: Deep equality for all types
- `op_less_than`, `op_greater_than`: Numeric and string comparison
- `op_logical_and`, `op_logical_or`, `op_logical_not`: Boolean logic

### 6. JSON Integration - `src/stdlib/json.c`

**Status:** STUB - Implement cJSON integration  
**Worker Assignment:** `swarm-4-zen-worker-stdlib-1`

Replace stubs with cJSON library integration:
- Link with `-lcjson`
- Convert between Value and cJSON objects
- Handle parsing errors gracefully
- Support all Value types in JSON conversion

### 7. File I/O - `src/stdlib/io.c`

**Status:** PARTIAL - Complete function rename and implementation  
**Worker Assignment:** `swarm-4-zen-worker-stdlib-2`

Current `get_file_contents()` needs:
1. Rename to match MANIFEST (if needed)
2. Add missing functions for writing/checking files
3. Proper error handling

---

## MANIFEST COMPLIANCE RULES FOR WORKERS

### Critical Requirements

1. **EXACT FUNCTION SIGNATURES**: Must match MANIFEST.json character-for-character
2. **CREATE MISSING HEADER**: `src/include/zen/types/value.h` is required
3. **NO NEW FILES**: Only implement in existing stub files
4. **DOXYGEN DOCUMENTATION**: Add proper @brief, @param, @return docs
5. **ERROR HANDLING**: Return NULL on allocation failures

### Before Starting Work

```bash
# 1. Check current system state
make vision

# 2. Create task file (MANDATORY)
TASK_FILE=$(node task.js create swarm-4-zen-worker-types-1 "Implement value.h header" src/include/zen/types/value.h | grep "Created task:" | cut -d' ' -f3)

# 3. Verify no conflicts
make vision | grep -E "\[swarm-.*\]" | grep -E "value\.h|value\.c|array\.c|operators\.c"

# 4. Claim your file
echo "Working on implementation..."
```

### Testing Requirements

```bash
# After implementation
make clean && make               # Must compile
make enforce                     # Must pass compliance  
valgrind --leak-check=full ./zen # Must be memory-clean
```

### Success Criteria

- ✅ `make enforce` shows no violations for your file
- ✅ All stub functions have real implementations
- ✅ Proper doxygen documentation added
- ✅ Reference counting works correctly
- ✅ Memory leaks resolved
- ✅ Integration tests pass

---

## WORKER ASSIGNMENTS

### Type System Team
- **swarm-4-zen-worker-types-1**: Create `value.h` header
- **swarm-4-zen-worker-types-2**: Implement `value.c` (reference counting)
- **swarm-4-zen-worker-types-3**: Implement `array.c` (dynamic arrays)
- **swarm-4-zen-worker-types-4**: Implement `object.c` (hash tables)

### Runtime Team  
- **swarm-4-zen-worker-runtime-1**: Complete `operators.c` (all 12 functions)
- **swarm-4-zen-worker-runtime-2**: Integration testing and optimization

### Standard Library Team
- **swarm-4-zen-worker-stdlib-1**: Complete `json.c` (cJSON integration)
- **swarm-4-zen-worker-stdlib-2**: Complete `io.c` (file operations)

**Coordination:** Use `swarm-4-zen-queen` for conflict resolution and progress tracking.

---

## REFERENCE COUNTING ARCHITECTURE

### Memory Model

**Every Value** is reference counted:
```c
Value* v = value_new_string("hello");  // ref_count = 1
Value* v2 = value_ref(v);              // ref_count = 2, same object
value_unref(v);                        // ref_count = 1
value_unref(v2);                       // ref_count = 0, freed
```

**Container Objects** own their contents:
```c
Value* array = array_new(10);
Value* item = value_new_number(42);
array_push(array, item);               // item ref_count++ 
value_unref(item);                     // Safe: array still owns it
value_unref(array);                    // Frees array and decrements item
```

**Function Returns** transfer ownership:
```c
Value* result = op_add(a, b);          // Caller owns result
// ... use result ...
value_unref(result);                   // Caller must unref
```

### Cycle Detection
- **Phase 1**: No cycle detection (acceptable)
- **Future**: Mark-and-sweep for complex object graphs

---

*Ready for swarm-4 worker implementation. Success requires completing ALL stub functions with proper reference counting, error handling, and MANIFEST compliance.*

#### Interface Contracts (MANIFEST-compliant)

```c
// All functions exactly as specified in MANIFEST.json
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);
Value* array_pop(Value* array);
Value* array_get(Value* array, size_t index);
void array_set(Value* array, size_t index, Value* item);
size_t array_length(const Value* array);
```

#### Implementation Requirements

**Dynamic Resizing:**
- Growth strategy: Double capacity when full
- Shrink strategy: Half capacity when 25% full (min 8)
- Reallocation: Use `realloc()` with NULL check

**Bounds Checking:**
- `array_get()`: Return NULL for out-of-bounds
- `array_set()`: Extend array if index > length
- `array_pop()`: Return NULL if empty

**Reference Management:**
- Push: Increment ref count of pushed item
- Pop: Return item without decrementing (caller owns)
- Set: Decrement old item, increment new item
- Free: Decrement all items, free backing array

**Error Handling:**
- Allocation failures: Return NULL or no-op
- Type checking: Ensure parameter is VALUE_ARRAY
- Memory safety: Check array_val.items != NULL

---

### Object Implementation Architecture

#### Hash Table Design

**Hash Function:**
```c
static size_t hash_string(const char* key) {
    size_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // djb2 algorithm
    }
    return hash;
}
```

**Collision Resolution:**
- Chaining with linked lists
- Load factor threshold: 0.75
- Rehashing: Double buckets, rehash all entries

**Interface Implementation:**
```c
Value* object_new(void);           // 16 initial buckets
void object_set(Value* object, const char* key, Value* value);
Value* object_get(Value* object, const char* key);
bool object_has(Value* object, const char* key);
void object_delete(Value* object, const char* key);
Value* object_keys(Value* object); // Returns VALUE_ARRAY
```

**Memory Management:**
- Keys: Strdup'd and owned by ObjectEntry
- Values: Reference counted
- Buckets: Realloc'd during rehashing
- Deletion: Properly free chains

---

### Runtime Operators Architecture

#### Operator Implementation Strategy

**Type-based Dispatch:**
```c
Value* op_add(Value* a, Value* b) {
    if (a->type == VALUE_NUMBER && b->type == VALUE_NUMBER) {
        return value_new_number(a->data.number_val + b->data.number_val);
    }
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        // String concatenation logic
    }
    // Error: incompatible types
    return NULL;
}
```

**Operator Behavior Matrix:**

| Operator | Number + Number | String + Any | Array + Array | Other |
|----------|----------------|--------------|---------------|-------|
| `+` | Add | Concatenate | Merge | Error |
| `-` | Subtract | Error | Error | Error |
| `*` | Multiply | Repeat | Error | Error |
| `/` | Divide | Error | Error | Error |
| `%` | Modulo | Error | Error | Error |

**Comparison Operators:**
- `op_equals()`: Deep equality for all types
- `op_less_than()`: Numeric comparison, lexicographic for strings
- String comparison: Use `strcmp()` for lexicographic order
- Array comparison: Length first, then element-wise
- Object comparison: Not supported (return false)

**Logical Operators:**
- `op_logical_and()`: Short-circuit evaluation
- `op_logical_or()`: Short-circuit evaluation  
- `op_logical_not()`: Truthiness: null/false/0/"" are false
- Return VALUE_BOOLEAN for all logical ops

**Error Handling:**
- Type mismatches: Return NULL (runtime will handle)
- Division by zero: Return error Value with message
- Overflow: Use double precision, no overflow handling
- Memory allocation: Return NULL on failure

---

### Standard Library Architecture

#### JSON Integration (cJSON-based)

**Dependencies:**
- Link with `-lcjson`
- Include `#include <cjson/cJSON.h>`
- Error handling for JSON parse failures

**Conversion Functions:**
```c
Value* json_parse(const char* json_string);
char* json_stringify(const Value* value);
cJSON* value_to_cjson(const Value* value);
Value* cjson_to_value(const cJSON* json);
```

**Type Mapping:**
- cJSON_String → VALUE_STRING
- cJSON_Number → VALUE_NUMBER
- cJSON_True/False → VALUE_BOOLEAN
- cJSON_NULL → VALUE_NULL
- cJSON_Array → VALUE_ARRAY
- cJSON_Object → VALUE_OBJECT

**Error Handling:**
- `json_parse()`: Return NULL for invalid JSON
- `json_stringify()`: Return NULL for unstringifiable types
- Memory: Free cJSON objects after conversion
- Encoding: Assume UTF-8 strings

#### File I/O Architecture

**File Operations:**
```c
char* io_read_file(const char* path);
bool io_write_file(const char* path, const char* content);
bool io_file_exists(const char* path);
```

**Implementation Requirements:**
- `io_read_file()`: Read entire file to heap-allocated string
- `io_write_file()`: Write string to file, create directories if needed
- `io_file_exists()`: Use `access()` or `stat()`
- Error handling: Return NULL/false on errors
- Memory: Caller owns returned strings

**Security Considerations:**
- Path traversal: No validation (simple implementation)
- File permissions: Respect system permissions
- Binary files: Handle as binary data (not text)
- Large files: No size limits (system-dependent)

---

## HEADER FILE SPECIFICATIONS

### Required Missing Headers

#### `src/include/zen/core/parser.h`
```c
#ifndef ZEN_CORE_PARSER_H
#define ZEN_CORE_PARSER_H

#include "zen/core/lexer.h"
#include "zen/core/ast.h"

typedef struct Parser {
    Lexer* lexer;
    Token* current_token;
} Parser;

// Exact signatures from MANIFEST.json
Parser* parser_new(Lexer* lexer);
void parser_free(Parser* parser);
AST_Node* parser_parse(Parser* parser);
AST_Node* parser_parse_statement(Parser* parser);
AST_Node* parser_parse_expression(Parser* parser);
AST_Node* parser_parse_function_def(Parser* parser);
AST_Node* parser_parse_if_statement(Parser* parser);

#endif
```

#### `src/include/zen/core/ast.h`
```c
#ifndef ZEN_CORE_AST_H
#define ZEN_CORE_AST_H

#include <stddef.h>

typedef enum {
    AST_PROGRAM,
    AST_VARIABLE_DEF,
    AST_FUNCTION_DEF,
    AST_FUNCTION_CALL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_IF_STATEMENT,
    AST_WHILE_LOOP,
    AST_FOR_LOOP,
    AST_RETURN_STATEMENT,
    AST_BLOCK
} AST_NodeType;

typedef struct AST_Node {
    AST_NodeType type;
    union {
        // Node-specific data
        struct {
            char* name;
            struct AST_Node* value;
        } variable_def;
        
        struct {
            char* name;
            char** parameters;
            size_t param_count;
            struct AST_Node* body;
        } function_def;
        
        struct {
            char* name;
            struct AST_Node** arguments;
            size_t arg_count;
        } function_call;
        
        struct {
            int operator;
            struct AST_Node* left;
            struct AST_Node* right;
        } binary_op;
        
        struct {
            int operator;
            struct AST_Node* operand;
        } unary_op;
        
        struct {
            char* string_value;
            double number_value;
            int boolean_value;
            int value_type; // 0=string, 1=number, 2=boolean, 3=null
        } literal;
        
        struct {
            char* name;
        } identifier;
        
        struct {
            struct AST_Node* condition;
            struct AST_Node* then_branch;
            struct AST_Node* else_branch;
        } if_statement;
        
        struct {
            struct AST_Node** statements;
            size_t statement_count;
        } block;
    } data;
    
    size_t line;
    size_t column;
} AST_Node;

// Exact signatures from MANIFEST.json
AST_Node* ast_node_new(AST_NodeType type);
void ast_node_free(AST_Node* node);
void ast_node_add_child(AST_Node* parent, AST_Node* child);

#endif
```

---

## IMPLEMENTATION PRIORITY FOR SWARM-4 WORKERS

### Phase 1: Core Type System (CRITICAL)
1. **swarm-4-zen-worker-types-1**: Implement `src/include/zen/types/value.h`
2. **swarm-4-zen-worker-types-2**: Implement `src/types/value.c` (reference counting)
3. **swarm-4-zen-worker-types-3**: Implement `src/types/array.c` (dynamic arrays)

### Phase 2: Runtime Operations (HIGH)
1. **swarm-4-zen-worker-runtime-1**: Implement `src/runtime/operators.c` (arithmetic)
2. **swarm-4-zen-worker-runtime-2**: Complete all logical/comparison operators
3. **swarm-4-zen-worker-types-4**: Implement `src/types/object.c` (hash tables)

### Phase 3: Standard Library (MEDIUM)
1. **swarm-4-zen-worker-stdlib-1**: Implement `src/stdlib/json.c` (cJSON integration)
2. **swarm-4-zen-worker-stdlib-2**: Implement `src/stdlib/io.c` (file operations)

### Phase 4: Testing & Integration (LOW)
1. **All workers**: Add comprehensive unit tests
2. **All workers**: Memory leak testing with Valgrind
3. **All workers**: Integration testing with existing core

---

## MANIFEST COMPLIANCE REQUIREMENTS

### Critical Rules for Workers

1. **EXACT SIGNATURES**: Function signatures must match MANIFEST.json character-for-character
2. **NO UNAUTHORIZED FUNCTIONS**: Only implement functions listed in MANIFEST
3. **NO UNAUTHORIZED FILES**: Do not create files not in MANIFEST
4. **DOXYGEN REQUIRED**: Every function needs proper doxygen documentation
5. **ERROR HANDLING**: All functions must handle allocation failures gracefully

### Before Implementation Checklist

- [ ] Read MANIFEST.json section for your assigned file
- [ ] Copy function signatures exactly from MANIFEST
- [ ] Understand reference counting requirements
- [ ] Plan error handling strategy
- [ ] Design unit tests
- [ ] Run `make enforce` to verify compliance

### After Implementation Checklist

- [ ] All functions implemented per MANIFEST signatures
- [ ] All functions have doxygen documentation
- [ ] Memory leaks checked with Valgrind
- [ ] Unit tests pass
- [ ] `make enforce` passes with no violations
- [ ] Integration tests with existing core work

---

## MEMORY MANAGEMENT STRATEGY

### Reference Counting Protocol

**Creation:** All new Values start with ref_count = 1
```c
Value* value_new_string(const char* str) {
    Value* v = malloc(sizeof(Value));
    v->ref_count = 1;  // Initial reference
    // ... initialize data
    return v;
}
```

**Assignment:**
```c
// When assigning a Value to a variable/field
old_value = container->field;
container->field = value_ref(new_value);  // Increment ref
value_unref(old_value);                   // Decrement old
```

**Parameter Passing:**
- Functions receive Values without changing ref count
- Functions returning Values transfer ownership to caller
- Functions that store Values must increment ref count

**Cleanup:**
```c
void value_unref(Value* value) {
    if (!value) return;
    if (--value->ref_count == 0) {
        // Free contained data based on type
        if (value->type == VALUE_STRING) {
            free(value->data.string_val);
        } else if (value->type == VALUE_ARRAY) {
            for (size_t i = 0; i < value->data.array_val.length; i++) {
                value_unref(value->data.array_val.items[i]);
            }
            free(value->data.array_val.items);
        }
        // ... handle other types
        free(value);
    }
}
```

### Cycle Detection

**Current Strategy:** No cycle detection (acceptable for initial implementation)
**Future Strategy:** Mark-and-sweep GC for cycles in objects/arrays

---

## TESTING REQUIREMENTS

### Unit Test Structure

Each implementation file needs corresponding test:
- `tests/unit/value_test.c`
- `tests/unit/array_test.c`
- `tests/unit/object_test.c`
- `tests/unit/operators_test.c`
- `tests/unit/json_test.c`
- `tests/unit/io_test.c`

### Test Categories

**Functionality Tests:**
- All normal operations work correctly
- Edge cases handled properly
- Type conversions work as expected

**Memory Tests:**
- No memory leaks (Valgrind clean)
- Reference counting works correctly
- Allocation failures handled gracefully

**Integration Tests:**
- Works with existing lexer/parser
- JSON roundtrip conversions
- File I/O with real files

### Valgrind Requirements

All implementations must be Valgrind-clean:
```bash
valgrind --leak-check=full --error-exitcode=1 ./test_program
```

---

## SUMMARY FOR SWARM-4 WORKERS

This specification provides everything needed to implement MANIFEST-compliant components:

1. **Clear interfaces** with exact function signatures
2. **Implementation strategies** for complex features like reference counting
3. **Memory management** protocols to prevent leaks
4. **Error handling** patterns for robustness
5. **Testing requirements** to ensure quality
6. **Integration guidelines** to work with existing code

**Success Criteria:**
- `make enforce` passes with zero violations
- All unit tests pass
- No memory leaks in Valgrind
- Integration with existing core works
- Performance acceptable for mobile devices

**Next Steps:**
1. Workers should claim specific files/functions
2. Implement following this specification exactly
3. Test thoroughly before marking complete
4. Coordinate through swarm-4-zen-queen for conflicts

---

*Architecture specification complete. Ready for swarm-4 worker implementation.*