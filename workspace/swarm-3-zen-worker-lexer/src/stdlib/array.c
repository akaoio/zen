/*
 * array_simple.c  
 * Simplified Advanced Array Operations for ZEN stdlib
 * 
 * Basic functional programming operations and array manipulation
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations for error handling
Value* value_new_error(const char* message, int error_code);
Value* value_ref(Value* value);
void value_unref(Value* value);
Value* value_new_null(void);
bool value_equals(const Value* a, const Value* b);
Value* value_copy(const Value* value);

// Forward declarations of array functions from types/array.c
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);
Value* array_pop(Value* array);
Value* array_get(Value* array, size_t index);
void array_set(Value* array, size_t index, Value* item);
size_t array_length(const Value* array);

/**
 * @brief Find first element matching a value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Index of first occurrence, or -1 if not found
 */
Value* zen_array_find_index(const Value* array_value, const Value* search_value) {
    if (!array_value || array_value->type != VALUE_ARRAY || !search_value) {
        return value_new_number(-1);
    }
    
    size_t length = array_length(array_value);
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_get((Value*)array_value, i);
        if (elem && value_equals(elem, search_value)) {
            value_unref(elem);
            return value_new_number((double)i);
        }
        if (elem) {
            value_unref(elem);
        }
    }
    
    return value_new_number(-1);
}

/**
 * @brief Check if array includes a specific value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Boolean indicating if value is found
 */
Value* zen_array_includes(const Value* array_value, const Value* search_value) {
    if (!array_value || array_value->type != VALUE_ARRAY || !search_value) {
        return value_new_boolean(false);
    }
    
    size_t length = array_length(array_value);
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_get((Value*)array_value, i);
        if (elem && value_equals(elem, search_value)) {
            value_unref(elem);
            return value_new_boolean(true);
        }
        if (elem) {
            value_unref(elem);
        }
    }
    
    return value_new_boolean(false);
}

/**
 * @brief Reverse array elements
 * @param array_value Array to reverse
 * @return New array with reversed elements
 */
Value* zen_array_reverse(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    size_t length = array_length(array_value);
    Value* result_array = array_new(length);
    
    // Add elements in reverse order
    for (int i = (int)length - 1; i >= 0; i--) {
        Value* elem = array_get((Value*)array_value, (size_t)i);
        if (elem) {
            array_push(result_array, elem);
            value_unref(elem); // Release the reference from array_get
        }
    }
    
    return result_array;
}

/**
 * @brief Slice array from start to end index
 * @param array_value Source array
 * @param start_value Start index (inclusive)
 * @param end_value End index (exclusive), optional
 * @return New array with sliced elements
 */
Value* zen_array_slice(const Value* array_value, const Value* start_value, const Value* end_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    if (!start_value || start_value->type != VALUE_NUMBER) {
        return value_copy(array_value);
    }
    
    size_t length = array_length(array_value);
    int start = (int)start_value->as.number;
    int end = end_value && end_value->type == VALUE_NUMBER ? 
              (int)end_value->as.number : (int)length;
    
    // Handle negative indices
    if (start < 0) start = (int)length + start;
    if (end < 0) end = (int)length + end;
    
    // Clamp to valid range
    if (start < 0) start = 0;
    if (start >= (int)length) start = (int)length;
    if (end < start) end = start;
    if (end > (int)length) end = (int)length;
    
    size_t result_length = (size_t)(end - start);
    Value* result_array = array_new(result_length);
    
    for (int i = start; i < end; i++) {
        Value* elem = array_get((Value*)array_value, (size_t)i);
        if (elem) {
            array_push(result_array, elem);
            value_unref(elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Concatenate two arrays
 * @param array1_value First array
 * @param array2_value Second array
 * @return New array with concatenated elements
 */
Value* zen_array_concat(const Value* array1_value, const Value* array2_value) {
    if (!array1_value || array1_value->type != VALUE_ARRAY) {
        return array2_value && array2_value->type == VALUE_ARRAY ? 
               value_copy(array2_value) : array_new(0);
    }
    
    if (!array2_value || array2_value->type != VALUE_ARRAY) {
        return value_copy(array1_value);
    }
    
    size_t len1 = array_length(array1_value);
    size_t len2 = array_length(array2_value);
    
    Value* result_array = array_new(len1 + len2);
    
    // Copy elements from first array
    for (size_t i = 0; i < len1; i++) {
        Value* elem = array_get((Value*)array1_value, i);
        if (elem) {
            array_push(result_array, elem);
            value_unref(elem);
        }
    }
    
    // Copy elements from second array
    for (size_t i = 0; i < len2; i++) {
        Value* elem = array_get((Value*)array2_value, i);
        if (elem) {
            array_push(result_array, elem);
            value_unref(elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Array push function for stdlib
 * @param args Array of arguments: [array, value_to_push]
 * @param argc Number of arguments (should be 2)
 * @return Updated array or error value
 */
Value* zen_array_push_stdlib(Value** args, size_t argc) {
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
Value* zen_array_pop_stdlib(Value** args, size_t argc) {
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