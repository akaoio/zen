/*
 * stdlib.c
 * ZEN Standard Library - Main registry and initialization
 * 
 * This module provides the main interface for registering all stdlib functions
 * with the ZEN runtime system.
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/stdlib.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/stdlib/string.h"
#include "zen/stdlib/math.h"
#include "zen/stdlib/convert.h"
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>

// Native function typedef for runtime integration
typedef Value* (*ZenNativeFunc)(Value** args, size_t argc);

// Global registry of stdlib functions
static ZenStdlibFunction stdlib_functions[] = {
    // I/O Functions
    {"print", zen_stdlib_print, "Print value to stdout with newline"},
    {"input", zen_stdlib_input, "Read line from stdin"},
    {"readFile", zen_stdlib_read_file, "Read entire file contents"},
    {"writeFile", zen_stdlib_write_file, "Write string to file"},
    {"appendFile", zen_stdlib_append_file, "Append string to file"},
    {"fileExists", zen_stdlib_file_exists, "Check if file exists"},
    
    // String Functions
    {"len", zen_stdlib_length, "Get length of string or array"},
    {"upper", zen_stdlib_upper, "Convert string to uppercase"},
    {"lower", zen_stdlib_lower, "Convert string to lowercase"},
    {"trim", zen_stdlib_trim, "Trim whitespace from string"},
    {"split", zen_stdlib_split, "Split string by delimiter"},
    {"contains", zen_stdlib_contains, "Check if string contains substring"},
    {"replace", zen_stdlib_replace, "Replace substring in string"},
    
    // Math Functions
    {"abs", zen_stdlib_abs, "Absolute value"},
    {"floor", zen_stdlib_floor, "Floor function"},
    {"ceil", zen_stdlib_ceil, "Ceiling function"},
    {"round", zen_stdlib_round, "Round to nearest integer"},
    {"sqrt", zen_stdlib_sqrt, "Square root"},
    {"pow", zen_stdlib_pow, "Power function"},
    {"sin", zen_stdlib_sin, "Sine function"},
    {"cos", zen_stdlib_cos, "Cosine function"},
    {"tan", zen_stdlib_tan, "Tangent function"},
    {"log", zen_stdlib_log, "Natural logarithm"},
    {"random", zen_stdlib_random, "Random number 0-1"},
    {"randomInt", zen_stdlib_random_int, "Random integer in range"},
    {"min", zen_stdlib_min, "Minimum of two numbers"},
    {"max", zen_stdlib_max, "Maximum of two numbers"},
    {"isNaN", zen_stdlib_is_nan, "Check if number is NaN"},
    {"isInfinite", zen_stdlib_is_infinite, "Check if number is infinite"},
    
    // Type Conversion Functions
    {"toString", zen_stdlib_to_string, "Convert value to string"},
    {"toNumber", zen_stdlib_to_number, "Convert value to number"},
    {"toBoolean", zen_stdlib_to_boolean, "Convert value to boolean"},
    {"typeOf", zen_stdlib_type_of, "Get type name of value"},
    {"isType", zen_stdlib_is_type, "Check if value is of type"},
    {"parseInt", zen_stdlib_parse_int, "Parse integer from string"},
    {"parseFloat", zen_stdlib_parse_float, "Parse float from string"},
    
    // JSON Functions
    {"jsonParse", zen_stdlib_json_parse, "Parse JSON string"},
    {"jsonStringify", zen_stdlib_json_stringify, "Convert value to JSON string"},
    
    // Sentinel - must be last
    {NULL, NULL, NULL}
};

/**
 * @brief Get count of stdlib functions
 * @param None No parameters
 * @return Number of stdlib functions available
 */
size_t zen_stdlib_count() {
    size_t count = 0;
    while (stdlib_functions[count].name != NULL) {
        count++;
    }
    return count;
}

/**
 * @brief Get stdlib function by name
 * @param name Function name to look up
 * @return Pointer to function info, or NULL if not found
 */
