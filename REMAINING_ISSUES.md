# Remaining Issues in ZEN Interpreter

## ✅ FIXED ISSUES

### 1. Self Property Access in Methods ✅
**Status**: FIXED
- Added AST_PROPERTY_ACCESS and AST_ASSIGNMENT cases to visitor_update_ast_scope
- Self can now be accessed in methods and properties can be set/retrieved

### 2. Property Assignment Syntax ✅
**Status**: FIXED
- Implemented `set obj.prop value` syntax
- Parser correctly identifies property assignments with peek offset fix

### 3. Built-in Print Function ✅
**Status**: FIXED
- Print function was already registered in stdlib_minimal.c
- Works correctly with multiple arguments

## ⚠️ PARTIALLY FIXED ISSUES

### 4. Method Calls Without Parentheses ⚠️
**Status**: PARTIALLY FIXED

**What Works**:
- Simple method calls in assignments work: `set result t.no_args`
- Methods that don't access self work correctly
- Auto-execution with self parameter works in most cases

**What Doesn't Work**:
- After calling a method with arguments (e.g., `t.set_name "Bob"`), subsequent zero-arg method calls in assignments return [function] instead of executing
- Parser context or state appears to be affected by previous method calls

**Example**:
```zen
class Test
    function set_name name
        set self.name name
    
    function get_name  
        return self.name

set t new Test
t.set_name "Bob"      # This works
set n t.get_name      # Returns [function] instead of "Bob"
```

**Root Cause**: Unknown - appears to be a parser state issue after method calls with arguments

## Summary

Major functionality has been implemented:
- ✅ Self property access works
- ✅ Property assignment syntax works
- ✅ Print function works
- ⚠️ Method auto-execution works in most cases but has edge cases

The ZEN interpreter is now largely functional for object-oriented programming, with only minor edge cases remaining.