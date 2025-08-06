# EMERGENCY FIX: Function Execution Hanging

## Root Cause
In `src/core/visitor.c` line 1254, the function definition's body scope is being permanently overwritten:
```c
fdef->function_definition_body->scope = function_scope;
```

This corrupts the AST for subsequent function calls.

## Fix
Replace lines 1252-1258 with:
```c
// Execute function body with correct scope but don't modify original AST
AST_T* result = NULL;
if (fdef->function_definition_body) {
    // Save original scope
    scope_T* original_scope = fdef->function_definition_body->scope;
    
    // Temporarily set function scope
    fdef->function_definition_body->scope = function_scope;
    
    // Execute function body
    result = visitor_visit(visitor, fdef->function_definition_body);
    
    // Restore original scope to prevent corruption
    fdef->function_definition_body->scope = original_scope;
} else {
    result = init_ast(AST_NULL);
}
```

## Agent Assignment
- swarm-4-zen-worker-runtime: Implement this fix in visitor.c
- Priority: CRITICAL - blocks all function execution