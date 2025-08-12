# Remaining Issues in ZEN Interpreter

## 1. Memory Corruption Issues

### Issue: Double-free in test suite
- **Location**: `tests/language/variables/test_variables.c`
- **Error**: `free(): invalid pointer` when running variable tests
- **Cause**: Likely double-free of scope or AST nodes
- **Impact**: Test suite fails, potential crashes in production

## 2. Multiple Fibonacci Calls Issue

### Issue: Sequential Fibonacci calls fail
- **Symptom**: `fib(3)` returns null when called after `fib(0)`, `fib(1)`, `fib(2)`
- **Works**: `fib(3)` works correctly when called in isolation
- **Fails**: Multiple sequential calls cause later calls to return null
- **Example**:
  ```zen
  print "fib(0) = " + fib 0  # Works: 0
  print "fib(1) = " + fib 1  # Works: 1
  print "fib(2) = " + fib 2  # Works: 1
  print "fib(3) = " + fib 3  # Fails: null (should be 2)
  ```

## 3. Potential Scope Management Issues

### Issue: Scope lifetime management
- **Context**: Using `visitor->current_scope` for execution
- **Risk**: Scope might be corrupted between multiple function calls
- **Related**: The double-free issue suggests scope ownership is unclear

## 4. Test Infrastructure Issues

### Issue: Tests hanging or crashing
- Some comprehensive tests hang indefinitely
- Memory leak detection shows issues with scope cleanup
- Integration between new scope management and old test code

## Root Causes to Investigate

1. **Scope Ownership**: Who owns scopes and when should they be freed?
2. **AST Lifecycle**: AST nodes may hold references to scopes that get freed
3. **Function State**: State might not be properly isolated between function calls
4. **Memory Pool**: AST memory pool interactions with scope management

## Priority Fixes Needed

1. Fix the double-free issue in test suite
2. Resolve the sequential function call issue
3. Clarify scope ownership and lifecycle
4. Add comprehensive recursion tests to the test suite