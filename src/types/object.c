/*
 * object.c
 * Object/hash map implementation
 *
 * This file is auto-generated from MANIFEST.json
 * DO NOT modify function signatures without updating the manifest
 */

#include "zen/core/memory.h"
#include "zen/types/array.h"
#include "zen/types/value.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for internal functions
static int object_find_key_index(const ZenObject *object, const char *key);
static bool object_expand_if_needed(ZenObject *object);

/**
 * @brief Create new object
 * @return New empty object Value, or NULL on failure
 */
Value *object_new(void)
{
    // Allocate the Value struct
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    // Allocate the ZenObject struct
    ZenObject *object = memory_alloc(sizeof(ZenObject));
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
void object_set(Value *object, const char *key, Value *value)
{
    if (!object || object->type != VALUE_OBJECT || !key || !value) {
        return;
    }

    ZenObject *zen_object = object->as.object;
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
            return;  // Failed to expand
        }

        // Add new key-value pair
        size_t new_index = zen_object->length;
        zen_object->pairs[new_index].key = memory_strdup(key);
        if (!zen_object->pairs[new_index].key) {
            return;  // Failed to duplicate key string
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
Value *object_get(Value *object, const char *key)
{
    if (!object || object->type != VALUE_OBJECT || !key) {
        return NULL;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }

    int index = object_find_key_index(zen_object, key);
    if (index >= 0) {
        // Return value with incremented reference count
        return value_ref(zen_object->pairs[index].value);
    }

    return NULL;  // Key not found
}

/**
 * @brief Check if key exists
 * @param object Object Value to check
 * @param key String key to check for
 * @return true if key exists, false otherwise
 */
bool object_has(Value *object, const char *key)
{
    if (!object || object->type != VALUE_OBJECT || !key) {
        return false;
    }

    ZenObject *zen_object = object->as.object;
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
void object_delete(Value *object, const char *key)
{
    if (!object || object->type != VALUE_OBJECT || !key) {
        return;
    }

    ZenObject *zen_object = object->as.object;
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
Value *object_keys(Value *object)
{
    if (!object || object->type != VALUE_OBJECT) {
        return NULL;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }

    // Create new array for keys
    Value *keys_array = array_new(zen_object->length);
    if (!keys_array) {
        return NULL;
    }

    // Add each key as a string value to the array
    for (size_t i = 0; i < zen_object->length; i++) {
        Value *key_value = value_new_string(zen_object->pairs[i].key);
        if (!key_value) {
            // Cleanup and return NULL on failure
            value_unref(keys_array);
            return NULL;
        }

        array_push(keys_array, key_value);
        value_unref(key_value);  // array_push increments ref count
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
static int object_find_key_index(const ZenObject *object, const char *key)
{
    if (!object || !key) {
        return -1;
    }

    for (size_t i = 0; i < object->length; i++) {
        if (object->pairs[i].key && strcmp(object->pairs[i].key, key) == 0) {
            return (int)i;
        }
    }

    return -1;  // Not found
}

/**
 * @brief Expand object capacity if needed
 * @param object ZenObject to potentially expand
 * @return true if expansion succeeded or wasn't needed, false on failure
 */
static bool object_expand_if_needed(ZenObject *object)
{
    if (!object) {
        return false;
    }

    if (object->length < object->capacity) {
        return true;  // No expansion needed
    }

    // Double the capacity
    size_t new_capacity = object->capacity * 2;
    ZenObjectPair *new_pairs = memory_realloc(object->pairs, sizeof(ZenObjectPair) * new_capacity);
    if (!new_pairs) {
        return false;  // Failed to expand
    }

    object->pairs = new_pairs;
    object->capacity = new_capacity;

    return true;
}

/**
 * @brief Create deep copy of object and all nested structures
 * @param object Object Value to deep copy
 * @return Deep copy of the object or NULL on failure
 */
Value *object_deep_clone(const Value *object)
{
    if (!object || object->type != VALUE_OBJECT) {
        return NULL;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }

    // Create new object
    Value *new_obj = object_new();
    if (!new_obj) {
        return NULL;
    }

    // Deep clone each key-value pair
    for (size_t i = 0; i < zen_object->length; i++) {
        const char *key = zen_object->pairs[i].key;
        Value *value = zen_object->pairs[i].value;

        if (!key || !value) {
            continue;
        }

        // Deep copy the value
        Value *cloned_value = value_copy(value);
        if (!cloned_value) {
            // Cleanup on failure
            value_unref(new_obj);
            return NULL;
        }

        // Set the cloned value in new object
        object_set(new_obj, key, cloned_value);
        value_unref(cloned_value);  // object_set increments ref count
    }

    return new_obj;
}

/**
 * @brief Merge two objects, right overwrites left on key conflicts
 * @param left Left object for merging
 * @param right Right object for merging (overwrites left on conflicts)
 * @return New merged object or NULL on failure
 */
Value *object_merge(const Value *left, const Value *right)
{
    if (!left || left->type != VALUE_OBJECT || !right || right->type != VALUE_OBJECT) {
        return NULL;
    }

    ZenObject *left_obj = left->as.object;
    ZenObject *right_obj = right->as.object;
    if (!left_obj || !right_obj) {
        return NULL;
    }

    // Create new object for result
    Value *result = object_new();
    if (!result) {
        return NULL;
    }

    // Copy all key-value pairs from left object
    for (size_t i = 0; i < left_obj->length; i++) {
        const char *key = left_obj->pairs[i].key;
        Value *value = left_obj->pairs[i].value;

        if (!key || !value) {
            continue;
        }

        object_set(result, key, value);
    }

    // Copy all key-value pairs from right object (overwrites left)
    for (size_t i = 0; i < right_obj->length; i++) {
        const char *key = right_obj->pairs[i].key;
        Value *value = right_obj->pairs[i].value;

        if (!key || !value) {
            continue;
        }

        object_set(result, key, value);
    }

    return result;
}

// Helper function for splitting path by dots
static char **split_path(const char *path, size_t *count)
{
    if (!path || !count) {
        return NULL;
    }

    // Count dots to determine array size
    size_t dot_count = 0;
    for (const char *p = path; *p; p++) {
        if (*p == '.')
            dot_count++;
    }
    *count = dot_count + 1;

    // Allocate array of string pointers
    char **parts = memory_alloc(sizeof(char *) * (*count));
    if (!parts) {
        return NULL;
    }

    // Split the path
    size_t index = 0;
    const char *start = path;
    const char *current = path;

    while (*current) {
        if (*current == '.') {
            // Found a dot, copy the part
            size_t len = current - start;
            if (len > 0) {
                parts[index] = memory_alloc(len + 1);
                if (parts[index]) {
                    memcpy(parts[index], start, len);
                    parts[index][len] = '\0';
                }
                index++;
            }
            start = current + 1;
        }
        current++;
    }

    // Handle the last part
    if (start < current) {
        size_t len = current - start;
        parts[index] = memory_alloc(len + 1);
        if (parts[index]) {
            memcpy(parts[index], start, len);
            parts[index][len] = '\0';
        }
        index++;
    }

    *count = index;
    return parts;
}

// Helper function to free split path array
static void free_path_parts(char **parts, size_t count)
{
    if (!parts)
        return;

    for (size_t i = 0; i < count; i++) {
        if (parts[i]) {
            memory_free(parts[i]);
        }
    }
    memory_free(parts);
}

/**
 * @brief Get value using dot notation path (e.g., 'user.profile.name')
 * @param object Object Value to get from
 * @param path Dot notation path string
 * @return Value at path or NULL if not found
 */
Value *object_get_path(const Value *object, const char *path)
{
    if (!object || object->type != VALUE_OBJECT || !path) {
        return NULL;
    }

    size_t part_count = 0;
    char **path_parts = split_path(path, &part_count);
    if (!path_parts || part_count == 0) {
        return NULL;
    }

    Value *current = (Value *)object;  // Start from root object
    Value *result = NULL;

    // Traverse the path
    for (size_t i = 0; i < part_count; i++) {
        if (!current || current->type != VALUE_OBJECT) {
            break;  // Can't traverse further
        }

        // Get the next level
        Value *next = object_get(current, path_parts[i]);

        // If this is not the first iteration, we need to unref the previous current
        if (i > 0 && current != object) {
            value_unref(current);
        }

        current = next;

        // If we reached the end, this is our result
        if (i == part_count - 1) {
            result = current;  // Don't unref - this is our return value
        }
    }

    free_path_parts(path_parts, part_count);
    return result;
}

/**
 * @brief Set value using dot notation path, creating intermediate objects
 * @param object Object Value to set in
 * @param path Dot notation path string
 * @param value Value to set at path
 * @return true if successful, false on failure
 */
bool object_set_path(Value *object, const char *path, Value *value)
{
    if (!object || object->type != VALUE_OBJECT || !path || !value) {
        return false;
    }

    size_t part_count = 0;
    char **path_parts = split_path(path, &part_count);
    if (!path_parts || part_count == 0) {
        return false;
    }

    Value *current = object;

    // Traverse/create path up to the last element
    for (size_t i = 0; i < part_count - 1; i++) {
        Value *next = object_get(current, path_parts[i]);

        if (!next) {
            // Create intermediate object
            next = object_new();
            if (!next) {
                free_path_parts(path_parts, part_count);
                return false;
            }
            object_set(current, path_parts[i], next);
            value_unref(next);  // object_set increments ref count
        } else if (next->type != VALUE_OBJECT) {
            // Path exists but is not an object - cannot traverse
            value_unref(next);
            free_path_parts(path_parts, part_count);
            return false;
        }

        // Only unref if this wasn't the original object
        if (current != object) {
            value_unref(current);
        }
        current = next;
    }

    // Set the final value
    object_set(current, path_parts[part_count - 1], value);

    // Cleanup
    if (current != object) {
        value_unref(current);
    }
    free_path_parts(path_parts, part_count);

    return true;
}

/**
 * @brief Get array of all values in object
 * @param object Object Value to get values from
 * @return Array Value containing all values or NULL on failure
 */
Value *object_values(const Value *object)
{
    if (!object || object->type != VALUE_OBJECT) {
        return NULL;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }

    // Create new array for values
    Value *values_array = array_new(zen_object->length);
    if (!values_array) {
        return NULL;
    }

    // Add each value to the array
    for (size_t i = 0; i < zen_object->length; i++) {
        Value *value = zen_object->pairs[i].value;
        if (value) {
            array_push(values_array, value);
        }
    }

    return values_array;
}

/**
 * @brief Get array of [key, value] pairs for iteration
 * @param object Object Value to get entries from
 * @return Array Value containing [key, value] arrays or NULL on failure
 */
Value *object_entries(const Value *object)
{
    if (!object || object->type != VALUE_OBJECT) {
        return NULL;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return NULL;
    }

    // Create new array for entries
    Value *entries_array = array_new(zen_object->length);
    if (!entries_array) {
        return NULL;
    }

    // Add each [key, value] pair as a sub-array
    for (size_t i = 0; i < zen_object->length; i++) {
        const char *key = zen_object->pairs[i].key;
        Value *value = zen_object->pairs[i].value;

        if (!key || !value) {
            continue;
        }

        // Create [key, value] array
        Value *entry_array = array_new(2);
        if (!entry_array) {
            value_unref(entries_array);
            return NULL;
        }

        // Add key as string
        Value *key_value = value_new_string(key);
        if (!key_value) {
            value_unref(entry_array);
            value_unref(entries_array);
            return NULL;
        }
        array_push(entry_array, key_value);
        value_unref(key_value);

        // Add value
        array_push(entry_array, value);

        // Add entry to main array
        array_push(entries_array, entry_array);
        value_unref(entry_array);
    }

    return entries_array;
}

/**
 * @brief Get number of properties in object
 * @param object Object Value to count
 * @return Number of properties in object
 */
size_t object_size(const Value *object)
{
    if (!object || object->type != VALUE_OBJECT) {
        return 0;
    }

    ZenObject *zen_object = object->as.object;
    if (!zen_object) {
        return 0;
    }

    return zen_object->length;
}
