#include <stdio.h>
#include <stdlib.h>
#include "zen/types/value.h"
#include "zen/stdlib/stdlib.h"
#include "zen/core/memory.h"

int main() {
    printf("Testing stdlib function directly...\n");
    
    // Enable debug memory
    memory_debug_enable(true);
    
    // Create a test string value
    Value* test_str = value_new_string("test");
    if (\!test_str) {
        printf("Failed to create test string\n");
        return 1;
    }
    printf("Created test string with ref_count: %zu\n", test_str->ref_count);
    
    // Prepare arguments array
    Value** args = malloc(sizeof(Value*) * 1);
    args[0] = test_str;
    
    // Call len function directly
    printf("Calling zen_stdlib_length...\n");
    Value* result = zen_stdlib_length(args, 1);
    
    if (result) {
        printf("Result type: %d\n", result->type);
        if (result->type == VALUE_NUMBER) {
            printf("Length result: %f\n", result->as.number);
        }
        printf("Result ref_count: %zu\n", result->ref_count);
        
        // Clean up result
        value_unref(result);
    } else {
        printf("NULL result\n");
    }
    
    // Clean up arguments
    value_unref(test_str);
    free(args);
    
    printf("Test completed successfully\n");
    return 0;
}
EOF < /dev/null
