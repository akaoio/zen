/*
 * logging.c
 * ZEN Logging System for ZEN stdlib
 *
 * This is a stub implementation to allow compilation.
 * Full logging features will be implemented in a later phase.
 */

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Logging levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} LogLevel;

// Global logging level (default to INFO)
static LogLevel current_log_level = LOG_INFO;

// Helper function to get timestamp
static const char *get_timestamp(void)
{
    static char buffer[32];
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

// Helper function to convert value to string for logging
static const char *value_to_log_string(RuntimeValue *value)
{
    if (!value)
        return "(null)";

    switch (value->type) {
    case RV_NULL:
        return "null";
    case RV_BOOLEAN:
        return value->data.boolean ? "true" : "false";
    case RV_NUMBER: {
        static char buffer[64];
        double num = value->data.number;
        if (num == (long)num) {
            snprintf(buffer, sizeof(buffer), "%ld", (long)num);
        } else {
            snprintf(buffer, sizeof(buffer), "%g", num);
        }
        return buffer;
    }
    case RV_STRING:
        return value->data.string.data;
    case RV_ARRAY:
        return "[Array]";
    case RV_OBJECT:
        return "{Object}";
    case RV_FUNCTION:
        return "[Function]";
    case RV_ERROR:
        return value->data.error.message;
    default:
        return "(unknown)";
    }
}

// Log a message at a given level
static void log_message(LogLevel level, const char *level_str, RuntimeValue **args, size_t argc)
{
    if (level < current_log_level) {
        return;  // Skip if below current log level
    }

    fprintf(stderr, "[%s] %s ", get_timestamp(), level_str);

    // Print all arguments
    for (size_t i = 0; i < argc; i++) {
        if (i > 0)
            fprintf(stderr, " ");
        fprintf(stderr, "%s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);
}

// Stub implementations for logging functions
// These return appropriate error messages indicating future implementation

RuntimeValue *logging_debug(RuntimeValue **args, size_t argc)
{
    log_message(LOG_DEBUG, "DEBUG", args, argc);
    return rv_new_null();
}

RuntimeValue *logging_info(RuntimeValue **args, size_t argc)
{
    log_message(LOG_INFO, "INFO", args, argc);
    return rv_new_null();
}

RuntimeValue *logging_warn(RuntimeValue **args, size_t argc)
{
    log_message(LOG_WARN, "WARN", args, argc);
    return rv_new_null();
}

RuntimeValue *logging_error(RuntimeValue **args, size_t argc)
{
    log_message(LOG_ERROR, "ERROR", args, argc);
    return rv_new_null();
}

RuntimeValue *logging_debugf(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("logDebugf requires at least 1 argument (format string)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logDebugf requires a string format", -1);
    }

    if (LOG_DEBUG < current_log_level) {
        return rv_new_null();  // Skip if below current log level
    }

    // For now, just print the format string and arguments separately
    // A full printf-style formatter would require more complex parsing
    fprintf(stderr, "[%s] DEBUG %s", get_timestamp(), args[0]->data.string.data);

    // Print additional arguments
    for (size_t i = 1; i < argc; i++) {
        fprintf(stderr, " %s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    return rv_new_null();
}

RuntimeValue *logging_infof(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("logInfof requires at least 1 argument (format string)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logInfof requires a string format", -1);
    }

    if (LOG_INFO < current_log_level) {
        return rv_new_null();  // Skip if below current log level
    }

    // For now, just print the format string and arguments separately
    fprintf(stderr, "[%s] INFO %s", get_timestamp(), args[0]->data.string.data);

    // Print additional arguments
    for (size_t i = 1; i < argc; i++) {
        fprintf(stderr, " %s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    return rv_new_null();
}

RuntimeValue *logging_warnf(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("logWarnf requires at least 1 argument (format string)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logWarnf requires a string format", -1);
    }

    if (LOG_WARN < current_log_level) {
        return rv_new_null();  // Skip if below current log level
    }

    // For now, just print the format string and arguments separately
    fprintf(stderr, "[%s] WARN %s", get_timestamp(), args[0]->data.string.data);

    // Print additional arguments
    for (size_t i = 1; i < argc; i++) {
        fprintf(stderr, " %s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    return rv_new_null();
}

RuntimeValue *logging_errorf(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("logErrorf requires at least 1 argument (format string)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logErrorf requires a string format", -1);
    }

    if (LOG_ERROR < current_log_level) {
        return rv_new_null();  // Skip if below current log level
    }

    // For now, just print the format string and arguments separately
    fprintf(stderr, "[%s] ERROR %s", get_timestamp(), args[0]->data.string.data);

    // Print additional arguments
    for (size_t i = 1; i < argc; i++) {
        fprintf(stderr, " %s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    return rv_new_null();
}

RuntimeValue *logging_debug_if(RuntimeValue **args, size_t argc)
{
    if (argc < 2) {
        return rv_new_error("logDebugIf requires at least 2 arguments (condition, message)", -1);
    }

    if (!args[0] || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logDebugIf requires (condition, string message)", -1);
    }

    // Check if condition is truthy
    bool condition = false;
    switch (args[0]->type) {
    case RV_BOOLEAN:
        condition = args[0]->data.boolean;
        break;
    case RV_NUMBER:
        condition = args[0]->data.number != 0.0;
        break;
    case RV_STRING:
        condition = args[0]->data.string.length > 0;
        break;
    case RV_NULL:
        condition = false;
        break;
    default:
        condition = true;
    }

    // Only log if condition is true
    if (condition) {
        log_message(LOG_DEBUG, "DEBUG", &args[1], argc - 1);
    }

    return rv_new_null();
}

RuntimeValue *logging_set_level(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("setLogLevel requires exactly 1 argument (level)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("setLogLevel requires a string level (DEBUG, INFO, WARN, ERROR)", -1);
    }

    const char *level_str = args[0]->data.string.data;

    if (strcmp(level_str, "DEBUG") == 0) {
        current_log_level = LOG_DEBUG;
    } else if (strcmp(level_str, "INFO") == 0) {
        current_log_level = LOG_INFO;
    } else if (strcmp(level_str, "WARN") == 0) {
        current_log_level = LOG_WARN;
    } else if (strcmp(level_str, "ERROR") == 0) {
        current_log_level = LOG_ERROR;
    } else {
        return rv_new_error("Invalid log level. Use: DEBUG, INFO, WARN, ERROR", -1);
    }

    return rv_new_null();
}

RuntimeValue *logging_with_context(RuntimeValue **args, size_t argc)
{
    if (argc < 2) {
        return rv_new_error("logWithContext requires at least 2 arguments (context, message)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logWithContext requires (string context, string message)", -1);
    }

    const char *context = args[0]->data.string.data;
    const char *message = args[1]->data.string.data;

    // Log with context prefix
    fprintf(stderr, "[%s] INFO [%s] %s", get_timestamp(), context, message);

    // Print additional arguments if any
    for (size_t i = 2; i < argc; i++) {
        fprintf(stderr, " %s", value_to_log_string(args[i]));
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    return rv_new_null();
}

RuntimeValue *logging_get_level(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    // Return the current log level as a string
    const char *level_str;
    switch (current_log_level) {
    case LOG_DEBUG:
        level_str = "DEBUG";
        break;
    case LOG_INFO:
        level_str = "INFO";
        break;
    case LOG_WARN:
        level_str = "WARN";
        break;
    case LOG_ERROR:
        level_str = "ERROR";
        break;
    default:
        level_str = "UNKNOWN";
    }

    return rv_new_string(level_str);
}