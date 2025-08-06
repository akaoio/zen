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

