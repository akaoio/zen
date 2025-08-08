#ifndef ZEN_SET_H
#define ZEN_SET_H

#include <stdbool.h>
#include <stddef.h>

#include "value.h"

/**
 * @brief Set data structure for ZEN
 *
 * Provides a hash-based set implementation with O(1) average operations.
 * Uses the existing ZEN object hash table infrastructure for optimal performance.
 * Supports any Value type as set elements.
 */

/**
 * @brief Create a new set
 * @return New set Value or NULL on failure
 */
Value *set_new(void);

/**
 * @brief Add item to set
 * @param set Set Value
 * @param item Item to add (will be copied)
 * @return Boolean Value indicating success (true if item was added, false if already present)
 */
Value *set_add(Value *set, const Value *item);

/**
 * @brief Check if set contains item
 * @param set Set Value
 * @param item Item to check for
 * @return Boolean Value indicating if item is present
 */
Value *set_contains(const Value *set, const Value *item);

/**
 * @brief Remove item from set
 * @param set Set Value
 * @param item Item to remove
 * @return Boolean Value indicating success (true if item was removed, false if not present)
 */
Value *set_remove(Value *set, const Value *item);

/**
 * @brief Get number of items in set
 * @param set Set Value
 * @return Number Value containing set size
 */
Value *set_size(const Value *set);

/**
 * @brief Convert set to array
 * @param set Set Value
 * @return Array Value containing all set items or NULL on failure
 */
Value *set_to_array(const Value *set);

/**
 * @brief Create union of two sets
 * @param set1 First set Value
 * @param set2 Second set Value
 * @return New set Value containing union of both sets
 */
Value *set_union(const Value *set1, const Value *set2);

/**
 * @brief Create intersection of two sets
 * @param set1 First set Value
 * @param set2 Second set Value
 * @return New set Value containing intersection of both sets
 */
Value *set_intersection(const Value *set1, const Value *set2);

/**
 * @brief Create difference of two sets (set1 - set2)
 * @param set1 First set Value
 * @param set2 Second set Value
 * @return New set Value containing items in set1 but not in set2
 */
Value *set_difference(const Value *set1, const Value *set2);

/**
 * @brief Check if first set is subset of second
 * @param subset Potential subset Value
 * @param superset Potential superset Value
 * @return Boolean Value indicating if subset is contained in superset
 */
Value *set_is_subset(const Value *subset, const Value *superset);

// Internal helper functions (used by value system)

/**
 * @brief Initialize a set structure
 * @return Newly allocated ZenSet or NULL on failure
 */
ZenSet *set_create(void);

/**
 * @brief Free set structure and its contents
 * @param set Set structure to free
 */
void set_free(ZenSet *set);

/**
 * @brief Initialize a set structure (datastructures naming)
 * @return Newly allocated ZenSet or NULL on failure
 */
ZenSet *datastructures_set_create(void);

/**
 * @brief Free set structure and its contents (datastructures naming)
 * @param set Set structure to free
 */
void datastructures_set_free(ZenSet *set);

/**
 * @brief Generate hash key for value (for internal use)
 * @param value Value to hash
 * @return String hash key (caller must free)
 */
char *set_value_to_key(const Value *value);

#endif