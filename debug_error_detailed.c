#include <stdio.h>
#include "zen/core/error.h"
#include "zen/types/value.h"
#include "zen/core/memory.h"

// Replicate the error_new function with debug output
Value* debug_error_new(const char* message) {
    printf("DEBUG: error_new called with message: '%s'\n", message ? message : "NULL");
    
    if (!message) {
        printf("DEBUG: message is NULL, returning NULL\n");
        return NULL;
    }
    
    printf("DEBUG: calling value_new(VALUE_ERROR)\n");
    Value* error = value_new(VALUE_ERROR);
    printf("DEBUG: value_new returned: %p\n", (void*)error);
    
    if (!error) {
        printf("DEBUG: error is NULL, returning NULL\n");
        return NULL;
    }
    
    printf("DEBUG: error->as.error = %p\n", (void*)error->as.error);
    if (!error->as.error) {
        printf("DEBUG: error->as.error is NULL, returning NULL\n");
        return NULL;
    }
    
    // Replace the default message with our custom message
    printf("DEBUG: current message: '%s'\n", error->as.error->message ? error->as.error->message : "NULL");
    if (error->as.error->message) {
        printf("DEBUG: freeing existing message\n");
        memory_free(error->as.error->message);
    }
    
    printf("DEBUG: calling memory_strdup('%s')\n", message);
    error->as.error->message = memory_strdup(message);
    printf("DEBUG: memory_strdup returned: %p\n", (void*)error->as.error->message);
    
    if (!error->as.error->message) {
        printf("DEBUG: memory_strdup failed, cleaning up error value\n");
        value_unref(error);  // Clean up the error value
        return NULL;
    }
    
    error->as.error->code = -1;  // Default error code
    printf("DEBUG: error_new succeeded, returning %p\n", (void*)error);
    
    return error;
}

int main() {
    printf("=== Detailed Error New Debug ===\n");
    
    Value* error = debug_error_new("Test error message");
    
    if (error) {
        printf("✅ debug_error_new succeeded\n");
        value_unref(error);
    } else {
        printf("❌ debug_error_new failed\n");
    }
    
    return 0;
}