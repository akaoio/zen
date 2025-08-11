/*
 * array.c
 * Array Operations for ZEN stdlib
 *
 * Basic functional programming operations and array manipulation
 */

#define _GNU_SOURCE
#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Push element to end of array
 * @param args Array arguments: [array, element]
 * @param argc Number of arguments (must be 2)
 * @return The array with element added, or error on failure
 */
RuntimeValue *array_push_stdlib(RuntimeValue **args, size_t argc)
{
    // Fast path: validate argc first (most common error)
    if (argc != 2) {
        return rv_new_error("Array.push requires exactly 2 arguments", -1);
    }

    // Validate arguments with early returns for performance
    if (!args) {
        return rv_new_error("Array.push: null arguments array", -1);
    }

    RuntimeValue *array_value = args[0];
    RuntimeValue *value_to_push = args[1];

    // Null checks with specific error messages
    if (!array_value) {
        return rv_new_error("Array.push: array argument is null", -1);
    }

    if (!value_to_push) {
        return rv_new_error("Array.push: value argument is null", -1);
    }

    // Type validation with early return
    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.push: first argument must be an array", -1);
    }

    // Use the built-in array push function
    rv_array_push(array_value, value_to_push);

    // Return the array (ZEN style - return the array itself)
    return rv_ref(array_value);
}

/**
 * @brief Pop element from end of array
 * @param args Array arguments: [array]
 * @param argc Number of arguments (must be 1)
 * @return The popped element, or null if array is empty, or error on failure
 */
RuntimeValue *array_pop_stdlib(RuntimeValue **args, size_t argc)
{
    // Fast path: validate argc first
    if (argc != 1) {
        return rv_new_error("Array.pop requires exactly 1 argument", -1);
    }

    // Validate arguments
    if (!args) {
        return rv_new_error("Array.pop: null arguments array", -1);
    }

    RuntimeValue *array_value = args[0];

    // Null check
    if (!array_value) {
        return rv_new_error("Array.pop: array argument is null", -1);
    }

    // Type validation
    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.pop: argument must be an array", -1);
    }

    // Check if array is empty
    if (array_value->data.array.count == 0) {
        return rv_new_null();
    }

    // Get last element and remove it from array
    size_t last_index = array_value->data.array.count - 1;
    RuntimeValue *last_element = array_value->data.array.elements[last_index];

    // Reference the element before removing it from array
    RuntimeValue *result = rv_ref(last_element);

    // Remove last element (decrement count)
    array_value->data.array.count--;

    return result;
}

/**
 * @brief Get length of array
 * @param args Array arguments: [array]
 * @param argc Number of arguments (must be 1)
 * @return Length as number, or error on failure
 */
RuntimeValue *array_length_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("Array.length requires exactly 1 argument", -1);
    }

    if (!args || !args[0]) {
        return rv_new_error("Array.length: null argument", -1);
    }

    RuntimeValue *array_value = args[0];

    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.length: argument must be an array", -1);
    }

    return rv_new_number((double)array_value->data.array.count);
}

/**
 * @brief Get array element at index
 * @param args Array arguments: [array, index]
 * @param argc Number of arguments (must be 2)
 * @return Element at index, or null if out of bounds, or error on failure
 */
RuntimeValue *array_get_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("Array.get requires exactly 2 arguments", -1);
    }

    if (!args || !args[0] || !args[1]) {
        return rv_new_error("Array.get: null arguments", -1);
    }

    RuntimeValue *array_value = args[0];
    RuntimeValue *index_value = args[1];

    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.get: first argument must be an array", -1);
    }

    if (index_value->type != RV_NUMBER) {
        return rv_new_error("Array.get: second argument must be a number", -1);
    }

    int index = (int)index_value->data.number;

    // Handle negative indexing
    if (index < 0) {
        index = (int)array_value->data.array.count + index;
    }

    // Check bounds
    if (index < 0 || (size_t)index >= array_value->data.array.count) {
        return rv_new_null();
    }

    return rv_ref(array_value->data.array.elements[index]);
}

/**
 * @brief Set array element at index
 * @param args Array arguments: [array, index, value]
 * @param argc Number of arguments (must be 3)
 * @return The array, or error on failure
 */
RuntimeValue *array_set_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error("Array.set requires exactly 3 arguments", -1);
    }

    if (!args || !args[0] || !args[1] || !args[2]) {
        return rv_new_error("Array.set: null arguments", -1);
    }

    RuntimeValue *array_value = args[0];
    RuntimeValue *index_value = args[1];
    RuntimeValue *value_to_set = args[2];

    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.set: first argument must be an array", -1);
    }

    if (index_value->type != RV_NUMBER) {
        return rv_new_error("Array.set: second argument must be a number", -1);
    }

    int index = (int)index_value->data.number;

    // Handle negative indexing
    if (index < 0) {
        index = (int)array_value->data.array.count + index;
    }

    // Check bounds
    if (index < 0 || (size_t)index >= array_value->data.array.count) {
        return rv_new_error("Array.set: index out of bounds", -1);
    }

    // Use the built-in array set function
    rv_array_set(array_value, (size_t)index, value_to_set);

    return rv_ref(array_value);
}

/**
 * @brief Get a slice of an array
 * @param args Array arguments: [array, start, end]
 * @param argc Number of arguments (must be 2 or 3)
 * @return New array containing slice, or error on failure
 */
RuntimeValue *array_slice_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return rv_new_error("Array.slice requires 2 or 3 arguments (array, start [, end])", -1);
    }

    if (!args || !args[0] || !args[1]) {
        return rv_new_error("Array.slice: null arguments", -1);
    }

    RuntimeValue *array_value = args[0];
    RuntimeValue *start_value = args[1];
    RuntimeValue *end_value = (argc == 3) ? args[2] : NULL;

    if (array_value->type != RV_ARRAY) {
        return rv_new_error("Array.slice: first argument must be an array", -1);
    }

    if (start_value->type != RV_NUMBER) {
        return rv_new_error("Array.slice: start index must be a number", -1);
    }

    int start = (int)start_value->data.number;
    int end = end_value ? (int)end_value->data.number : (int)array_value->data.array.count;

    // Handle negative indices
    if (start < 0) {
        start = (int)array_value->data.array.count + start;
    }
    if (end < 0) {
        end = (int)array_value->data.array.count + end;
    }

    // Clamp to valid range
    if (start < 0)
        start = 0;
    if (end > (int)array_value->data.array.count)
        end = (int)array_value->data.array.count;
    if (start > end)
        start = end;

    // Create new array with slice
    RuntimeValue *result = rv_new_array();
    for (int i = start; i < end; i++) {
        rv_array_push(result, array_value->data.array.elements[i]);
    }

    return result;
}