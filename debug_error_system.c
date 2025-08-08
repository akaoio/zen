#include <stdio.h>
#include "zen/core/error.h"
#include "zen/types/value.h"
#include "zen/core/memory.h"

int main() {
    printf("=== Debugging Error System ===\n");
    
    // Test 1: Can we create a basic value?
    printf("Test 1: Creating basic value...\n");
    Value* test_val = value_new(VALUE_STRING);
    if (test_val) {
        printf("✅ value_new(VALUE_STRING) succeeded\n");
        value_unref(test_val);
    } else {
        printf("❌ value_new(VALUE_STRING) failed\n");
        return 1;
    }
    
    // Test 2: Can we create an error value?
    printf("\nTest 2: Creating error value...\n");
    Value* error_val = value_new(VALUE_ERROR);
    if (error_val) {
        printf("✅ value_new(VALUE_ERROR) succeeded\n");
        printf("Error structure: %p\n", (void*)error_val->as.error);
        if (error_val->as.error) {
            printf("Default message: %s\n", error_val->as.error->message ? error_val->as.error->message : "NULL");
        }
        value_unref(error_val);
    } else {
        printf("❌ value_new(VALUE_ERROR) failed\n");
        return 1;
    }
    
    // Test 3: Can memory_strdup work?
    printf("\nTest 3: Testing memory_strdup...\n");
    char* test_str = memory_strdup("test string");
    if (test_str) {
        printf("✅ memory_strdup succeeded: '%s'\n", test_str);
        memory_free(test_str);
    } else {
        printf("❌ memory_strdup failed\n");
        return 1;
    }
    
    // Test 4: Can error_new work?
    printf("\nTest 4: Testing error_new...\n");
    Value* error = error_new("Test error message");
    if (error) {
        printf("✅ error_new succeeded\n");
        printf("Error type: %d (expected: %d)\n", error->type, VALUE_ERROR);
        if (error->as.error && error->as.error->message) {
            printf("Error message: '%s'\n", error->as.error->message);
        }
        value_unref(error);
    } else {
        printf("❌ error_new failed\n");
        return 1;
    }
    
    printf("\n✅ All tests passed - error system is working!\n");
    return 0;
}