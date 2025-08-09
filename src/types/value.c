/*
 * value.c
 * Value type system
 *
 * This file implements the complete ZEN value system with reference counting,
 * type conversions, and memory management following MANIFEST.json specifications.
 */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include "zen/types/value.h"

#include "zen/core/memory.h"
#include "zen/types/object.h"
#include "zen/types/priority_queue.h"
#include "zen/types/set.h"

#include <assert.h>
#include <math.h>       // For NAN, INFINITY, isnan, isinf, floor
#include <stdatomic.h>  // For atomic operations and SIZE_MAX
#include <stdbool.h>
#include <stdint.h>  // For SIZE_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal helper functions
static void value_free_data(Value *value);
static ZenArray *array_create(size_t initial_capacity);
static void array_free(ZenArray *array);
static ZenObject *object_create(void);
static void object_free(ZenObject *object);
static ZenString *string_create(const char *str);
static void string_free(ZenString *string);
static ZenClass *class_create(const char *name, const char *parent_name);
static void class_free(ZenClass *class_def);
static ZenInstance *instance_create(Value *class_def);
static void instance_free(ZenInstance *instance);

/**
 * @brief Create new value of specified type
 * @param type The type of value to create
 * @return Newly allocated value or NULL on failure
 */
Value *value_new(ValueType type)
{
    if (type < VALUE_NULL || type > VALUE_PRIORITY_QUEUE) {
        return NULL;
    }

    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = type;
    value->ref_count = 1;

    // Initialize type-specific data
    switch (type) {
    case VALUE_NULL:
        // No additional initialization needed
        break;
    case VALUE_BOOLEAN:
        value->as.boolean = false;
        break;
    case VALUE_UNDECIDABLE:
        // Undecidable values represent Gödel's incompleteness - no data needed
        break;
    case VALUE_NUMBER:
        value->as.number = 0.0;
        break;
    case VALUE_STRING:
        value->as.string = string_create("");  // Empty string
        if (!value->as.string) {
            memory_free(value);
            return NULL;
        }
        break;
    case VALUE_ARRAY:
        value->as.array = array_create(8);  // Default capacity
        if (!value->as.array) {
            memory_free(value);
            return NULL;
        }
        break;
    case VALUE_OBJECT:
        value->as.object = object_create();
        if (!value->as.object) {
            memory_free(value);
            return NULL;
        }
        break;
    case VALUE_FUNCTION:
        value->as.function = NULL;  // For future function support
        break;
    case VALUE_ERROR:
        value->as.error = memory_alloc(sizeof(ZenError));
        if (!value->as.error) {
            memory_free(value);
            return NULL;
        }
        value->as.error->message = memory_strdup("Unknown error");
        if (!value->as.error->message) {
            memory_free(value->as.error);
            memory_free(value);
            return NULL;
        }
        value->as.error->code = -1;
        break;
    case VALUE_CLASS:
        value->as.class_def = class_create("", NULL);
        if (!value->as.class_def) {
            memory_free(value);
            return NULL;
        }
        break;
    case VALUE_INSTANCE:
        // Instance needs to be created with a specific class
        value->as.instance = NULL;
        break;
    case VALUE_SET:
        value->as.set = datastructures_set_create();
        if (!value->as.set) {
            memory_free(value);
            return NULL;
        }
        break;
    case VALUE_PRIORITY_QUEUE:
        value->as.priority_queue = datastructures_priority_queue_create();
        if (!value->as.priority_queue) {
            memory_free(value);
            return NULL;
        }
        break;
    }

    return value;
}

/**
 * @brief Create string value
 * @param str String to store (will be copied)
 * @return Newly allocated string value or NULL on failure
 */
Value *value_new_string(const char *str)
{
    if (!str) {
        return NULL;
    }

    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_STRING;
    value->ref_count = 1;
    value->as.string = string_create(str);

    if (!value->as.string) {
        memory_free(value);
        return NULL;
    }

    return value;
}

/**
 * @brief Create number value
 * @param num Number to store
 * @return Newly allocated number value or NULL on failure
 */
Value *value_new_number(double num)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_NUMBER;
    value->ref_count = 1;
    value->as.number = num;

    return value;
}

/**
 * @brief Create boolean value
 * @param val Boolean value to store
 * @return Newly allocated boolean value or NULL on failure
 */
Value *value_new_boolean(bool val)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_BOOLEAN;
    value->ref_count = 1;
    value->as.boolean = val;

    return value;
}

/**
 * @brief Create a new undecidable value (Gödel-approved for incompleteness theorem)
 * @return New undecidable Value representing statements that cannot be proven true or false
 */
Value *value_new_undecidable(void)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_UNDECIDABLE;
    value->ref_count = 1;
    // Undecidable values don't need additional data

    return value;
}

/**
 * @brief Create null value
 * @return Newly allocated null value or NULL on failure
 */
Value *value_new_null(void)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_NULL;
    value->ref_count = 1;
    // No data initialization needed for null

    return value;
}

/**
 * @brief Deep copy value with proper reference counting
 * @param value Value to copy
 * @return Deep copy of the value or NULL on failure
 */
