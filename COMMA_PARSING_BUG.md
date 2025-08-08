# Comma Parsing Infinite Loop Bug - GitHub Issue

## Summary
**Critical Bug**: The ZEN parser enters an infinite loop when parsing comma-separated expressions, specifically in multi-key object creation syntax like `set x name, age`.

## Bug Details

### Affected Code
- **File**: `src/core/parser.c`
- **Functions**: Expression parsing chain, object literal detection, lexer peek functionality
- **Severity**: High - causes complete program hang

### Reproduction Steps
1. Create a ZEN file with comma-separated identifiers: 
   ```zen
   set x name, age
   ```
2. Run the ZEN interpreter: `./zen test.zen`
3. **Result**: Program hangs in infinite loop (no timeout)

### Expected Behavior
The parser should handle comma-separated expressions and create multi-key objects like:
```zen
set person name "Alice", age 30, active true
```

## Debugging Analysis Completed

### ✅ Root Cause Investigation
1. **Lexer Functions**: Fixed `lexer_peek_token` infinite loop bug
   - **Issue**: Incorrect loop bounds and incomplete state restoration
   - **Fix**: Corrected loop iteration and added missing line/column number restoration
   - **Status**: ✅ FIXED - `lexer_peek_token` works correctly in isolation

2. **Object Detection Logic**: Enhanced `parser_peek_for_object_literal` 
   - **Issue**: Didn't recognize `ID, ID` pattern as object literal
   - **Fix**: Added `TOKEN_COMMA` case to object literal detection
   - **Status**: ✅ IMPLEMENTED

3. **Object Parsing**: Added comma termination handling in `parser_parse_object`
   - **Issue**: Comma wasn't treated as expression terminator
   - **Fix**: Added comma handling with proper key-value pair creation
   - **Status**: ✅ IMPLEMENTED

### 🔍 Current Status
- **Single-key objects work**: `set x name "Alice"` ✅
- **Simple expressions work**: `print "test"` ✅  
- **Comma expressions fail**: `set x name, age` ❌ (infinite loop)

### 🔧 Technical Details

#### Parser Chain Analysis
```
parser_parse_variable_definition
  ├── parser_parse_expr
  │   └── parser_parse_ternary_expr
  │       └── parser_parse_null_coalescing_expr  
  │           └── parser_parse_binary_expr
  │               └── parser_parse_unary_expr
  │                   └── parser_parse_primary_expr
  │                       └── parser_parse_id_or_object
  │                           ├── parser_peek_for_object_literal ← INFINITE LOOP HERE
  │                           └── parser_parse_object
```

#### Key Findings
- Infinite loop occurs **before** `parser_parse_object` is called
- Issue is in `parser_peek_for_object_literal` function
- Even with extensive debug output, no prints appear (loop is very early)
- `lexer_peek_token` works correctly when tested in isolation

## Files Modified During Investigation

### Core Fixes Applied
1. **lexer.c**: Fixed `lexer_peek_token` state restoration
2. **parser.c**: Enhanced object literal detection and parsing
3. **Test files**: Created comprehensive test cases

### Debug Infrastructure Added
- Extensive debug logging in parser functions
- Isolation tests for lexer peek functionality
- Step-by-step parser tracing

## Test Cases

### ✅ Working Cases
```zen
print "test"                    # ✅ Works
set x name "Alice"             # ✅ Works  
set x 5                        # ✅ Works
```

### ❌ Failing Cases  
```zen
set x name, age                # ❌ Infinite loop
set x name "Alice", age 30     # ❌ Infinite loop
```

## Impact Assessment
- **Severity**: High - Complete program hang
- **Scope**: Multi-key object creation (core ZEN feature)
- **Workaround**: Use single-key objects only
- **User Experience**: Requires process termination (timeout/kill)

## Next Steps Required

### Immediate Actions
1. **Deep lexer state debugging**: Add printf debugging inside `lexer_get_next_token`
2. **Memory corruption check**: Verify lexer state isn't being corrupted
3. **Alternative approach**: Consider parsing comma as binary operator temporarily
4. **Valgrind analysis**: Check for memory issues causing infinite loops

### Implementation Strategy
1. **Short-term**: Disable comma expressions to prevent hangs
2. **Long-term**: Implement proper comma expression parsing
3. **Testing**: Add comprehensive comma parsing test suite

## Workaround for Users
Until fixed, avoid comma-separated expressions:

```zen
# Instead of:
set person name "Alice", age 30

# Use:
set person name "Alice"
set person age 30
```

## Related Code Files
- `src/core/parser.c` (lines 565-572, 741-815, 1093-1155)
- `src/core/lexer.c` (lines 758-807)
- `src/include/zen/core/lexer.h` (line 244)
- `src/include/zen/core/parser.h`

---

**Priority**: High
**Assignee**: Core Parser Team  
**Labels**: bug, parser, infinite-loop, comma-parsing
**Milestone**: Parser Stability Release