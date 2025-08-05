# SWARM-2 STRATEGIC COORDINATION
## Multi-Swarm ZEN Implementation Strategy

**Task File**: tasks/20250805-2325.yaml (active)
**Swarm**: swarm-2 (1 of 4 active swarms)
**Total Agents**: 32 (8 per swarm × 4 swarms)
**Current State**: Foundation Building - 11% complete
**Critical Path**: Value System → Operators → Parser → Runtime

## MULTI-SWARM ANALYSIS (from make vision)

### File Ownership Status
- `src/core/lexer.c` [swarm-1-zen-worker-types] - IN USE
- `src/core/parser.c` [swarm-1-zen-worker-types] - IN USE
- `src/core/ast.c` [swarm-1-zen-worker-types] - IN USE
- `src/include/zen/core/parser.h` [swarm-3-zen-worker-parser] - IN USE
- **AVAILABLE CRITICAL FILES**:
  - `src/types/value.c` - **HIGHEST PRIORITY**
  - `src/types/array.c` - **HIGH PRIORITY**
  - `src/types/object.c` - **HIGH PRIORITY**
  - `src/runtime/operators.c` - **HIGH PRIORITY**
  - `src/core/scope.c` - **MEDIUM PRIORITY**
  - `src/core/visitor.c` - **MEDIUM PRIORITY**
  - `src/stdlib/io.c` - **LOW PRIORITY**
  - `src/stdlib/json.c` - **LOW PRIORITY**

### Cross-Swarm Conflict Analysis
- **No immediate conflicts** for swarm-2 target files
- Other swarms focused on lexer/parser/headers
- **Opportunity**: Swarm-2 can own the entire type/runtime foundation

## SWARM-2 IMMEDIATE WORK ASSIGNMENTS

### 1. swarm-2-zen-worker-types - CRITICAL FOUNDATION
**Priority**: URGENT - Everything depends on this
**Target**: Complete value system implementation
**Files**: 
- `src/types/value.c` (12 functions to implement)
- `src/types/array.c` (6 functions to implement)  
- `src/types/object.c` (6 functions to implement)
- `src/include/zen/types/value.h` (create header)

**Key Requirements**:
- Reference counting system (value_ref/value_unref)
- All basic types: string, number, boolean, null
- Type conversion and equality
- Memory management with proper cleanup
- MUST create task file FIRST: `node task.js create swarm-2-zen-worker-types "Implement complete ZEN value type system" src/types/value.c src/types/array.c src/types/object.c`

### 2. swarm-2-zen-worker-runtime - OPERATORS FOUNDATION
**Priority**: HIGH - Needed for expressions
**Target**: Complete operator system
**Files**: `src/runtime/operators.c` (12 functions to implement)

**Key Requirements**:
- All arithmetic operators (+, -, *, /, %)
- All comparison operators (=, !=, <, >, <=, >=)
- All logical operators (&, |, !)
- Work with Value* types from types worker
- MUST create task file FIRST

### 3. swarm-2-zen-worker-parser - MANIFEST COMPLIANCE  
**Priority**: HIGH - Fix parser violations
**Target**: Complete parser implementation per updated MANIFEST
**Files**: `src/core/parser.c` (8 incomplete functions)

**Key Requirements**:
- Fix signature mismatches with MANIFEST.json
- Add proper Doxygen documentation
- Implement all parsing functions
- Coordinate with swarm-1 to avoid conflicts
- MUST create task file FIRST

### 4. swarm-2-zen-worker-memory - MEMORY MANAGEMENT
**Priority**: MEDIUM - Support for value system
**Target**: Memory utilities and leak prevention
**Files**: `src/core/memory.c` (needs functions added to manifest)

**Key Requirements**:
- Support reference counting
- Memory pool management
- Leak detection utilities
- Integration with value system
- MUST create task file FIRST

### 5. swarm-2-zen-worker-lexer - AST SYSTEM
**Priority**: MEDIUM - Complete AST implementation
**Target**: Robust AST node system
**Files**: `src/core/ast.c`, `src/core/token.c`

**Key Requirements**:
- Complete AST node creation/management
- Token system improvements
- Integration with parser
- MUST create task file FIRST

### 6. swarm-2-zen-worker-stdlib - STANDARD LIBRARY
**Priority**: LOW - After core functionality
**Target**: I/O and JSON support
**Files**: `src/stdlib/io.c`, `src/stdlib/json.c`

**Key Requirements**:
- File I/O operations
- JSON parsing with cJSON integration
- Integration with value system
- MUST create task file FIRST

## COORDINATION PROTOCOL

### Enforcement Requirements
1. **MANDATORY**: Every worker creates task file BEFORE starting
2. **CHECK**: Run `make vision` every 5-10 minutes for conflicts
3. **COMPLY**: Follow MANIFEST.json signatures exactly
4. **DOCUMENT**: Add Doxygen documentation for all functions
5. **TEST**: Verify implementations with `make && ./zen`

### Success Metrics
- ✅ All 63 functions implemented per MANIFEST.json
- ✅ Zero memory leaks (valgrind clean)
- ✅ All tests passing
- ✅ Complete ZEN programs executable
- ✅ Build succeeds without errors

### Multi-Swarm Communication
- **Report conflicts** to swarm-2-zen-queen immediately
- **Check other swarms** progress via make vision
- **Coordinate handoffs** when files become available
- **Share learnings** about manifest compliance

## TIMELINE EXPECTATIONS

**Phase 1** (Priority 1-2 workers): 2-4 hours
- Complete value system
- Complete operators  
- Parser compliance fixes

**Phase 2** (Priority 3-4 workers): 1-2 hours  
- Memory management
- AST improvements

**Phase 3** (Priority 5-6 workers): 1-2 hours
- Standard library
- Documentation completion

**Target**: 100% ZEN implementation within 6 hours of focused work

## ACTIVATION COMMAND

Each worker MUST start with:
```bash
# Create task file FIRST (mandatory)
TASK_FILE=$(node task.js create swarm-2-zen-worker-[specialization] "Brief description" file1.c file2.c | grep "Created task:" | cut -d' ' -f3)

# Check for conflicts
make vision

# Begin implementation
node task.js activity $TASK_FILE "Starting implementation"
```

---
**Queen's Directive**: Drive continuous progress. No stopping until 100% complete.
**Multi-Swarm Awareness**: 32 agents working in parallel - coordinate smartly.
**Quality Focus**: Every function includes error handling and documentation.