Value *value_copy(const Value *value)
{
    if (!value) {
        return value_new_null();
    }

    switch (value->type) {
    case VALUE_NULL:
        return value_new_null();
    case VALUE_BOOLEAN:
        return value_new_boolean(value->as.boolean);
    case VALUE_UNDECIDABLE:
        return value_new_undecidable();
    case VALUE_NUMBER:
        return value_new_number(value->as.number);
    case VALUE_STRING:
        return value_new_string(value->as.string && value->as.string->data ? value->as.string->data
                                                                           : "");
    case VALUE_ARRAY: {
        Value *new_value = value_new(VALUE_ARRAY);
        if (!new_value)
            return NULL;

        ZenArray *src_array = value->as.array;
        ZenArray *dst_array = new_value->as.array;

        // Copy all elements
        for (size_t i = 0; i < src_array->length; i++) {
            Value *copied_item = value_copy(src_array->items[i]);
            if (!copied_item) {
                value_free(new_value);
                return NULL;
            }

            // Grow array if needed
            if (dst_array->length >= dst_array->capacity) {
                size_t new_capacity = dst_array->capacity * 2;
                Value **new_items =
                    memory_realloc(dst_array->items, new_capacity * sizeof(Value *));
                if (!new_items) {
                    value_free(copied_item);
                    value_free(new_value);
                    return NULL;
                }
                dst_array->items = new_items;
                dst_array->capacity = new_capacity;
            }

            dst_array->items[dst_array->length++] = copied_item;
        }

        return new_value;
    }
    case VALUE_OBJECT: {
        Value *new_value = value_new(VALUE_OBJECT);
        if (!new_value)
            return NULL;

        ZenObject *src_obj = value->as.object;
        if (!src_obj) {
            return new_value;
        }

        // Deep copy all key-value pairs using object_set
        for (size_t i = 0; i < src_obj->length; i++) {
            ZenObjectPair *pair = &src_obj->pairs[i];
            if (pair->key && pair->value) {
                Value *copied_value = value_copy(pair->value);
                if (!copied_value) {
                    value_free(new_value);
                    return NULL;
                }

                // Set the copied value in the new object
                object_set(new_value, pair->key, copied_value);

                // Unref our temporary reference (object_set adds its own ref)
                value_unref(copied_value);
            }
        }

        return new_value;
    }
    case VALUE_FUNCTION:
        // Functions are typically shared/referenced, not copied
        // Return a new reference to the original function
        if (value->as.function) {
            return value_ref((Value *)value);
        } else {
            return value_new(VALUE_FUNCTION);
        }
    case VALUE_ERROR:
        // Deep copy error with message and code
        if (value->as.error) {
            return value_new_error(value->as.error->message, value->as.error->code);
        }
        return value_new(VALUE_ERROR);
    case VALUE_CLASS:
        // Classes are typically not copied, but referenced
        return value_ref((Value *)value);
    case VALUE_INSTANCE: {
        // Create a new instance of the same class
        if (value->as.instance && value->as.instance->class_def) {
            Value *new_instance = value_new_instance(value->as.instance->class_def);
            // Would need to copy all properties here
            return new_instance;
        }
        return NULL;
    }
    case VALUE_SET: {
        // Deep copy set by converting to array, then back to set
        Value *array = set_to_array(value);
        if (!array)
            return NULL;

        Value *new_set = set_new();
        if (!new_set) {
            value_unref(array);
            return NULL;
        }

        if (array->type == VALUE_ARRAY) {
            ZenArray *arr_data = array->as.array;
            for (size_t i = 0; i < arr_data->length; i++) {
                Value *copied_item = value_copy(arr_data->items[i]);
                if (copied_item) {
                    Value *result = set_add(new_set, copied_item);
                    value_unref(copied_item);
                    if (result)
                        value_unref(result);
                }
            }
        }

        value_unref(array);
        return new_set;
    }
    case VALUE_PRIORITY_QUEUE: {
        // Deep copy priority queue by converting to array, then rebuilding
        Value *array = priority_queue_to_array(value);
        if (!array)
            return NULL;

        Value *new_queue = priority_queue_new();
        if (!new_queue) {
            value_unref(array);
            return NULL;
        }

        if (array->type == VALUE_ARRAY) {
            ZenArray *arr_data = array->as.array;
            // Note: This loses priority information, but copies structure
            for (size_t i = 0; i < arr_data->length; i++) {
                Value *copied_item = value_copy(arr_data->items[i]);
                Value *priority_val = value_new_number(1.0);  // Default priority
                if (copied_item && priority_val) {
                    Value *result = priority_queue_push(new_queue, copied_item, priority_val);
                    if (result)
                        value_unref(result);
                }
                if (copied_item)
                    value_unref(copied_item);
                if (priority_val)
                    value_unref(priority_val);
            }
        }

        value_unref(array);
        return new_queue;
    }
    default:
        return NULL;
    }
}

/**
 * @brief Free value memory
 * @param value Value to free (may be NULL)
 */
void value_free(Value *value)
{
    if (!value) {
        return;
    }

    // CRITICAL FIX: Prevent re-entrant freeing during value_free_data cascade
    // Check if ref_count is already marked as being freed (SIZE_MAX)
    size_t current_ref = atomic_load((_Atomic size_t *)&value->ref_count);
    if (current_ref != 0 && current_ref != SIZE_MAX) {
        // This value is being freed but ref_count wasn't properly set
        // This shouldn't happen, but let's be defensive
        fprintf(stderr, "WARNING: value_free called on value with ref_count=%zu\n", current_ref);
        return;
    }

    value_free_data(value);
    memory_free(value);
}

/**
 * @brief Convert value to string representation
 * @param value Value to convert
 * @return String representation (caller must free) or NULL on failure
 */
// Internal helper with depth limit to prevent infinite recursion
static char *value_to_string_internal(const Value *value, int depth);

/**
 * @brief Convert a value to its string representation
 * @param value The value to convert to string
 * @return Dynamically allocated string representation (caller must free) or NULL on failure
 */
