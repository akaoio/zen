/*
 * array_simple.c  
 * Simplified Advanced Array Operations for ZEN stdlib
 * 
 * Basic functional programming operations and array manipulation
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include "zen/types/array.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



/**
 * @brief Find first element matching a value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Index of first occurrence, or -1 if not found
 */
Value* array_push_stdlib(Value** args, size_t argc) {
    if (argc != 2) {
        return value_new_error("Array.push requires exactly 2 arguments", -1);
    }
    
    if (!args || !args[0] || !args[1]) {
        return value_new_error("Array.push: null arguments", -1);
    }
    
    Value* array_value = args[0];
    Value* value_to_push = args[1];
    
    if (array_value->type != VALUE_ARRAY) {
        return value_new_error("Array.push: first argument must be an array", -1);
    }
    
    // Use the core array_push function to add the item
    array_push(array_value, value_to_push);
    
    // Return a reference to the updated array
    return value_ref(array_value);
}

/**
 * @brief Array pop function for stdlib
 * @param args Array of arguments: [array]
 * @param argc Number of arguments (should be 1)
 * @return Popped value or error value
 */
Value* array_pop_stdlib(Value** args, size_t argc) {
    if (argc != 1) {
        return value_new_error("Array.pop requires exactly 1 argument", -1);
    }
    
    if (!args || !args[0]) {
        return value_new_error("Array.pop: null argument", -1);
    }
    
    Value* array_value = args[0];
    
    if (array_value->type != VALUE_ARRAY) {
        return value_new_error("Array.pop: argument must be an array", -1);
    }
    
    // Use the core array_push function to remove the last item
    Value* popped_value = array_pop(array_value);
    
    if (!popped_value) {
        // Array was empty, return null instead of error
        return value_new_null();
    }
    
    // Return the popped value (caller now owns the reference)
    return popped_value;
}