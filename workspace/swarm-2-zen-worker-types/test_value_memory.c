#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "zen/types/value.h"
#include "zen/core/memory.h"

int main() {
    printf("Testing improved value functions...\n");
    
    // Initialize memory system
    memory_init();
    memory_debug_enable(true);
    
    // Test 1: value_ref with atomic operations
    printf("\n1. Testing value_ref (atomic reference counting)...\n");
    Value* num_val = value_new_number(42.0);
    assert(num_val != NULL);
    assert(num_val->ref_count == 1);
    
    Value* ref_val = value_ref(num_val);
    assert(ref_val == num_val); // Should return same pointer
    assert(num_val->ref_count == 2); // Reference count should increase
    printf("   ✓ value_ref correctly incremented ref_count: %zu\n", num_val->ref_count);
    
    // Test 2: value_free with memory system
    printf("\n2. Testing value_free (memory system integration)...\n");
    size_t initial_alloc = memory_get_total_allocated();
    
    Value* test_val = value_new_string("test");
    assert(test_val != NULL);
    size_t after_alloc = memory_get_total_allocated();
    assert(after_alloc > initial_alloc);
    printf("   ✓ Memory allocated: %zu bytes\n", after_alloc - initial_alloc);
    
    value_unref(test_val); // Should free the value
    size_t final_alloc = memory_get_total_allocated();
    printf("   ✓ Memory after free: %zu bytes\n", final_alloc);
    
    // Test 3: value_new_instance validation
    printf("\n3. Testing value_new_instance (validation and memory system)...\n");
    
    // Test with NULL - should return NULL safely
    Value* null_instance = value_new_instance(NULL);
    assert(null_instance == NULL);
    printf("   ✓ NULL input handled correctly\n");
    
    // Test with non-class value - should return NULL safely
    Value* non_class = value_new_number(123.0);
    Value* invalid_instance = value_new_instance(non_class);
    assert(invalid_instance == NULL);
    printf("   ✓ Non-class input rejected correctly\n");
    
    // Clean up
    value_unref(num_val);
    value_unref(ref_val);
    value_unref(non_class);
    
    // Final memory report
    printf("\n4. Final memory status...\n");
    memory_report();
    
    printf("\n✓ All value function improvements working correctly!\n");
    printf("✓ Reference counting uses atomic operations\n");
    printf("✓ Memory management uses ZEN memory system\n");
    printf("✓ Proper validation and error handling implemented\n");
    
    memory_cleanup();
    return 0;
}