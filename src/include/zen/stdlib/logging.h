#ifndef ZEN_STDLIB_LOGGING_H
#define ZEN_STDLIB_LOGGING_H

#include "zen/types/value.h"
#include <stddef.h>

/**
 * @brief Logging level enumeration for ZEN logging system
 */
typedef enum {
    ZEN_LOG_DEBUG = 0,  /**< Debug level - detailed information for debugging */
    ZEN_LOG_INFO  = 1,  /**< Info level - general information messages */
    ZEN_LOG_WARN  = 2,  /**< Warning level - potential issues */
    ZEN_LOG_ERROR = 3   /**< Error level - error conditions */
} ZenLogLevel;

// Stdlib logging function declarations

/**
 * @brief Log a debug message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value* logging_debug(Value** args, size_t argc);

/**
 * @brief Log an info message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value* logging_info(Value** args, size_t argc);

/**
 * @brief Log a warning message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value* logging_warn(Value** args, size_t argc);

/**
 * @brief Log an error message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value* logging_error(Value** args, size_t argc);

/**
 * @brief Log a formatted debug message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value* logging_debugf(Value** args, size_t argc);

/**
 * @brief Log a formatted info message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value* logging_infof(Value** args, size_t argc);

/**
 * @brief Log a formatted warning message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value* logging_warnf(Value** args, size_t argc);

/**
 * @brief Log a formatted error message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value* logging_errorf(Value** args, size_t argc);

/**
 * @brief Log a debug message only if condition is true
 * @param args Arguments array (condition boolean, message string)
 * @param argc Number of arguments (should be 2)
 * @return Null value
 */
Value* logging_debug_if(Value** args, size_t argc);

/**
 * @brief Set minimum logging level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
 * @param args Arguments array (level number)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value* logging_set_level(Value** args, size_t argc);

/**
 * @brief Log a message with context information
 * @param args Arguments array (context string, level string, message)
 * @param argc Number of arguments (should be 3)
 * @return Null value
 */
Value* logging_with_context(Value** args, size_t argc);

/**
 * @brief Get current logging level
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Number value representing current log level
 */
Value* logging_get_level(Value** args, size_t argc);

// Internal helper functions (not exposed to ZEN programs)

/**
 * @brief Get current timestamp string for logging
 * @return Dynamically allocated timestamp string (must be freed)
 */
char* logging_get_timestamp(void);

/**
 * @brief Get level string for given log level
 * @param level Log level
 * @return Static string representation of level
 */
const char* logging_level_string(ZenLogLevel level);

/**
 * @brief Parse log level from string
 * @param level_str String representation of level
 * @return Log level enum value, or -1 if invalid
 */
int logging_parse_level(const char* level_str);

/**
 * @brief Check if given level should be logged
 * @param level Level to check
 * @return true if should log, false otherwise
 */
bool logging_should_log(ZenLogLevel level);

#endif /* ZEN_STDLIB_LOGGING_H */