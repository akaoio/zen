/*
 * array.c
 * Array implementation
 * 
 * This file is auto-generated from MANIFEST.json
 * DO NOT modify function signatures without updating the manifest
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "zen/types/value.h"
#include "zen/core/memory.h"

/**
 * @brief Create new array
 * @param initial_capacity Initial capacity for the array (minimum 1)
 * @return Pointer to new array Value, or NULL on failure
 */
Value* array_new(size_t initial_capacity) {
    // Ensure minimum capacity
    if (initial_capacity == 0) {
        initial_capacity = 1;
    }
    
    // Allocate the Value struct
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    // Allocate the ZenArray struct
    ZenArray* array = memory_alloc(sizeof(ZenArray));
    if (!array) {
        memory_free(value);
        return NULL;
    }
    
    // Allocate the items array
    array->items = memory_alloc(sizeof(Value*) * initial_capacity);
    if (!array->items) {
        memory_free(array);
        memory_free(value);
        return NULL;
    }
    
    // Initialize array structure
    array->length = 0;
    array->capacity = initial_capacity;
    
    // Initialize value structure
    value->type = VALUE_ARRAY;
    value->ref_count = 1;
    value->as.array = array;
    
    return value;
}

/**
 * @brief Add item to end of array
 * @param array Array Value to push to
 * @param item Value to add to the array
 */
void array_push(Value* array, Value* item) {
    if (!array || array->type != VALUE_ARRAY || !item) {
        return;
    }
    
    ZenArray* zen_array = array->as.array;
    if (!zen_array) {
        return;
    }
    
    // Check if we need to resize
    if (zen_array->length >= zen_array->capacity) {
        size_t new_capacity = zen_array->capacity * 2;
        Value** new_items = memory_realloc(zen_array->items, sizeof(Value*) * new_capacity);
        if (!new_items) {
            return; // Failed to resize, item not added
        }
        zen_array->items = new_items;
        zen_array->capacity = new_capacity;
    }
    
    // Add item with reference counting
    zen_array->items[zen_array->length] = value_ref(item);
    zen_array->length++;
}

/**
 * @brief Remove and return last item
 * @param array Array Value to pop from
 * @return Last item in array, or NULL if empty or invalid
 */
Value* array_pop(Value* array) {
    if (!array || array->type != VALUE_ARRAY) {
        return NULL;
    }
    
    ZenArray* zen_array = array->as.array;
    if (!zen_array || zen_array->length == 0) {
        return NULL;
    }
    
    // Get the last item
    Value* item = zen_array->items[zen_array->length - 1];
    zen_array->length--;
    
    // Return the item (caller now owns the reference)
    return item;
}

/**
 * @brief Get item at index
 * @param array Array Value to get from
 * @param index Zero-based index of item to get
 * @return Item at index, or NULL if out of bounds or invalid
 */
Value* array_get(Value* array, size_t index) {
    if (!array || array->type != VALUE_ARRAY) {
        return NULL;
    }
    
    ZenArray* zen_array = array->as.array;
    if (!zen_array || index >= zen_array->length) {
        return NULL;
    }
    
    // Return item with incremented reference count
    return value_ref(zen_array->items[index]);
}

/**
 * @brief Set item at index
 * @param array Array Value to set in
 * @param index Zero-based index to set at
 * @param item Value to set at the index
 */
void array_set(Value* array, size_t index, Value* item) {
    if (!array || array->type != VALUE_ARRAY || !item) {
        return;
    }
    
    ZenArray* zen_array = array->as.array;
    if (!zen_array || index >= zen_array->length) {
        return; // Index out of bounds
    }
    
    // Decrement reference count of old item
    if (zen_array->items[index]) {
        value_unref(zen_array->items[index]);
    }
    
    // Set new item with incremented reference count
    zen_array->items[index] = value_ref(item);
}

/**
 * @brief Get array length
 * @param array Array Value to get length of
 * @return Length of array, or 0 if invalid
 */
