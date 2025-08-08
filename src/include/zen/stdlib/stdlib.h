#ifndef ZEN_STDLIB_H
#define ZEN_STDLIB_H

#include "zen/types/value.h"
#include <stddef.h>

/**
 * @brief Function pointer type for stdlib functions
 */
typedef Value* (*ZenStdlibFunc)(Value** args, size_t argc);

/**
 * @brief Structure representing a stdlib function
 */
typedef struct {
    const char* name;        // Function name
    ZenStdlibFunc func;      // Function pointer
    const char* description; // Function description
} ZenStdlibFunction;

/**
 * @brief Get count of stdlib functions
 * @return Number of stdlib functions available
 */
size_t stdlib_count(void);

/**
 * @brief Get stdlib function by name
 * @param name Function name to look up
 * @return Pointer to function info, or NULL if not found
 */
const ZenStdlibFunction* stdlib_get(const char* name);

/**
 * @brief Get all stdlib functions
 * @return Array of all stdlib functions (terminated by NULL entry)
 */
const ZenStdlibFunction* stdlib_get_all(void);

// Stdlib function declarations (wrappers for integration)

// I/O Functions
/**
 * @brief Print values to stdout with newline
 * @param args Arguments array (values to print)
 * @param argc Number of arguments
 * @return Null value
 */
Value* io_print(Value** args, size_t argc);

/**
 * @brief Read line from stdin with optional prompt
 * @param args Arguments array (optional prompt string)
 * @param argc Number of arguments
 * @return String value containing input line
 */
Value* io_input(Value** args, size_t argc);

/**
 * @brief Read entire file contents as string
 * @param args Arguments array (filename string)
 * @param argc Number of arguments
 * @return String value with file contents or error
 */
Value* io_read_file(Value** args, size_t argc);

/**
 * @brief Write string content to file
 * @param args Arguments array (filename, content)
 * @param argc Number of arguments
 * @return Boolean value indicating success
 */
Value* io_write_file(Value** args, size_t argc);

/**
 * @brief Append string content to file
 * @param args Arguments array (filename, content)
 * @param argc Number of arguments
 * @return Boolean value indicating success
 */
Value* io_append_file(Value** args, size_t argc);

/**
 * @brief Check if file exists
 * @param args Arguments array (filename string)
 * @param argc Number of arguments
 * @return Boolean value indicating if file exists
 */
Value* io_file_exists(Value** args, size_t argc);

// String Functions
/**
 * @brief Get length of string, array, or object
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Number value containing length
 */
Value* string_length(Value** args, size_t argc);

/**
 * @brief Convert string to uppercase
 * @param args Arguments array (string to convert)
 * @param argc Number of arguments
 * @return String value with uppercase content
 */
Value* string_upper(Value** args, size_t argc);

/**
 * @brief Convert string to lowercase
 * @param args Arguments array (string to convert)
 * @param argc Number of arguments
 * @return String value with lowercase content
 */
Value* string_lower(Value** args, size_t argc);

/**
 * @brief Trim whitespace from both ends of string
 * @param args Arguments array (string to trim)
 * @param argc Number of arguments
 * @return String value with whitespace trimmed
 */
Value* string_trim(Value** args, size_t argc);

/**
 * @brief Split string by delimiter
 * @param args Arguments array (string, delimiter)
 * @param argc Number of arguments
 * @return Array value containing split parts
 */
Value* string_split(Value** args, size_t argc);

/**
 * @brief Check if string contains substring
 * @param args Arguments array (string, substring)
 * @param argc Number of arguments
 * @return Boolean value indicating if substring found
 */
Value* string_contains(Value** args, size_t argc);

/**
 * @brief Replace substring in string
 * @param args Arguments array (string, search, replacement)
 * @param argc Number of arguments
 * @return String value with replacements made
 */
Value* string_replace(Value** args, size_t argc);

// Math Functions
/**
 * @brief Calculate absolute value
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with absolute value
 */
Value* math_abs(Value** args, size_t argc);

/**
 * @brief Calculate floor (round down to nearest integer)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded down
 */
Value* math_floor(Value** args, size_t argc);

/**
 * @brief Calculate ceiling (round up to nearest integer)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded up
 */
Value* math_ceil(Value** args, size_t argc);

/**
 * @brief Round to nearest integer
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded to nearest integer
 */
Value* math_round(Value** args, size_t argc);

/**
 * @brief Calculate square root
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with square root
 */
Value* math_sqrt(Value** args, size_t argc);

/**
 * @brief Calculate power (base^exponent)
 * @param args Arguments array (base, exponent)
 * @param argc Number of arguments
 * @return Number value with result of power operation
 */
Value* math_pow(Value** args, size_t argc);

/**
 * @brief Calculate sine of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with sine result
 */
Value* math_sin(Value** args, size_t argc);

/**
 * @brief Calculate cosine of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with cosine result
 */
Value* math_cos(Value** args, size_t argc);

/**
 * @brief Calculate tangent of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with tangent result
 */
Value* math_tan(Value** args, size_t argc);

/**
 * @brief Calculate natural logarithm
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with natural log result
 */
Value* math_log(Value** args, size_t argc);

/**
 * @brief Generate random number between 0 and 1
 * @param args Arguments array (unused)
 * @param argc Number of arguments
 * @return Number value with random number 0-1
 */
Value* math_random(Value** args, size_t argc);

/**
 * @brief Generate random integer in range
 * @param args Arguments array (min, max)
 * @param argc Number of arguments
 * @return Number value with random integer in range
 */
Value* math_random_int(Value** args, size_t argc);

