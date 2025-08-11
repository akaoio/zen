/*
 * convert.c
 * Type conversion utilities for ZEN stdlib - Fixed version
 */

#define _GNU_SOURCE
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
RuntimeValue *convert_to_string_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_string("null");
    }

    // Check value type and convert accordingly
    if (value->type == RV_TYPE_NULL) {
        return rv_new_string("null");
    }
    
    if (value->type == RV_TYPE_BOOLEAN) {
        return rv_new_string(value->data.boolean ? "true" : "false");
    }
    
    if (value->type == RV_TYPE_NUMBER) {
        char buffer[64];
        double num = value->data.number;
        if (num == (long long)num && num >= LLONG_MIN && num <= LLONG_MAX) {
            snprintf(buffer, sizeof(buffer), "%.0f", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%.15g", num);
        }
        return rv_new_string(buffer);
    }
    
    if (value->type == RV_TYPE_STRING) {
        return rv_new_string(value->data.string);
    }

    if (value->type == RV_TYPE_ARRAY) {
        // For arrays, return simple string representation
        return rv_new_string("[Array]");
    }

    if (value->type == RV_TYPE_OBJECT) {
        // For objects, return simple string representation
        return rv_new_string("[Object]");
    }

    if (value->type == RV_TYPE_ERROR) {
        if (value->data.error && value->data.error->message) {
            char *error_str = memory_alloc(strlen("Error: ") + strlen(value->data.error->message) + 1);
            if (error_str) {
                sprintf(error_str, "Error: %s", value->data.error->message);
                RuntimeValue *result = rv_new_string(error_str);
                memory_free(error_str);
                return result;
            }
        }
        return rv_new_string("Error: unknown error");
    }

    if (value->type == RV_TYPE_FUNCTION) {
        return rv_new_string("[Function]");
    }

    return rv_new_string("unknown");
}

/**
 * @brief Convert value to number
 * @param value Value to convert
 * @return Number value representing the input value
 */
RuntimeValue *convert_to_number_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_number(0.0);
    }

    switch (value->type) {
    case RV_TYPE_NULL:
        return rv_new_number(0.0);

    case RV_TYPE_BOOLEAN:
        return rv_new_number(value->data.boolean ? 1.0 : 0.0);

    case RV_TYPE_NUMBER:
        return rv_new_number(value->data.number);

    case RV_TYPE_STRING: {
        if (!value->data.string) {
            return rv_new_number(0.0);
        }

        char *str = (char*)value->data.string;
        while (isspace(*str)) {
            str++;
        }

        if (*str == '\0') {
            return rv_new_number(0.0);
        }

        char *endptr;
        errno = 0;
        double result = strtod(str, &endptr);

        while (isspace(*endptr)) {
            endptr++;
        }

        if (errno == ERANGE || *endptr != '\0') {
            return rv_new_number(0.0);
        }

        return rv_new_number(result);
    }

    case RV_TYPE_ARRAY:
        // Array length as number - simplified for now
        return rv_new_number(0.0);

    case RV_TYPE_OBJECT:
        // Object property count as number - simplified for now
        return rv_new_number(0.0);

    case RV_TYPE_ERROR:
        if (value->data.error) {
            return rv_new_number((double)value->data.error->code);
        }
        return rv_new_number(-1.0);

    case RV_TYPE_FUNCTION:
        return rv_new_number(1.0);

    default:
        return rv_new_number(0.0);
    }
}

/**
 * @brief Convert value to boolean
 * @param value Value to convert
 * @return Boolean value representing the truthiness of input value
 */
RuntimeValue *convert_to_boolean_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_boolean(false);
    }

    switch (value->type) {
    case RV_TYPE_NULL:
        return rv_new_boolean(false);

    case RV_TYPE_BOOLEAN:
        return rv_new_boolean(value->data.boolean);

    case RV_TYPE_NUMBER:
        return rv_new_boolean(value->data.number != 0.0 && !isnan(value->data.number));

    case RV_TYPE_STRING:
        if (!value->data.string) {
            return rv_new_boolean(false);
        }
        return rv_new_boolean(strlen(value->data.string) > 0);

    case RV_TYPE_ARRAY:
    case RV_TYPE_OBJECT:
    case RV_TYPE_FUNCTION:
        return rv_new_boolean(true);

    case RV_TYPE_ERROR:
        return rv_new_boolean(false);

    default:
        return rv_new_boolean(false);
    }
}

/**
 * @brief Get the type name of a value
 * @param value Value to get type of
 * @return String value containing the type name
 */
