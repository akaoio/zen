/*
 * object.c
 * Object/hash map implementation
 * 
 * This file is auto-generated from MANIFEST.json
 * DO NOT modify function signatures without updating the manifest
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "zen/types/value.h"
#include "zen/core/memory.h"

// Forward declarations for array functions (no header file exists yet)
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);

// Forward declarations for internal functions
static int object_find_key_index(const ZenObject* object, const char* key);
static bool object_expand_if_needed(ZenObject* object);

/**
 * @brief Create new object
 * @return New empty object Value, or NULL on failure
 */
Value* object_new(void) {
    // Allocate the Value struct
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    // Allocate the ZenObject struct
    ZenObject* object = memory_alloc(sizeof(ZenObject));
    if (!object) {
        memory_free(value);
        return NULL;
    }
    
    // Initialize object with small initial capacity
    const size_t initial_capacity = 4;
    object->pairs = memory_alloc(sizeof(ZenObjectPair) * initial_capacity);
    if (!object->pairs) {
        memory_free(object);
        memory_free(value);
        return NULL;
    }
    
    // Initialize object structure
    object->length = 0;
    object->capacity = initial_capacity;
    
    // Initialize value structure
    value->type = VALUE_OBJECT;
    value->ref_count = 1;
    value->as.object = object;
    
    return value;
}

/**
 * @brief Set key-value pair
 * @param object Object Value to set in
 * @param key String key to set
 * @param value Value to associate with key
 */
void object_set(Value* object, const char* key, Value* value) {
    if (!object || object->type != VALUE_OBJECT || !key || !value) {
        return;
    }
    
    ZenObject* zen_object = object->as.object;
    if (!zen_object) {
        return;
    }
    
    // Find existing key
    int index = object_find_key_index(zen_object, key);
    
    if (index >= 0) {
        // Key exists, replace value
        if (zen_object->pairs[index].value) {
            value_unref(zen_object->pairs[index].value);
        }
        zen_object->pairs[index].value = value_ref(value);
    } else {
        // Key doesn't exist, add new pair
        
        // Expand capacity if needed
        if (!object_expand_if_needed(zen_object)) {
            return; // Failed to expand
        }
        
        // Add new key-value pair
        size_t new_index = zen_object->length;
        zen_object->pairs[new_index].key = memory_strdup(key);
        if (!zen_object->pairs[new_index].key) {
            return; // Failed to duplicate key string
        }
        
        zen_object->pairs[new_index].value = value_ref(value);
        zen_object->length++;
    }
}

/**
 * @brief Get value by key
 * @param object Object Value to get from
 * @param key String key to look up
 * @return Value associated with key, or NULL if not found
 */
Value* object_get(Value* object, const char* key) {
    if (!object || object->type != VALUE_OBJECT || !key) {
        return NULL;
    }
    
    ZenObject* zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }
    
    int index = object_find_key_index(zen_object, key);
    if (index >= 0) {
        // Return value with incremented reference count
        return value_ref(zen_object->pairs[index].value);
    }
    
    return NULL; // Key not found
}

/**
 * @brief Check if key exists
 * @param object Object Value to check
 * @param key String key to check for
 * @return true if key exists, false otherwise
 */
bool object_has(Value* object, const char* key) {
    if (!object || object->type != VALUE_OBJECT || !key) {
        return false;
    }
    
    ZenObject* zen_object = object->as.object;
    if (!zen_object) {
        return false;
    }
    
    return object_find_key_index(zen_object, key) >= 0;
}

/**
 * @brief Remove key-value pair
 * @param object Object Value to remove from
 * @param key String key to remove
 */
void object_delete(Value* object, const char* key) {
    if (!object || object->type != VALUE_OBJECT || !key) {
        return;
    }
    
    ZenObject* zen_object = object->as.object;
    if (!zen_object) {
        return;
    }
    
    int index = object_find_key_index(zen_object, key);
    if (index >= 0) {
        // Free the key and value
        memory_free(zen_object->pairs[index].key);
        value_unref(zen_object->pairs[index].value);
        
        // Move last element to fill the gap
        if (index < (int)(zen_object->length - 1)) {
            zen_object->pairs[index] = zen_object->pairs[zen_object->length - 1];
        }
        
        zen_object->length--;
    }
}

/**
 * @brief Get array of keys
 * @param object Object Value to get keys from
 * @return Array Value containing all keys, or NULL on failure
 */
Value* object_keys(Value* object) {
    if (!object || object->type != VALUE_OBJECT) {
        return NULL;
    }
    
    ZenObject* zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }
    
    // Create new array for keys
    Value* keys_array = array_new(zen_object->length);
    if (!keys_array) {
        return NULL;
    }
    
    // Add each key as a string value to the array
    for (size_t i = 0; i < zen_object->length; i++) {
        Value* key_value = value_new_string(zen_object->pairs[i].key);
        if (!key_value) {
            // Cleanup and return NULL on failure
            value_unref(keys_array);
            return NULL;
        }
        
        array_push(keys_array, key_value);
        value_unref(key_value); // array_push increments ref count
    }
    
    return keys_array;
}

// Internal helper functions

/**
 * @brief Find the index of a key in the object's pairs array
 * @param object ZenObject to search in
 * @param key String key to find
 * @return Index of key if found, -1 if not found
 */
static int object_find_key_index(const ZenObject* object, const char* key) {
    if (!object || !key) {
        return -1;
    }
    
    for (size_t i = 0; i < object->length; i++) {
        if (object->pairs[i].key && strcmp(object->pairs[i].key, key) == 0) {
            return (int)i;
        }
    }
    
    return -1; // Not found
}

/**
 * @brief Expand object capacity if needed
 * @param object ZenObject to potentially expand
 * @return true if expansion succeeded or wasn't needed, false on failure
 */
static bool object_expand_if_needed(ZenObject* object) {
    if (!object) {
        return false;
    }
    
    if (object->length < object->capacity) {
        return true; // No expansion needed
    }
    
    // Double the capacity
    size_t new_capacity = object->capacity * 2;
    ZenObjectPair* new_pairs = memory_realloc(object->pairs, 
                                            sizeof(ZenObjectPair) * new_capacity);
    if (!new_pairs) {
        return false; // Failed to expand
    }
    
    object->pairs = new_pairs;
    object->capacity = new_capacity;
    
    return true;
}

