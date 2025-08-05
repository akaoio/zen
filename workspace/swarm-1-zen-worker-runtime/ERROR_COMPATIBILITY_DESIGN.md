# ZEN Error System Compatibility Design

## Problem Analysis

The ZEN codebase has conflicting ZenError definitions:

### Existing ZenError (in value.h):
```c
typedef struct {
    char* message;
    int code;
} ZenError;
```

### My Enhanced ZenError (in my error.h):
```c
typedef struct ZenError {
    ZenErrorCode code;
    char* message;
    char* filename;
    size_t line;
    size_t column;
    char* source_line;
    struct ZenError* cause;
    size_t ref_count;
} ZenError;
```

## Compatibility Solution

Instead of replacing the existing ZenError, I'll create a parallel enhanced error system that can:

1. **Work with existing ZenError**: Convert to/from existing error structure
2. **Provide enhanced features**: Location info, error chaining, etc.
3. **Maintain backward compatibility**: All existing code continues to work
4. **Progressive enhancement**: New code can use enhanced features

## Implementation Strategy

### 1. Enhanced Error Type
Create `ZenDetailedError` alongside existing `ZenError`:

```c
typedef struct ZenDetailedError {
    ZenErrorCode code;
    char* message;
    char* filename;
    size_t line;
    size_t column;
    char* source_line;
    struct ZenDetailedError* cause;
    size_t ref_count;
} ZenDetailedError;
```

### 2. Conversion Functions
```c
// Convert between error types
ZenDetailedError* error_detailed_from_simple(const ZenError* simple_error);
ZenError* error_simple_from_detailed(const ZenDetailedError* detailed_error);

// Convert to/from Value
Value* error_detailed_to_value(const ZenDetailedError* error);
ZenDetailedError* error_detailed_from_value(const Value* value);
```

### 3. Wrapper Functions
Provide enhanced functions that work with both types:
```c
// Enhanced error creation (returns ZenDetailedError)
ZenDetailedError* error_create(ZenErrorCode code, const char* message);
ZenDetailedError* error_create_with_location(ZenErrorCode code, const char* message, 
                                            const char* filename, size_t line, size_t column);

// Simple error creation (returns ZenError for existing code)
ZenError* error_simple_create(int code, const char* message);
```

### 4. Integration Points
- **Operators**: Update create_error() to use enhanced system
- **Parser/Lexer**: Add location tracking with enhanced errors
- **Main**: Better error display with context
- **Existing code**: Continue to work with simple ZenError

## Benefits

1. **Non-breaking**: Existing code continues to work
2. **Opt-in enhancement**: New code can use enhanced features
3. **Gradual migration**: Components can be upgraded incrementally
4. **Backward compatibility**: Always maintain simple error interface

## Migration Path

### Phase 1: Core Infrastructure
- Implement ZenDetailedError alongside ZenError
- Add conversion functions
- Update error.c to use enhanced system internally

### Phase 2: Enhanced Components
- Update operators.c to use detailed errors
- Add location tracking to lexer/parser
- Enhance main.c error display

### Phase 3: Integration
- Migrate stdlib functions to use enhanced errors
- Add context-aware error reporting
- Implement error recovery strategies

This approach allows me to implement the comprehensive error system without breaking existing code or requiring immediate changes to MANIFEST.json or value.h.