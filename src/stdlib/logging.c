/*
 * logging.c
 * ZEN Logging System - Structured logging for ZEN programs
 *
 * Provides clean, structured logging functions that replace debug printf statements
 * and offer formatted output with timestamps and log levels.
 */

#include "zen/stdlib/logging.h"

#include "zen/config.h"
#include "zen/core/memory.h"
#include "zen/types/value.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global log level (defaults to INFO)
static ZenLogLevel current_log_level = ZEN_LOG_INFO;

// Maximum formatted message size
// Use configuration system for message size limit
#define MAX_TIMESTAMP_SIZE 32

/**
 * @brief Get current timestamp string for logging
 * @return Dynamically allocated timestamp string (must be freed)
 */
char *logging_get_timestamp(void)
{
    char *timestamp = memory_alloc(MAX_TIMESTAMP_SIZE);
    if (!timestamp) {
        return NULL;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (strftime(timestamp, MAX_TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        memory_free(timestamp);
        return NULL;
    }

    return timestamp;
}

/**
 * @brief Get level string for given log level
 * @param level Log level
 * @return Static string representation of level
 */
const char *logging_level_string(ZenLogLevel level)
{
    switch (level) {
    case ZEN_LOG_DEBUG:
        return "DEBUG";
    case ZEN_LOG_INFO:
        return "INFO ";
    case ZEN_LOG_WARN:
        return "WARN ";
    case ZEN_LOG_ERROR:
        return "ERROR";
    default:
        return "UNKN ";
    }
}

/**
 * @brief Parse log level from string
 * @param level_str String representation of level
 * @return Log level enum value, or -1 if invalid
 */
int logging_parse_level(const char *level_str)
{
    if (!level_str)
        return -1;

    if (strcmp(level_str, "DEBUG") == 0 || strcmp(level_str, "debug") == 0) {
        return ZEN_LOG_DEBUG;
    } else if (strcmp(level_str, "INFO") == 0 || strcmp(level_str, "info") == 0) {
        return ZEN_LOG_INFO;
    } else if (strcmp(level_str, "WARN") == 0 || strcmp(level_str, "warn") == 0) {
        return ZEN_LOG_WARN;
    } else if (strcmp(level_str, "ERROR") == 0 || strcmp(level_str, "error") == 0) {
        return ZEN_LOG_ERROR;
    }

    return -1;
}

/**
 * @brief Check if given level should be logged
 * @param level Level to check
 * @return true if should log, false otherwise
 */
bool logging_should_log(ZenLogLevel level) { return level >= current_log_level; }

/**
 * @brief Internal function to format and output log message
 * @param level Log level
 * @param context Optional context string (can be NULL)
 * @param message Message to log
 */
static void log_output(ZenLogLevel level, const char *context, const char *message)
{
    if (!logging_should_log(level)) {
        return;
    }

    char *timestamp = logging_get_timestamp();
    if (!timestamp) {
        // Fallback without timestamp
        if (context) {
            fprintf(stderr, "[%s] [%s] %s\n", logging_level_string(level), context, message);
        } else {
            fprintf(stderr, "[%s] %s\n", logging_level_string(level), message);
        }
        return;
    }

    if (context) {
        fprintf(stderr,
                "[%s] [%s] [%s] %s\n",
                timestamp,
                logging_level_string(level),
                context,
                message);
    } else {
        fprintf(stderr, "[%s] [%s] %s\n", timestamp, logging_level_string(level), message);
    }

    memory_free(timestamp);
}

/**
 * @brief Log a debug message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value *logging_debug(Value **args, size_t argc)
{
    if (argc != 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logDebug expects one string argument", 1);
    }

    log_output(ZEN_LOG_DEBUG, NULL, args[0]->as.string->data);
    return value_new_null();
}

/**
 * @brief Log an info message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value *logging_info(Value **args, size_t argc)
{
    if (argc != 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logInfo expects one string argument", 1);
    }

    log_output(ZEN_LOG_INFO, NULL, args[0]->as.string->data);
    return value_new_null();
}

/**
 * @brief Log a warning message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value *logging_warn(Value **args, size_t argc)
{
    if (argc != 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logWarn expects one string argument", 1);
    }

    log_output(ZEN_LOG_WARN, NULL, args[0]->as.string->data);
    return value_new_null();
}

/**
 * @brief Log an error message
 * @param args Arguments array (message string)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value *logging_error(Value **args, size_t argc)
{
    if (argc != 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logError expects one string argument", 1);
    }

    log_output(ZEN_LOG_ERROR, NULL, args[0]->as.string->data);
    return value_new_null();
}

/**
 * @brief Internal helper to format message with arguments
 * @param format_str Format string
 * @param args Arguments for formatting
 * @param argc Number of arguments
 * @return Formatted message string (must be freed), or NULL on error
 */
static char *log_format_message(const char *format_str, Value **args, size_t argc)
{
    char *buffer = memory_alloc(ZEN_MAX_LOG_MESSAGE_SIZE);
    if (!buffer) {
        return NULL;
    }

    // Simple implementation - just concatenate args as strings
    // For a full printf implementation, we'd need more complex parsing
    int offset = snprintf(buffer, ZEN_MAX_LOG_MESSAGE_SIZE, "%s", format_str);

    for (size_t i = 1; i < argc && offset < ZEN_MAX_LOG_MESSAGE_SIZE - 1; i++) {
        if (args[i]) {
            const char *str_repr = NULL;
            char number_buf[32];

            switch (args[i]->type) {
            case VALUE_STRING:
                str_repr = args[i]->as.string->data;
                break;
            case VALUE_NUMBER:
                snprintf(number_buf, sizeof(number_buf), "%.15g", args[i]->as.number);
                str_repr = number_buf;
                break;
            case VALUE_BOOLEAN:
                str_repr = args[i]->as.boolean ? "true" : "false";
                break;
            case VALUE_NULL:
                str_repr = "null";
                break;
            default:
                str_repr = "<object>";
                break;
            }

            offset +=
                snprintf(buffer + offset, ZEN_MAX_LOG_MESSAGE_SIZE - offset - 1, " %s", str_repr);
        }
    }

    return buffer;
}

/**
 * @brief Log a formatted debug message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value *logging_debugf(Value **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logDebugf expects format string as first argument", 1);
    }

    char *formatted = log_format_message(args[0]->as.string->data, args, argc);
    if (formatted) {
        log_output(ZEN_LOG_DEBUG, NULL, formatted);
        memory_free(formatted);
    }

    return value_new_null();
}

/**
 * @brief Log a formatted info message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value *logging_infof(Value **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logInfof expects format string as first argument", 1);
    }

    char *formatted = log_format_message(args[0]->as.string->data, args, argc);
    if (formatted) {
        log_output(ZEN_LOG_INFO, NULL, formatted);
        memory_free(formatted);
    }

    return value_new_null();
}

/**
 * @brief Log a formatted warning message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value *logging_warnf(Value **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logWarnf expects format string as first argument", 1);
    }

    char *formatted = log_format_message(args[0]->as.string->data, args, argc);
    if (formatted) {
        log_output(ZEN_LOG_WARN, NULL, formatted);
        memory_free(formatted);
    }

    return value_new_null();
}

/**
 * @brief Log a formatted error message (printf-style)
 * @param args Arguments array (format string, values...)
 * @param argc Number of arguments (format + values)
 * @return Null value
 */
Value *logging_errorf(Value **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("logErrorf expects format string as first argument", 1);
    }

    char *formatted = log_format_message(args[0]->as.string->data, args, argc);
    if (formatted) {
        log_output(ZEN_LOG_ERROR, NULL, formatted);
        memory_free(formatted);
    }

    return value_new_null();
}

/**
 * @brief Log a debug message only if condition is true
 * @param args Arguments array (condition boolean, message string)
 * @param argc Number of arguments (should be 2)
 * @return Null value
 */
Value *logging_debug_if(Value **args, size_t argc)
{
    if (argc != 2 || !args[0] || !args[1]) {
        return value_new_error("logDebugIf expects condition and message arguments", 1);
    }

    if (args[1]->type != VALUE_STRING) {
        return value_new_error("logDebugIf second argument must be a string", 1);
    }

    // Check if condition is truthy
    bool condition = false;
    switch (args[0]->type) {
    case VALUE_BOOLEAN:
        condition = args[0]->as.boolean;
        break;
    case VALUE_NUMBER:
        condition = args[0]->as.number != 0.0;
        break;
    case VALUE_STRING:
        condition = args[0]->as.string->length > 0;
        break;
    case VALUE_NULL:
        condition = false;
        break;
    default:
        condition = true;  // Objects are truthy
        break;
    }

    if (condition) {
        log_output(ZEN_LOG_DEBUG, NULL, args[1]->as.string->data);
    }

    return value_new_null();
}

/**
 * @brief Set minimum logging level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
 * @param args Arguments array (level number)
 * @param argc Number of arguments (should be 1)
 * @return Null value
 */
Value *logging_set_level(Value **args, size_t argc)
{
    if (argc != 1 || !args[0] || args[0]->type != VALUE_NUMBER) {
        return value_new_error("logSetLevel expects one number argument", 1);
    }

    int level = (int)args[0]->as.number;
    if (level < ZEN_LOG_DEBUG || level > ZEN_LOG_ERROR) {
        return value_new_error("logSetLevel level must be 0-3 (DEBUG, INFO, WARN, ERROR)", 1);
    }

    current_log_level = (ZenLogLevel)level;
    return value_new_null();
}

/**
 * @brief Log a message with context information
 * @param args Arguments array (context string, level string, message)
 * @param argc Number of arguments (should be 3)
 * @return Null value
 */
Value *logging_with_context(Value **args, size_t argc)
{
    if (argc != 3 || !args[0] || !args[1] || !args[2]) {
        return value_new_error("logWithContext expects context, level, and message arguments", 1);
    }

    if (args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING ||
        args[2]->type != VALUE_STRING) {
        return value_new_error("logWithContext all arguments must be strings", 1);
    }

    int level = logging_parse_level(args[1]->as.string->data);
    if (level < 0) {
        return value_new_error("logWithContext invalid level (use DEBUG, INFO, WARN, ERROR)", 1);
    }

    log_output((ZenLogLevel)level, args[0]->as.string->data, args[2]->as.string->data);
    return value_new_null();
}

/**
 * @brief Get current logging level
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Number value representing current log level
 */
Value *logging_get_level(Value **args, size_t argc)
{
    (void)args;  // Suppress unused parameter warning
    (void)argc;

    return value_new_number((double)current_log_level);
}