RuntimeValue *convert_type_of_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_string("null");
    }

    switch (value->type) {
    case RV_TYPE_NULL:
        return rv_new_string("null");
    case RV_TYPE_BOOLEAN:
        return rv_new_string("boolean");
    case RV_TYPE_NUMBER:
        return rv_new_string("number");
    case RV_TYPE_STRING:
        return rv_new_string("string");
    case RV_TYPE_ARRAY:
        return rv_new_string("array");
    case RV_TYPE_OBJECT:
        return rv_new_string("object");
    case RV_TYPE_ERROR:
        return rv_new_string("error");
    case RV_TYPE_FUNCTION:
        return rv_new_string("function");
    default:
        return rv_new_string("unknown");
    }
}

/**
 * @brief Check if value is of specified type
 * @param value Value to check
 * @param type_name Type name to check against
 * @return Boolean value indicating if value is of specified type
 */
RuntimeValue *convert_is_type_internal(const RuntimeValue *value, const char *type_name)
{
    if (!type_name) {
        return rv_new_boolean(false);
    }

    RuntimeValue *type_val = convert_type_of_internal(value);
    if (!type_val || !type_val->data.string) {
        rv_unref(type_val);
        return rv_new_boolean(false);
    }

    bool result = strcmp(type_val->data.string, type_name) == 0;
    rv_unref(type_val);
    return rv_new_boolean(result);
}

/**
 * @brief Parse integer from string with specified base
 * @param str_value String value to parse
 * @param base_value Base value (2-36, default 10)
 * @return Number value containing parsed integer, or error on failure
 */
RuntimeValue *convert_parse_int_internal(const RuntimeValue *str_value, const RuntimeValue *base_value)
{
    if (!str_value || str_value->type != RV_TYPE_STRING) {
        return rv_new_error("parseint requires a string argument", -1);
    }

    if (!str_value->data.string) {
        return rv_new_number(0.0);
    }

    int base = 10;
    if (base_value && base_value->type == RV_TYPE_NUMBER) {
        base = (int)base_value->data.number;
        if (base < 2 || base > 36) {
            return rv_new_error("parseint base must be between 2 and 36", -1);
        }
    }

    char *str = (char*)str_value->data.string;
    while (isspace(*str)) {
        str++;
    }

    if (*str == '\0') {
        return rv_new_number(0.0);
    }

    char *endptr;
    errno = 0;
    long result = strtol(str, &endptr, base);

    if (errno == ERANGE) {
        return rv_new_error("parseint result out of range", -1);
    }

    return rv_new_number((double)result);
}

/**
 * @brief Parse float from string
 * @param str_value String value to parse
 * @return Number value containing parsed float, or error on failure
 */
RuntimeValue *convert_parse_float_internal(const RuntimeValue *str_value)
{
    if (!str_value || str_value->type != RV_TYPE_STRING) {
        return rv_new_error("parsefloat requires a string argument", -1);
    }

    if (!str_value->data.string) {
        return rv_new_number(0.0);
    }

    char *str = (char*)str_value->data.string;
    while (isspace(*str)) {
        str++;
    }

    if (*str == '\0') {
        return rv_new_number(0.0);
    }

    char *endptr;
    errno = 0;
    double result = strtod(str, &endptr);

    if (errno == ERANGE) {
        return rv_new_error("parsefloat result out of range", -1);
    }

    return rv_new_number(result);
}

// Public API Functions for stdlib integration

/**
 * @brief Convert value to string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return String representation of the value
 */
RuntimeValue *convert_to_string(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_string("");
    }
    return convert_to_string_internal(args[0]);
}

/**
 * @brief Convert value to number
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Number value or error
 */
RuntimeValue *convert_to_number(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_number(0);
    }
    return convert_to_number_internal(args[0]);
}

/**
 * @brief Convert value to boolean
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Boolean value
 */
RuntimeValue *convert_to_boolean(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_boolean(false);
    }
    return convert_to_boolean_internal(args[0]);
}

/**
 * @brief Get type name of value
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return String containing type name
 */
RuntimeValue *convert_type_of(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_string("undefined");
    }
    return convert_type_of_internal(args[0]);
}

/**
 * @brief Check if value is of specific type
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Boolean indicating if value is of the specified type
 */
RuntimeValue *convert_is_type(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || !args[1] || args[1]->type != RV_TYPE_STRING) {
        return rv_new_boolean(false);
    }
    return convert_is_type_internal(args[0], args[1]->data.string);
}

/**
 * @brief Parse integer from string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Parsed number or error
 */
RuntimeValue *convert_parse_int(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("parseInt requires at least 1 argument", -1);
    }

    RuntimeValue *base_value = (argc >= 2) ? args[1] : NULL;
    return convert_parse_int_internal(args[0], base_value);
}

/**
 * @brief Parse float from string
 * @param args Array of Value arguments
 * @param argc Number of arguments
 * @return Parsed number or error
 */
RuntimeValue *convert_parse_float(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("parseFloat requires 1 argument", -1);
    }
    return convert_parse_float_internal(args[0]);
}