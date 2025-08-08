#include "zen/types/priority_queue.h"
#include "zen/types/array.h"
#include "zen/core/memory.h"
#include "zen/core/error.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @file priority_queue.c
 * @brief Priority queue implementation using binary heap
 * 
 * Priority queues are implemented as binary max heaps stored in a dynamic array.
 * Higher priority values have precedence over lower values.
 * Operations maintain the heap property for efficient O(log n) insertion and removal.
 */

// Initial capacity for priority queue
#define PRIORITY_QUEUE_INITIAL_CAPACITY 16
#define PRIORITY_QUEUE_GROWTH_FACTOR 2

/**
 * @brief Initialize a priority queue structure
 * @return New ZenPriorityQueue instance or NULL on memory allocation failure
 */
ZenPriorityQueue* datastructures_priority_queue_create(void) {
    ZenPriorityQueue* queue = (ZenPriorityQueue*)memory_alloc(sizeof(ZenPriorityQueue));
    if (!queue) {
        return NULL;
    }

    // Allocate initial array
    queue->items = (ZenPriorityItem*)memory_alloc(sizeof(ZenPriorityItem) * PRIORITY_QUEUE_INITIAL_CAPACITY);
    if (!queue->items) {
        memory_free(queue);
        return NULL;
    }

    queue->length = 0;
    queue->capacity = PRIORITY_QUEUE_INITIAL_CAPACITY;

    return queue;
}

/**
 * @brief Create a new priority queue
 * @return New priority queue Value or error Value on failure
 */
Value* priority_queue_new(void) {
    // Create a new priority queue value
    Value* queue_val = value_new(VALUE_PRIORITY_QUEUE);
    if (!queue_val) {
        return error_memory_allocation();
    }

    // Initialize the priority queue structure
    ZenPriorityQueue* queue = datastructures_priority_queue_create();
    if (!queue) {
        value_unref(queue_val);
        return error_memory_allocation();
    }

    queue_val->as.priority_queue = queue;
    return queue_val;
}

/**
 * @brief Free priority queue structure and its contents
 * @param queue Priority queue structure to free (may be NULL)
 */
void datastructures_priority_queue_free(ZenPriorityQueue* queue) {
    if (!queue) return;

    // Free all stored values
    for (size_t i = 0; i < queue->length; i++) {
        if (queue->items[i].value) {
            value_unref(queue->items[i].value);
        }
    }

    if (queue->items) {
        memory_free(queue->items);
    }
    
    memory_free(queue);
}

/**
 * @brief Ensure priority queue has enough capacity
 * @param queue Priority queue to resize
 * @param min_capacity Minimum required capacity
 * @return true if capacity is ensured, false on memory allocation failure
 */
bool datastructures_priority_queue_ensure_capacity(ZenPriorityQueue* queue, size_t min_capacity) {
    if (!queue) return false;
    
    if (queue->capacity >= min_capacity) {
        return true;
    }

    size_t new_capacity = queue->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= PRIORITY_QUEUE_GROWTH_FACTOR;
    }

    ZenPriorityItem* new_items = (ZenPriorityItem*)memory_realloc(
        queue->items, 
        sizeof(ZenPriorityItem) * new_capacity
    );
    
    if (!new_items) {
        return false;
    }

    queue->items = new_items;
    queue->capacity = new_capacity;
    return true;
}

/**
 * @brief Get parent index in binary heap
 * @param index Child index
 * @return Parent index in the heap
 */
size_t datastructures_priority_queue_parent(size_t index) {
    return (index - 1) / 2;
}

/**
 * @brief Get left child index in binary heap
 * @param index Parent index
 * @return Left child index in the heap
 */
size_t datastructures_priority_queue_left_child(size_t index) {
    return 2 * index + 1;
}

/**
 * @brief Get right child index in binary heap
 * @param index Parent index
 * @return Right child index in the heap
 */
size_t datastructures_priority_queue_right_child(size_t index) {
    return 2 * index + 2;
}

/**
 * @brief Heap up operation (restore heap property upward)
 * @param queue Priority queue to operate on
 * @param index Starting index to heap up from
 */
void datastructures_priority_queue_heap_up(ZenPriorityQueue* queue, size_t index) {
    while (index > 0) {
        size_t parent_index = datastructures_priority_queue_parent(index);
        
        // If parent has higher or equal priority, heap property is satisfied
        if (queue->items[parent_index].priority >= queue->items[index].priority) {
            break;
        }

        // Swap with parent
        ZenPriorityItem temp = queue->items[index];
        queue->items[index] = queue->items[parent_index];
        queue->items[parent_index] = temp;

        index = parent_index;
    }
}

/**
 * @brief Heap down operation (restore heap property downward)
 * @param queue Priority queue to operate on
 * @param index Starting index to heap down from
 */
void datastructures_priority_queue_heap_down(ZenPriorityQueue* queue, size_t index) {
    while (true) {
        size_t largest = index;
        size_t left = datastructures_priority_queue_left_child(index);
        size_t right = datastructures_priority_queue_right_child(index);

        // Check left child
        if (left < queue->length && queue->items[left].priority > queue->items[largest].priority) {
            largest = left;
        }

        // Check right child
        if (right < queue->length && queue->items[right].priority > queue->items[largest].priority) {
            largest = right;
        }

        // If largest is still the current index, heap property is satisfied
        if (largest == index) {
            break;
        }

        // Swap with largest child
        ZenPriorityItem temp = queue->items[index];
        queue->items[index] = queue->items[largest];
        queue->items[largest] = temp;

        index = largest;
    }
}