const ZenStdlibFunction* zen_stdlib_get(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (size_t i = 0; stdlib_functions[i].name != NULL; i++) {
        if (strcmp(stdlib_functions[i].name, name) == 0) {
            return &stdlib_functions[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Get all stdlib functions
 * @param None No parameters
 * @return Array of all stdlib functions (terminated by NULL entry)
 */
const ZenStdlibFunction* zen_stdlib_get_all() {
    return stdlib_functions;
}

// Wrapper functions for stdlib integration

/**
 * @brief Print values to stdout with newline
 * @param args Array of Value pointers to print
 * @param argc Number of arguments
 * @return NULL value
 */
Value* zen_stdlib_print(Value** args, size_t argc) {
    if (argc >= 1) {
        zen_print(args[0]);
    } else {
        printf("\n");
    }
    return value_new_null();
}

/**
 * @brief Read input from stdin with optional prompt
 * @param args Optional prompt string as first argument
 * @param argc Number of arguments (0 or 1)
 * @return String value containing user input
 */
Value* zen_stdlib_input(Value** args, size_t argc) {
    char* input;
    if (argc >= 1 && args[0]->type == VALUE_STRING) {
        input = zen_input_prompt(args[0]->as.string->data);
    } else {
        input = zen_input();
    }
    
    if (input) {
        Value* result = value_new_string(input);
        free(input);
        return result;
    }
    
    return value_new_string("");
}

/**
 * @brief Read entire file contents as string
 * @param args Filename string as first argument
 * @param argc Number of arguments (must be 1)
 * @return String value with file contents or error
 */
Value* zen_stdlib_read_file(Value** args, size_t argc) {
    if (argc < 1 || args[0]->type != VALUE_STRING) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("readFile requires a string filename");
            error->as.error->code = -1;
        }
        return error;
    }
    
    char* contents = get_file_contents(args[0]->as.string->data);
    if (contents) {
        Value* result = value_new_string(contents);
        free(contents);
        return result;
    }
    
    Value* error = value_new(VALUE_ERROR);
    if (error && error->as.error) {
        error->as.error->message = strdup("Failed to read file");
        error->as.error->code = -1;
    }
    return error;
}

/**
 * @brief Write string content to file
 * @param args Filename and content strings as arguments
 * @param argc Number of arguments (must be 2)
 * @return Boolean indicating success
 */
Value* zen_stdlib_write_file(Value** args, size_t argc) {
    if (argc < 2 || args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = zen_write_file(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief Append string content to file
 * @param args Filename and content strings as arguments
 * @param argc Number of arguments (must be 2)
 * @return Boolean indicating success
 */
Value* zen_stdlib_append_file(Value** args, size_t argc) {
    if (argc < 2 || args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = zen_append_file(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief Check if file exists
 * @param args Filename string as first argument
 * @param argc Number of arguments (must be 1)
 * @return Boolean indicating if file exists
 */
Value* zen_stdlib_file_exists(Value** args, size_t argc) {
    if (argc < 1 || args[0]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool exists = zen_file_exists(args[0]->as.string->data);
    return value_new_boolean(exists);
}

/**
 * @brief Get length of string, array, or object
 * @param args Value to get length of
 * @param argc Number of arguments (must be 1)
 * @return Number value representing length
 */
Value* zen_stdlib_length(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    
    if (args[0]->type == VALUE_STRING) {
        return zen_string_length(args[0]);
    } else if (args[0]->type == VALUE_ARRAY) {
        if (args[0]->as.array) {
            return value_new_number((double)args[0]->as.array->length);
        }
    }
    
    return value_new_number(0);
}

// String function wrappers

/**
 * @brief Convert string to uppercase
 * @param args String value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Uppercase string value
 */
Value* zen_stdlib_upper(Value** args, size_t argc) {
    return argc >= 1 ? zen_string_upper(args[0]) : value_new_string("");
}

/**
 * @brief Convert string to lowercase
 * @param args String value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Lowercase string value
 */
Value* zen_stdlib_lower(Value** args, size_t argc) {
    return argc >= 1 ? zen_string_lower(args[0]) : value_new_string("");
}

/**
 * @brief Trim whitespace from string ends
 * @param args String value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Trimmed string value
 */
Value* zen_stdlib_trim(Value** args, size_t argc) {
    return argc >= 1 ? zen_string_trim(args[0]) : value_new_string("");
}

/**
 * @brief Split string by delimiter
 * @param args String and delimiter as arguments
 * @param argc Number of arguments (1 or 2)
 * @return Array of split string parts
 */
Value* zen_stdlib_split(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_string_split(args[0], args[1]->as.string->data);
    }
    return zen_string_split(args[0], " "); // Default to space
}

/**
 * @brief Check if string contains substring
 * @param args String and substring as arguments
 * @param argc Number of arguments (must be 2)
 * @return Boolean indicating if substring found
 */
Value* zen_stdlib_contains(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_string_contains(args[0], args[1]->as.string->data);
    }
    return value_new_boolean(false);
}

/**
 * @brief Replace substring in string
 * @param args String, search, and replacement as arguments
 * @param argc Number of arguments (must be 3)
 * @return String with replacements made
 */
Value* zen_stdlib_replace(Value** args, size_t argc) {
    if (argc >= 3 && args[1]->type == VALUE_STRING && args[2]->type == VALUE_STRING) {
        return zen_string_replace(args[0], args[1]->as.string->data, args[2]->as.string->data);
    }
    return argc >= 1 ? value_copy(args[0]) : value_new_string("");
}

// Math function wrappers

/**
 * @brief Get absolute value of number
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Absolute value as number
 */
Value* zen_stdlib_abs(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_abs(args[0]) : value_new_number(0);
}

/**
 * @brief Get floor of number (round down)
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Floor value as number
 */
Value* zen_stdlib_floor(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_floor(args[0]) : value_new_number(0);
}

/**
 * @brief Get ceiling of number (round up)
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Ceiling value as number
 */
Value* zen_stdlib_ceil(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_ceil(args[0]) : value_new_number(0);
}

/**
 * @brief Round number to nearest integer
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Rounded value as number
 */
Value* zen_stdlib_round(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_round(args[0]) : value_new_number(0);
}

/**
 * @brief Get square root of number
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Square root as number
 */
Value* zen_stdlib_sqrt(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_sqrt(args[0]) : value_new_number(0);
}

/**
 * @brief Raise number to power
 * @param args Base and exponent as arguments
 * @param argc Number of arguments (must be 2)
 * @return Result of base^exponent
 */
Value* zen_stdlib_pow(Value** args, size_t argc) {
    return argc >= 2 ? zen_math_pow(args[0], args[1]) : value_new_number(0);
}

/**
 * @brief Get sine of angle in radians
 * @param args Angle in radians as first argument
 * @param argc Number of arguments (must be 1)
 * @return Sine value as number
 */
Value* zen_stdlib_sin(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_sin(args[0]) : value_new_number(0);
}

/**
 * @brief Get cosine of angle in radians
 * @param args Angle in radians as first argument
 * @param argc Number of arguments (must be 1)
 * @return Cosine value as number
 */
Value* zen_stdlib_cos(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_cos(args[0]) : value_new_number(1);
}

/**
 * @brief Get tangent of angle in radians
 * @param args Angle in radians as first argument
 * @param argc Number of arguments (must be 1)
 * @return Tangent value as number
 */
Value* zen_stdlib_tan(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_tan(args[0]) : value_new_number(0);
}

/**
 * @brief Get natural logarithm of number
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Natural logarithm as number
 */
Value* zen_stdlib_log(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_log(args[0]) : value_new_number(0);
}

/**
 * @brief Generate random number between 0 and 1
 * @param args Unused
 * @param argc Number of arguments (ignored)
 * @return Random number between 0 and 1
 */
Value* zen_stdlib_random(Value** args, size_t argc) {
    (void)args; (void)argc; // Unused parameters
    return zen_math_random();
}

/**
 * @brief Generate random integer in range
 * @param args Min and max values as arguments
 * @param argc Number of arguments (must be 2)
 * @return Random integer in range [min, max]
 */
Value* zen_stdlib_random_int(Value** args, size_t argc) {
    return argc >= 2 ? zen_math_random_int(args[0], args[1]) : value_new_number(0);
}

/**
 * @brief Get minimum of two numbers
 * @param args Two number values as arguments
 * @param argc Number of arguments (must be 2)
 * @return Minimum value as number
 */
Value* zen_stdlib_min(Value** args, size_t argc) {
    return argc >= 2 ? zen_math_min(args[0], args[1]) : (argc >= 1 ? value_copy(args[0]) : value_new_number(0));
}

/**
 * @brief Get maximum of two numbers
 * @param args Two number values as arguments
 * @param argc Number of arguments (must be 2)
 * @return Maximum value as number
 */
Value* zen_stdlib_max(Value** args, size_t argc) {
    return argc >= 2 ? zen_math_max(args[0], args[1]) : (argc >= 1 ? value_copy(args[0]) : value_new_number(0));
}

/**
 * @brief Check if number is NaN (Not a Number)
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Boolean indicating if value is NaN
 */
Value* zen_stdlib_is_nan(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_is_nan(args[0]) : value_new_boolean(false);
}

/**
 * @brief Check if number is infinite
 * @param args Number value as first argument
 * @param argc Number of arguments (must be 1)
 * @return Boolean indicating if value is infinite
 */
Value* zen_stdlib_is_infinite(Value** args, size_t argc) {
    return argc >= 1 ? zen_math_is_infinite(args[0]) : value_new_boolean(false);
}

// Type conversion function wrappers

/**
 * @brief Convert value to string representation
 * @param args Value to convert as first argument
 * @param argc Number of arguments (must be 1)
 * @return String representation of value
 */
Value* zen_stdlib_to_string(Value** args, size_t argc) {
    return argc >= 1 ? zen_to_string(args[0]) : value_new_string("");
}

/**
 * @brief Convert value to number
 * @param args Value to convert as first argument
 * @param argc Number of arguments (must be 1)
 * @return Number representation of value
 */
Value* zen_stdlib_to_number(Value** args, size_t argc) {
    return argc >= 1 ? zen_to_number(args[0]) : value_new_number(0);
}

/**
 * @brief Convert value to boolean
 * @param args Value to convert as first argument
 * @param argc Number of arguments (must be 1)
 * @return Boolean representation of value
 */
Value* zen_stdlib_to_boolean(Value** args, size_t argc) {
    return argc >= 1 ? zen_to_boolean(args[0]) : value_new_boolean(false);
}

/**
 * @brief Get type name of value
 * @param args Value to get type of as first argument
 * @param argc Number of arguments (must be 1)
 * @return String containing type name
 */
Value* zen_stdlib_type_of(Value** args, size_t argc) {
    return argc >= 1 ? zen_type_of(args[0]) : value_new_string("undefined");
}

/**
 * @brief Check if value is of specific type
 * @param args Value and type name as arguments
 * @param argc Number of arguments (must be 2)
 * @return Boolean indicating if value matches type
 */
Value* zen_stdlib_is_type(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_is_type(args[0], args[1]->as.string->data);
    }
    return value_new_boolean(false);
}

/**
 * @brief Parse integer from string
 * @param args String and optional base as arguments
 * @param argc Number of arguments (1 or 2)
 * @return Integer value or NaN if invalid
 */
Value* zen_stdlib_parse_int(Value** args, size_t argc) {
    return argc >= 1 ? zen_parse_int(args[0], argc >= 2 ? args[1] : NULL) : value_new_number(0);
}

/**
 * @brief Parse floating point number from string
 * @param args String to parse as first argument
 * @param argc Number of arguments (must be 1)
 * @return Float value or NaN if invalid
 */
Value* zen_stdlib_parse_float(Value** args, size_t argc) {
    return argc >= 1 ? zen_parse_float(args[0]) : value_new_number(0);
}

// JSON function wrappers

/**
 * @brief Parse JSON string to value
 * @param args JSON string as first argument
 * @param argc Number of arguments (must be 1)
 * @return Parsed value or null if invalid
 */
Value* zen_stdlib_json_parse(Value** args, size_t argc) {
    if (argc >= 1 && args[0]->type == VALUE_STRING) {
        return json_parse(args[0]->as.string->data);
    }
    return value_new_null();
}

/**
 * @brief Convert value to JSON string
 * @param args Value to convert as first argument
 * @param argc Number of arguments (must be 1)
 * @return JSON string representation
 */
Value* zen_stdlib_json_stringify(Value** args, size_t argc) {
    if (argc >= 1) {
        char* json_str = json_stringify(args[0]);
        if (json_str) {
            Value* result = value_new_string(json_str);
            free(json_str);
            return result;
        }
    }
    return value_new_string("null");
}