char *value_to_string(const Value *value) { return value_to_string_internal(value, 0); }

static char *value_to_string_internal(const Value *value, int depth)
{
    if (!value) {
        return memory_strdup("null");
    }

    // Prevent infinite recursion from circular references
    if (depth > 10) {
        return memory_strdup("...[too deep]");
    }

    switch (value->type) {
    case VALUE_NULL:
        return memory_strdup("null");
    case VALUE_BOOLEAN:
        return memory_strdup(value->as.boolean ? "true" : "false");
    case VALUE_UNDECIDABLE:
        return memory_strdup("undecidable");
    case VALUE_NUMBER: {
        char buffer[32];
        // Check for invalid numbers
        if (isnan(value->as.number)) {
            return memory_strdup("NaN");
        }
        if (isinf(value->as.number)) {
            return memory_strdup(value->as.number > 0 ? "Infinity" : "-Infinity");
        }
        snprintf(buffer, sizeof(buffer), "%.15g", value->as.number);
        return memory_strdup(buffer);
    }
    case VALUE_STRING:
        return memory_strdup(value->as.string && value->as.string->data ? value->as.string->data
                                                                        : "");
    case VALUE_ARRAY: {
        // Simple array representation [item1, item2, ...]
        ZenArray *array = value->as.array;
        if (array->length == 0) {
            return memory_strdup("[]");
        }

        // Estimate size needed
        size_t total_size = 32;  // Base size for brackets and commas
        char **item_strings = memory_alloc(array->length * sizeof(char *));
        if (!item_strings)
            return NULL;

        // Convert each item to string
        for (size_t i = 0; i < array->length; i++) {
            item_strings[i] = value_to_string_internal(array->items[i], depth + 1);
            if (!item_strings[i]) {
                // Clean up on failure
                for (size_t j = 0; j < i; j++) {
                    memory_free(item_strings[j]);
                }
                memory_free(item_strings);
                return NULL;
            }
            total_size += strlen(item_strings[i]) + 2;  // +2 for ", "
        }

        char *result = memory_alloc(total_size);
        if (!result) {
            for (size_t i = 0; i < array->length; i++) {
                memory_free(item_strings[i]);
            }
            memory_free(item_strings);
            return NULL;
        }

        snprintf(result, total_size, "[");
        for (size_t i = 0; i < array->length; i++) {
            size_t current_len = strlen(result);
            size_t remaining = total_size - current_len;
            if (remaining > 0) {
                strncat(result, item_strings[i], remaining - 1);
                current_len = strlen(result);
                remaining = total_size - current_len;
                if (i < array->length - 1 && remaining > 2) {
                    strncat(result, ", ", remaining - 1);
                }
            }
            memory_free(item_strings[i]);
        }
        size_t current_len = strlen(result);
        size_t remaining = total_size - current_len;
        if (remaining > 1) {
            strncat(result, "]", remaining - 1);
        }

        memory_free(item_strings);
        return result;
    }
    case VALUE_OBJECT: {
        if (!value->as.object || value->as.object->length == 0) {
            return memory_strdup("{}");
        }

        // Calculate required buffer size
        size_t buffer_size = 3;  // "{}"\0
        for (size_t i = 0; i < value->as.object->length; i++) {
            if (value->as.object->pairs[i].key) {
                buffer_size += strlen(value->as.object->pairs[i].key) + 4;  // "key":
            }
            if (value->as.object->pairs[i].value) {
                char *value_str = value_to_string(value->as.object->pairs[i].value);
                if (value_str) {
                    buffer_size += strlen(value_str) + 3;  // value + ", "
                    memory_free(value_str);
                }
            }
        }

        char *result = memory_alloc(buffer_size);
        if (!result)
            return memory_strdup("{}");

        strcpy(result, "{");
        for (size_t i = 0; i < value->as.object->length; i++) {
            if (i > 0)
                strcat(result, ", ");

            if (value->as.object->pairs[i].key) {
                strcat(result, value->as.object->pairs[i].key);
                strcat(result, ": ");
            }

            if (value->as.object->pairs[i].value) {
                char *value_str = value_to_string(value->as.object->pairs[i].value);
                if (value_str) {
                    strcat(result, value_str);
                    memory_free(value_str);
                } else {
                    strcat(result, "null");
                }
            } else {
                strcat(result, "null");
            }
        }
        strcat(result, "}");
        return result;
    }
    case VALUE_FUNCTION:
        return memory_strdup("<function>");
    case VALUE_ERROR:
        if (value->as.error && value->as.error->message) {
            char *result = memory_alloc(strlen(value->as.error->message) + 16);
            if (!result)
                return NULL;
            snprintf(result,
                     strlen(value->as.error->message) + 16,
                     "<error: %s>",
                     value->as.error->message);
            return result;
        }
        return memory_strdup("<error>");
    case VALUE_CLASS:
        if (value->as.class_def && value->as.class_def->name) {
            char *result = memory_alloc(strlen(value->as.class_def->name) + 16);
            if (!result)
                return NULL;
            snprintf(result,
                     strlen(value->as.class_def->name) + 16,
                     "<class %s>",
                     value->as.class_def->name);
            return result;
        }
        return memory_strdup("<class>");
    case VALUE_INSTANCE:
        if (value->as.instance && value->as.instance->class_def &&
            value->as.instance->class_def->as.class_def &&
            value->as.instance->class_def->as.class_def->name) {
            char *result =
                memory_alloc(strlen(value->as.instance->class_def->as.class_def->name) + 32);
            if (!result)
                return NULL;
            snprintf(result,
                     strlen(value->as.instance->class_def->as.class_def->name) + 32,
                     "<instance of %s>",
                     value->as.instance->class_def->as.class_def->name);
            return result;
        }
        return memory_strdup("<instance>");
    case VALUE_SET: {
        // Convert set to array and display as {item1, item2, ...}
        Value *array = set_to_array(value);
        if (!array || array->type != VALUE_ARRAY) {
            if (array)
                value_unref(array);
            return memory_strdup("{}");
        }

        ZenArray *arr_data = array->as.array;
        if (arr_data->length == 0) {
            value_unref(array);
            return memory_strdup("{}");
        }

        // Build set string representation
        size_t total_size = 32;
        char **item_strings = memory_alloc(arr_data->length * sizeof(char *));
        if (!item_strings) {
            value_unref(array);
            return memory_strdup("{}");
        }

        for (size_t i = 0; i < arr_data->length; i++) {
            item_strings[i] = value_to_string_internal(arr_data->items[i], depth + 1);
            if (item_strings[i]) {
                total_size += strlen(item_strings[i]) + 2;
            }
        }

        char *result = memory_alloc(total_size);
        if (!result) {
            for (size_t i = 0; i < arr_data->length; i++) {
                if (item_strings[i])
                    memory_free(item_strings[i]);
            }
            memory_free(item_strings);
            value_unref(array);
            return memory_strdup("{}");
        }

        snprintf(result, total_size, "{");
        for (size_t i = 0; i < arr_data->length; i++) {
            if (item_strings[i]) {
                size_t current_len = strlen(result);
                size_t remaining = total_size - current_len;
                if (i > 0 && remaining > 2) {
                    strncat(result, ", ", remaining - 1);
                    current_len = strlen(result);
                    remaining = total_size - current_len;
                }
                if (remaining > 0) {
                    strncat(result, item_strings[i], remaining - 1);
                }
                memory_free(item_strings[i]);
            }
        }
        size_t current_len = strlen(result);
        size_t remaining = total_size - current_len;
        if (remaining > 1) {
            strncat(result, "}", remaining - 1);
        }

        memory_free(item_strings);
        value_unref(array);
        return result;
    }
    case VALUE_PRIORITY_QUEUE: {
        // Show priority queue size and type
        Value *size_val = priority_queue_size(value);
        if (size_val && size_val->type == VALUE_NUMBER) {
            char *result = memory_alloc(32);
            if (result) {
                snprintf(result, 32, "<priority_queue size=%.0f>", size_val->as.number);
            }
            value_unref(size_val);
            return result ? result : memory_strdup("<priority_queue>");
        }
        if (size_val)
            value_unref(size_val);
        return memory_strdup("<priority_queue>");
    }
    default:
        return memory_strdup("<unknown>");
    }
}

