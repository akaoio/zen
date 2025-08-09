/*
 * convert.c
 * Type conversion utilities for ZEN stdlib
 *
 * These are internal stdlib functions that extend the core functionality
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/convert.h"

#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Convert value to string
 * @param value Value to convert
 * @return String value representing the input value
 */
Value *convert_to_string_internal(const Value *value)
{
    if (!value) {
        return value_new_string("null");
    }

    switch (value->type) {
    case VALUE_NULL:
        return value_new_string("null");

    case VALUE_BOOLEAN:
        return value_new_string(value->as.boolean ? "true" : "false");

    case VALUE_NUMBER: {
        char buffer[64];
        // Check if it's an integer
        double num = value->as.number;
        if (num == (long long)num && num >= LLONG_MIN && num <= LLONG_MAX) {
            snprintf(buffer, sizeof(buffer), "%.0f", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%.15g", num);
        }
        return value_new_string(buffer);
    }

    case VALUE_STRING:
        return value_copy(value);

    case VALUE_ARRAY: {
        // Convert array to string representation like "[item1, item2, item3]"
        if (!value->as.array) {
            return value_new_string("[]");
        }

        // Calculate approximate size needed
        size_t total_size = 2;  // "[]"
        for (size_t i = 0; i < value->as.array->length; i++) {
            Value *item_str = convert_to_string_internal(value->as.array->items[i]);
            if (item_str && item_str->as.string) {
                total_size += item_str->as.string->length + 2;  // ", "
            }
            value_unref(item_str);
        }

        char *result = memory_alloc(total_size + 1);
        if (!result) {
            return value_new_string("[]");
        }

        strcpy(result, "[");
        for (size_t i = 0; i < value->as.array->length; i++) {
            if (i > 0) {
                strcat(result, ", ");
            }
            Value *item_str = convert_to_string_internal(value->as.array->items[i]);
            if (item_str && item_str->as.string) {
                strcat(result, item_str->as.string->data);
            }
            value_unref(item_str);
        }
        strcat(result, "]");

        Value *result_value = value_new_string(result);
        memory_free(result);
        return result_value;
    }

    case VALUE_OBJECT: {
        // Convert object to string representation like "{key1: value1, key2: value2}"
        if (!value->as.object) {
            return value_new_string("{}");
        }

        // Calculate approximate size needed
        size_t total_size = 2;  // "{}"
        for (size_t i = 0; i < value->as.object->length; i++) {
            ZenObjectPair *pair = &value->as.object->pairs[i];
            if (pair->key) {
                total_size += strlen(pair->key) + 2;  // ": "
                Value *val_str = convert_to_string_internal(pair->value);
                if (val_str && val_str->as.string) {
                    total_size += val_str->as.string->length + 2;  // ", "
                }
                value_unref(val_str);
            }
        }

        char *result = memory_alloc(total_size + 1);
        if (!result) {
            return value_new_string("{}");
        }

        strcpy(result, "{");
        bool first = true;
        for (size_t i = 0; i < value->as.object->length; i++) {
            ZenObjectPair *pair = &value->as.object->pairs[i];
            if (pair->key) {
                if (!first) {
                    strcat(result, ", ");
                }
                strcat(result, pair->key);
                strcat(result, ": ");

                Value *val_str = convert_to_string_internal(pair->value);
                if (val_str && val_str->as.string) {
                    strcat(result, val_str->as.string->data);
                }
                value_unref(val_str);
                first = false;
            }
        }
        strcat(result, "}");

        Value *result_value = value_new_string(result);
        memory_free(result);
        return result_value;
    }

    case VALUE_ERROR:
        if (value->as.error && value->as.error->message) {
            char *error_str =
                memory_alloc(strlen("Error: ") + strlen(value->as.error->message) + 1);
            if (error_str) {
                sprintf(error_str, "Error: %s", value->as.error->message);
                Value *result = value_new_string(error_str);
                memory_free(error_str);
                return result;
            }
        }
        return value_new_string("Error: unknown error");

    case VALUE_FUNCTION:
        return value_new_string("[Function]");

    default:
        return value_new_string("unknown");
    }
}

/**
 * @brief Convert value to number
 * @param value Value to convert
 * @return Number value representing the input value
 */
Value *convert_to_number_internal(const Value *value)
{
    if (!value) {
        return value_new_number(0.0);
    }

    switch (value->type) {
    case VALUE_NULL:
        return value_new_number(0.0);

    case VALUE_BOOLEAN:
        return value_new_number(value->as.boolean ? 1.0 : 0.0);

    case VALUE_NUMBER:
        return value_copy(value);

    case VALUE_STRING: {
        if (!value->as.string || !value->as.string->data) {
            return value_new_number(0.0);
        }

        char *str = value->as.string->data;

        // Skip leading whitespace
        while (isspace(*str)) {
            str++;
        }

        // Empty string or only whitespace
        if (*str == '\0') {
            return value_new_number(0.0);
        }

        char *endptr;
        errno = 0;
        double result = strtod(str, &endptr);

        // Skip trailing whitespace
        while (isspace(*endptr)) {
            endptr++;
        }

        // Check for conversion errors
        if (errno == ERANGE || *endptr != '\0') {
            return value_new_number(0.0);  // NaN could be used here
        }

        return value_new_number(result);
    }

    case VALUE_ARRAY:
        // Array length as number
        if (value->as.array) {
            return value_new_number((double)value->as.array->length);
        }
        return value_new_number(0.0);

    case VALUE_OBJECT:
        // Object property count as number
        if (value->as.object) {
            return value_new_number((double)value->as.object->length);
        }
        return value_new_number(0.0);

    case VALUE_ERROR:
        // Error code as number
        if (value->as.error) {
            return value_new_number((double)value->as.error->code);
        }
        return value_new_number(-1.0);

    case VALUE_FUNCTION:
        return value_new_number(1.0);  // Functions are truthy

    default:
        return value_new_number(0.0);
    }
}

/**
 * @brief Convert value to boolean
 * @param value Value to convert
 * @return Boolean value representing the truthiness of input value
 */
Value *convert_to_boolean_internal(const Value *value)
{
    if (!value) {
        return value_new_boolean(false);
    }

    switch (value->type) {
    case VALUE_NULL:
        return value_new_boolean(false);

    case VALUE_BOOLEAN:
        return value_copy(value);

    case VALUE_NUMBER:
        // 0, -0, NaN are falsy
        return value_new_boolean(value->as.number != 0.0 && !isnan(value->as.number));

    case VALUE_STRING:
        // Empty string is falsy
        if (!value->as.string || !value->as.string->data) {
            return value_new_boolean(false);
        }
        return value_new_boolean(value->as.string->length > 0);

    case VALUE_ARRAY:
        // Arrays are always truthy (even empty ones in ZEN)
        return value_new_boolean(true);

    case VALUE_OBJECT:
        // Objects are always truthy (even empty ones in ZEN)
        return value_new_boolean(true);

    case VALUE_ERROR:
        // Errors are falsy
        return value_new_boolean(false);

    case VALUE_FUNCTION:
        // Functions are truthy
        return value_new_boolean(true);

    default:
        return value_new_boolean(false);
    }
}

/**
 * @brief Get the type name of a value
 * @param value Value to get type of
 * @return String value containing the type name
 */
Value *convert_type_of_internal(const Value *value)
{
    if (!value) {
        return value_new_string("null");
    }

    switch (value->type) {
    case VALUE_NULL:
        return value_new_string("null");
    case VALUE_BOOLEAN:
        return value_new_string("boolean");
    case VALUE_NUMBER:
        return value_new_string("number");
    case VALUE_STRING:
        return value_new_string("string");
    case VALUE_ARRAY:
        return value_new_string("array");
    case VALUE_OBJECT:
        return value_new_string("object");
    case VALUE_ERROR:
        return value_new_string("error");
    case VALUE_FUNCTION:
        return value_new_string("function");
    default:
        return value_new_string("unknown");
    }
}

/**
 * @brief Check if value is of specified type
 * @param value Value to check
 * @param type_name Type name to check against
 * @return Boolean value indicating if value is of specified type
 */
Value *convert_is_type_internal(const Value *value, const char *type_name)
{
    if (!type_name) {
        return value_new_boolean(false);
    }

    Value *type_val = convert_type_of_internal(value);
    if (!type_val || !type_val->as.string) {
        value_unref(type_val);
        return value_new_boolean(false);
    }

    bool result = strcmp(type_val->as.string->data, type_name) == 0;
    value_unref(type_val);
    return value_new_boolean(result);
}

/**
 * @brief Parse integer from string with specified base
 * @param str_value String value to parse
 * @param base_value Base value (2-36, default 10)
 * @return Number value containing parsed integer, or error on failure
 */
Value *convert_parse_int_internal(const Value *str_value, const Value *base_value)
{
    if (!str_value || str_value->type != VALUE_STRING) {
        Value *error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("parseint requires a string argument");
            error->as.error->code = -1;
        }
        return error;
    }

    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_number(0.0);
    }

    int base = 10;
    if (base_value && base_value->type == VALUE_NUMBER) {
        base = (int)base_value->as.number;
        if (base < 2 || base > 36) {
            Value *error = value_new(VALUE_ERROR);
            if (error && error->as.error) {
                error->as.error->message = memory_strdup("parseint base must be between 2 and 36");
                error->as.error->code = -1;
            }
            return error;
        }
    }

    char *str = str_value->as.string->data;

    // Skip leading whitespace
    while (isspace(*str)) {
        str++;
    }

    if (*str == '\0') {
        return value_new_number(0.0);
    }

    char *endptr;
    errno = 0;
    long result = strtol(str, &endptr, base);

    if (errno == ERANGE) {
        Value *error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("parseint result out of range");
            error->as.error->code = -1;
        }
        return error;
    }

    return value_new_number((double)result);
}