/**
 * @brief Add item with priority to queue
 * @param queue Priority queue Value to add item to
 * @param item Value to add to the queue
 * @param priority Number Value representing the priority (higher values have precedence)
 * @return Boolean Value indicating success or error Value on failure
 */
Value* priority_queue_push(Value* queue, const Value* item, const Value* priority) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE || !item || !priority) {
        return error_invalid_argument("priority_queue_push", "queue, item, and priority required");
    }

    if (priority->type != VALUE_NUMBER) {
        return error_type_mismatch("number", "non-number for priority");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    // Ensure capacity
    if (!datastructures_priority_queue_ensure_capacity(queue_data, queue_data->length + 1)) {
        return error_memory_allocation();
    }

    // Add new item at the end
    ZenPriorityItem new_item;
    new_item.value = value_copy(item); // Deep copy the item
    new_item.priority = priority->as.number;

    if (!new_item.value) {
        return error_memory_allocation();
    }

    value_ref(new_item.value); // Increment reference count
    queue_data->items[queue_data->length] = new_item;
    queue_data->length++;

    // Restore heap property upward
    datastructures_priority_queue_heap_up(queue_data, queue_data->length - 1);

    return value_new_boolean(true);
}

/**
 * @brief Remove and return highest priority item
 * @param queue Priority queue Value to pop from
 * @return Highest priority Value from the queue, null if empty, or error Value on failure
 */
Value* priority_queue_pop(Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_pop", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    if (queue_data->length == 0) {
        return value_new_null(); // Empty queue
    }

    // Get the highest priority item (at root)
    Value* result = queue_data->items[0].value;
    value_ref(result); // Increment reference before removing from queue

    // Move last item to root
    queue_data->length--;
    if (queue_data->length > 0) {
        queue_data->items[0] = queue_data->items[queue_data->length];
        
        // Restore heap property downward
        datastructures_priority_queue_heap_down(queue_data, 0);
    }

    return result;
}

/**
 * @brief Peek at highest priority item without removing
 * @param queue Priority queue Value to peek into
 * @return Copy of highest priority Value from the queue, null if empty, or error Value on failure
 */
Value* priority_queue_peek(const Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_peek", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    if (queue_data->length == 0) {
        return value_new_null(); // Empty queue
    }

    // Return copy of highest priority item
    return value_copy(queue_data->items[0].value);
}

/**
 * @brief Get number of items in queue
 * @param queue Priority queue Value to get size of
 * @return Number Value representing the count of items in the queue or error Value
 */
Value* priority_queue_size(const Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_size", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    return value_new_number((double)queue_data->length);
}

/**
 * @brief Check if priority queue is empty
 * @param queue Priority queue Value to check
 * @return Boolean Value indicating whether queue is empty or error Value
 */
Value* priority_queue_is_empty(const Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_is_empty", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    return value_new_boolean(queue_data->length == 0);
}

/**
 * @brief Clear all items from priority queue
 * @param queue Priority queue Value to clear
 * @return Boolean Value indicating success or error Value on failure
 */
Value* priority_queue_clear(Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_clear", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    // Free all stored values
    for (size_t i = 0; i < queue_data->length; i++) {
        if (queue_data->items[i].value) {
            value_unref(queue_data->items[i].value);
        }
    }

    queue_data->length = 0;
    return value_new_boolean(true);
}

/**
 * @brief Convert priority queue to array (sorted by priority, highest first)
 * @param queue Priority queue Value to convert
 * @return Array Value containing all items sorted by priority (highest first) or error Value
 */
Value* priority_queue_to_array(const Value* queue) {
    if (!queue || queue->type != VALUE_PRIORITY_QUEUE) {
        return error_invalid_argument("priority_queue_to_array", "queue required");
    }

    ZenPriorityQueue* queue_data = queue->as.priority_queue;
    if (!queue_data) {
        return error_null_pointer("Priority queue data is null");
    }

    // Create result array
    Value* result = value_new(VALUE_ARRAY);
    if (!result) {
        return error_memory_allocation();
    }

    if (queue_data->length == 0) {
        return result; // Return empty array
    }

    // Create a copy of the queue to avoid modifying original
    Value* queue_copy = value_copy(queue);
    if (!queue_copy) {
        value_unref(result);
        return error_memory_allocation();
    }

    // Pop all items to get them in priority order
    // result_array not needed since we use array_push
    
    while (true) {
        Value* size_val = priority_queue_size(queue_copy);
        if (!size_val || size_val->type != VALUE_NUMBER || size_val->as.number == 0) {
            if (size_val) value_unref(size_val);
            break;
        }
        value_unref(size_val);

        Value* item = priority_queue_pop(queue_copy);
        if (!item) break;

        array_push(result, item);
        value_unref(item);
    }

    value_unref(queue_copy);
    return result;
}