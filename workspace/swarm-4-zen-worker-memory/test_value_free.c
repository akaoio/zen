#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/include/zen/types/value.h"
#include "src/include/zen/core/memory.h"

// Simple test to verify value_free works properly
int main(void) {
    printf("Testing value_free() function...\n");
    
    // Initialize memory debugging
    memory_debug_enable(true);
    memory_reset_stats();
    
    // Test freeing NULL (should not crash)
    printf("1. Testing NULL value...\n");
    value_free(NULL);
    printf("   ✓ NULL handled safely\n");
    
    // Test freeing different value types
    printf("2. Testing different value types...\n");
    
    // Test null value
    Value* null_val = value_new_null();
    assert(null_val != NULL);
    value_free(null_val);
    printf("   ✓ NULL value freed\n");
    
    // Test boolean value
    Value* bool_val = value_new_boolean(true);
    assert(bool_val != NULL);
    value_free(bool_val);
    printf("   ✓ Boolean value freed\n");
    
    // Test number value
    Value* num_val = value_new_number(42.5);
    assert(num_val != NULL);
    value_free(num_val);
    printf("   ✓ Number value freed\n");
    
    // Test string value
    Value* str_val = value_new_string("Hello, World!");
    assert(str_val != NULL);
    value_free(str_val);
    printf("   ✓ String value freed\n");
    
    // Test array value
    Value* arr_val = value_new(VALUE_ARRAY);
    assert(arr_val != NULL);
    value_free(arr_val);
    printf("   ✓ Array value freed\n");
    
    // Test object value
    Value* obj_val = value_new(VALUE_OBJECT);
    assert(obj_val != NULL);
    value_free(obj_val);
    printf("   ✓ Object value freed\n");
    
    // Test error value
    Value* err_val = value_new_error("Test error", 123);
    assert(err_val != NULL);
    value_free(err_val);
    printf("   ✓ Error value freed\n");
    
    // Test class value
    Value* class_val = value_new_class("TestClass", NULL);
    assert(class_val != NULL);
    value_free(class_val);
    printf("   ✓ Class value freed\n");
    
    // Test instance value (this may be NULL if class is required)
    Value* instance_class = value_new_class("InstanceTest", NULL);
    Value* instance_val = value_new_instance(instance_class);
    if (instance_val) {
        value_free(instance_val);
        printf("   ✓ Instance value freed\n");
    }
    value_free(instance_class);
    
    // Check for memory leaks
    printf("3. Checking for memory leaks...\n");
    if (memory_check_leaks()) {
        printf("   ❌ Memory leaks detected!\n");
        memory_print_leak_report();
        return 1;
    } else {
        printf("   ✓ No memory leaks detected\n");
    }
    
    printf("\nAll tests passed! value_free() is working correctly.\n");
    return 0;
}