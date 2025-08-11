/*
 * datastructures.c
 * Advanced data structures for ZEN stdlib
 *
 * This file follows MANIFEST.json specification
 * Function signatures must match manifest exactly
 */

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdio.h>

/**
 * @file datastructures.c
 * @brief ZEN stdlib wrapper functions for advanced data structures
 *
 * Provides the ZEN language stdlib interface for sets and priority queues.
 * These functions follow the exact signatures specified in MANIFEST.json.
 */

// Set stdlib wrapper functions - TODO: Implement properly
RuntimeValue *datastructures_set_new(RuntimeValue **args, size_t argc)
{
    (void)args;  // Unused parameter
    if (argc != 0) {
        return rv_new_error("set_new() requires no arguments", -1);
    }
    // For now, return an empty object to represent a set
    return rv_new_object();
}

RuntimeValue *datastructures_set_add(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("set_add() requires exactly 2 arguments", -1);
    }
    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("set_add() requires a set as first argument", -1);
    }

    RuntimeValue *set = args[0];
    RuntimeValue *value = args[1];

    // Convert value to string for use as key
    char key_str[256];
    if (value->type == RV_STRING) {
        snprintf(key_str, sizeof(key_str), "s:%s", value->data.string.data);
    } else if (value->type == RV_NUMBER) {
        snprintf(key_str, sizeof(key_str), "n:%g", value->data.number);
    } else if (value->type == RV_BOOLEAN) {
        snprintf(key_str, sizeof(key_str), "b:%s", value->data.boolean ? "true" : "false");
    } else {
        snprintf(key_str, sizeof(key_str), "o:%p", (void *)value);
    }

    // Add value to set (key=value string, value=true)
    rv_object_set(set, key_str, rv_new_boolean(true));

    return rv_new_boolean(true);
}

RuntimeValue *datastructures_set_has(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("set_has() requires exactly 2 arguments", -1);
    }
    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("set_has() requires a set as first argument", -1);
    }

    RuntimeValue *set = args[0];
    RuntimeValue *value = args[1];

    // Convert value to string key (same logic as set_add)
    char key_str[256];
    if (value->type == RV_STRING) {
        snprintf(key_str, sizeof(key_str), "s:%s", value->data.string.data);
    } else if (value->type == RV_NUMBER) {
        snprintf(key_str, sizeof(key_str), "n:%g", value->data.number);
    } else if (value->type == RV_BOOLEAN) {
        snprintf(key_str, sizeof(key_str), "b:%s", value->data.boolean ? "true" : "false");
    } else {
        snprintf(key_str, sizeof(key_str), "o:%p", (void *)value);
    }

    // Check if key exists in set
    return rv_new_boolean(rv_object_has(set, key_str));
}

RuntimeValue *datastructures_set_remove(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("set_remove() requires exactly 2 arguments", -1);
    }
    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("set_remove() requires a set as first argument", -1);
    }

    RuntimeValue *set = args[0];
    RuntimeValue *value = args[1];

    // Convert value to string key (same logic as set_add)
    char key_str[256];
    if (value->type == RV_STRING) {
        snprintf(key_str, sizeof(key_str), "s:%s", value->data.string.data);
    } else if (value->type == RV_NUMBER) {
        snprintf(key_str, sizeof(key_str), "n:%g", value->data.number);
    } else if (value->type == RV_BOOLEAN) {
        snprintf(key_str, sizeof(key_str), "b:%s", value->data.boolean ? "true" : "false");
    } else {
        snprintf(key_str, sizeof(key_str), "o:%p", (void *)value);
    }

    // Check if key exists before removal
    bool existed = rv_object_has(set, key_str);
    if (existed) {
        rv_object_delete(set, key_str);
    }

    return rv_new_boolean(existed);
}

RuntimeValue *datastructures_set_size(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("set_size() requires exactly 1 argument", -1);
    }
    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("set_size() requires a set argument", -1);
    }

    RuntimeValue *set = args[0];
    size_t size = rv_object_size(set);

    return rv_new_number((double)size);
}

// Priority Queue stdlib wrapper functions - TODO: Implement properly
RuntimeValue *datastructures_pqueue_new(RuntimeValue **args, size_t argc)
{
    (void)args;  // Unused parameter
    if (argc != 0) {
        return rv_new_error("pqueue_new() requires no arguments", -1);
    }
    // For now, return an empty array to represent a priority queue
    return rv_new_array();
}

