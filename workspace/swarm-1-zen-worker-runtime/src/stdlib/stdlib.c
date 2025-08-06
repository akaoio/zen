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

// Forward declarations for array functions  
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);

// Forward declarations for stdlib functions
Value* zen_stdlib_get_all_wrapper(Value** args, size_t argc);

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
    
    // Introspection Functions  
    {"getAll", zen_stdlib_get_all_wrapper, "Get all stdlib function names"},
    
    // Sentinel - must be last
    {NULL, NULL, NULL}
};

/**
 * @brief Get count of stdlib functions
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
 * @return Array of all stdlib functions (terminated by NULL entry)
 */
const ZenStdlibFunction* zen_stdlib_get_all() {
    // Return the complete array of stdlib functions
    // The array is NULL-terminated for safe iteration
    // This enables runtime discovery of all available stdlib functions
    return stdlib_functions;
}

// Wrapper functions for stdlib integration

/**
 * @brief Print wrapper function
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
 * @brief Input wrapper function
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
 * @brief Read file wrapper function
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
 * @brief Write file wrapper function
 */
Value* zen_stdlib_write_file(Value** args, size_t argc) {
    if (argc < 2 || args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = zen_write_file(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief Append file wrapper function
 */
Value* zen_stdlib_append_file(Value** args, size_t argc) {
    if (argc < 2 || args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = zen_append_file(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief File exists wrapper function
 */
Value* zen_stdlib_file_exists(Value** args, size_t argc) {
    if (argc < 1 || args[0]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool exists = zen_file_exists(args[0]->as.string->data);
    return value_new_boolean(exists);
}

/**
 * @brief Length wrapper function
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
Value* zen_stdlib_upper(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_string("");
    }
    // Validate argument and delegate to underlying implementation
    return zen_string_upper(args[0]);
}

Value* zen_stdlib_lower(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_string("");
    }
    // Validate argument and delegate to underlying implementation
    return zen_string_lower(args[0]);
}

Value* zen_stdlib_trim(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_string("");
    }
    // Validate argument and delegate to underlying implementation
    return zen_string_trim(args[0]);
}

Value* zen_stdlib_split(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_string_split(args[0], args[1]->as.string->data);
    }
    return zen_string_split(args[0], " "); // Default to space
}

Value* zen_stdlib_contains(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_string_contains(args[0], args[1]->as.string->data);
    }
    return value_new_boolean(false);
}

Value* zen_stdlib_replace(Value** args, size_t argc) {
    if (argc >= 3 && args[1]->type == VALUE_STRING && args[2]->type == VALUE_STRING) {
        return zen_string_replace(args[0], args[1]->as.string->data, args[2]->as.string->data);
    }
    return argc >= 1 ? value_copy(args[0]) : value_new_string("");
}

// Math function wrappers
Value* zen_stdlib_abs(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_abs(args[0]);
}

Value* zen_stdlib_floor(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_floor(args[0]);
}

Value* zen_stdlib_ceil(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_ceil(args[0]);
}

Value* zen_stdlib_round(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_round(args[0]);
}

Value* zen_stdlib_sqrt(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_sqrt(args[0]);
}

Value* zen_stdlib_pow(Value** args, size_t argc) {
    if (argc < 2) {
        return value_new_number(0);
    }
    // Validate arguments and delegate to underlying implementation
    return zen_math_pow(args[0], args[1]);
}

Value* zen_stdlib_sin(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_sin(args[0]);
}

Value* zen_stdlib_cos(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(1);  // cos(0) = 1
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_cos(args[0]);
}

Value* zen_stdlib_tan(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_tan(args[0]);
}

Value* zen_stdlib_log(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_log(args[0]);
}

Value* zen_stdlib_random(Value** args, size_t argc) {
    (void)args; (void)argc; // Unused parameters
    return zen_math_random();
}

Value* zen_stdlib_random_int(Value** args, size_t argc) {
    if (argc < 2) {
        return value_new_number(0);
    }
    // Validate arguments and delegate to underlying implementation
    return zen_math_random_int(args[0], args[1]);
}

Value* zen_stdlib_min(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    if (argc < 2) {
        return value_copy(args[0]);
    }
    // Validate arguments and delegate to underlying implementation
    return zen_math_min(args[0], args[1]);
}

Value* zen_stdlib_max(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    if (argc < 2) {
        return value_copy(args[0]);
    }
    // Validate arguments and delegate to underlying implementation
    return zen_math_max(args[0], args[1]);
}

Value* zen_stdlib_is_nan(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_boolean(false);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_is_nan(args[0]);
}

Value* zen_stdlib_is_infinite(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_boolean(false);
    }
    // Validate argument and delegate to underlying implementation
    return zen_math_is_infinite(args[0]);
}

// Type conversion function wrappers
Value* zen_stdlib_to_string(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_string("");
    }
    // Validate argument and delegate to underlying implementation
    return zen_to_string(args[0]);
}

Value* zen_stdlib_to_number(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_to_number(args[0]);
}

Value* zen_stdlib_to_boolean(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_boolean(false);
    }
    // Validate argument and delegate to underlying implementation
    return zen_to_boolean(args[0]);
}

Value* zen_stdlib_type_of(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_string("undefined");
    }
    // Validate argument and delegate to underlying implementation
    return zen_type_of(args[0]);
}

Value* zen_stdlib_is_type(Value** args, size_t argc) {
    if (argc >= 2 && args[1]->type == VALUE_STRING) {
        return zen_is_type(args[0], args[1]->as.string->data);
    }
    return value_new_boolean(false);
}

Value* zen_stdlib_parse_int(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Support optional radix argument and delegate to underlying implementation
    Value* radix = (argc >= 2) ? args[1] : NULL;
    return zen_parse_int(args[0], radix);
}

Value* zen_stdlib_parse_float(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_number(0);
    }
    // Validate argument and delegate to underlying implementation
    return zen_parse_float(args[0]);
}

// JSON function wrappers
Value* zen_stdlib_json_parse(Value** args, size_t argc) {
    if (argc >= 1 && args[0]->type == VALUE_STRING) {
        return json_parse(args[0]->as.string->data);
    }
    return value_new_null();
}

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

/**
 * @brief Wrapper function to get all stdlib function names as Value array
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Array Value containing all stdlib function names as strings
 */
Value* zen_stdlib_get_all_wrapper(Value** args, size_t argc) {
    (void)args; // Unused parameters
    (void)argc;
    
    // Count the number of functions
    size_t count = zen_stdlib_count();
    
    // Create array to hold function names
    Value* result = array_new(count);
    if (!result) {
        return value_new_null();
    }
    
    // Add each function name from stdlib_functions registry
    for (size_t i = 0; i < count && stdlib_functions[i].name != NULL; i++) {
        Value* name = value_new_string(stdlib_functions[i].name);
        if (name) {
            array_push(result, name);
        }
    }
    
    return result;
}