/**
 * @brief Compare two values for equality
 * @param a First value to compare
 * @param b Second value to compare
 * @return true if values are equal, false otherwise
 */
bool value_equals(const Value *a, const Value *b)
{
    if (a == b) {
        return true;  // Same pointer
    }

    if (!a || !b) {
        return false;  // One is NULL
    }

    if (a->type != b->type) {
        return false;  // Different types
    }

    switch (a->type) {
    case VALUE_NULL:
        return true;  // All nulls are equal
    case VALUE_BOOLEAN:
        return a->as.boolean == b->as.boolean;
    case VALUE_UNDECIDABLE:
        // All undecidable values are equivalent - they represent the same logical state
        return true;
    case VALUE_NUMBER:
        return a->as.number == b->as.number;
    case VALUE_STRING:
        if (!a->as.string && !b->as.string)
            return true;
        if (!a->as.string || !b->as.string)
            return false;
        if (!a->as.string->data && !b->as.string->data)
            return true;
        if (!a->as.string->data || !b->as.string->data)
            return false;
        return strcmp(a->as.string->data, b->as.string->data) == 0;
    case VALUE_ARRAY: {
        ZenArray *arr_a = a->as.array;
        ZenArray *arr_b = b->as.array;

        if (arr_a->length != arr_b->length) {
            return false;
        }

        for (size_t i = 0; i < arr_a->length; i++) {
            if (!value_equals(arr_a->items[i], arr_b->items[i])) {
                return false;
            }
        }

        return true;
    }
    case VALUE_OBJECT:
        // Simplified - would need full object comparison
        return a->as.object == b->as.object;
    case VALUE_FUNCTION:
        return a->as.function == b->as.function;
    case VALUE_ERROR:
        // Compare error codes
        if (!a->as.error || !b->as.error) {
            return a->as.error == b->as.error;
        }
        return a->as.error->code == b->as.error->code;
    case VALUE_CLASS:
        // Classes are equal if they're the same object
        return a->as.class_def == b->as.class_def;
    case VALUE_INSTANCE:
        // Instances are equal if they're the same object
        return a->as.instance == b->as.instance;
    case VALUE_SET: {
        // Sets are equal if they contain the same elements
        Value *size_a = set_size(a);
        Value *size_b = set_size(b);

        if (!size_a || !size_b || size_a->type != VALUE_NUMBER || size_b->type != VALUE_NUMBER) {
            if (size_a)
                value_unref(size_a);
            if (size_b)
                value_unref(size_b);
            return false;
        }

        bool equal = size_a->as.number == size_b->as.number;
        value_unref(size_a);
        value_unref(size_b);

        if (!equal)
            return false;

        // Check if all elements in a are in b
        Value *array_a = set_to_array(a);
        if (!array_a || array_a->type != VALUE_ARRAY) {
            if (array_a)
                value_unref(array_a);
            return false;
        }

        ZenArray *arr_data = array_a->as.array;
        for (size_t i = 0; i < arr_data->length; i++) {
            Value *contains = set_contains(b, arr_data->items[i]);
            if (!contains || contains->type != VALUE_BOOLEAN || !contains->as.boolean) {
                if (contains)
                    value_unref(contains);
                value_unref(array_a);
                return false;
            }
            value_unref(contains);
        }

        value_unref(array_a);
        return true;
    }
    case VALUE_PRIORITY_QUEUE:
        // Priority queues are equal if they're the same object
        // (Content comparison would require destructive operations)
        return a->as.priority_queue == b->as.priority_queue;
    default:
        return false;
    }
}

