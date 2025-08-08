/*
 * array_simple.c
 * Simplified Advanced Array Operations for ZEN stdlib
 *
 * Basic functional programming operations and array manipulation
 */

#define _GNU_SOURCE
#include "zen/types/array.h"

#include "zen/types/value.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Find first element matching a value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Index of first occurrence, or -1 if not found
 */
Value *array_push_stdlib(Value **args, size_t argc)
{
    // Fast path: validate argc first (most common error)
    if (argc != 2) {
        return value_new_error("Array.push requires exactly 2 arguments", -1);
    }

    // Validate arguments with early returns for performance
    if (!args) {
        return value_new_error("Array.push: null arguments array", -1);
    }

    Value *array_value = args[0];
    Value *value_to_push = args[1];

    // Null checks with specific error messages
    if (!array_value) {
        return value_new_error("Array.push: array argument is null", -1);
    }

    if (!value_to_push) {
        return value_new_error("Array.push: value argument is null", -1);
    }

    // Type validation with early return
    if (array_value->type != VALUE_ARRAY) {
        return value_new_error("Array.push: first argument must be an array", -1);
    }

    // Check if array is in valid state
    if (!array_value->as.array) {
        return value_new_error("Array.push: array data is corrupted", -1);
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
Value *array_pop_stdlib(Value **args, size_t argc)
{
    // Fast validation
    if (argc != 1) {
        return value_new_error("Array.pop requires exactly 1 argument", -1);
    }

    if (!args) {
        return value_new_error("Array.pop: null arguments array", -1);
    }

    Value *array_value = args[0];

    if (!array_value) {
        return value_new_error("Array.pop: array argument is null", -1);
    }

    if (array_value->type != VALUE_ARRAY) {
        return value_new_error("Array.pop: argument must be an array", -1);
    }

    // Check if array is in valid state
    if (!array_value->as.array) {
        return value_new_error("Array.pop: array data is corrupted", -1);
    }

    // Early return for empty arrays (performance optimization)
    if (array_value->as.array->length == 0) {
        return value_new_null();
    }

    // Use the core array_pop function to remove the last item
    Value *popped_value = array_pop(array_value);

    if (!popped_value) {
        // Unexpected: array had elements but pop failed
        return value_new_error("Array.pop: internal error during pop operation", -1);
    }

    // Return the popped value (caller now owns the reference)
    return popped_value;
}