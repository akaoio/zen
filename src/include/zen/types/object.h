#ifndef ZEN_TYPES_OBJECT_H
#define ZEN_TYPES_OBJECT_H

#include "zen/types/value.h"
#include <stdbool.h>

/**
 * @brief Create new object
 * @return New empty object Value, or NULL on failure
 */
Value* object_new(void);

/**
 * @brief Set key-value pair
 * @param object Object Value to set in
 * @param key String key to set
 * @param value Value to associate with key
 */
void object_set(Value* object, const char* key, Value* value);

/**
 * @brief Get value by key
 * @param object Object Value to get from
 * @param key String key to look up
 * @return Value associated with key, or NULL if not found
 */
Value* object_get(Value* object, const char* key);

/**
 * @brief Check if key exists
 * @param object Object Value to check
 * @param key String key to check for
 * @return true if key exists, false otherwise
 */
bool object_has(Value* object, const char* key);

/**
 * @brief Remove key-value pair
 * @param object Object Value to remove from
 * @param key String key to remove
 */
void object_delete(Value* object, const char* key);

/**
 * @brief Get array of keys
 * @param object Object Value to get keys from
 * @return Array Value containing all keys, or NULL on failure
 */
Value* object_keys(Value* object);

/**
 * @brief Create deep copy of object and all nested structures
 * @param object Object Value to deep copy
 * @return Deep copy of the object or NULL on failure
 */
Value* object_deep_clone(const Value* object);

/**
 * @brief Merge two objects, right overwrites left on key conflicts
 * @param left Left object for merging
 * @param right Right object for merging (overwrites left on conflicts)
 * @return New merged object or NULL on failure
 */
Value* object_merge(const Value* left, const Value* right);

/**
 * @brief Get value using dot notation path (e.g., 'user.profile.name')
 * @param object Object Value to get from
 * @param path Dot notation path string
 * @return Value at path or NULL if not found
 */
Value* object_get_path(const Value* object, const char* path);

/**
 * @brief Set value using dot notation path, creating intermediate objects
 * @param object Object Value to set in
 * @param path Dot notation path string
 * @param value Value to set at path
 * @return true if successful, false on failure
 */
bool object_set_path(Value* object, const char* path, Value* value);

/**
 * @brief Get array of all values in object
 * @param object Object Value to get values from
 * @return Array Value containing all values or NULL on failure
 */
Value* object_values(const Value* object);

/**
 * @brief Get array of [key, value] pairs for iteration
 * @param object Object Value to get entries from
 * @return Array Value containing [key, value] arrays or NULL on failure
 */
Value* object_entries(const Value* object);

/**
 * @brief Get number of properties in object
 * @param object Object Value to count
 * @return Number of properties in object
 */
size_t object_size(const Value* object);

#endif // ZEN_TYPES_OBJECT_H