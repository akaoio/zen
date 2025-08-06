#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "zen/types/value.h"
#include "zen/core/memory.h"

void test_value_new_error() {
    printf("Testing value_new_error()...\n");
    
    // Test creating error value
    Value* error = value_new_error("Test error message", 42);
    assert(error != NULL);
    assert(error->type == VALUE_ERROR);
    assert(error->as.error != NULL);
    assert(error->as.error->code == 42);
    assert(strcmp(error->as.error->message, "Test error message") == 0);
    
    // Test string representation
    char* str = value_to_string(error);
    assert(str != NULL);
    assert(strstr(str, "Test error message") != NULL);
    free(str);
    
    value_unref(error);
    printf("✅ value_new_error() working correctly\n");
}

void test_value_is_truthy_public() {
    printf("Testing value_is_truthy_public()...\n");
    
    // Test null - should be falsy
    Value* null_val = value_new_null();
    assert(!value_is_truthy_public(null_val));
    value_unref(null_val);
    
    // Test boolean true - should be truthy
    Value* true_val = value_new_boolean(true);
    assert(value_is_truthy_public(true_val));
    value_unref(true_val);
    
    // Test boolean false - should be falsy
    Value* false_val = value_new_boolean(false);
    assert(!value_is_truthy_public(false_val));
    value_unref(false_val);
    
    // Test non-zero number - should be truthy
    Value* num_val = value_new_number(42.5);
    assert(value_is_truthy_public(num_val));
    value_unref(num_val);
    
    // Test zero - should be falsy
    Value* zero_val = value_new_number(0.0);
    assert(!value_is_truthy_public(zero_val));
    value_unref(zero_val);
    
    // Test NaN - should be falsy
    Value* nan_val = value_new_number(NAN);
    assert(!value_is_truthy_public(nan_val));
    value_unref(nan_val);
    
    // Test non-empty string - should be truthy
    Value* str_val = value_new_string("hello");
    assert(value_is_truthy_public(str_val));
    value_unref(str_val);
    
    // Test empty string - should be falsy
    Value* empty_str = value_new_string("");
    assert(!value_is_truthy_public(empty_str));
    value_unref(empty_str);
    
    // Test error - should be falsy
    Value* error_val = value_new_error("Some error", 1);
    assert(!value_is_truthy_public(error_val));
    value_unref(error_val);
    
    printf("✅ value_is_truthy_public() working correctly\n");
}

void test_additional_functions() {
    printf("Testing additional value system functions...\n");
    
    // Test value_to_number_or_nan
    Value* num_val = value_new_number(3.14);
    double num_result = value_to_number_or_nan(num_val);
    assert(num_result == 3.14);
    value_unref(num_val);
    
    Value* bool_val = value_new_boolean(true);
    double bool_result = value_to_number_or_nan(bool_val);
    assert(bool_result == 1.0);
    value_unref(bool_val);
    
    Value* str_val = value_new_string("invalid");
    double str_result = value_to_number_or_nan(str_val);
    assert(isnan(str_result));
    value_unref(str_val);
    
    // Test value_to_string_safe
    Value* test_val = value_new_string("test");
    char* safe_str = value_to_string_safe(test_val);
    assert(safe_str != NULL);
    assert(strcmp(safe_str, "test") == 0);
    free(safe_str);
    value_unref(test_val);
    
    // Test with NULL - should still return something
    char* null_str = value_to_string_safe(NULL);
    assert(null_str != NULL);
    free(null_str);
    
    // Test value_get_length
    Value* str_len = value_new_string("hello");
    assert(value_get_length(str_len) == 5);
    value_unref(str_len);
    
    Value* empty_len = value_new_string("");
    assert(value_get_length(empty_len) == 0);
    value_unref(empty_len);
    
    printf("✅ Additional functions working correctly\n");
}

int main() {
    printf("🔧 Testing ZEN Value System Priority Functions\n");
    printf("==============================================\n");
    
    test_value_new_error();
    test_value_is_truthy_public();
    test_additional_functions();
    
    printf("\n🎉 All value system tests passed!\n");
    printf("The value system is ready for cross-swarm integration.\n");
    
    return 0;
}