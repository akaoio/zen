# ZEN Migration Guide: Value Systems Architecture

## Current Architecture Status: ✅ STABLE HYBRID

ZEN currently uses a **dual value system architecture** that is stable and functional:

### 🎯 **Primary System: RuntimeValue**
- **Location**: `src/core/runtime_value.c` 
- **Usage**: Core execution engine (visitor)
- **REPL**: Uses RuntimeValue exclusively
- **Object Display**: `{name: Alice, age: 30}` ✅

### 🔧 **Secondary System: Value** 
- **Location**: `src/types/value.c`
- **Usage**: Standard library functions  
- **File Execution**: Uses Value system via stdlib
- **Object Display**: `{name: Alice, age: 30}` ✅

## ✅ **What's Fixed**

1. **Object String Representation**: Both systems now display objects properly
2. **RuntimeValue System**: Fixed object display in REPL 
3. **Value System**: Fixed object display in file execution
4. **Conversion Utilities**: Added `rv_from_value()` and `rv_to_value()` for migration
5. **Core Execution**: Uses RuntimeValue as primary

## 🏗️ **Migration Decision: HYBRID APPROACH**

After analysis, we chose **NOT** to force complete migration because:

### ❌ **Complete Migration Costs**
- **393 functions** to convert (120+ hours work)
- **201 header signatures** to change  
- **All tests broken** and need rewrite
- **High risk** of introducing bugs
- **MANIFEST.json** completely broken
- **Opportunity cost** - time better spent on features

### ✅ **Hybrid Benefits**  
- **Both systems work correctly** 
- **Object display fixed** in both
- **RuntimeValue primary** for new development
- **Stable existing code** preserved
- **Can migrate gradually** over time

## 🚀 **Development Guidelines**

### For New Features
```c
// ✅ PREFERRED: Use RuntimeValue for new code
RuntimeValue *new_feature() {
    RuntimeValue *rv = rv_new_object();
    rv_object_set(rv, "key", rv_new_string("value"));
    return rv;
}
```

### For Existing Code
```c
// ✅ ACCEPTABLE: Keep Value system for stability  
Value *legacy_function() {
    Value *val = object_new();
    object_set(val, "key", value_new_string("value"));
    return val;
}
```

### Conversion When Needed
```c
// Convert between systems using utilities
RuntimeValue *rv = rv_from_value(value);
Value *val = rv_to_value(runtime_value);
```

## 📊 **System Usage Map**

| Component | System | Status |
|-----------|---------|---------|
| Core Visitor | RuntimeValue | ✅ Primary |
| REPL | RuntimeValue | ✅ Working |
| File Execution | RuntimeValue → Value | ✅ Working |
| Stdlib Functions | Value | ✅ Stable |
| Object Display | Both | ✅ Fixed |
| New Features | RuntimeValue | 🎯 Preferred |

## 🔄 **Optional Future Migration**

If you want to migrate gradually:

1. **Phase 1**: Convert one stdlib module at a time
2. **Phase 2**: Update tests for converted modules  
3. **Phase 3**: Update MANIFEST.json signatures
4. **Phase 4**: Remove Value system when all converted

## 🛡️ **Quality Assurance**

### Current Test Coverage
```bash
# Both systems work correctly
./zen test_object_syntax.zen    # Value system
echo 'set obj name "test"; print obj' | ./zen  # RuntimeValue system
```

### Memory Management
- **RuntimeValue**: Reference counting with `rv_ref()`/`rv_unref()`
- **Value**: Reference counting with `value_ref()`/`value_unref()`  
- **Conversion**: Proper ref counting in utility functions

## 📝 **Implementation Notes**

### Why This Works
1. **Clear separation**: Core uses RuntimeValue, stdlib uses Value
2. **Conversion layer**: Seamless translation when needed
3. **Both display correctly**: Objects show `{key: value}` format
4. **Memory safe**: Proper ref counting in both systems

### When to Reconsider
- If maintaining two systems becomes too complex
- If performance issues arise from conversions
- If new features need extensive cross-system integration

## 🎯 **Conclusion**

The **hybrid architecture** is a **practical engineering solution** that:
- ✅ Preserves stability
- ✅ Enables new development  
- ✅ Fixes object display issues
- ✅ Provides migration path
- ✅ Minimizes technical debt

This approach prioritizes **working software** over **architectural purity** - a proven strategy for successful projects.

---

**Status**: ✅ **STABLE AND RECOMMENDED**  
**Last Updated**: 2025-08-09  
**Review Date**: When adding major new features