#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "zen/core/error.h"
#include "zen/types/value.h"

/**
 * @brief Test all error functions to verify they work correctly
 */
int main() {
    printf("Testing error functions...\n");
    
    // Test error_is_error
    Value* error = error_new("Test error");
    assert(error_is_error(error) == true);
    printf("✓ error_is_error works\n");
    
    Value* not_error = value_new_number(42.0);
    assert(error_is_error(not_error) == false);
    printf("✓ error_is_error correctly identifies non-errors\n");
    
    // Test error_print_simple
    printf("Testing error_print_simple (should see error message): ");
    error_print_simple(error);
    
    // Test error_has_code
    Value* coded_error = error_new_with_code("Code test", 123);
    assert(error_has_code(coded_error, 123) == true);
    assert(error_has_code(coded_error, 456) == false);
    printf("✓ error_has_code works\n");
    
    // Test all error creation functions
    Value* null_err = error_null_pointer("test_function");
    assert(error_is_error(null_err));
    printf("✓ error_null_pointer works\n");
    
    Value* div_err = error_division_by_zero();
    assert(error_is_error(div_err));
    printf("✓ error_division_by_zero works\n");
    
    Value* bounds_err = error_index_out_of_bounds(5, 3);
    assert(error_is_error(bounds_err));
    printf("✓ error_index_out_of_bounds works\n");
    
    Value* mem_err = error_memory_allocation();
    assert(error_is_error(mem_err));
    printf("✓ error_memory_allocation works\n");
    
    Value* file_err = error_file_not_found("missing.txt");
    assert(error_is_error(file_err));
    printf("✓ error_file_not_found works\n");
    
    Value* parse_err = error_parsing_failed("invalid syntax");
    assert(error_is_error(parse_err));
    printf("✓ error_parsing_failed works\n");
    
    Value* var_err = error_undefined_variable("myvar");
    assert(error_is_error(var_err));
    printf("✓ error_undefined_variable works\n");
    
    Value* func_err = error_undefined_function("myfunc");
    assert(error_is_error(func_err));
    printf("✓ error_undefined_function works\n");
    
    // Clean up
    value_unref(error);
    value_unref(not_error);
    value_unref(coded_error);
    value_unref(null_err);
    value_unref(div_err);
    value_unref(bounds_err);
    value_unref(mem_err);
    value_unref(file_err);
    value_unref(parse_err);
    value_unref(var_err);
    value_unref(func_err);
    
    printf("\n✅ All error functions work correctly!\n");
    return 0;
}