/**
 * @brief Get string name of value type
 * @param type Value type
 * @return String name of the type (static string)
 */
const char *value_type_name(ValueType type)
{
    switch (type) {
    case VALUE_NULL:
        return "null";
    case VALUE_BOOLEAN:
        return "boolean";
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
    case VALUE_ARRAY:
        return "array";
    case VALUE_OBJECT:
        return "object";
    case VALUE_FUNCTION:
        return "function";
    case VALUE_ERROR:
        return "error";
    case VALUE_CLASS:
        return "class";
    case VALUE_INSTANCE:
        return "instance";
    case VALUE_SET:
        return "set";
    case VALUE_PRIORITY_QUEUE:
        return "priority_queue";
    default:
        return "unknown";
    }
}

/**
 * @brief Increment reference count of value using thread-safe atomic operations
 * @param value Value to reference (may be NULL)
 * @return The same value pointer for convenience
 * @note Thread-safe atomic operation using memory.c utilities
 */
Value *value_ref(Value *value)
{
    if (value) {
        memory_ref_inc(&value->ref_count);
    }
    return value;
}

/**
 * @brief Decrement reference count and free if zero using thread-safe atomic operations
 * @param value Value to unreference (may be NULL)
 * @note Thread-safe atomic operation using memory.c utilities
 */
void value_unref(Value *value)
{
    if (!value) {
        return;
    }

    // CRITICAL FIX: Use atomic operations for consistent state checking
    size_t old_count = atomic_load((_Atomic size_t *)&value->ref_count);

    // Additional safety check - prevent underflow and detect being freed
    if (old_count == 0 || old_count == SIZE_MAX) {
        // Already freed or being freed - prevent double-free cascade
        if (old_count == 0) {
            fprintf(
                stderr,
                "WARNING: Attempting to unref Value with ref_count=0 (double-free protection)\n");
        }
        return;
    }

    size_t new_count = memory_ref_dec(&value->ref_count);

    if (new_count == 0) {
        // Mark as being freed immediately to prevent re-entry
        atomic_store((_Atomic size_t *)&value->ref_count, SIZE_MAX);
        value_free(value);
    }
}

// Internal helper function implementations

/**
 * @brief Free the data portion of a value without freeing the value itself
 * @param value Value whose data should be freed
 */
static void value_free_data(Value *value)
{
    if (!value) {
        return;
    }

    switch (value->type) {
    case VALUE_NULL:
    case VALUE_BOOLEAN:
    case VALUE_UNDECIDABLE:
    case VALUE_NUMBER:
        // No dynamic data to free
        break;
    case VALUE_STRING:
        if (value->as.string) {
            string_free(value->as.string);
            value->as.string = NULL;
        }
        break;
    case VALUE_ARRAY:
        if (value->as.array) {
            array_free(value->as.array);
            value->as.array = NULL;
        }
        break;
    case VALUE_OBJECT:
        if (value->as.object) {
            object_free(value->as.object);
            value->as.object = NULL;
        }
        break;
    case VALUE_FUNCTION:
        // Function pointers are typically statically allocated or
        // managed elsewhere in the runtime, so we just clear the pointer
        // In a full implementation, this might free function closures
        // or decrement reference counts on captured variables
        value->as.function = NULL;
        break;
    case VALUE_ERROR:
        if (value->as.error) {
            if (value->as.error->message) {
                memory_free(value->as.error->message);
            }
            memory_free(value->as.error);
            value->as.error = NULL;
        }
        break;
    case VALUE_CLASS:
        if (value->as.class_def) {
            class_free(value->as.class_def);
            value->as.class_def = NULL;
        }
        break;
    case VALUE_INSTANCE:
        if (value->as.instance) {
            instance_free(value->as.instance);
            value->as.instance = NULL;
        }
        break;
    case VALUE_SET:
        if (value->as.set) {
            datastructures_set_free(value->as.set);
            value->as.set = NULL;
        }
        break;
    case VALUE_PRIORITY_QUEUE:
        if (value->as.priority_queue) {
            datastructures_priority_queue_free(value->as.priority_queue);
            value->as.priority_queue = NULL;
        }
        break;
    }
}

/**
 * @brief Create a new array with specified initial capacity
 * @param initial_capacity Initial capacity of the array
 * @return Newly allocated array or NULL on failure
 */
static ZenArray *array_create(size_t initial_capacity)
{
    if (initial_capacity == 0) {
        initial_capacity = 8;  // Minimum capacity
    }

    ZenArray *array = memory_alloc(sizeof(ZenArray));
    if (!array) {
        return NULL;
    }

    array->items = memory_alloc(initial_capacity * sizeof(Value *));
    if (!array->items) {
        memory_free(array);
        return NULL;
    }

    array->length = 0;
    array->capacity = initial_capacity;

    return array;
}