size_t array_length(const Value* array) {
    if (!array || array->type != VALUE_ARRAY) {
        return 0;
    }
    
    const ZenArray* zen_array = array->as.array;
    if (!zen_array) {
        return 0;
    }
    
    return zen_array->length;
}

/**
 * @brief Create deep copy of array and all nested structures
 * @param array Array Value to deep clone
 * @return Deep copy of array, or NULL on failure
 */
Value* array_deep_clone(const Value* array) {
    if (!array || array->type != VALUE_ARRAY) {
        return NULL;
    }
    
    const ZenArray* source = array->as.array;
    if (!source) {
        return NULL;
    }
    
    // Create new array with same capacity
    Value* result = array_new(source->capacity);
    if (!result) {
        return NULL;
    }
    
    ZenArray* dest = result->as.array;
    
    // Deep clone each element
    for (size_t i = 0; i < source->length; i++) {
        Value* cloned_item = value_copy(source->items[i]);
        if (!cloned_item) {
            // Failed to clone item - cleanup and return NULL
            value_unref(result);
            return NULL;
        }
        
        // Add to destination array
        dest->items[dest->length] = cloned_item;
        dest->length++;
    }
    
    return result;
}

/**
 * @brief Concatenate two arrays into new array
 * @param array1 First array
 * @param array2 Second array 
 * @return New array containing elements from both arrays, or NULL on failure
 */
Value* array_concat(const Value* array1, const Value* array2) {
    if (!array1 || array1->type != VALUE_ARRAY || 
        !array2 || array2->type != VALUE_ARRAY) {
        return NULL;
    }
    
    const ZenArray* arr1 = array1->as.array;
    const ZenArray* arr2 = array2->as.array;
    if (!arr1 || !arr2) {
        return NULL;
    }
    
    // Create new array with combined capacity
    size_t total_length = arr1->length + arr2->length;
    Value* result = array_new(total_length > 0 ? total_length : 1);
    if (!result) {
        return NULL;
    }
    
    // Copy elements from first array
    for (size_t i = 0; i < arr1->length; i++) {
        array_push(result, arr1->items[i]);
    }
    
    // Copy elements from second array
    for (size_t i = 0; i < arr2->length; i++) {
        array_push(result, arr2->items[i]);
    }
    
    return result;
}

/**
 * @brief Create subarray from start to end indices (Python-style)
 * @param array Source array
 * @param start Start index (inclusive, supports negative indices)
 * @param end End index (exclusive, supports negative indices)
 * @return New array containing slice, or NULL on failure
 */
Value* array_slice(const Value* array, int start, int end) {
    if (!array || array->type != VALUE_ARRAY) {
        return NULL;
    }
    
    const ZenArray* source = array->as.array;
    if (!source) {
        return NULL;
    }
    
    int len = (int)source->length;
    
    // Handle negative indices (Python-style)
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    
    // Clamp indices to valid range
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) {
        // Empty slice
        return array_new(1);
    }
    
    // Create new array for slice
    size_t slice_length = (size_t)(end - start);
    Value* result = array_new(slice_length);
    if (!result) {
        return NULL;
    }
    
    // Copy elements in the slice range
    for (int i = start; i < end; i++) {
        array_push(result, source->items[i]);
    }
    
    return result;
}

/**
 * @brief Check if array contains specific item
 * @param array Array to search in
 * @param item Item to search for
 * @return true if item found, false otherwise
 */
