#ifndef ZEN_STDLIB_H
#define ZEN_STDLIB_H

#include "zen/core/runtime_value.h"

#include <stddef.h>

/**
 * @brief Function pointer type for stdlib functions
 */
typedef RuntimeValue *(*ZenStdlibFunc)(RuntimeValue **args, size_t argc);

/**
 * @brief Structure representing a stdlib function
 */
typedef struct {
    const char *name;         // Function name
    ZenStdlibFunc func;       // Function pointer
    const char *description;  // Function description
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
const ZenStdlibFunction *stdlib_get(const char *name);

/**
 * @brief Get all stdlib functions
 * @return Array of all stdlib functions (terminated by NULL entry)
 */
const ZenStdlibFunction *stdlib_get_all(void);

// Stdlib function declarations (wrappers for integration)

// I/O Functions
/**
 * @brief Print values to stdout with newline
 * @param args Arguments array (values to print)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *io_print(RuntimeValue **args, size_t argc);

/**
 * @brief Read line from stdin with optional prompt
 * @param args Arguments array (optional prompt string)
 * @param argc Number of arguments
 * @return String value containing input line
 */
RuntimeValue *io_input(RuntimeValue **args, size_t argc);

/**
 * @brief Read entire file contents as string
 * @param args Arguments array (filename string)
 * @param argc Number of arguments
 * @return String value with file contents or error
 */
RuntimeValue *io_read_file(RuntimeValue **args, size_t argc);

/**
 * @brief Write string content to file
 * @param args Arguments array (filename, content)
 * @param argc Number of arguments
 * @return Boolean value indicating success
 */
RuntimeValue *io_write_file(RuntimeValue **args, size_t argc);

/**
 * @brief Append string content to file
 * @param args Arguments array (filename, content)
 * @param argc Number of arguments
 * @return Boolean value indicating success
 */
RuntimeValue *io_append_file(RuntimeValue **args, size_t argc);

/**
 * @brief Check if file exists
 * @param args Arguments array (filename string)
 * @param argc Number of arguments
 * @return Boolean value indicating if file exists
 */
RuntimeValue *io_file_exists(RuntimeValue **args, size_t argc);

// String Functions
/**
 * @brief Get length of string, array, or object
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Number value containing length
 */
RuntimeValue *string_length(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to uppercase
 * @param args Arguments array (string to convert)
 * @param argc Number of arguments
 * @return String value with uppercase content
 */
RuntimeValue *string_upper(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to lowercase
 * @param args Arguments array (string to convert)
 * @param argc Number of arguments
 * @return String value with lowercase content
 */
RuntimeValue *string_lower(RuntimeValue **args, size_t argc);

/**
 * @brief Trim whitespace from both ends of string
 * @param args Arguments array (string to trim)
 * @param argc Number of arguments
 * @return String value with whitespace trimmed
 */
RuntimeValue *string_trim(RuntimeValue **args, size_t argc);

/**
 * @brief Split string by delimiter
 * @param args Arguments array (string, delimiter)
 * @param argc Number of arguments
 * @return Array value containing split parts
 */
RuntimeValue *string_split(RuntimeValue **args, size_t argc);

/**
 * @brief Check if string contains substring
 * @param args Arguments array (string, substring)
 * @param argc Number of arguments
 * @return Boolean value indicating if substring found
 */
RuntimeValue *string_contains(RuntimeValue **args, size_t argc);

/**
 * @brief Replace substring in string
 * @param args Arguments array (string, search, replacement)
 * @param argc Number of arguments
 * @return String value with replacements made
 */
RuntimeValue *string_replace(RuntimeValue **args, size_t argc);

// Math Functions
/**
 * @brief Calculate absolute value
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with absolute value
 */
RuntimeValue *math_abs(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate floor (round down to nearest integer)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded down
 */
RuntimeValue *math_floor(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate ceiling (round up to nearest integer)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded up
 */
RuntimeValue *math_ceil(RuntimeValue **args, size_t argc);

/**
 * @brief Round to nearest integer
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value rounded to nearest integer
 */
RuntimeValue *math_round(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate square root
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with square root
 */
RuntimeValue *math_sqrt(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate power (base^exponent)
 * @param args Arguments array (base, exponent)
 * @param argc Number of arguments
 * @return Number value with result of power operation
 */
RuntimeValue *math_pow(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate sine of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with sine result
 */
RuntimeValue *math_sin(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate cosine of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with cosine result
 */
RuntimeValue *math_cos(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate tangent of angle in radians
 * @param args Arguments array (angle in radians)
 * @param argc Number of arguments
 * @return Number value with tangent result
 */
RuntimeValue *math_tan(RuntimeValue **args, size_t argc);

/**
 * @brief Calculate natural logarithm
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Number value with natural log result
 */
RuntimeValue *math_log(RuntimeValue **args, size_t argc);

/**
 * @brief Generate random number between 0 and 1
 * @param args Arguments array (unused)
 * @param argc Number of arguments
 * @return Number value with random number 0-1
 */
RuntimeValue *math_random(RuntimeValue **args, size_t argc);

/**
 * @brief Generate random integer in range
 * @param args Arguments array (min, max)
 * @param argc Number of arguments
 * @return Number value with random integer in range
 */
RuntimeValue *math_random_int(RuntimeValue **args, size_t argc);

/**
 * @brief Get minimum of two numbers
 * @param args Arguments array (number1, number2)
 * @param argc Number of arguments
 * @return Number value with minimum value
 */
RuntimeValue *math_min(RuntimeValue **args, size_t argc);

/**
 * @brief Get maximum of two numbers
 * @param args Arguments array (number1, number2)
 * @param argc Number of arguments
 * @return Number value with maximum value
 */
RuntimeValue *math_max(RuntimeValue **args, size_t argc);

/**
 * @brief Check if number is NaN (Not a Number)
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Boolean value indicating if NaN
 */
RuntimeValue *math_is_nan(RuntimeValue **args, size_t argc);

/**
 * @brief Check if number is infinite
 * @param args Arguments array (number)
 * @param argc Number of arguments
 * @return Boolean value indicating if infinite
 */
RuntimeValue *math_is_infinite(RuntimeValue **args, size_t argc);

// Type Conversion Functions
/**
 * @brief Convert value to string representation
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return String value with string representation
 */
RuntimeValue *convert_to_string(RuntimeValue **args, size_t argc);

/**
 * @brief Convert value to number
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return Number value with numeric representation
 */
RuntimeValue *convert_to_number(RuntimeValue **args, size_t argc);

/**
 * @brief Convert value to boolean
 * @param args Arguments array (value to convert)
 * @param argc Number of arguments
 * @return Boolean value with boolean representation
 */
RuntimeValue *convert_to_boolean(RuntimeValue **args, size_t argc);

/**
 * @brief Get type name of value
 * @param args Arguments array (value to check)
 * @param argc Number of arguments
 * @return String value with type name
 */
RuntimeValue *convert_type_of(RuntimeValue **args, size_t argc);

/**
 * @brief Check if value is of specific type
 * @param args Arguments array (value, type_name)
 * @param argc Number of arguments
 * @return Boolean value indicating type match
 */
RuntimeValue *convert_is_type(RuntimeValue **args, size_t argc);

/**
 * @brief Parse integer from string with optional radix
 * @param args Arguments array (string, optional radix)
 * @param argc Number of arguments
 * @return Number value with parsed integer
 */
RuntimeValue *convert_parse_int(RuntimeValue **args, size_t argc);

/**
 * @brief Parse floating point number from string
 * @param args Arguments array (string)
 * @param argc Number of arguments
 * @return Number value with parsed float
 */
RuntimeValue *convert_parse_float(RuntimeValue **args, size_t argc);

// JSON Functions
/**
 * @brief Parse JSON string to value
 * @param args Arguments array (JSON string)
 * @param argc Number of arguments
 * @return Parsed value or null on error
 */
RuntimeValue *json_parse_stdlib(RuntimeValue **args, size_t argc);

/**
 * @brief Convert value to JSON string
 * @param args Arguments array (value to stringify)
 * @param argc Number of arguments
 * @return String value with JSON representation
 */
RuntimeValue *json_stringify_stdlib(RuntimeValue **args, size_t argc);

// Additional Stdlib Functions
/**
 * @brief Convert value to pretty JSON string
 * @param args Arguments array (value, optional indent)
 * @param argc Number of arguments
 * @return String value with formatted JSON representation
 */
RuntimeValue *json_stringify_pretty_stdlib(RuntimeValue **args, size_t argc);

/**
 * @brief Parse JSON file with error handling
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
RuntimeValue *json_parse_file_stdlib(RuntimeValue **args, size_t argc);

/**
 * @brief Load and parse JSON file
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
RuntimeValue *json_load_file(RuntimeValue **args, size_t argc);

/**
 * @brief Load and parse YAML file
 * @param args Arguments array (filename)
 * @param argc Number of arguments
 * @return Parsed Value or error Value
 */
RuntimeValue *yaml_load_file_wrapper(RuntimeValue **args, size_t argc);

// Array Functions
/**
 * @brief Add element to end of array
 * @param args Arguments array (array, element)
 * @param argc Number of arguments
 * @return Modified array or error
 */
RuntimeValue *array_push_stdlib(RuntimeValue **args, size_t argc);

/**
 * @brief Remove and return last element
 * @param args Arguments array (array)
 * @param argc Number of arguments
 * @return Removed element or error
 */
RuntimeValue *array_pop_stdlib(RuntimeValue **args, size_t argc);

// System Functions
/**
 * @brief Execute system command
 * @param args Arguments array (command string)
 * @param argc Number of arguments
 * @return Command output or error
 */
RuntimeValue *system_exec(RuntimeValue **args, size_t argc);

/**
 * @brief Get environment variable
 * @param args Arguments array (variable name)
 * @param argc Number of arguments
 * @return Environment variable value or null
 */
RuntimeValue *system_env_get(RuntimeValue **args, size_t argc);

/**
 * @brief Set environment variable
 * @param args Arguments array (variable name, value)
 * @param argc Number of arguments
 * @return Boolean indicating success
 */
RuntimeValue *system_env_set(RuntimeValue **args, size_t argc);

/**
 * @brief List all environment variables
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Array of environment variable names
 */
RuntimeValue *system_env_list(RuntimeValue **args, size_t argc);
/**
 * @brief List running processes
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Array of process objects with pid, ppid, user, command
 */
RuntimeValue *system_process_list(RuntimeValue **args, size_t argc);
/**
 * @brief Kill a process by PID
 * @param args Arguments array (pid, optional signal)
 * @param argc Number of arguments
 * @return Boolean indicating success
 */
RuntimeValue *system_process_kill(RuntimeValue **args, size_t argc);
/**
 * @brief List files in directory
 * @param args Arguments array (optional path, defaults to current directory)
 * @param argc Number of arguments
 * @return Array of file objects with name, size, isDirectory, etc.
 */
RuntimeValue *system_filesystem_list(RuntimeValue **args, size_t argc);
/**
 * @brief Get file/directory information
 * @param args Arguments array (path)
 * @param argc Number of arguments
 * @return Object with file information
 */
RuntimeValue *system_filesystem_info(RuntimeValue **args, size_t argc);
/**
 * @brief Get CPU hardware information
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Object with CPU information
 */
RuntimeValue *system_hardware_cpu(RuntimeValue **args, size_t argc);
/**
 * @brief Get memory hardware information
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Object with memory information
 */
RuntimeValue *system_hardware_memory(RuntimeValue **args, size_t argc);

// DateTime Functions
/**
 * @brief Get current date and time
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Current date/time value
 */
RuntimeValue *datetime_now(RuntimeValue **args, size_t argc);

/**
 * @brief Format timestamp as string
 * @param args Arguments array (timestamp, optional format)
 * @param argc Number of arguments (1 or 2)
 * @return Formatted date string
 */
RuntimeValue *datetime_format(RuntimeValue **args, size_t argc);

/**
 * @brief Parse date string to timestamp
 * @param args Arguments array (date_string, optional format)
 * @param argc Number of arguments (1 or 2)
 * @return Timestamp number
 */
RuntimeValue *datetime_parse(RuntimeValue **args, size_t argc);

// Logging Functions
/**
 * @brief Log debug message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_debug(RuntimeValue **args, size_t argc);

/**
 * @brief Log info message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_info(RuntimeValue **args, size_t argc);

/**
 * @brief Log warning message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_warn(RuntimeValue **args, size_t argc);

/**
 * @brief Log error message
 * @param args Arguments array (message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_error(RuntimeValue **args, size_t argc);

/**
 * @brief Log formatted debug message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_debugf(RuntimeValue **args, size_t argc);

/**
 * @brief Log formatted info message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_infof(RuntimeValue **args, size_t argc);

/**
 * @brief Log formatted warning message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_warnf(RuntimeValue **args, size_t argc);

/**
 * @brief Log formatted error message
 * @param args Arguments array (format, ...)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_errorf(RuntimeValue **args, size_t argc);

/**
 * @brief Log debug message conditionally
 * @param args Arguments array (condition, message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_debug_if(RuntimeValue **args, size_t argc);

/**
 * @brief Set minimum log level
 * @param args Arguments array (level)
 * @param argc Number of arguments
 * @return Previous level or error
 */
RuntimeValue *logging_set_level(RuntimeValue **args, size_t argc);

/**
 * @brief Log with context information
 * @param args Arguments array (context, message)
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *logging_with_context(RuntimeValue **args, size_t argc);

/**
 * @brief Get current log level
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Current log level
 */
RuntimeValue *logging_get_level(RuntimeValue **args, size_t argc);

// HTTP Functions - declared in http.h with wrapper functions
// The actual stdlib wrapper functions are:
// stdlib_http_get, stdlib_http_post, stdlib_http_put,
// stdlib_http_delete, stdlib_http_timeout

// Regex Functions - declared in regex.h with wrapper functions
// The actual stdlib wrapper functions are:
// regex_match_stdlib, regex_replace_stdlib, regex_split_stdlib, regex_compile_stdlib

#endif