/**
 * @brief Free an array and all its elements
 * @param array Array to free
 */
static void array_free(ZenArray *array)
{
    if (!array) {
        return;
    }

    // Unreference all items
    for (size_t i = 0; i < array->length; i++) {
        value_unref(array->items[i]);
    }

    memory_free(array->items);
    memory_free(array);
}

/**
 * @brief Create a new object (hash map)
 * @return Newly allocated object or NULL on failure
 */
static ZenObject *object_create(void)
{
    ZenObject *object = memory_alloc(sizeof(ZenObject));
    if (!object) {
        return NULL;
    }

    object->capacity = 8;  // Initial capacity
    object->pairs = memory_alloc(object->capacity * sizeof(ZenObjectPair));
    if (!object->pairs) {
        memory_free(object);
        return NULL;
    }

    object->length = 0;

    return object;
}

/**
 * @brief Free an object and all its entries
 * @param object Object to free
 */
static void object_free(ZenObject *object)
{
    if (!object) {
        return;
    }

    // Free all pairs
    for (size_t i = 0; i < object->length; i++) {
        if (object->pairs[i].key) {
            memory_free(object->pairs[i].key);
        }
        if (object->pairs[i].value) {
            value_unref(object->pairs[i].value);
        }
    }

    memory_free(object->pairs);
    memory_free(object);
}

/**
 * @brief Create a new ZenString with specified content
 * @param str String content to copy
 * @return Newly allocated ZenString or NULL on failure
 */
static ZenString *string_create(const char *str)
{
    if (!str) {
        str = "";
    }

    ZenString *zen_str = memory_alloc(sizeof(ZenString));
    if (!zen_str) {
        return NULL;
    }

    zen_str->length = strlen(str);
    zen_str->capacity = zen_str->length + 1;  // +1 for null terminator
    if (zen_str->capacity < 8) {
        zen_str->capacity = 8;  // Minimum capacity
    }

    zen_str->data = memory_alloc(zen_str->capacity);
    if (!zen_str->data) {
        memory_free(zen_str);
        return NULL;
    }

    strncpy(zen_str->data, str, zen_str->capacity - 1);
    zen_str->data[zen_str->capacity - 1] = '\0';
    return zen_str;
}

/**
 * @brief Free a ZenString
 * @param zen_str String to free
 */
static void string_free(ZenString *zen_str)
{
    if (!zen_str) {
        return;
    }

    if (zen_str->data) {
        memory_free(zen_str->data);
    }
    memory_free(zen_str);
}

/**
 * @brief Create a new ZenClass
 * @param name Class name
 * @param parent_name Parent class name (may be NULL)
 * @return Newly allocated ZenClass or NULL on failure
 */
static ZenClass *class_create(const char *name, const char *parent_name)
{
    ZenClass *class_def = memory_alloc(sizeof(ZenClass));
    if (!class_def) {
        return NULL;
    }

    class_def->name = name ? memory_strdup(name) : NULL;
    class_def->parent_class_name = parent_name ? memory_strdup(parent_name) : NULL;
    class_def->parent_class = NULL;
    class_def->methods = value_new(VALUE_OBJECT);
    if (!class_def->methods) {
        if (class_def->name)
            memory_free(class_def->name);
        if (class_def->parent_class_name)
            memory_free(class_def->parent_class_name);
        memory_free(class_def);
        return NULL;
    }
    class_def->constructor = NULL;

    return class_def;
}

/**
 * @brief Free a ZenClass
 * @param class_def Class to free
 */
static void class_free(ZenClass *class_def)
{
    if (!class_def) {
        return;
    }

    if (class_def->name) {
        memory_free(class_def->name);
    }
    if (class_def->parent_class_name) {
        memory_free(class_def->parent_class_name);
    }
    if (class_def->parent_class) {
        value_unref(class_def->parent_class);
    }
    if (class_def->methods) {
        value_unref(class_def->methods);
    }
    if (class_def->constructor) {
        value_unref(class_def->constructor);
    }

    memory_free(class_def);
}

/**
 * @brief Create a new ZenInstance
 * @param class_def Class definition for this instance
 * @return Newly allocated ZenInstance or NULL on failure
 */
static ZenInstance *instance_create(Value *class_def)
{
    if (!class_def || class_def->type != VALUE_CLASS) {
        return NULL;
    }

    ZenInstance *instance = memory_alloc(sizeof(ZenInstance));
    if (!instance) {
        return NULL;
    }

    instance->class_def = value_ref(class_def);
    instance->properties = value_new(VALUE_OBJECT);
    if (!instance->properties) {
        value_unref(instance->class_def);
        memory_free(instance);
        return NULL;
    }

    return instance;
}

/**
 * @brief Free a ZenInstance
 * @param instance Instance to free
 */
static void instance_free(ZenInstance *instance)
{
    if (!instance) {
        return;
    }

    if (instance->class_def) {
        value_unref(instance->class_def);
    }
    if (instance->properties) {
        value_unref(instance->properties);
    }

    memory_free(instance);
}

/**
 * @brief Create a new class value
 * @param name Class name
 * @param parent_name Parent class name (may be NULL)
 * @return Newly allocated class value or NULL on failure
 */
Value *value_new_class(const char *name, const char *parent_name)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_CLASS;
    value->ref_count = 1;
    value->as.class_def = class_create(name, parent_name);
    if (!value->as.class_def) {
        memory_free(value);
        return NULL;
    }

    return value;
}