/**
 * @brief Get minimum of two numbers
 * @param args Arguments array (number1, number2)
 * @param argc Number of arguments
 * @return Number value with minimum value
 */
Value* math_min(Value** args, size_t argc);

/**
 * @brief Get maximum of two numbers
 * @param args Arguments array (number1, number2)
 * @param argc Number of arguments
 * @return Number value with maximum value
 */
Value* math_max(Value** args, size_t argc);

/**
 * @brief Check if number is NaN (Not a Number)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Boolean value indicating if NaN
 */
Value* math_is_nan(Value** args, size_t argc);

/**
 * @brief Check if number is infinite
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Boolean value indicating if infinite
 */
Value* math_is_infinite(Value** args, size_t argc);

// Type Conversion Functions
/**
 * @brief Convert value to string representation
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return String value with string representation
 */
Value* convert_to_string(Value** args, size_t argc);

/**
 * @brief Convert value to number
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return Number value with numeric representation
 */
Value* convert_to_number(Value** args, size_t argc);

/**
 * @brief Convert value to boolean
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return Boolean value with boolean representation
 */
Value* convert_to_boolean(Value** args, size_t argc);

/**
 * @brief Get type name of value
 * @param args Arguments array (value to check)
 * @param argc Number of arguments
 * @return String value with type name
 */
Value* convert_type_of(Value** args, size_t argc);

/**
 * @brief Check if value is of specific type
 * @param args Arguments array (value, type_name)
 * @param argc Number of arguments
 * @return Boolean value indicating type match
 */
Value* convert_is_type(Value** args, size_t argc);

/**
 * @brief Parse integer from string with optional radix
 * @param args Arguments array (string, optional radix)
 * @param argc Number of arguments
 * @return Number value with parsed integer
 */
Value* convert_parse_int(Value** args, size_t argc);

/**
 * @brief Parse floating point number from string
 * @param args Arguments array (string)
 * @param argc Number of arguments
 * @return Number value with parsed float
 */
Value* convert_parse_float(Value** args, size_t argc);

// JSON Functions
/**
 * @brief Parse JSON string to value
 * @param args Arguments array (JSON string)
 * @param argc Number of arguments
 * @return Parsed value or null on error
 */
Value* json_parse_stdlib(Value** args, size_t argc);

/**
 * @brief Convert value to JSON string
 * @param args Arguments array (value to stringify)
 * @param argc Number of arguments
 * @return String value with JSON representation
 */
Value* json_stringify_stdlib(Value** args, size_t argc);

// Additional Stdlib Functions
/**
 * @brief Convert value to pretty JSON string
 * @param args Arguments array (value, optional indent)
 * @param argc Number of arguments
 * @return String value with formatted JSON representation
 */
Value* json_stringify_pretty_stdlib(Value** args, size_t argc);

/**
 * @brief Parse JSON file with error handling
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
Value* json_parse_file_stdlib(Value** args, size_t argc);

/**
 * @brief Load and parse JSON file
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
Value* json_load_file(Value** args, size_t argc);

/**
 * @brief Load and parse YAML file
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
Value* yaml_load_file_wrapper(Value** args, size_t argc);

// Array Functions
/**
 * @brief Add element to end of array
 * @param args Arguments array (array, element)
 * @param argc Number of arguments
 * @return Modified array or error
 */
Value* array_push_stdlib(Value** args, size_t argc);

/**
 * @brief Remove and return last element
 * @param args Arguments array (array)
 * @param argc Number of arguments
 * @return Removed element or error
 */
Value* array_pop_stdlib(Value** args, size_t argc);

// System Functions
/**
 * @brief Execute system command
 * @param args Arguments array (command string)
 * @param argc Number of arguments
 * @return Command output or error
 */
Value* system_exec(Value** args, size_t argc);

// DateTime Functions
/**
 * @brief Get current date and time
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Current date/time value
 */
Value* datetime_now(Value** args, size_t argc);

// Logging Functions
/**
 * @brief Log debug message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_debug(Value** args, size_t argc);

/**
 * @brief Log info message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_info(Value** args, size_t argc);

/**
 * @brief Log warning message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_warn(Value** args, size_t argc);

/**
 * @brief Log error message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_error(Value** args, size_t argc);

/**
 * @brief Log formatted debug message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_debugf(Value** args, size_t argc);

/**
 * @brief Log formatted info message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_infof(Value** args, size_t argc);

/**
 * @brief Log formatted warning message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_warnf(Value** args, size_t argc);

/**
 * @brief Log formatted error message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_errorf(Value** args, size_t argc);

/**
 * @brief Log debug message conditionally
 * @param args Arguments array (condition, message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_debug_if(Value** args, size_t argc);

/**
 * @brief Set minimum log level
 * @param args Arguments array (level)
 * @param argc Number of arguments
 * @return Previous level or error
 */
Value* logging_set_level(Value** args, size_t argc);

/**
 * @brief Log with context information
 * @param args Arguments array (context, message)
 * @param argc Number of arguments
 * @return Null value
 */
Value* logging_with_context(Value** args, size_t argc);

/**
 * @brief Get current log level
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Current log level
 */
Value* logging_get_level(Value** args, size_t argc);

// HTTP Functions - declared in http.h with wrapper functions
// The actual stdlib wrapper functions are:
// stdlib_http_get, stdlib_http_post, stdlib_http_put, 
// stdlib_http_delete, stdlib_http_timeout

// Regex Functions - declared in regex.h with wrapper functions
// The actual stdlib wrapper functions are:
// regex_match_stdlib, regex_replace_stdlib, regex_split_stdlib, regex_compile_stdlib

#endif
