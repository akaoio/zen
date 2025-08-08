#ifndef ZEN_CORE_ERROR_H
#define ZEN_CORE_ERROR_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Value Value;

/**
 * @brief Error creation functions
 */

/**
 * @brief Create a new error Value with a message
 * @param message Error message string
 * @return Newly created error Value
 */
Value *error_new(const char *message);

/**
 * @brief Create a new error Value with a message and error code
 * @param message Error message string
 * @param code Error code
 * @return Newly created error Value
 */
Value *error_new_with_code(const char *message, int code);

/**
 * @brief Create a formatted error Value with printf-style formatting
 * @param format Printf-style format string
 * @param ... Variable arguments for formatting
 * @return Newly created error Value
 */
Value *error_new_formatted(const char *format, ...);

/**
 * @brief Error inspection functions
 */

/**
 * @brief Check if a Value is an error
 * @param value Value to check
 * @return true if value is an error, false otherwise
 */
bool error_is_error(const Value *value);

/**
 * @brief Get error message from an error Value
 * @param error Error Value
 * @return Error message string, or NULL if not an error
 */
const char *error_get_message(const Value *error);

/**
 * @brief Get error code from an error Value
 * @param error Error Value
 * @return Error code, or 0 if not an error
 */
int error_get_code(const Value *error);

/**
 * @brief Error printing functions
 */

/**
 * @brief Print error to stderr with optional prefix
 * @param error Error Value to print
 * @param prefix Optional prefix string (can be NULL)
 */
void error_print(const Value *error, const char *prefix);

/**
 * @brief Print error to stderr with "Error" prefix
 * @param error Error Value to print
 */
void error_print_simple(const Value *error);

/**
 * @brief Error checking utilities
 */

/**
 * @brief Check if a Value represents a specific error code
 * @param error Error Value to check
 * @param code Error code to match
 * @return true if error matches the code, false otherwise
 */
bool error_has_code(const Value *error, int code);

/**
 * @brief Check if a Value contains a specific error message
 * @param error Error Value to check
 * @param message_substring Substring to search for in error message
 * @return true if error message contains the substring, false otherwise
 */
bool error_has_message(const Value *error, const char *message_substring);

/**
 * @brief Common error creators
 */

/**
 * @brief Create null pointer error
 * @param operation Optional operation name that caused the error
 * @return Newly created error Value with null pointer message
 */
Value *error_null_pointer(const char *operation);

/**
 * @brief Create type mismatch error
 * @param expected Expected type name
 * @param actual Actual type name received
 * @return Newly created error Value with type mismatch message
 */
Value *error_type_mismatch(const char *expected, const char *actual);

/**
 * @brief Create division by zero error
 * @return Newly created error Value with division by zero message
 */
Value *error_division_by_zero(void);

/**
 * @brief Create index out of bounds error
 * @param index Index that was accessed
 * @param length Length/size of the container
 * @return Newly created error Value with index out of bounds message
 */
Value *error_index_out_of_bounds(size_t index, size_t length);

/**
 * @brief Create invalid argument error
 * @param function_name Name of function that received invalid argument
 * @param argument_description Description of what was invalid about the argument
 * @return Newly created error Value with invalid argument message
 */
Value *error_invalid_argument(const char *function_name, const char *argument_description);

/**
 * @brief Create memory allocation error
 * @return Newly created error Value with memory allocation failure message
 */
Value *error_memory_allocation(void);

/**
 * @brief Create file not found error
 * @param filename Name of file that was not found
 * @return Newly created error Value with file not found message
 */
Value *error_file_not_found(const char *filename);

/**
 * @brief Create parsing failed error
 * @param context Context or details about what failed to parse
 * @return Newly created error Value with parsing failure message
 */
Value *error_parsing_failed(const char *context);

/**
 * @brief Create undefined variable error
 * @param variable_name Name of the undefined variable
 * @return Newly created error Value with undefined variable message
 */
Value *error_undefined_variable(const char *variable_name);

/**
 * @brief Create undefined function error
 * @param function_name Name of the undefined function
 * @return Newly created error Value with undefined function message
 */
Value *error_undefined_function(const char *function_name);

/**
 * @brief Error handling utilities
 */

/**
 * @brief Propagate error or return value unchanged
 * @param value Value to check and potentially propagate
 * @return Original value if not an error, or error value to propagate up
 */
Value *error_propagate(Value *value);

/**
 * @brief Check if any value in array is an error
 * @param values Array of Value pointers to check
 * @param count Number of values in the array
 * @return true if any value is an error, false otherwise
 */
bool error_occurred(Value **values, size_t count);

/**
 * @brief Get first error from array of values
 * @param values Array of Value pointers to check
 * @param count Number of values in the array
 * @return First error Value found, or NULL if no errors
 */
Value *error_first_error(Value **values, size_t count);

#endif /* ZEN_CORE_ERROR_H */