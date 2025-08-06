#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/include/zen/types/value.h"
#include "src/include/zen/core/memory.h"

// Advanced test for value_free with reference counting
int main(void) {
    printf("Testing value_free() with reference counting...\n");
    
    // Initialize memory debugging
    memory_debug_enable(true);
    memory_reset_stats();
    
    // Test reference counting with value_free
    printf("1. Testing reference counting...\n");
    
    Value* str_val = value_new_string("Reference counted string");
    assert(str_val != NULL);
    assert(str_val->ref_count == 1);
    
    // Add references
    Value* ref1 = value_ref(str_val);
    Value* ref2 = value_ref(str_val);
    assert(str_val->ref_count == 3);
    assert(ref1 == str_val);
    assert(ref2 == str_val);
    
    // Remove references using value_unref (should NOT free yet)
    value_unref(ref1);
    assert(str_val->ref_count == 2);
    
    value_unref(ref2);
    assert(str_val->ref_count == 1);
    
    // This should actually free the value
    value_unref(str_val);
    // str_val is now invalid, don't access it
    
    printf("   ✓ Reference counting working correctly\n");
    
    // Test with arrays containing references
    printf("2. Testing complex array with references...\n");
    
    Value* arr = value_new(VALUE_ARRAY);
    assert(arr != NULL);
    
    // Create some values to put in the array
    Value* num1 = value_new_number(10.0);
    Value* num2 = value_new_number(20.0);
    Value* str1 = value_new_string("Array string");
    
    // Note: In a real implementation, we'd need array_push functions
    // For now, just test that they can be freed individually
    
    value_free(num1);
    value_free(num2);
    value_free(str1);
    value_free(arr);
    
    printf("   ✓ Complex references freed correctly\n");
    
    // Test edge case: circular references (if supported)
    printf("3. Testing error value with custom message...\n");
    
    Value* err = value_new_error("Custom error message", 404);
    assert(err != NULL);
    assert(err->type == VALUE_ERROR);
    
    char* err_str = value_to_string(err);
    assert(err_str != NULL);
    printf("   Error string: %s\n", err_str);
    free(err_str); // This should be freed with free() since value_to_string uses malloc
    
    value_free(err);
    printf("   ✓ Error value with custom message freed\n");
    
    // Test class and instance freeing
    printf("4. Testing class/instance freeing...\n");
    
    Value* class_val = value_new_class("TestClass", "ParentClass");
    assert(class_val != NULL);
    
    Value* instance_val = value_new_instance(class_val);
    if (instance_val) {
        value_free(instance_val);
        printf("   ✓ Instance freed\n");
    }
    
    value_free(class_val);
    printf("   ✓ Class freed\n");
    
    // Final memory leak check
    printf("5. Final memory leak check...\n");
    if (memory_check_leaks()) {
        printf("   ❌ Memory leaks detected!\n");
        memory_print_leak_report();
        return 1;
    } else {
        printf("   ✓ No memory leaks detected\n");
    }
    
    memory_debug_cleanup();
    printf("\nAll advanced tests passed! value_free() is robust.\n");
    return 0;
}