bool array_contains(const Value* array, const Value* item) {
    if (!array || array->type != VALUE_ARRAY || !item) {
        return false;
    }
    
    const ZenArray* arr = array->as.array;
    if (!arr) {
        return false;
    }
    
    // Search for item using value_equals
    for (size_t i = 0; i < arr->length; i++) {
        if (value_equals(arr->items[i], item)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Filter array elements using predicate function
 * @param array Source array to filter
 * @param predicate Function that returns truthy value for items to keep
 * @return New array with filtered elements, or NULL on failure
 */
Value* array_filter(const Value* array, Value* (*predicate)(Value* item)) {
    if (!array || array->type != VALUE_ARRAY || !predicate) {
        return NULL;
    }
    
    const ZenArray* source = array->as.array;
    if (!source) {
        return NULL;
    }
    
    // Create new array for filtered results
    Value* result = array_new(source->length > 0 ? source->length : 1);
    if (!result) {
        return NULL;
    }
    
    // Apply predicate to each element
    for (size_t i = 0; i < source->length; i++) {
        Value* test_result = predicate(source->items[i]);
        if (test_result) {
            // Check if result is truthy
            bool is_truthy = false;
            switch (test_result->type) {
                case VALUE_BOOLEAN:
                    is_truthy = test_result->as.boolean;
                    break;
                case VALUE_NUMBER:
                    is_truthy = (test_result->as.number != 0.0);
                    break;
                case VALUE_STRING:
                    is_truthy = (test_result->as.string && 
                                test_result->as.string->length > 0);
                    break;
                case VALUE_NULL:
                    is_truthy = false;
                    break;
                default:
                    is_truthy = true;
                    break;
            }
            
            if (is_truthy) {
                array_push(result, source->items[i]);
            }
            
            // Clean up predicate result
            value_unref(test_result);
        }
    }
    
    return result;
}

/**
 * @brief Transform array elements using mapping function
 * @param array Source array to transform
 * @param transform Function to transform each element
 * @return New array with transformed elements, or NULL on failure
 */
Value* array_map(const Value* array, Value* (*transform)(Value* item)) {
    if (!array || array->type != VALUE_ARRAY || !transform) {
        return NULL;
    }
    
    const ZenArray* source = array->as.array;
    if (!source) {
        return NULL;
    }
    
    // Create new array for mapped results
    Value* result = array_new(source->length > 0 ? source->length : 1);
    if (!result) {
        return NULL;
    }
    
    // Transform each element
    for (size_t i = 0; i < source->length; i++) {
        Value* transformed = transform(source->items[i]);
        if (transformed) {
            array_push(result, transformed);
            value_unref(transformed); // array_push refs it
        } else {
            // Transform failed - add NULL
            Value* null_val = value_new_null();
            if (null_val) {
                array_push(result, null_val);
                value_unref(null_val);
            }
        }
    }
    
    return result;
}

/**
 * @brief Join array elements into string with separator
 * @param array Array to join
 * @param separator String separator between elements
 * @return New string Value with joined elements, or NULL on failure
 */
Value* array_join(const Value* array, const char* separator) {
    if (!array || array->type != VALUE_ARRAY || !separator) {
        return NULL;
    }
    
    const ZenArray* arr = array->as.array;
    if (!arr) {
        return NULL;
    }
    
    // Handle empty array
    if (arr->length == 0) {
        return value_new_string("");
    }
    
    // Calculate total length needed
    size_t total_length = 0;
    size_t sep_len = strlen(separator);
    char** element_strings = memory_alloc(sizeof(char*) * arr->length);
    if (!element_strings) {
        return NULL;
    }
    
    // Convert each element to string and calculate lengths
    for (size_t i = 0; i < arr->length; i++) {
        element_strings[i] = value_to_string(arr->items[i]);
        if (element_strings[i]) {
            total_length += strlen(element_strings[i]);
        }
        if (i > 0) {
            total_length += sep_len;
        }
    }
    
    // Allocate result buffer
    char* result_str = memory_alloc(total_length + 1);
    if (!result_str) {
        // Cleanup element strings
        for (size_t i = 0; i < arr->length; i++) {
            if (element_strings[i]) memory_free(element_strings[i]);
        }
        memory_free(element_strings);
        return NULL;
    }
    
    // Build the joined string
    result_str[0] = '\0';
    for (size_t i = 0; i < arr->length; i++) {
        if (i > 0) {
            strcat(result_str, separator);
        }
        if (element_strings[i]) {
            strcat(result_str, element_strings[i]);
        }
    }
    
    // Create string Value
    Value* result = value_new_string(result_str);
    
    // Cleanup
    for (size_t i = 0; i < arr->length; i++) {
        if (element_strings[i]) memory_free(element_strings[i]);
    }
    memory_free(element_strings);
    memory_free(result_str);
    
    return result;
}

