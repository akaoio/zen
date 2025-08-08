#ifndef ZEN_TYPES_ARRAY_H
#define ZEN_TYPES_ARRAY_H

#include "zen/types/value.h"

#include <stddef.h>

// Function pointer typedefs for array operations
typedef Value *(*ArrayPredicate)(Value *item);
typedef Value *(*ArrayTransform)(Value *item);

/**
 * @brief Create new array
 * @param initial_capacity Initial capacity for the array (minimum 1)
 * @return Pointer to new array Value, or NULL on failure
 */
Value *array_new(size_t initial_capacity);

/**
 * @brief Add item to end of array
 * @param array Array Value to push to
 * @param item Value to add to the array
 */
void array_push(Value *array, Value *item);

/**
 * @brief Remove and return last item
 * @param array Array Value to pop from
 * @return Last item in array, or NULL if empty or invalid
 */
Value *array_pop(Value *array);

/**
 * @brief Get item at index
 * @param array Array Value to get from
 * @param index Zero-based index of item to get
 * @return Item at index, or NULL if out of bounds or invalid
 */
Value *array_get(Value *array, size_t index);

/**
 * @brief Set item at index
 * @param array Array Value to set in
 * @param index Zero-based index to set at
 * @param item Value to set at the index
 */
void array_set(Value *array, size_t index, Value *item);

/**
 * @brief Get array length
 * @param array Array Value to get length of
 * @return Length of array, or 0 if invalid
 */
size_t array_length(const Value *array);

/**
 * @brief Create deep copy of array and all nested structures
 * @param array Array Value to deep clone
 * @return Deep copy of array, or NULL on failure
 */
Value *array_deep_clone(const Value *array);

/**
 * @brief Concatenate two arrays into new array
 * @param array1 First array
 * @param array2 Second array
 * @return New array containing elements from both arrays, or NULL on failure
 */
Value *array_concat(const Value *array1, const Value *array2);

/**
 * @brief Create subarray from start to end indices (Python-style)
 * @param array Source array
 * @param start Start index (inclusive, supports negative indices)
 * @param end End index (exclusive, supports negative indices)
 * @return New array containing slice, or NULL on failure
 */
Value *array_slice(const Value *array, int start, int end);

/**
 * @brief Check if array contains specific item
 * @param array Array to search in
 * @param item Item to search for
 * @return true if item found, false otherwise
 */
bool array_contains(const Value *array, const Value *item);

/**
 * @brief Filter array elements using predicate function
 * @param array Source array to filter
 * @param predicate Function that returns truthy value for items to keep
 * @return New array with filtered elements, or NULL on failure
 */
Value *array_filter(const Value *array, ArrayPredicate predicate);

/**
 * @brief Transform array elements using mapping function
 * @param array Source array to transform
 * @param transform Function to transform each element
 * @return New array with transformed elements, or NULL on failure
 */
Value *array_map(const Value *array, ArrayTransform transform);

/**
 * @brief Join array elements into string with separator
 * @param array Array to join
 * @param separator String separator between elements
 * @return New string Value with joined elements, or NULL on failure
 */
Value *array_join(const Value *array, const char *separator);

#endif  // ZEN_TYPES_ARRAY_H