RuntimeValue *datastructures_pqueue_push(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error("pqueue_push() requires exactly 3 arguments: queue, value, priority",
                            -1);
    }
    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("pqueue_push() requires a priority queue as first argument", -1);
    }
    if (!args[2] || args[2]->type != RV_NUMBER) {
        return rv_new_error("pqueue_push() requires numeric priority as third argument", -1);
    }

    RuntimeValue *queue = args[0];
    RuntimeValue *value = args[1];
    double priority = args[2]->data.number;

    // Create item object with value and priority
    RuntimeValue *item = rv_new_object();
    rv_object_set(item, "value", rv_ref(value));
    rv_object_set(item, "priority", rv_new_number(priority));

    // Insert in sorted order (higher priority first)
    size_t insert_pos = queue->data.array.count;
    for (size_t i = 0; i < queue->data.array.count; i++) {
        RuntimeValue *existing_item = queue->data.array.elements[i];
        if (existing_item && existing_item->type == RV_OBJECT) {
            RuntimeValue *existing_priority = rv_object_get(existing_item, "priority");
            if (existing_priority && existing_priority->type == RV_NUMBER) {
                if (priority > existing_priority->data.number) {
                    insert_pos = i;
                    break;
                }
            }
        }
    }

    // Expand array if needed
    if (queue->data.array.count >= queue->data.array.capacity) {
        size_t new_capacity = queue->data.array.capacity == 0 ? 4 : queue->data.array.capacity * 2;
        RuntimeValue **new_elements =
            memory_realloc(queue->data.array.elements, new_capacity * sizeof(RuntimeValue *));
        if (!new_elements) {
            rv_unref(item);
            return rv_new_error("pqueue_push() failed to allocate memory", -1);
        }
        queue->data.array.elements = new_elements;
        queue->data.array.capacity = new_capacity;
    }

    // Shift elements to make space
    for (size_t i = queue->data.array.count; i > insert_pos; i--) {
        queue->data.array.elements[i] = queue->data.array.elements[i - 1];
    }

    // Insert the item
    queue->data.array.elements[insert_pos] = item;
    queue->data.array.count++;

    return rv_new_null();
}

RuntimeValue *datastructures_pqueue_pop(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("pqueue_pop() requires exactly 1 argument", -1);
    }
    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("pqueue_pop() requires a priority queue argument", -1);
    }
    RuntimeValue *queue = args[0];

    if (queue->data.array.count == 0) {
        return rv_new_null();
    }

    // Get highest priority item (first item in sorted array)
    RuntimeValue *item = queue->data.array.elements[0];
    RuntimeValue *result = rv_new_null();

    if (item && item->type == RV_OBJECT) {
        RuntimeValue *value = rv_object_get(item, "value");
        if (value) {
            result = rv_ref(value);
        }
    }

    // Remove first item and shift remaining elements
    rv_unref(queue->data.array.elements[0]);
    for (size_t i = 0; i < queue->data.array.count - 1; i++) {
        queue->data.array.elements[i] = queue->data.array.elements[i + 1];
    }
    queue->data.array.count--;

    return result;
}

RuntimeValue *datastructures_pqueue_peek(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("pqueue_peek() requires exactly 1 argument", -1);
    }
    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("pqueue_peek() requires a priority queue argument", -1);
    }
    RuntimeValue *queue = args[0];

    if (queue->data.array.count == 0) {
        return rv_new_null();
    }

    // Peek at highest priority item (first item in sorted array)
    RuntimeValue *item = queue->data.array.elements[0];

    if (item && item->type == RV_OBJECT) {
        RuntimeValue *value = rv_object_get(item, "value");
        if (value) {
            return rv_ref(value);
        }
    }

    return rv_new_null();
}

RuntimeValue *datastructures_pqueue_size(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("pqueue_size() requires exactly 1 argument", -1);
    }
    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("pqueue_size() requires a priority queue argument", -1);
    }
    return rv_new_number((double)args[0]->data.array.count);
}

RuntimeValue *datastructures_pqueue_is_empty(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("pqueue_is_empty() requires exactly 1 argument", -1);
    }
    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("pqueue_is_empty() requires a priority queue argument", -1);
    }
    return rv_new_boolean(args[0]->data.array.count == 0);
}