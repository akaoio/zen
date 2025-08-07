#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Include the necessary headers
#include "zen/core/error.h"
#include "zen/types/value.h"
#include "zen/core/memory.h"

void test_error_has_code_enhanced() {
    printf("Running enhanced error_has_code tests...\n");
    
    // Test 1: NULL error value
    assert(!error_has_code(NULL, 42));
    printf("✓ Test 1 passed: NULL error value handled correctly\n");
    
    // Test 2: Non-error value
    Value* string_val = value_new_string("not an error");
    assert(!error_has_code(string_val, 42));
    value_unref(string_val);
    printf("✓ Test 2 passed: Non-error value handled correctly\n");
    
    // Test 3: Error with specific code
    Value* error_val = error_new_with_code("Test error", 42);
    assert(error_has_code(error_val, 42));
    assert(!error_has_code(error_val, 43));
    value_unref(error_val);
    printf("✓ Test 3 passed: Error with specific code handled correctly\n");
    
    // Test 4: Error with zero code (default)
    Value* error_zero = error_new("Default error");  // Should have code -1 by default
    // Let's manually set it to 0 for testing
    if (error_zero && error_zero->as.error) {
        error_zero->as.error->code = 0;
    }
    assert(error_has_code(error_zero, 0));
    assert(!error_has_code(error_zero, 1));
    value_unref(error_zero);
    printf("✓ Test 4 passed: Error with zero code handled correctly\n");
    
    // Test 5: Error with negative code
    Value* error_neg = error_new_with_code("Negative error", -1);
    assert(error_has_code(error_neg, -1));
    assert(!error_has_code(error_neg, 1));
    value_unref(error_neg);
    printf("✓ Test 5 passed: Error with negative code handled correctly\n");
    
    // Test 6: Multiple errors with different codes
    Value* error1 = error_new_with_code("Error 1", 100);
    Value* error2 = error_new_with_code("Error 2", 200);
    Value* error3 = error_new_with_code("Error 3", 300);
    
    assert(error_has_code(error1, 100));
    assert(error_has_code(error2, 200));
    assert(error_has_code(error3, 300));
    
    assert(!error_has_code(error1, 200));
    assert(!error_has_code(error2, 300));
    assert(!error_has_code(error3, 100));
    
    value_unref(error1);
    value_unref(error2);
    value_unref(error3);
    printf("✓ Test 6 passed: Multiple errors with different codes handled correctly\n");
    
    printf("All enhanced error_has_code tests passed! ✅\n\n");
}

int main() {
    // Initialize memory debugging
    memory_debug_enable(true);
    
    // Run the enhanced tests
    test_error_has_code_enhanced();
    
    // Check for memory leaks
    if (memory_check_leaks()) {
        printf("❌ Memory leaks detected!\n");
        memory_print_leak_report();
        return 1;
    } else {
        printf("✅ No memory leaks detected!\n");
    }
    
    memory_debug_cleanup();
    return 0;
}