/**
 * @file logger.c
 * @brief Implementation of centralized logging system for ZEN interpreter
 */

#include "zen/core/logger.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Global logging configuration
static struct {
    LogLevel level;
    int categories;
    FILE* file;
    bool initialized;
} g_logger = {
    .level = LOG_LEVEL_SILENT,  // Default to silent for production
    .categories = LOG_CAT_ALL,
    .file = NULL,
    .initialized = false
};

// Level names for output formatting
static const char* level_names[] = {
    "SILENT",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

// Category names for debugging
static const char* category_names[] = {
    "GENERAL",
    "LEXER",
    "PARSER", 
    "AST",
    "VISITOR",
    "MEMORY",
    "VALUES",
    "STDLIB"
};

/**
 * @brief Initialize the internal logging system
 * @param void Function takes no parameters
 */
void logger_init(void) {
    if (g_logger.initialized) {
        return;
    }
    
    // Check environment variables for default configuration
    const char* level_env = getenv("ZEN_LOG_LEVEL");
    if (level_env) {
        if (strcmp(level_env, "ERROR") == 0) {
            g_logger.level = LOG_LEVEL_ERROR;
        } else if (strcmp(level_env, "WARN") == 0) {
            g_logger.level = LOG_LEVEL_WARN;
        } else if (strcmp(level_env, "INFO") == 0) {
            g_logger.level = LOG_LEVEL_INFO;
        } else if (strcmp(level_env, "DEBUG") == 0) {
            g_logger.level = LOG_LEVEL_DEBUG;
        }
    }
    
    const char* categories_env = getenv("ZEN_LOG_CATEGORIES");
    if (categories_env) {
        g_logger.categories = 0;
        if (strstr(categories_env, "GENERAL")) g_logger.categories |= LOG_CAT_GENERAL;
        if (strstr(categories_env, "LEXER")) g_logger.categories |= LOG_CAT_LEXER;
        if (strstr(categories_env, "PARSER")) g_logger.categories |= LOG_CAT_PARSER;
        if (strstr(categories_env, "AST")) g_logger.categories |= LOG_CAT_AST;
        if (strstr(categories_env, "VISITOR")) g_logger.categories |= LOG_CAT_VISITOR;
        if (strstr(categories_env, "MEMORY")) g_logger.categories |= LOG_CAT_MEMORY;
        if (strstr(categories_env, "VALUES")) g_logger.categories |= LOG_CAT_VALUES;
        if (strstr(categories_env, "STDLIB")) g_logger.categories |= LOG_CAT_STDLIB;
        if (strstr(categories_env, "ALL")) g_logger.categories = LOG_CAT_ALL;
    }
    
    const char* file_env = getenv("ZEN_LOG_FILE");
    if (file_env) {
        logger_set_file(file_env);
    }
    
    g_logger.initialized = true;
}

/**
 * @brief Set the global log level for internal logging
 * @param level The log level to set
 */
void logger_set_level(LogLevel level) {
    if (!g_logger.initialized) {
        logger_init();
    }
    g_logger.level = level;
}

/**
 * @brief Get the current internal log level
 * @param void Function takes no parameters
 * @return Current log level
 */
LogLevel logger_get_level(void) {
    if (!g_logger.initialized) {
        logger_init();
    }
    return g_logger.level;
}

/**
 * @brief Set which categories to log (bitmask)
 * @param categories Bitmask of categories to log
 */
void logger_set_categories(int categories) {
    if (!g_logger.initialized) {
        logger_init();
    }
    g_logger.categories = categories;
}

/**
 * @brief Get the current category bitmask
 * @param void Function takes no parameters
 * @return Current category bitmask
 */
int logger_get_categories(void) {
    if (!g_logger.initialized) {
        logger_init();
    }
    return g_logger.categories;
}

/**
 * @brief Enable/disable logging to file
 * @param filename File to log to, or NULL to disable file logging
 * @return true if successful, false on error
 */
bool logger_set_file(const char* filename) {
    if (!g_logger.initialized) {
        logger_init();
    }
    
    // Close existing file if any
    if (g_logger.file && g_logger.file != stderr && g_logger.file != stdout) {
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
    
    if (!filename) {
        return true;
    }
    
    g_logger.file = fopen(filename, "a");
    return g_logger.file != NULL;
}

/**
 * @brief Check if a message should be logged
 * @param level The log level of the message
 * @param category The log category of the message
 * @return true if message should be logged, false otherwise
 */
bool logger_should_log(LogLevel level, LogCategory category) {
    if (!g_logger.initialized) {
        logger_init();
    }
    
    return (level <= g_logger.level) && (g_logger.categories & category);
}

/**
 * @brief Log a message with specified level and category
 * @param level The log level for this message
 * @param category The log category for this message
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void logger_log(LogLevel level, LogCategory category, const char* format, ...) {
    if (!logger_should_log(level, category)) {
        return;
    }
    
    // Get output stream (file if set, otherwise stderr for errors, stdout for others)
    FILE* output = g_logger.file;
    if (!output) {
        output = (level == LOG_LEVEL_ERROR) ? stderr : stdout;
    }
    
    // Get current timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
    
    // Find category name
    const char* cat_name = "UNKNOWN";
    for (int i = 0; i < 8; i++) {
        if (category & (1 << i)) {
            cat_name = category_names[i];
            break;
        }
    }
    
    // Print prefix: [TIMESTAMP] LEVEL CATEGORY: 
    fprintf(output, "[%s] %s %s: ", timestamp, level_names[level], cat_name);
    
    // Print the actual message
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
    
    // Ensure newline
    if (format[strlen(format) - 1] != '\n') {
        fprintf(output, "\n");
    }
    
    // Flush immediately for debugging
    fflush(output);
}

/**
 * @brief Cleanup the internal logging system
 * @param void Function takes no parameters
 */
void logger_cleanup(void) {
    if (g_logger.file && g_logger.file != stderr && g_logger.file != stdout) {
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
    g_logger.initialized = false;
}