/**
 * @brief Create a new instance of a class
 * @param class_def Class definition value
 * @return Newly allocated instance value or NULL on failure
 */
Value *value_new_instance(Value *class_def)
{
    if (!class_def || class_def->type != VALUE_CLASS) {
        return NULL;
    }

    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_INSTANCE;
    value->ref_count = 1;
    value->as.instance = instance_create(class_def);
    if (!value->as.instance) {
        memory_free(value);
        return NULL;
    }

    return value;
}

/**
 * @brief Add a method to a class
 * @param class_val Class value
 * @param method_name Method name
 * @param method_func Method function value
 */
void value_class_add_method(Value *class_val, const char *method_name, Value *method_func)
{
    if (!class_val || class_val->type != VALUE_CLASS || !method_name || !method_func) {
        return;
    }

    if (!class_val->as.class_def || !class_val->as.class_def->methods) {
        return;
    }

    // Store methods in the class's methods object using object_set
    if (strcmp(method_name, "init") == 0) {
        // Special handling for constructor
        if (class_val->as.class_def->constructor) {
            value_unref(class_val->as.class_def->constructor);
        }
        class_val->as.class_def->constructor = value_ref(method_func);
    } else {
        // Store regular method in methods object
        object_set(class_val->as.class_def->methods, method_name, method_func);
    }
}

/**
 * @brief Get a method from a class
 * @param class_val Class value
 * @param method_name Method name to find
 * @return Method function value or NULL if not found
 */
Value *value_class_get_method(Value *class_val, const char *method_name)
{
    if (!class_val || class_val->type != VALUE_CLASS || !method_name) {
        return NULL;
    }

    if (!class_val->as.class_def) {
        return NULL;
    }

    // Check for constructor
    if (strcmp(method_name, "init") == 0) {
        return class_val->as.class_def->constructor;
    }

    // Look up method in methods object
    Value *method = object_get(class_val->as.class_def->methods, method_name);
    if (method) {
        return method;  // object_get already increments ref count
    }

    // Check parent class if method not found
    if (class_val->as.class_def->parent_class) {
        return value_class_get_method(class_val->as.class_def->parent_class, method_name);
    }

    return NULL;
}

/**
 * @brief Set a property on an instance
 * @param instance Instance value
 * @param property_name Property name
 * @param value Property value
 */
void value_instance_set_property(Value *instance, const char *property_name, Value *value)
{
    if (!instance || instance->type != VALUE_INSTANCE || !property_name || !value) {
        return;
    }

    if (!instance->as.instance || !instance->as.instance->properties) {
        return;
    }

    // Set property using object_set on the properties object
    object_set(instance->as.instance->properties, property_name, value);
}

/**
 * @brief Get a property from an instance
 * @param instance Instance value
 * @param property_name Property name to get
 * @return Property value or NULL if not found
 */
Value *value_instance_get_property(Value *instance, const char *property_name)
{
    if (!instance || instance->type != VALUE_INSTANCE || !property_name) {
        return NULL;
    }

    if (!instance->as.instance || !instance->as.instance->properties) {
        return NULL;
    }

    // Get property using object_get on the properties object
    return object_get(instance->as.instance->properties, property_name);
}

// Enhanced type system functions for advanced operations

/**
 * @brief Create an error value for stdlib functions
 * @param message Error message
 * @param error_code Error code
 * @return New error Value
 */
Value *value_new_error(const char *message, int error_code)
{
    Value *value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }

    value->type = VALUE_ERROR;
    value->ref_count = 1;
    value->as.error = memory_alloc(sizeof(ZenError));
    if (!value->as.error) {
        memory_free(value);
        return NULL;
    }

    value->as.error->message = message ? memory_strdup(message) : memory_strdup("Unknown error");
    value->as.error->code = error_code;

    if (!value->as.error->message) {
        memory_free(value->as.error);
        memory_free(value);
        return NULL;
    }

    return value;
}

/**
 * @brief Check if value is truthy (for conditional expressions)
 * @param value Value to test
 * @return true if truthy, false otherwise
 */
bool value_is_truthy_public(const Value *value)
{
    if (!value) {
        return false;
    }

    switch (value->type) {
    case VALUE_NULL:
        return false;
    case VALUE_BOOLEAN:
        return value->as.boolean;
    case VALUE_NUMBER:
        return value->as.number != 0.0 && !isnan(value->as.number);
    case VALUE_STRING:
        return value->as.string && value->as.string->data && value->as.string->length > 0;
    case VALUE_ARRAY:
        return value->as.array && value->as.array->length > 0;
    case VALUE_OBJECT:
        return value->as.object && value->as.object->length > 0;
    case VALUE_FUNCTION:
        return value->as.function != NULL;
    case VALUE_ERROR:
        return false;  // Errors are considered falsy
    case VALUE_CLASS:
        return value->as.class_def != NULL;
    case VALUE_INSTANCE:
        return value->as.instance != NULL;
    case VALUE_SET: {
        Value *size_val = set_size(value);
        bool is_truthy = size_val && size_val->type == VALUE_NUMBER && size_val->as.number > 0;
        if (size_val)
            value_unref(size_val);
        return is_truthy;
    }
    case VALUE_PRIORITY_QUEUE: {
        Value *empty_val = priority_queue_is_empty(value);
        bool is_truthy = empty_val && empty_val->type == VALUE_BOOLEAN && !empty_val->as.boolean;
        if (empty_val)
            value_unref(empty_val);
        return is_truthy;
    }
    default:
        return false;
    }
}

