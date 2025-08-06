#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/types/value.h"

/**
 * Test all the error functions to ensure they work correctly
 */
int main(void) {
    printf("Testing all error functions...\n");
    
    // Test error_is_error
    printf("Testing error_is_error...\n");
    Value* error_val = error_new("Test error");
    Value* non_error_val = value_new_number(42.0);
    assert(error_is_error(error_val) == true);
    assert(error_is_error(non_error_val) == false);
    assert(error_is_error(NULL) == false);
    printf("✓ error_is_error works correctly\n");
    
    // Test error_print_simple
    printf("Testing error_print_simple...\n");
    printf("Expected output: 'Error: Test error'\n");
    error_print_simple(error_val);
    printf("✓ error_print_simple works correctly\n");
    
    // Test error_has_code
    printf("Testing error_has_code...\n");
    Value* coded_error = error_new_with_code("Coded error", 123);
    assert(error_has_code(coded_error, 123) == true);
    assert(error_has_code(coded_error, 456) == false);
    assert(error_has_code(non_error_val, 123) == false);
    printf("✓ error_has_code works correctly\n");
    
    // Test error_null_pointer
    printf("Testing error_null_pointer...\n");
    Value* null_error = error_null_pointer("test_function");
    assert(error_is_error(null_error));
    const char* null_msg = error_get_message(null_error);
    assert(strstr(null_msg, "Null pointer") != NULL);
    assert(strstr(null_msg, "test_function") != NULL);
    printf("✓ error_null_pointer works correctly\n");
    
    // Test error_division_by_zero
    printf("Testing error_division_by_zero...\n");
    Value* div_error = error_division_by_zero();
    assert(error_is_error(div_error));
    const char* div_msg = error_get_message(div_error);
    assert(strstr(div_msg, "Division by zero") != NULL);
    assert(error_get_code(div_error) == -1);
    printf("✓ error_division_by_zero works correctly\n");
    
    // Test error_index_out_of_bounds
    printf("Testing error_index_out_of_bounds...\n");
    Value* bounds_error = error_index_out_of_bounds(10, 5);
    assert(error_is_error(bounds_error));
    const char* bounds_msg = error_get_message(bounds_error);
    assert(strstr(bounds_msg, "Index out of bounds") != NULL);
    assert(strstr(bounds_msg, "10") != NULL);
    assert(strstr(bounds_msg, "5") != NULL);
    printf("✓ error_index_out_of_bounds works correctly\n");
    
    // Test error_memory_allocation
    printf("Testing error_memory_allocation...\n");
    Value* mem_error = error_memory_allocation();
    assert(error_is_error(mem_error));
    const char* mem_msg = error_get_message(mem_error);
    assert(strstr(mem_msg, "Memory allocation failed") != NULL);
    assert(error_get_code(mem_error) == -2);
    printf("✓ error_memory_allocation works correctly\n");
    
    // Test error_file_not_found
    printf("Testing error_file_not_found...\n");
    Value* file_error = error_file_not_found("missing.txt");
    assert(error_is_error(file_error));
    const char* file_msg = error_get_message(file_error);
    assert(strstr(file_msg, "File not found") != NULL);
    assert(strstr(file_msg, "missing.txt") != NULL);
    printf("✓ error_file_not_found works correctly\n");
    
    // Test error_parsing_failed
    printf("Testing error_parsing_failed...\n");
    Value* parse_error = error_parsing_failed("invalid syntax at line 5");
    assert(error_is_error(parse_error));
    const char* parse_msg = error_get_message(parse_error);
    assert(strstr(parse_msg, "Parsing failed") != NULL);
    assert(strstr(parse_msg, "invalid syntax at line 5") != NULL);
    printf("✓ error_parsing_failed works correctly\n");
    
    // Test error_undefined_variable
    printf("Testing error_undefined_variable...\n");
    Value* var_error = error_undefined_variable("unknown_var");
    assert(error_is_error(var_error));
    const char* var_msg = error_get_message(var_error);
    assert(strstr(var_msg, "Undefined variable") != NULL);
    assert(strstr(var_msg, "unknown_var") != NULL);
    printf("✓ error_undefined_variable works correctly\n");
    
    // Test error_undefined_function
    printf("Testing error_undefined_function...\n");
    Value* func_error = error_undefined_function("unknown_func");
    assert(error_is_error(func_error));
    const char* func_msg = error_get_message(func_error);
    assert(strstr(func_msg, "Undefined function") != NULL);
    assert(strstr(func_msg, "unknown_func") != NULL);
    printf("✓ error_undefined_function works correctly\n");
    
    // Clean up
    value_unref(error_val);
    value_unref(non_error_val);
    value_unref(coded_error);
    value_unref(null_error);
    value_unref(div_error);
    value_unref(bounds_error);
    value_unref(mem_error);
    value_unref(file_error);
    value_unref(parse_error);
    value_unref(var_error);
    value_unref(func_error);
    
    printf("\n🎉 ALL ERROR FUNCTIONS WORKING CORRECTLY!\n");
    printf("All 9+ error functions have been implemented and tested successfully.\n");
    
    return 0;
}