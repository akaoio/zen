#include "zen/types/set.h"
#include "zen/types/object.h"
#include "zen/types/array.h"
#include "zen/core/memory.h"
#include "zen/core/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

/**
 * @file set.c
 * @brief Set data structure implementation using hash table
 * 
 * Sets are implemented as hash tables using ZEN's object infrastructure.
 * Each unique value is stored as a key in the object with a dummy value.
 * This provides O(1) average-case operations for add, contains, and remove.
 */

// Initial capacity for set operations
#define SET_INITIAL_CAPACITY 16

/**
 * @brief Initialize a set structure
 */
ZenSet* datastructures_set_create(void) {
    ZenSet* set = (ZenSet*)memory_alloc(sizeof(ZenSet));
    if (!set) {
        return NULL;
    }

    // Use an object as the underlying hash table
    set->hash_table = value_new(VALUE_OBJECT);
    if (!set->hash_table) {
        memory_free(set);
        return NULL;
    }

    value_ref(set->hash_table); // Ensure hash table stays alive
    set->size = 0;

    return set;
}

/**
 * @brief Create a new set
 */
Value* set_new(void) {
    // Create a new set value
    Value* set_val = value_new(VALUE_SET);
    if (!set_val) {
        return error_memory_allocation();
    }

    // Initialize the set structure
    ZenSet* set = datastructures_set_create();
    if (!set) {
        value_unref(set_val);
        return error_memory_allocation();
    }

    set_val->as.set = set;
    return set_val;
}

/**
 * @brief Free set structure and its contents
 */
void datastructures_set_free(ZenSet* set) {
    if (!set) return;

    if (set->hash_table) {
        value_unref(set->hash_table);
    }
    
    memory_free(set);
}

/**
 * @brief Generate hash key for value
 */
char* datastructures_set_value_to_key(const Value* value) {
    if (!value) {
        return memory_strdup("null");
    }

    char* key = NULL;
    size_t key_size = 64; // Initial buffer size

    switch (value->type) {
        case VALUE_NULL:
            key = memory_strdup("null");
            break;

        case VALUE_BOOLEAN:
            key = memory_strdup(value->as.boolean ? "true" : "false");
            break;

        case VALUE_NUMBER: {
            key = (char*)memory_alloc(key_size);
            if (key) {
                // Handle integer vs float representation
                double num = value->as.number;
                if (num == floor(num) && num >= INT_MIN && num <= INT_MAX) {
                    snprintf(key, key_size, "n:%.0f", num);
                } else {
                    snprintf(key, key_size, "n:%.17g", num);
                }
            }
            break;
        }

        case VALUE_STRING: {
            const char* str = value->as.string->data;
            size_t str_len = value->as.string->length;
            key_size = str_len + 3; // "s:" prefix + null terminator
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "s:%s", str);
            }
            break;
        }

        case VALUE_ARRAY: {
            // Arrays are compared by reference, not content
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "a:%p", (void*)value);
            }
            break;
        }

        case VALUE_OBJECT: {
            // Objects are compared by reference, not content
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "o:%p", (void*)value);
            }
            break;
        }

        case VALUE_SET: {
            // Sets are compared by reference
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "set:%p", (void*)value);
            }
            break;
        }

        case VALUE_PRIORITY_QUEUE: {
            // Priority queues are compared by reference
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "pq:%p", (void*)value);
            }
            break;
        }

        default: {
            // Other types compared by reference
            key = (char*)memory_alloc(key_size);
            if (key) {
                snprintf(key, key_size, "ref:%p", (void*)value);
            }
            break;
        }
    }

    return key;
}

/**
 * @brief Add item to set
 */
Value* set_add(Value* set, const Value* item) {
    if (!set || set->type != VALUE_SET || !item) {
        return error_invalid_argument("set_add", "set and item required");
    }

    ZenSet* set_data = set->as.set;
    if (!set_data) {
        return error_null_pointer("Set data is null");
    }

    // Generate hash key for the item
    char* key = datastructures_set_value_to_key(item);
    if (!key) {
        return error_memory_allocation();
    }

    // Check if item already exists
    Value* existing = object_get(set_data->hash_table, key);
    if (existing) {
        // Item already in set
        memory_free(key);
        return value_new_boolean(false);
    }

    // Add item to hash table with dummy value
    Value* dummy = value_new_boolean(true);
    if (!dummy) {
        memory_free(key);
        return error_memory_allocation();
    }

    object_set(set_data->hash_table, key, dummy);
    value_unref(dummy);
    memory_free(key);

    set_data->size++;
    return value_new_boolean(true);
}

