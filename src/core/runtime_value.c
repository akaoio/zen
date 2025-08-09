#include "zen/core/runtime_value.h"

#include "zen/core/memory.h"

#include <stdio.h>
#include <string.h>

// Creation functions

RuntimeValue *rv_new_null(void)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_NULL;
    rv->ref_count = 1;
    return rv;
}

RuntimeValue *rv_new_number(double value)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_NUMBER;
    rv->ref_count = 1;
    rv->data.number = value;
    return rv;
}

RuntimeValue *rv_new_string(const char *value)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_STRING;
    rv->ref_count = 1;

    if (value) {
        rv->data.string.length = strlen(value);
        rv->data.string.data = memory_strdup(value);
        if (!rv->data.string.data) {
            memory_free(rv);
            return NULL;
        }
    } else {
        rv->data.string.data = NULL;
        rv->data.string.length = 0;
    }

    return rv;
}

RuntimeValue *rv_new_boolean(bool value)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_BOOLEAN;
    rv->ref_count = 1;
    rv->data.boolean = value;
    return rv;
}

RuntimeValue *rv_new_array(void)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_ARRAY;
    rv->ref_count = 1;
    rv->data.array.elements = NULL;
    rv->data.array.count = 0;
    rv->data.array.capacity = 0;
    return rv;
}

RuntimeValue *rv_new_object(void)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_OBJECT;
    rv->ref_count = 1;
    rv->data.object.keys = NULL;
    rv->data.object.values = NULL;
    rv->data.object.count = 0;
    rv->data.object.capacity = 0;
    return rv;
}

RuntimeValue *rv_new_function(void *ast_node, void *scope)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_FUNCTION;
    rv->ref_count = 1;
    rv->data.function.ast_node = ast_node;
    rv->data.function.scope = scope;
    return rv;
}

RuntimeValue *rv_new_error(const char *message, int code)
{
    RuntimeValue *rv = memory_alloc(sizeof(RuntimeValue));
    if (!rv)
        return NULL;

    rv->type = RV_ERROR;
    rv->ref_count = 1;
    rv->data.error.message = message ? memory_strdup(message) : NULL;
    rv->data.error.code = code;
    return rv;
}

// Reference counting

RuntimeValue *rv_ref(RuntimeValue *value)
{
    if (value) {
        value->ref_count++;
    }
    return value;
}

void rv_unref(RuntimeValue *value)
{
    if (!value)
        return;

    value->ref_count--;
    if (value->ref_count > 0)
        return;

    // Free resources based on type
    switch (value->type) {
    case RV_STRING:
        if (value->data.string.data) {
            memory_free(value->data.string.data);
        }
        break;

    case RV_ARRAY:
        if (value->data.array.elements) {
            for (size_t i = 0; i < value->data.array.count; i++) {
                rv_unref(value->data.array.elements[i]);
            }
            memory_free(value->data.array.elements);
        }
        break;

    case RV_OBJECT:
        if (value->data.object.keys) {
            for (size_t i = 0; i < value->data.object.count; i++) {
                memory_free(value->data.object.keys[i]);
                rv_unref(value->data.object.values[i]);
            }
            memory_free(value->data.object.keys);
            memory_free(value->data.object.values);
        }
        break;

    case RV_ERROR:
        if (value->data.error.message) {
            memory_free(value->data.error.message);
        }
        break;

    default:
        break;
    }

    memory_free(value);
}

// Array operations

void rv_array_push(RuntimeValue *array, RuntimeValue *element)
{
    if (!array || array->type != RV_ARRAY || !element)
        return;

    // Grow array if needed
    if (array->data.array.count >= array->data.array.capacity) {
        size_t new_capacity = array->data.array.capacity == 0 ? 8 : array->data.array.capacity * 2;
        RuntimeValue **new_elements =
            memory_realloc(array->data.array.elements, sizeof(RuntimeValue *) * new_capacity);
        if (!new_elements)
            return;

        array->data.array.elements = new_elements;
        array->data.array.capacity = new_capacity;
    }

    array->data.array.elements[array->data.array.count++] = rv_ref(element);
}

RuntimeValue *rv_array_get(RuntimeValue *array, size_t index)
{
    if (!array || array->type != RV_ARRAY || index >= array->data.array.count) {
        return NULL;
    }

    return array->data.array.elements[index];
}

void rv_array_set(RuntimeValue *array, size_t index, RuntimeValue *element)
{
    if (!array || array->type != RV_ARRAY || index >= array->data.array.count || !element) {
        return;
    }

    rv_unref(array->data.array.elements[index]);
    array->data.array.elements[index] = rv_ref(element);
}

