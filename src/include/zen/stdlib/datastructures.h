#ifndef ZEN_STDLIB_DATA_STRUCTURES_H
#define ZEN_STDLIB_DATA_STRUCTURES_H

#include "zen/types/value.h"

/**
 * @brief ZEN stdlib interface for advanced data structures
 * 
 * This file provides the stdlib wrapper functions for sets and priority queues
 * that are directly callable from ZEN language code.
 */

// Set stdlib functions (as specified in MANIFEST.json)

/**
 * @brief Create new set data structure
 * @return New set Value or error Value
 */
Value* datastructures_set_new(void);

/**
 * @brief Add item to set
 * @param set_value Set Value to add to
 * @param item_value Item to add
 * @return Boolean Value indicating success, or error Value
 */
Value* datastructures_set_add(Value* set_value, const Value* item_value);

/**
 * @brief Check if set contains item
 * @param set_value Set Value to check
 * @param item_value Item to look for
 * @return Boolean Value indicating presence, or error Value
 */
Value* datastructures_set_contains(Value* set_value, const Value* item_value);

/**
 * @brief Remove item from set
 * @param set_value Set Value to remove from
 * @param item_value Item to remove
 * @return Boolean Value indicating success, or error Value
 */
Value* datastructures_set_remove(Value* set_value, const Value* item_value);

/**
 * @brief Get size of set
 * @param set_value Set Value
 * @return Number Value containing size, or error Value
 */
Value* datastructures_set_size(Value* set_value);

/**
 * @brief Convert set to array
 * @param set_value Set Value
 * @return Array Value containing all items, or error Value
 */
Value* datastructures_set_to_array(Value* set_value);

/**
 * @brief Create union of two sets
 * @param set1_value First set Value
 * @param set2_value Second set Value
 * @return New set Value containing union, or error Value
 */
Value* datastructures_set_union(Value* set1_value, Value* set2_value);

/**
 * @brief Create intersection of two sets
 * @param set1_value First set Value
 * @param set2_value Second set Value
 * @return New set Value containing intersection, or error Value
 */
Value* datastructures_set_intersection(Value* set1_value, Value* set2_value);

/**
 * @brief Create difference of two sets
 * @param set1_value First set Value
 * @param set2_value Second set Value
 * @return New set Value containing difference, or error Value
 */
Value* datastructures_set_difference(Value* set1_value, Value* set2_value);

/**
 * @brief Check if first set is subset of second
 * @param subset_value Potential subset Value
 * @param superset_value Potential superset Value
 * @return Boolean Value indicating subset relationship, or error Value
 */
Value* datastructures_set_is_subset(Value* subset_value, Value* superset_value);

// Priority queue stdlib functions (as specified in MANIFEST.json)

/**
 * @brief Create new priority queue
 * @return New priority queue Value or error Value
 */
Value* datastructures_priority_queue_new(void);

/**
 * @brief Add item with priority to queue
 * @param queue_value Priority queue Value
 * @param item_value Item to add
 * @param priority_value Priority number Value
 * @return Boolean Value indicating success, or error Value
 */
Value* datastructures_priority_queue_push(Value* queue_value, const Value* item_value, const Value* priority_value);

/**
 * @brief Remove and return highest priority item
 * @param queue_value Priority queue Value
 * @return Highest priority item Value, or null/error Value
 */
Value* datastructures_priority_queue_pop(Value* queue_value);

/**
 * @brief Peek at highest priority item without removing
 * @param queue_value Priority queue Value
 * @return Highest priority item Value, or null/error Value
 */
Value* datastructures_priority_queue_peek(Value* queue_value);

/**
 * @brief Get size of priority queue
 * @param queue_value Priority queue Value
 * @return Number Value containing size, or error Value
 */
Value* datastructures_priority_queue_size(Value* queue_value);

/**
 * @brief Check if priority queue is empty
 * @param queue_value Priority queue Value
 * @return Boolean Value indicating emptiness, or error Value
 */
Value* datastructures_priority_queue_is_empty(Value* queue_value);

/**
 * @brief Clear all items from priority queue
 * @param queue_value Priority queue Value
 * @return Boolean Value indicating success, or error Value
 */
Value* datastructures_priority_queue_clear(Value* queue_value);

/**
 * @brief Convert priority queue to array
 * @param queue_value Priority queue Value
 * @return Array Value sorted by priority, or error Value
 */
Value* datastructures_priority_queue_to_array(Value* queue_value);

#endif