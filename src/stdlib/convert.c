/*
 * convert.c
 * Type conversion functions for ZEN stdlib
 *
 * This file follows MANIFEST.json specification
 * Function signatures must match manifest exactly
 */

#include "zen/stdlib/convert.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal conversion functions
RuntimeValue *convert_to_string_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_string("null");
    }

    switch (value->type) {
    case RV_NULL:
        return rv_new_string("null");

    case RV_BOOLEAN:
        return rv_new_string(value->data.boolean ? "true" : "false");

    case RV_NUMBER: {
        char buffer[32];
        double num = value->data.number;
        // Check if it's an integer
        if (num == floor(num)) {
            snprintf(buffer, sizeof(buffer), "%.0f", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%g", num);
        }
        return rv_new_string(buffer);
    }

    case RV_STRING:
        return rv_copy((RuntimeValue *)value);

    case RV_ARRAY:
        return rv_new_string("[Array]");

    case RV_OBJECT:
        return rv_new_string("[Object]");

    case RV_ERROR:
        return rv_new_string("[Error]");

    default:
        return rv_new_string("[Unknown]");
    }
}

RuntimeValue *convert_to_number_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_number(0.0);
    }

    switch (value->type) {
    case RV_NULL:
        return rv_new_number(0.0);

    case RV_BOOLEAN:
        return rv_new_number(value->data.boolean ? 1.0 : 0.0);

    case RV_NUMBER:
        return rv_copy((RuntimeValue *)value);

    case RV_STRING: {
        if (!value->data.string.data) {
            return rv_new_number(0.0);
        }

        char *str = value->data.string.data;

        // Skip whitespace
        while (isspace(*str))
            str++;

        if (*str == '\0') {
            return rv_new_number(0.0);
        }

        char *endptr;
        errno = 0;
        double result = strtod(str, &endptr);

        // Skip trailing whitespace
        while (isspace(*endptr))
            endptr++;

        if (*endptr != '\0' || errno == ERANGE) {
            return rv_new_number(NAN);  // Invalid conversion
        }

        return rv_new_number(result);
    }

    case RV_ARRAY:
        return rv_new_number(NAN);

    case RV_OBJECT:
        return rv_new_number(NAN);

    case RV_ERROR:
        return rv_new_number(NAN);

    default:
        return rv_new_number(NAN);
    }
}

RuntimeValue *convert_to_boolean_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_boolean(false);
    }

    switch (value->type) {
    case RV_NULL:
        return rv_new_boolean(false);

    case RV_BOOLEAN:
        return rv_copy((RuntimeValue *)value);

    case RV_NUMBER:
        return rv_new_boolean(value->data.number != 0.0 && !isnan(value->data.number));

    case RV_STRING:
        return rv_new_boolean(value->data.string.data != NULL && value->data.string.length > 0);

    case RV_ARRAY:
        return rv_new_boolean(value->data.array.count > 0);

    case RV_OBJECT:
        return rv_new_boolean(value->data.object.count > 0);

    case RV_ERROR:
        return rv_new_boolean(false);

    default:
        return rv_new_boolean(false);
    }
}

RuntimeValue *convert_type_of_internal(const RuntimeValue *value)
{
    if (!value) {
        return rv_new_string("null");
    }

    switch (value->type) {
    case RV_NULL:
        return rv_new_string("null");
    case RV_BOOLEAN:
        return rv_new_string("boolean");
    case RV_NUMBER:
        return rv_new_string("number");
    case RV_STRING:
        return rv_new_string("string");
    case RV_ARRAY:
        return rv_new_string("array");
    case RV_OBJECT:
        return rv_new_string("object");
    case RV_FUNCTION:
        return rv_new_string("function");
    case RV_ERROR:
        return rv_new_string("error");
    default:
        return rv_new_string("unknown");
    }
}

// Stdlib wrapper functions
RuntimeValue *convert_to_string(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("toString() requires exactly 1 argument", -1);
    }
    return convert_to_string_internal(args[0]);
}

RuntimeValue *convert_to_number(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("toNumber() requires exactly 1 argument", -1);
    }
    return convert_to_number_internal(args[0]);
}

RuntimeValue *convert_to_boolean(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("toBoolean() requires exactly 1 argument", -1);
    }
    return convert_to_boolean_internal(args[0]);
}

RuntimeValue *convert_type_of(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("typeOf() requires exactly 1 argument", -1);
    }
    return convert_type_of_internal(args[0]);
}

RuntimeValue *convert_is_type(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("isType() requires exactly 2 arguments", -1);
    }

    if (!args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("isType() requires a string type name as second argument", -1);
    }

    RuntimeValue *type_str = convert_type_of_internal(args[0]);
    if (!type_str || type_str->type != RV_STRING) {
        return rv_new_boolean(false);
    }

    bool result = strcmp(type_str->data.string.data, args[1]->data.string.data) == 0;
    rv_unref(type_str);
    return rv_new_boolean(result);
}

RuntimeValue *convert_parse_int(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("parseInt() requires exactly 1 argument", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("parseInt() requires a string argument", -1);
    }

    if (!args[0]->data.string.data) {
        return rv_new_number(NAN);
    }

    char *str = args[0]->data.string.data;
    char *endptr;

    // Skip whitespace
    while (isspace(*str))
        str++;

    long result = strtol(str, &endptr, 10);

    if (str == endptr) {
        return rv_new_number(NAN);  // No digits found
    }

    return rv_new_number((double)result);
}

RuntimeValue *convert_parse_float(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("parseFloat() requires exactly 1 argument", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("parseFloat() requires a string argument", -1);
    }

    if (!args[0]->data.string.data) {
        return rv_new_number(NAN);
    }

    char *str = args[0]->data.string.data;
    char *endptr;

    // Skip whitespace
    while (isspace(*str))
        str++;

    double result = strtod(str, &endptr);

    if (str == endptr) {
        return rv_new_number(NAN);  // No digits found
    }

    return rv_new_number(result);
}