/**
 * @brief Check if set contains item
 */
Value* set_contains(const Value* set, const Value* item) {
    if (!set || set->type != VALUE_SET || !item) {
        return error_invalid_argument("set_contains", "set and item required");
    }

    ZenSet* set_data = set->as.set;
    if (!set_data) {
        return error_null_pointer("Set data is null");
    }

    // Generate hash key for the item
    char* key = datastructures_set_value_to_key(item);
    if (!key) {
        return error_memory_allocation();
    }

    // Check if key exists in hash table
    Value* exists = object_get(set_data->hash_table, key);
    memory_free(key);

    return value_new_boolean(exists != NULL);
}

/**
 * @brief Remove item from set
 */
Value* set_remove(Value* set, const Value* item) {
    if (!set || set->type != VALUE_SET || !item) {
        return error_invalid_argument("set_remove", "set and item required");
    }

    ZenSet* set_data = set->as.set;
    if (!set_data) {
        return error_null_pointer("Set data is null");
    }

    // Generate hash key for the item
    char* key = datastructures_set_value_to_key(item);
    if (!key) {
        return error_memory_allocation();
    }

    // Check if item exists before removing
    Value* existing = object_get(set_data->hash_table, key);
    if (!existing) {
        // Item not in set
        memory_free(key);
        return value_new_boolean(false);
    }

    // Remove from hash table and check if successful
    bool removed = object_delete(set_data->hash_table, key);
    memory_free(key);

    if (removed) {
        set_data->size--;
        return value_new_boolean(true);
    }

    return value_new_boolean(false);
}

/**
 * @brief Get number of items in set
 */
Value* set_size(const Value* set) {
    if (!set || set->type != VALUE_SET) {
        return error_invalid_argument("set_size", "set required");
    }

    ZenSet* set_data = set->as.set;
    if (!set_data) {
        return error_null_pointer("Set data is null");
    }

    return value_new_number((double)set_data->size);
}

/**
 * @brief Convert set to array
 */
Value* set_to_array(const Value* set) {
    if (!set || set->type != VALUE_SET) {
        return error_invalid_argument("set_to_array", "set required");
    }

    ZenSet* set_data = set->as.set;
    if (!set_data) {
        return error_null_pointer("Set data is null");
    }

    // Get keys array from the underlying object
    Value* keys = object_keys(set_data->hash_table);
    if (!keys || keys->type != VALUE_ARRAY) {
        if (keys) value_unref(keys);
        return error_memory_allocation();
    }

    // Create result array
    Value* result = value_new(VALUE_ARRAY);
    if (!result) {
        value_unref(keys);
        return error_memory_allocation();
    }

    ZenArray* keys_array = keys->as.array;
    // result_array not needed since we use array_push

    // Convert each key back to original value
    for (size_t i = 0; i < keys_array->length; i++) {
        Value* key_val = keys_array->items[i];
        if (key_val->type != VALUE_STRING) continue;

        const char* key_str = key_val->as.string->data;
        
        // Parse key to reconstruct original value
        Value* original_val = NULL;
        
        if (strcmp(key_str, "null") == 0) {
            original_val = value_new_null();
        } else if (strcmp(key_str, "true") == 0) {
            original_val = value_new_boolean(true);
        } else if (strcmp(key_str, "false") == 0) {
            original_val = value_new_boolean(false);
        } else if (strncmp(key_str, "n:", 2) == 0) {
            double num = strtod(key_str + 2, NULL);
            original_val = value_new_number(num);
        } else if (strncmp(key_str, "s:", 2) == 0) {
            original_val = value_new_string(key_str + 2);
        } else {
            // For reference types, we can't reconstruct the original
            // So we store a string representation instead
            original_val = value_new_string(key_str);
        }

        if (original_val) {
            array_push(result, original_val);
            value_unref(original_val);
        }
    }

    value_unref(keys);
    return result;
}

/**
 * @brief Create union of two sets
 */
