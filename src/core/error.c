/*
 * error.c
 * Error handling and reporting
 */

#define _GNU_SOURCE  // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "zen/core/error.h"
#include "zen/types/value.h"
#include "zen/core/memory.h"

// Use memory_strdup instead of system strdup
#define strdup memory_strdup

/**
 * @brief Create a new error Value with a message
 * @param message Error message string
 * @return Newly created error Value
 */
Value* error_new(const char* message) {
    if (!message) {
        return NULL;
    }
    
    Value* error = value_new(VALUE_ERROR);
    if (!error || !error->as.error) {
        return NULL;
    }
    
    // Replace the default message with our custom message
    if (error->as.error->message) {
        memory_free(error->as.error->message);
    }
    error->as.error->message = memory_strdup(message);
    if (!error->as.error->message) {
        value_unref(error);  // Clean up the error value
        return NULL;
    }
    error->as.error->code = -1;  // Default error code
    
    return error;
}

/**
 * @brief Create a new error Value with a message and error code
 * @param message Error message string
 * @param code Error code
 * @return Newly created error Value
 */
Value* error_new_with_code(const char* message, int code) {
    Value* error = error_new(message);
    if (error && error->as.error) {
        error->as.error->code = code;
    }
    return error;
}

/**
 * @brief Create a formatted error Value with printf-style formatting
 * @param format Printf-style format string
 * @param ... Variable arguments for formatting
 * @return Newly created error Value
 */
Value* error_new_formatted(const char* format, ...) {
    if (!format) {
        return NULL;
    }
    
    // Calculate required buffer size
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);
    
    int len = vsnprintf(NULL, 0, format, args1);
    va_end(args1);
    
    if (len < 0) {
        va_end(args2);
        return NULL;
    }
    
    // Allocate buffer and format message
    char* message = memory_alloc(len + 1);
    if (!message) {
        va_end(args2);
        return NULL;
    }
    
    vsnprintf(message, len + 1, format, args2);
    va_end(args2);
    
    // Create error value
    Value* error = error_new(message);
    memory_free(message);
    
    return error;
}

/**
 * @brief Check if a Value is an error
 * @param value Value to check
 * @return true if value is an error, false otherwise
 */
bool error_is_error(const Value* value) {
    return value && value->type == VALUE_ERROR;
}

/**
 * @brief Get error message from an error Value
 * @param error Error Value
 * @return Error message string, or NULL if not an error or no message
 */
const char* error_get_message(const Value* error) {
    if (!error_is_error(error) || !error->as.error) {
        return NULL;
    }
    return error->as.error->message;
}

/**
 * @brief Get error code from an error Value
 * @param error Error Value
 * @return Error code, or 0 if not an error
 */
int error_get_code(const Value* error) {
    if (!error_is_error(error) || !error->as.error) {
        return 0;
    }
    return error->as.error->code;
}

/**
 * @brief Print error to stderr with optional prefix
 * @param error Error Value to print
 * @param prefix Optional prefix string (can be NULL)
 */
void error_print(const Value* error, const char* prefix) {
    if (!error_is_error(error)) {
        return;
    }
    
    const char* message = error_get_message(error);
    int code = error_get_code(error);
    
    if (prefix) {
        fprintf(stderr, "%s: ", prefix);
    }
    
    if (message) {
        fprintf(stderr, "%s", message);
    } else {
        fprintf(stderr, "Unknown error");
    }
    
    if (code != 0) {
        fprintf(stderr, " (code: %d)", code);
    }
    
    fprintf(stderr, "\n");
}

/**
 * @brief Print error to stderr with "Error" prefix
 * @param error Error Value to print
 */
void error_print_simple(const Value* error) {
    error_print(error, "Error");
}

/**
 * @brief Check if a Value represents a specific error code
 * @param error Error Value to check
 * @param code Error code to match
 * @return true if error matches the code, false otherwise
 */
bool error_has_code(const Value* error, int code) {
    // Comprehensive NULL pointer validation
    if (!error) {
        return false;
    }
    
    // Validate that the value is actually an error type
    if (!error_is_error(error)) {
        return false;
    }
    
    // Validate that the error structure is properly initialized
    if (!error->as.error) {
        return false;
    }
    
    // Get the error code using the safe accessor function
    int error_code = error_get_code(error);
    
    // Perform the comparison - handle special case of zero codes
    if (code == 0 && error_code == 0) {
        // Both are zero - this might indicate default/unset error codes
        return true;
    }
    
    // Standard comparison for all other cases
    return error_code == code;
}