size_t rv_array_length(RuntimeValue *array)
{
    if (!array || array->type != RV_ARRAY) {
        return 0;
    }

    return array->data.array.count;
}

// Object operations

void rv_object_set(RuntimeValue *object, const char *key, RuntimeValue *value)
{
    if (!object || object->type != RV_OBJECT || !key || !value)
        return;

    // Check if key already exists
    for (size_t i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.keys[i], key) == 0) {
            rv_unref(object->data.object.values[i]);
            object->data.object.values[i] = rv_ref(value);
            return;
        }
    }

    // Grow arrays if needed
    if (object->data.object.count >= object->data.object.capacity) {
        size_t new_capacity =
            object->data.object.capacity == 0 ? 8 : object->data.object.capacity * 2;

        char **new_keys = memory_realloc(object->data.object.keys, sizeof(char *) * new_capacity);
        if (!new_keys)
            return;

        RuntimeValue **new_values =
            memory_realloc(object->data.object.values, sizeof(RuntimeValue *) * new_capacity);
        if (!new_values)
            return;

        object->data.object.keys = new_keys;
        object->data.object.values = new_values;
        object->data.object.capacity = new_capacity;
    }

    // Add new key-value pair
    object->data.object.keys[object->data.object.count] = memory_strdup(key);
    object->data.object.values[object->data.object.count] = rv_ref(value);
    object->data.object.count++;
}

RuntimeValue *rv_object_get(RuntimeValue *object, const char *key)
{
    if (!object || object->type != RV_OBJECT || !key)
        return NULL;

    for (size_t i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.keys[i], key) == 0) {
            return object->data.object.values[i];
        }
    }

    return NULL;
}

bool rv_object_has(RuntimeValue *object, const char *key)
{
    return rv_object_get(object, key) != NULL;
}

void rv_object_delete(RuntimeValue *object, const char *key)
{
    if (!object || object->type != RV_OBJECT || !key)
        return;

    for (size_t i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.keys[i], key) == 0) {
            memory_free(object->data.object.keys[i]);
            rv_unref(object->data.object.values[i]);

            // Shift remaining elements
            for (size_t j = i; j < object->data.object.count - 1; j++) {
                object->data.object.keys[j] = object->data.object.keys[j + 1];
                object->data.object.values[j] = object->data.object.values[j + 1];
            }

            object->data.object.count--;
            return;
        }
    }
}

// Type checking

bool rv_is_null(RuntimeValue *value) { return value && value->type == RV_NULL; }

bool rv_is_number(RuntimeValue *value) { return value && value->type == RV_NUMBER; }

bool rv_is_string(RuntimeValue *value) { return value && value->type == RV_STRING; }

bool rv_is_boolean(RuntimeValue *value) { return value && value->type == RV_BOOLEAN; }

bool rv_is_array(RuntimeValue *value) { return value && value->type == RV_ARRAY; }

bool rv_is_object(RuntimeValue *value) { return value && value->type == RV_OBJECT; }

bool rv_is_function(RuntimeValue *value) { return value && value->type == RV_FUNCTION; }

bool rv_is_error(RuntimeValue *value) { return value && value->type == RV_ERROR; }

bool rv_is_truthy(RuntimeValue *value)
{
    if (!value || value->type == RV_NULL)
        return false;
    if (value->type == RV_BOOLEAN)
        return value->data.boolean;
    if (value->type == RV_NUMBER)
        return value->data.number != 0.0;
    if (value->type == RV_STRING)
        return value->data.string.length > 0;
    return true;  // Arrays, objects, functions are truthy
}

// Utility functions

RuntimeValue *rv_copy(RuntimeValue *value)
{
    if (!value)
        return NULL;

    switch (value->type) {
    case RV_NULL:
        return rv_new_null();
    case RV_NUMBER:
        return rv_new_number(value->data.number);
    case RV_STRING:
        return rv_new_string(value->data.string.data);
    case RV_BOOLEAN:
        return rv_new_boolean(value->data.boolean);
    case RV_ARRAY: {
        RuntimeValue *copy = rv_new_array();
        for (size_t i = 0; i < value->data.array.count; i++) {
            rv_array_push(copy, value->data.array.elements[i]);
        }
        return copy;
    }
    case RV_OBJECT: {
        RuntimeValue *copy = rv_new_object();
        for (size_t i = 0; i < value->data.object.count; i++) {
            rv_object_set(copy, value->data.object.keys[i], value->data.object.values[i]);
        }
        return copy;
    }
    case RV_FUNCTION:
        return rv_new_function(value->data.function.ast_node, value->data.function.scope);
    case RV_ERROR:
        return rv_new_error(value->data.error.message, value->data.error.code);
    }

    return NULL;
}

