#ifndef ZEN_PRIORITY_QUEUE_H
#define ZEN_PRIORITY_QUEUE_H

#include "value.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Priority queue data structure for ZEN
 * 
 * Provides a binary heap-based priority queue with O(log n) operations.
 * Higher priority values have precedence over lower values.
 * Supports any Value type with associated priority values.
 */

/**
 * @brief Create a new priority queue
 * @return New priority queue Value or NULL on failure
 */
Value* priority_queue_new(void);

/**
 * @brief Add item with priority to queue
 * @param queue Priority queue Value
 * @param item Item to add (will be copied)
 * @param priority Priority Value (number) - higher values have higher priority
 * @return Boolean Value indicating success
 */
Value* priority_queue_push(Value* queue, const Value* item, const Value* priority);

/**
 * @brief Remove and return highest priority item
 * @param queue Priority queue Value
 * @return Highest priority item or NULL Value if queue is empty
 */
Value* priority_queue_pop(Value* queue);

/**
 * @brief Peek at highest priority item without removing
 * @param queue Priority queue Value
 * @return Highest priority item or NULL Value if queue is empty
 */
Value* priority_queue_peek(const Value* queue);

/**
 * @brief Get number of items in queue
 * @param queue Priority queue Value
 * @return Number Value containing queue size
 */
Value* priority_queue_size(const Value* queue);

/**
 * @brief Check if priority queue is empty
 * @param queue Priority queue Value
 * @return Boolean Value indicating if queue is empty
 */
Value* priority_queue_is_empty(const Value* queue);

/**
 * @brief Clear all items from priority queue
 * @param queue Priority queue Value
 * @return Boolean Value indicating success
 */
Value* priority_queue_clear(Value* queue);

/**
 * @brief Convert priority queue to array (sorted by priority, highest first)
 * @param queue Priority queue Value
 * @return Array Value containing all items sorted by priority
 */
Value* priority_queue_to_array(const Value* queue);

// Internal helper functions (used by value system)

/**
 * @brief Initialize a priority queue structure
 * @return Newly allocated ZenPriorityQueue or NULL on failure
 */
ZenPriorityQueue* priority_queue_create(void);

/**
 * @brief Free priority queue structure and its contents
 * @param queue Priority queue structure to free
 */
void priority_queue_free(ZenPriorityQueue* queue);

/**
 * @brief Initialize a priority queue structure (datastructures naming)
 * @return Newly allocated ZenPriorityQueue or NULL on failure
 */
ZenPriorityQueue* datastructures_priority_queue_create(void);

/**
 * @brief Free priority queue structure and its contents (datastructures naming)
 * @param queue Priority queue structure to free
 */
void datastructures_priority_queue_free(ZenPriorityQueue* queue);

/**
 * @brief Ensure priority queue has enough capacity
 * @param queue Priority queue structure
 * @param min_capacity Minimum required capacity
 * @return true if successful, false on failure
 */
bool priority_queue_ensure_capacity(ZenPriorityQueue* queue, size_t min_capacity);

/**
 * @brief Heap up operation (restore heap property upward)
 * @param queue Priority queue structure
 * @param index Starting index for heap up
 */
void priority_queue_heap_up(ZenPriorityQueue* queue, size_t index);

/**
 * @brief Heap down operation (restore heap property downward)
 * @param queue Priority queue structure
 * @param index Starting index for heap down
 */
void priority_queue_heap_down(ZenPriorityQueue* queue, size_t index);

/**
 * @brief Get parent index in binary heap
 * @param index Child index
 * @return Parent index
 */
size_t priority_queue_parent(size_t index);

/**
 * @brief Get left child index in binary heap
 * @param index Parent index
 * @return Left child index
 */
size_t priority_queue_left_child(size_t index);

/**
 * @brief Get right child index in binary heap
 * @param index Parent index
 * @return Right child index
 */
size_t priority_queue_right_child(size_t index);

#endif