/**
 * @brief Check if a Value contains a specific error message
 * @param error Error Value to check
 * @param message_substring Substring to search for in error message
 * @return true if error message contains the substring, false otherwise
 */
bool error_has_message(const Value* error, const char* message_substring) {
    if (!error_is_error(error) || !message_substring) {
        return false;
    }
    
    const char* error_message = error_get_message(error);
    if (!error_message) {
        return false;
    }
    
    return strstr(error_message, message_substring) != NULL;
}

/**
 * @brief Create common runtime errors
 */

/**
 * @brief Create null pointer error
 * @param operation Optional operation name that caused the error
 * @return Newly created error Value with null pointer message
 */
Value* error_null_pointer(const char* operation) {
    return error_new_formatted("Null pointer in %s", operation ? operation : "operation");
}

/**
 * @brief Create type mismatch error
 * @param expected Expected type name
 * @param actual Actual type name received
 * @return Newly created error Value with type mismatch message
 */
Value* error_type_mismatch(const char* expected, const char* actual) {
    return error_new_formatted("Type mismatch: expected %s, got %s", 
                              expected ? expected : "unknown", 
                              actual ? actual : "unknown");
}

/**
 * @brief Create division by zero error
 * @return Newly created error Value with division by zero message
 */
Value* error_division_by_zero(void) {
    return error_new_with_code("Division by zero", -1);
}

/**
 * @brief Create index out of bounds error
 * @param index Index that was accessed
 * @param length Length/size of the container
 * @return Newly created error Value with index out of bounds message
 */
Value* error_index_out_of_bounds(size_t index, size_t length) {
    return error_new_formatted("Index out of bounds: %zu >= %zu", index, length);
}

/**
 * @brief Create invalid argument error
 * @param function_name Name of function that received invalid argument
 * @param argument_description Description of what was invalid about the argument
 * @return Newly created error Value with invalid argument message
 */
Value* error_invalid_argument(const char* function_name, const char* argument_description) {
    return error_new_formatted("Invalid argument in %s: %s", 
                              function_name ? function_name : "function",
                              argument_description ? argument_description : "bad argument");
}

/**
 * @brief Create memory allocation error
 * @return Newly created error Value with memory allocation failure message
 */
Value* error_memory_allocation(void) {
    return error_new_with_code("Memory allocation failed", -2);
}

/**
 * @brief Create file not found error
 * @param filename Name of file that was not found
 * @return Newly created error Value with file not found message
 */
Value* error_file_not_found(const char* filename) {
    return error_new_formatted("File not found: %s", filename ? filename : "unknown file");
}

/**
 * @brief Create parsing failed error
 * @param context Context or details about what failed to parse
 * @return Newly created error Value with parsing failure message
 */
Value* error_parsing_failed(const char* context) {
    return error_new_formatted("Parsing failed: %s", context ? context : "syntax error");
}

/**
 * @brief Create undefined variable error
 * @param variable_name Name of the undefined variable
 * @return Newly created error Value with undefined variable message
 */
Value* error_undefined_variable(const char* variable_name) {
    return error_new_formatted("Undefined variable: %s", variable_name ? variable_name : "unknown");
}

/**
 * @brief Create undefined function error
 * @param function_name Name of the undefined function
 * @return Newly created error Value with undefined function message
 */
Value* error_undefined_function(const char* function_name) {
    return error_new_formatted("Undefined function: %s", function_name ? function_name : "unknown");
}

/**
 * @brief Error handling utilities for operations
 */

/**
 * @brief Propagate error or return value unchanged
 * @param value Value to check and potentially propagate
 * @return Original value if not an error, or error value to propagate up
 */
Value* error_propagate(Value* value) {
    // If value is already an error, just return it (propagate it up)
    if (error_is_error(value)) {
        return value;
    }
    
    // If not an error, return as-is
    return value;
}

/**
 * @brief Check if any value in array is an error
 * @param values Array of Value pointers to check
 * @param count Number of values in the array
 * @return true if any value is an error, false otherwise
 */
bool error_occurred(Value** values, size_t count) {
    if (!values) {
        return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (error_is_error(values[i])) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get first error from array of values
 * @param values Array of Value pointers to check
 * @param count Number of values in the array
 * @return First error Value found, or NULL if no errors
 */
Value* error_first_error(Value** values, size_t count) {
    if (!values) {
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (error_is_error(values[i])) {
            return values[i];
        }
    }
    return NULL;
}