/**
 * @brief Safe conversion to number for stdlib operations
 * @param value Value to convert
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_or_nan(const Value *value)
{
    if (!value) {
        return NAN;
    }

    switch (value->type) {
    case VALUE_NULL:
        return 0.0;
    case VALUE_BOOLEAN:
        return value->as.boolean ? 1.0 : 0.0;
    case VALUE_NUMBER:
        return value->as.number;
    case VALUE_STRING: {
        if (!value->as.string || !value->as.string->data || value->as.string->length == 0) {
            return 0.0;
        }

        char *endptr;
        double result = strtod(value->as.string->data, &endptr);

        // Check if entire string was consumed (valid number)
        if (*endptr == '\0') {
            return result;
        }

        // Handle special cases
        if (strcmp(value->as.string->data, "Infinity") == 0) {
            return INFINITY;
        }
        if (strcmp(value->as.string->data, "-Infinity") == 0) {
            return -INFINITY;
        }
        if (strcmp(value->as.string->data, "NaN") == 0) {
            return NAN;
        }

        return NAN;  // Invalid string
    }
    case VALUE_ARRAY:
        return value->as.array ? (double)value->as.array->length : NAN;
    case VALUE_OBJECT:
        return value->as.object ? (double)value->as.object->length : NAN;
    default:
        return NAN;
    }
}

/**
 * @brief Enhanced string conversion with better error handling
 * @param value Value to convert
 * @return String representation or NULL on critical failure
 */
char *value_to_string_safe(const Value *value)
{
    if (!value) {
        return memory_strdup("null");
    }

    // Use the existing value_to_string implementation
    // but add additional safety checks
    char *result = value_to_string(value);
    if (!result) {
        // Fallback for critical failures
        switch (value->type) {
        case VALUE_NULL:
            return memory_strdup("null");
        case VALUE_BOOLEAN:
            return memory_strdup("<boolean>");
        case VALUE_UNDECIDABLE:
            return memory_strdup("undecidable");
        case VALUE_NUMBER:
            return memory_strdup("<number>");
        case VALUE_STRING:
            return memory_strdup("<string>");
        case VALUE_ARRAY:
            return memory_strdup("<array>");
        case VALUE_OBJECT:
            return memory_strdup("<object>");
        case VALUE_FUNCTION:
            return memory_strdup("<function>");
        case VALUE_ERROR:
            return memory_strdup("<error>");
        case VALUE_CLASS:
            return memory_strdup("<class>");
        case VALUE_INSTANCE:
            return memory_strdup("<instance>");
        default:
            return memory_strdup("<unknown>");
        }
    }

    return result;
}

/**
 * @brief Check if two values can be compared
 * @param a First value
 * @param b Second value
 * @return true if values are comparable
 */
bool value_is_comparable(const Value *a, const Value *b)
{
    if (!a || !b) {
        return true;  // Can compare with null
    }

    // Same types are always comparable
    if (a->type == b->type) {
        return true;
    }

    // Numbers and booleans can be compared with each other
    if ((a->type == VALUE_NUMBER && b->type == VALUE_BOOLEAN) ||
        (a->type == VALUE_BOOLEAN && b->type == VALUE_NUMBER)) {
        return true;
    }

    // Strings can be compared with numbers for lexicographic ordering
    if ((a->type == VALUE_STRING && b->type == VALUE_NUMBER) ||
        (a->type == VALUE_NUMBER && b->type == VALUE_STRING)) {
        return true;
    }

    // Arrays can be compared by length
    if (a->type == VALUE_ARRAY && b->type == VALUE_ARRAY) {
        return true;
    }

    // Objects can be compared by number of properties
    if (a->type == VALUE_OBJECT && b->type == VALUE_OBJECT) {
        return true;
    }

    return false;
}

/**
 * @brief Get the "length" of a value for stdlib length() function
 * @param value Value to get length of
 * @return Length or 0 if not applicable
 */
size_t value_get_length(const Value *value)
{
    if (!value) {
        return 0;
    }

    switch (value->type) {
    case VALUE_NULL:
        return 0;
    case VALUE_BOOLEAN:
        return 1;  // Booleans have conceptual length of 1
    case VALUE_NUMBER:
        return 1;  // Numbers have conceptual length of 1
    case VALUE_STRING:
        return value->as.string ? value->as.string->length : 0;
    case VALUE_ARRAY:
        return value->as.array ? value->as.array->length : 0;
    case VALUE_OBJECT:
        return value->as.object ? value->as.object->length : 0;
    case VALUE_FUNCTION:
        return 0;  // Functions don't have meaningful length
    case VALUE_ERROR:
        return value->as.error && value->as.error->message ? strlen(value->as.error->message) : 0;
    case VALUE_CLASS:
        return 0;  // Classes don't have meaningful length
    case VALUE_INSTANCE:
        return 0;  // Instances length would depend on properties
    case VALUE_SET: {
        Value *size_val = set_size(value);
        size_t length = 0;
        if (size_val && size_val->type == VALUE_NUMBER) {
            length = (size_t)size_val->as.number;
        }
        if (size_val)
            value_unref(size_val);
        return length;
    }
    case VALUE_PRIORITY_QUEUE: {
        Value *size_val = priority_queue_size(value);
        size_t length = 0;
        if (size_val && size_val->type == VALUE_NUMBER) {
            length = (size_t)size_val->as.number;
        }
        if (size_val)
            value_unref(size_val);
        return length;
    }
    default:
        return 0;
    }
}

// Set and priority queue constructors removed - use set_new and priority_queue_new instead
