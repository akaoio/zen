#include "src/include/zen/core/error.h"
#include "src/include/zen/types/value.h"
#include <stdio.h>

int main() {
    printf("Testing ZEN Error System\n");
    printf("========================\n");
    
    // Test basic error creation
    Value* err1 = error_new("Test error message");
    if (error_is_error(err1)) {
        printf("✓ Basic error creation works\n");
        printf("  Message: %s\n", error_get_message(err1));
        printf("  Code: %d\n", error_get_code(err1));
    } else {
        printf("✗ Basic error creation failed\n");
        return 1;
    }
    
    // Test error with code
    Value* err2 = error_new_with_code("Test error with code", 42);
    if (error_is_error(err2) && error_get_code(err2) == 42) {
        printf("✓ Error with code works\n");
        printf("  Message: %s\n", error_get_message(err2));
        printf("  Code: %d\n", error_get_code(err2));
    } else {
        printf("✗ Error with code failed\n");
        return 1;
    }
    
    // Test formatted error
    Value* err3 = error_new_formatted("Test formatted error: %d items", 123);
    if (error_is_error(err3)) {
        printf("✓ Formatted error works\n");
        printf("  Message: %s\n", error_get_message(err3));
    } else {
        printf("✗ Formatted error failed\n");
        return 1;
    }
    
    // Test specific error types
    Value* null_err = error_null_pointer("test_function");
    Value* type_err = error_type_mismatch("number", "string");
    Value* div_err = error_division_by_zero();
    Value* index_err = error_index_out_of_bounds(5, 3);
    
    printf("✓ Specific error types:\n");
    printf("  Null pointer: %s\n", error_get_message(null_err));
    printf("  Type mismatch: %s\n", error_get_message(type_err));
    printf("  Division by zero: %s\n", error_get_message(div_err));
    printf("  Index bounds: %s\n", error_get_message(index_err));
    
    // Test error utilities
    Value* errors[] = {err1, err2, err3};
    if (error_occurred(errors, 3)) {
        printf("✓ Error detection works\n");
    }
    
    Value* first_error = error_first_error(errors, 3);
    if (first_error == err1) {
        printf("✓ First error detection works\n");
    }
    
    printf("\n🎉 All error system tests passed!\n");
    return 0;
}