Value* set_union(const Value* set1, const Value* set2) {
    if (!set1 || set1->type != VALUE_SET || !set2 || set2->type != VALUE_SET) {
        return error_invalid_argument("set_union", "two sets required");
    }

    // Create new set for result
    Value* result = set_new();
    if (error_is_error(result)) {
        return result;
    }

    // Add all items from set1
    Value* array1 = set_to_array(set1);
    if (array1 && array1->type == VALUE_ARRAY) {
        ZenArray* arr_data = array1->as.array;
        for (size_t i = 0; i < arr_data->length; i++) {
            Value* success = set_add(result, arr_data->items[i]);
            if (success) value_unref(success);
        }
        value_unref(array1);
    }

    // Add all items from set2
    Value* array2 = set_to_array(set2);
    if (array2 && array2->type == VALUE_ARRAY) {
        ZenArray* arr_data = array2->as.array;
        for (size_t i = 0; i < arr_data->length; i++) {
            Value* success = set_add(result, arr_data->items[i]);
            if (success) value_unref(success);
        }
        value_unref(array2);
    }

    return result;
}

/**
 * @brief Create intersection of two sets
 */
Value* set_intersection(const Value* set1, const Value* set2) {
    if (!set1 || set1->type != VALUE_SET || !set2 || set2->type != VALUE_SET) {
        return error_invalid_argument("set_intersection", "two sets required");
    }

    // Create new set for result
    Value* result = set_new();
    if (error_is_error(result)) {
        return result;
    }

    // Add items from set1 that are also in set2
    Value* array1 = set_to_array(set1);
    if (array1 && array1->type == VALUE_ARRAY) {
        ZenArray* arr_data = array1->as.array;
        for (size_t i = 0; i < arr_data->length; i++) {
            Value* item = arr_data->items[i];
            Value* in_set2 = set_contains(set2, item);
            
            if (in_set2 && in_set2->type == VALUE_BOOLEAN && in_set2->as.boolean) {
                Value* success = set_add(result, item);
                if (success) value_unref(success);
            }
            
            if (in_set2) value_unref(in_set2);
        }
        value_unref(array1);
    }

    return result;
}

/**
 * @brief Create difference of two sets (set1 - set2)
 */
Value* set_difference(const Value* set1, const Value* set2) {
    if (!set1 || set1->type != VALUE_SET || !set2 || set2->type != VALUE_SET) {
        return error_invalid_argument("set_difference", "two sets required");
    }

    // Create new set for result
    Value* result = set_new();
    if (error_is_error(result)) {
        return result;
    }

    // Add items from set1 that are not in set2
    Value* array1 = set_to_array(set1);
    if (array1 && array1->type == VALUE_ARRAY) {
        ZenArray* arr_data = array1->as.array;
        for (size_t i = 0; i < arr_data->length; i++) {
            Value* item = arr_data->items[i];
            Value* in_set2 = set_contains(set2, item);
            
            if (in_set2 && in_set2->type == VALUE_BOOLEAN && !in_set2->as.boolean) {
                Value* success = set_add(result, item);
                if (success) value_unref(success);
            }
            
            if (in_set2) value_unref(in_set2);
        }
        value_unref(array1);
    }

    return result;
}

/**
 * @brief Check if first set is subset of second
 */
Value* set_is_subset(const Value* subset, const Value* superset) {
    if (!subset || subset->type != VALUE_SET || !superset || superset->type != VALUE_SET) {
        return error_invalid_argument("set_is_subset", "two sets required");
    }

    // Check if every item in subset is in superset
    Value* array = set_to_array(subset);
    if (!array || array->type != VALUE_ARRAY) {
        if (array) value_unref(array);
        return error_memory_allocation();
    }

    bool is_subset = true;
    ZenArray* arr_data = array->as.array;
    
    for (size_t i = 0; i < arr_data->length && is_subset; i++) {
        Value* item = arr_data->items[i];
        Value* in_superset = set_contains(superset, item);
        
        if (!in_superset || in_superset->type != VALUE_BOOLEAN || !in_superset->as.boolean) {
            is_subset = false;
        }
        
        if (in_superset) value_unref(in_superset);
    }

    value_unref(array);
    return value_new_boolean(is_subset);
}