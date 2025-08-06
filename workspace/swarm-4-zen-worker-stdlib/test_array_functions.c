#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "src/include/zen/types/value.h"

// Function prototypes
extern Value* zen_array_push_stdlib(Value** args, size_t argc);
extern Value* zen_array_pop_stdlib(Value** args, size_t argc);
extern Value* array_new(size_t initial_capacity);
extern Value* value_new_number(double num);
extern Value* value_new_string(const char* str);
extern size_t array_length(const Value* array);

int main() {
    printf("Testing zen_array_push_stdlib and zen_array_pop_stdlib functions...\n");
    
    // Create a new array
    Value* array = array_new(2);
    if (!array) {
        printf("FAILED: Could not create array\n");
        return 1;
    }
    
    printf("✓ Created empty array\n");
    
    // Create values to push
    Value* num1 = value_new_number(42.0);
    Value* str1 = value_new_string("hello");
    
    // Test push function
    Value* args_push[2] = {array, num1};
    Value* result_push = zen_array_push_stdlib(args_push, 2);
    
    if (!result_push) {
        printf("FAILED: zen_array_push_stdlib returned NULL\n");
        return 1;
    }
    
    printf("✓ Successfully pushed number to array\n");
    
    // Check array length
    size_t length = array_length(array);
    if (length != 1) {
        printf("FAILED: Expected array length 1, got %zu\n", length);
        return 1;
    }
    
    printf("✓ Array length is correct after push: %zu\n", length);
    
    // Push second item
    Value* args_push2[2] = {array, str1};
    Value* result_push2 = zen_array_push_stdlib(args_push2, 2);
    
    if (!result_push2) {
        printf("FAILED: zen_array_push_stdlib returned NULL on second push\n");
        return 1;
    }
    
    length = array_length(array);
    if (length != 2) {
        printf("FAILED: Expected array length 2, got %zu\n", length);
        return 1;
    }
    
    printf("✓ Successfully pushed string to array, length now: %zu\n", length);
    
    // Test pop function
    Value* args_pop[1] = {array};
    Value* popped_value = zen_array_pop_stdlib(args_pop, 1);
    
    if (!popped_value) {
        printf("FAILED: zen_array_pop_stdlib returned NULL\n");
        return 1;
    }
    
    length = array_length(array);
    if (length != 1) {
        printf("FAILED: Expected array length 1 after pop, got %zu\n", length);
        return 1;
    }
    
    printf("✓ Successfully popped value from array, length now: %zu\n", length);
    
    // Test error conditions
    Value* result_error = zen_array_push_stdlib(NULL, 0);
    if (!result_error || result_error->type != VALUE_ERROR) {
        printf("FAILED: Expected error for NULL args\n");
        return 1;
    }
    
    printf("✓ Properly handled NULL arguments error\n");
    
    // Test wrong argument count
    Value* result_wrong_argc = zen_array_push_stdlib(args_push, 1);
    if (!result_wrong_argc || result_wrong_argc->type != VALUE_ERROR) {
        printf("FAILED: Expected error for wrong argument count\n");
        return 1;
    }
    
    printf("✓ Properly handled wrong argument count error\n");
    
    printf("\n🎉 All tests passed! zen_array_push_stdlib and zen_array_pop_stdlib are working correctly.\n");
    
    return 0;
}