/**
 * @brief Parse float from string
 * @param str_value String value to parse
 * @return Number value containing parsed float, or error on failure
 */
Value *convert_parse_float_internal(const Value *str_value)
{
    if (!str_value || str_value->type != VALUE_STRING) {
        Value *error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("parsefloat requires a string argument");
            error->as.error->code = -1;
        }
        return error;
    }

    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_number(0.0);
    }

    char *str = str_value->as.string->data;

    // Skip leading whitespace
    while (isspace(*str)) {
        str++;
    }

    if (*str == '\0') {
        return value_new_number(0.0);
    }

    char *endptr;
    errno = 0;
    double result = strtod(str, &endptr);

    if (errno == ERANGE) {
        Value *error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("parsefloat result out of range");
            error->as.error->code = -1;
        }
        return error;
    }

    return value_new_number(result);
}

// Public API Functions for stdlib integration

/**
 * @brief Convert value to string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return String representation of the value
 */
Value *convert_to_string(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_string("");
    }
    return convert_to_string_internal(args[0]);
}

/**
 * @brief Convert value to number
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Number value or error
 */
Value *convert_to_number(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_number(0);
    }
    return convert_to_number_internal(args[0]);
}

/**
 * @brief Convert value to boolean
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Boolean value
 */
Value *convert_to_boolean(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_boolean(false);
    }
    return convert_to_boolean_internal(args[0]);
}

/**
 * @brief Get type name of value
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return String containing type name
 */
Value *convert_type_of(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_string("undefined");
    }
    return convert_type_of_internal(args[0]);
}

/**
 * @brief Check if value is of specific type
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Boolean indicating if value is of the specified type
 */
Value *convert_is_type(Value **args, size_t argc)
{
    if (argc < 2 || !args[1] || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    return convert_is_type_internal(args[0], args[1]->as.string->data);
}

/**
 * @brief Parse integer from string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Parsed number or error
 */
Value *convert_parse_int(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_error("parseInt requires at least 1 argument", -1);
    }

    Value *base_value = (argc >= 2) ? args[1] : NULL;
    return convert_parse_int_internal(args[0], base_value);
}

/**
 * @brief Parse float from string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Parsed number or error
 */
Value *convert_parse_float(Value **args, size_t argc)
{
    if (argc < 1) {
        return value_new_error("parseFloat requires 1 argument", -1);
    }
    return convert_parse_float_internal(args[0]);
}