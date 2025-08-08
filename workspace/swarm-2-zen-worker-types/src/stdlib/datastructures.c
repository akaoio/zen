#include "zen/types/set.h"
#include "zen/types/priority_queue.h"
#include "zen/types/value.h"
#include "zen/core/error.h"

/**
 * @file data_structures.c
 * @brief ZEN stdlib wrapper functions for advanced data structures
 * 
 * Provides the ZEN language stdlib interface for sets and priority queues.
 * These functions follow the exact signatures specified in MANIFEST.json.
 */

// Set stdlib wrapper functions

/**
 * @brief Create new set data structure
 */
Value* datastructures_set_new(void) {
    return value_new(VALUE_SET);
}

/**
 * @brief Add item to set
 */
Value* datastructures_set_add(Value* set_value, const Value* item_value) {
    if (!set_value || !item_value) {
        return error_invalid_argument("datastructures_set_add", "datastructures_set_add requires both set and item arguments");
    }
    
    if (set_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "First argument must be a set");
    }
    
    return set_add(set_value, item_value);
}

/**
 * @brief Check if set contains item
 */
Value* datastructures_set_contains(Value* set_value, const Value* item_value) {
    if (!set_value || !item_value) {
        return error_invalid_argument("datastructures_set_contains", "datastructures_set_contains requires both set and item arguments");
    }
    
    if (set_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "First argument must be a set");
    }
    
    return set_contains(set_value, item_value);
}

/**
 * @brief Remove item from set
 */
Value* datastructures_set_remove(Value* set_value, const Value* item_value) {
    if (!set_value || !item_value) {
        return error_invalid_argument("datastructures_set_remove", "datastructures_set_remove requires both set and item arguments");
    }
    
    if (set_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "First argument must be a set");
    }
    
    return set_remove(set_value, item_value);
}

/**
 * @brief Get size of set
 */
Value* datastructures_set_size(Value* set_value) {
    if (!set_value) {
        return error_invalid_argument("datastructures_set_size", "datastructures_set_size requires a set argument");
    }
    
    if (set_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Argument must be a set");
    }
    
    return set_size(set_value);
}

/**
 * @brief Convert set to array
 */
Value* datastructures_set_to_array(Value* set_value) {
    if (!set_value) {
        return error_invalid_argument("datastructures_set_to_array", "datastructures_set_to_array requires a set argument");
    }
    
    if (set_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Argument must be a set");
    }
    
    return set_to_array(set_value);
}

/**
 * @brief Create union of two sets
 */
Value* datastructures_set_union(Value* set1_value, Value* set2_value) {
    if (!set1_value || !set2_value) {
        return error_invalid_argument("datastructures_set_union", "datastructures_set_union requires two set arguments");
    }
    
    if (set1_value->type != VALUE_SET || set2_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Both arguments must be sets");
    }
    
    return set_union(set1_value, set2_value);
}

/**
 * @brief Create intersection of two sets
 */
Value* datastructures_set_intersection(Value* set1_value, Value* set2_value) {
    if (!set1_value || !set2_value) {
        return error_invalid_argument("datastructures_set_intersection", "datastructures_set_intersection requires two set arguments");
    }
    
    if (set1_value->type != VALUE_SET || set2_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Both arguments must be sets");
    }
    
    return set_intersection(set1_value, set2_value);
}

/**
 * @brief Create difference of two sets
 */
Value* datastructures_set_difference(Value* set1_value, Value* set2_value) {
    if (!set1_value || !set2_value) {
        return error_invalid_argument("datastructures_set_difference", "datastructures_set_difference requires two set arguments");
    }
    
    if (set1_value->type != VALUE_SET || set2_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Both arguments must be sets");
    }
    
    return set_difference(set1_value, set2_value);
}

/**
 * @brief Check if first set is subset of second
 */
Value* datastructures_set_is_subset(Value* subset_value, Value* superset_value) {
    if (!subset_value || !superset_value) {
        return error_invalid_argument("datastructures_set_is_subset", "datastructures_set_is_subset requires two set arguments");
    }
    
    if (subset_value->type != VALUE_SET || superset_value->type != VALUE_SET) {
        return error_type_mismatch("expected", "Both arguments must be sets");
    }
    
    return set_is_subset(subset_value, superset_value);
}

// Priority queue stdlib wrapper functions

/**
 * @brief Create new priority queue
 */
Value* datastructures_priority_queue_new(void) {
    return value_new(VALUE_PRIORITY_QUEUE);
}

/**
 * @brief Add item with priority to queue
 */
Value* datastructures_priority_queue_push(Value* queue_value, const Value* item_value, const Value* priority_value) {
    if (!queue_value || !item_value || !priority_value) {
        return error_invalid_argument("datastructures_priority_queue_push", "datastructures_priority_queue_push requires queue, item, and priority arguments");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "First argument must be a priority queue");
    }
    
    if (priority_value->type != VALUE_NUMBER) {
        return error_type_mismatch("expected", "Priority must be a number");
    }
    
    return priority_queue_push(queue_value, item_value, priority_value);
}

/**
 * @brief Remove and return highest priority item
 */
Value* datastructures_priority_queue_pop(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_pop", "datastructures_priority_queue_pop requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_pop(queue_value);
}

/**
 * @brief Peek at highest priority item without removing
 */
Value* datastructures_priority_queue_peek(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_peek", "datastructures_priority_queue_peek requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_peek(queue_value);
}

/**
 * @brief Get size of priority queue
 */
Value* datastructures_priority_queue_size(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_size", "datastructures_priority_queue_size requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_size(queue_value);
}

/**
 * @brief Check if priority queue is empty
 */
Value* datastructures_priority_queue_is_empty(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_is_empty", "datastructures_priority_queue_is_empty requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_is_empty(queue_value);
}

/**
 * @brief Clear all items from priority queue
 */
Value* datastructures_priority_queue_clear(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_clear", "datastructures_priority_queue_clear requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_clear(queue_value);
}

/**
 * @brief Convert priority queue to array
 */
Value* datastructures_priority_queue_to_array(Value* queue_value) {
    if (!queue_value) {
        return error_invalid_argument("datastructures_priority_queue_to_array", "datastructures_priority_queue_to_array requires a queue argument");
    }
    
    if (queue_value->type != VALUE_PRIORITY_QUEUE) {
        return error_type_mismatch("expected", "Argument must be a priority queue");
    }
    
    return priority_queue_to_array(queue_value);
}