char *rv_to_string(RuntimeValue *value)
{
    if (!value)
        return memory_strdup("null");

    char buffer[1024];

    switch (value->type) {
    case RV_NULL:
        return memory_strdup("null");

    case RV_NUMBER:
        snprintf(buffer, sizeof(buffer), "%g", value->data.number);
        return memory_strdup(buffer);

    case RV_STRING:
        return memory_strdup(value->data.string.data ? value->data.string.data : "");

    case RV_BOOLEAN:
        return memory_strdup(value->data.boolean ? "true" : "false");

    case RV_ARRAY: {
        if (value->data.array.count == 0) {
            return memory_strdup("[]");
        }

        // Calculate required buffer size
        size_t buffer_size = 3;  // "[]\0"
        for (size_t i = 0; i < value->data.array.count; i++) {
            if (value->data.array.elements[i]) {
                char *elem_str = rv_to_string(value->data.array.elements[i]);
                if (elem_str) {
                    buffer_size += strlen(elem_str) + 3;  // element + ", "
                    memory_free(elem_str);
                }
            }
        }

        char *result = memory_alloc(buffer_size);
        if (!result)
            return memory_strdup("[]");

        strcpy(result, "[");
        for (size_t i = 0; i < value->data.array.count; i++) {
            if (i > 0)
                strcat(result, ", ");

            if (value->data.array.elements[i]) {
                char *elem_str = rv_to_string(value->data.array.elements[i]);
                if (elem_str) {
                    strcat(result, elem_str);
                    memory_free(elem_str);
                } else {
                    strcat(result, "null");
                }
            } else {
                strcat(result, "null");
            }
        }
        strcat(result, "]");
        return result;
    }

    case RV_OBJECT: {
        if (value->data.object.count == 0) {
            return memory_strdup("{}");
        }

        // Calculate required buffer size
        size_t buffer_size = 3;  // "{}\0"
        for (size_t i = 0; i < value->data.object.count; i++) {
            if (value->data.object.keys[i]) {
                buffer_size += strlen(value->data.object.keys[i]) + 4;  // "key":
            }
            if (value->data.object.values[i]) {
                char *value_str = rv_to_string(value->data.object.values[i]);
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
        for (size_t i = 0; i < value->data.object.count; i++) {
            if (i > 0)
                strcat(result, ", ");

            if (value->data.object.keys[i]) {
                strcat(result, value->data.object.keys[i]);
                strcat(result, ": ");
            }

            if (value->data.object.values[i]) {
                char *value_str = rv_to_string(value->data.object.values[i]);
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

    case RV_FUNCTION:
        return memory_strdup("[function]");

    case RV_ERROR:
        snprintf(buffer,
                 sizeof(buffer),
                 "[error: %s]",
                 value->data.error.message ? value->data.error.message : "unknown");
        return memory_strdup(buffer);
    }

    return memory_strdup("unknown");
}

const char *rv_get_string(RuntimeValue *value)
{
    if (!value || value->type != RV_STRING) {
        return "";
    }
    return value->data.string.data ? value->data.string.data : "";
}

bool rv_equals(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b)
        return a == b;
    if (a->type != b->type)
        return false;

    switch (a->type) {
    case RV_NULL:
        return true;

    case RV_NUMBER:
        return a->data.number == b->data.number;

    case RV_STRING:
        if (!a->data.string.data || !b->data.string.data) {
            return a->data.string.data == b->data.string.data;
        }
        return strcmp(a->data.string.data, b->data.string.data) == 0;

    case RV_BOOLEAN:
        return a->data.boolean == b->data.boolean;

    case RV_ARRAY:
    case RV_OBJECT:
    case RV_FUNCTION:
    case RV_ERROR:
        // Reference equality for complex types
        return a == b;
    }

    return false;
}

const char *rv_type_name(RuntimeValue *value)
{
    if (!value)
        return "null";

    switch (value->type) {
    case RV_NULL:
        return "null";
    case RV_NUMBER:
        return "number";
    case RV_STRING:
        return "string";
    case RV_BOOLEAN:
        return "boolean";
    case RV_ARRAY:
        return "array";
    case RV_OBJECT:
        return "object";
    case RV_FUNCTION:
        return "function";
    case RV_ERROR:
        return "error";
    }

    return "unknown";
}
