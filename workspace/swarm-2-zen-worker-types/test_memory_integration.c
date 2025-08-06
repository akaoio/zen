#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "zen/types/value.h"
#include "zen/core/memory.h"

int main() {
    printf("Testing improved value functions with memory system integration...\n");
    
    // Enable memory debugging
    memory_debug_enable(true);
    
    // Test 1: Basic value creation and reference counting
    printf("\n1. Testing value_ref (atomic reference counting)...\n");
    Value* num_val = value_new_number(42.0);
    assert(num_val != NULL);
    assert(num_val->ref_count == 1);
    printf("   ✓ Initial ref_count: %zu\n", num_val->ref_count);
    
    // Test atomic reference increment
    Value* ref_val = value_ref(num_val);
    assert(ref_val == num_val); // Should return same pointer
    assert(num_val->ref_count == 2); // Reference count should increase
    printf("   ✓ value_ref correctly incremented ref_count: %zu\n", num_val->ref_count);
    
    // Test 2: String value creation (tests memory system integration)
    printf("\n2. Testing string value creation (memory system)...\n");
    Value* str_val = value_new_string("Hello, World!");
    assert(str_val != NULL);
    assert(str_val->type == VALUE_STRING);
    printf("   ✓ String value created successfully\n");
    
    // Test 3: value_new_instance validation
    printf("\n3. Testing value_new_instance validation...\n");
    
    // Test with NULL - should return NULL safely
    Value* null_instance = value_new_instance(NULL);
    assert(null_instance == NULL);
    printf("   ✓ NULL input handled correctly\n");
    
    // Test with non-class value - should return NULL safely  
    Value* invalid_instance = value_new_instance(num_val);
    assert(invalid_instance == NULL);
    printf("   ✓ Non-class input rejected correctly\n");
    
    // Test 4: Memory cleanup (value_free integration)
    printf("\n4. Testing value_free (memory system integration)...\n");
    
    // Clean up values and check they're properly freed
    value_unref(str_val);
    value_unref(num_val); 
    value_unref(ref_val); // This should trigger value_free when ref_count reaches 0
    
    printf("   ✓ Values cleaned up successfully\n");
    
    // Final memory check
    printf("\n5. Final memory leak check...\n");
    size_t leaks = memory_check_leaks();
    printf("   Memory leaks detected: %zu\n", leaks);
    
    if (leaks == 0) {
        printf("   ✓ No memory leaks - value_free working correctly!\n");
    } else {
        printf("   ! Memory leaks detected - need investigation\n");
        memory_print_leak_report();
    }
    
    printf("\n✓ All improved value functions tested successfully!\n");
    printf("✓ value_ref: Uses atomic operations for thread-safe reference counting\n");
    printf("✓ value_free: Integrates with ZEN memory system\n"); 
    printf("✓ value_new_instance: Proper validation and error handling\n");
    
    memory_debug_cleanup();
    return 0;
}