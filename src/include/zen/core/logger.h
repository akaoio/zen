/**
 * @file logger.h
 * @brief Centralized logging system for ZEN interpreter
 * 
 * This provides a centralized logging system that can be controlled at runtime
 * to replace scattered printf/fprintf debug statements throughout the codebase.
 */

#ifndef ZEN_CORE_LOGGER_H
#define ZEN_CORE_LOGGER_H

#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Log levels for controlling verbosity
 */
typedef enum {
    LOG_LEVEL_SILENT = 0,  // No output
    LOG_LEVEL_ERROR = 1,   // Errors only
    LOG_LEVEL_WARN = 2,    // Warnings and errors
    LOG_LEVEL_INFO = 3,    // Info, warnings, and errors
    LOG_LEVEL_DEBUG = 4    // All messages including debug
} LogLevel;

/**
 * @brief Log categories for fine-grained control
 */
typedef enum {
    LOG_CAT_GENERAL = 0x01,   // General messages
    LOG_CAT_LEXER = 0x02,     // Lexer operations
    LOG_CAT_PARSER = 0x04,    // Parser operations
    LOG_CAT_AST = 0x08,       // AST operations
    LOG_CAT_VISITOR = 0x10,   // Visitor/runtime operations
    LOG_CAT_MEMORY = 0x20,    // Memory management
    LOG_CAT_VALUES = 0x40,    // Value operations
    LOG_CAT_STDLIB = 0x80,    // Standard library
    LOG_CAT_ALL = 0xFF        // All categories
} LogCategory;

/**
 * @brief Initialize the logging system
 */
void logger_init(void);

/**
 * @brief Set the global log level
 * @param level Maximum level of messages to output
 */
void logger_set_level(LogLevel level);

/**
 * @brief Get the current log level
 * @return Current log level
 */
LogLevel logger_get_level(void);

/**
 * @brief Set which categories to log
 * @param categories Bitmask of categories to enable
 */
void logger_set_categories(int categories);

/**
 * @brief Get the current category mask
 * @return Current category bitmask
 */
int logger_get_categories(void);

/**
 * @brief Enable/disable logging to file
 * @param filename File to log to (NULL to disable file logging)
 * @return true on success, false on error
 */
bool logger_set_file(const char* filename);

/**
 * @brief Check if a message should be logged
 * @param level Log level of the message
 * @param category Log category of the message
 * @return true if message should be logged
 */
bool logger_should_log(LogLevel level, LogCategory category);

/**
 * @brief Log a message with specified level and category
 * @param level Log level
 * @param category Log category
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void logger_log(LogLevel level, LogCategory category, const char* format, ...);

/**
 * @brief Cleanup the logging system
 */
void logger_cleanup(void);

// Convenience macros for common logging operations
#define LOG_ERROR(category, ...) logger_log(LOG_LEVEL_ERROR, category, __VA_ARGS__)
#define LOG_WARN(category, ...) logger_log(LOG_LEVEL_WARN, category, __VA_ARGS__)
#define LOG_INFO(category, ...) logger_log(LOG_LEVEL_INFO, category, __VA_ARGS__)
#define LOG_DEBUG(category, ...) logger_log(LOG_LEVEL_DEBUG, category, __VA_ARGS__)

// Specific category macros
#define LOG_LEXER_DEBUG(...) LOG_DEBUG(LOG_CAT_LEXER, __VA_ARGS__)
#define LOG_PARSER_DEBUG(...) LOG_DEBUG(LOG_CAT_PARSER, __VA_ARGS__)
#define LOG_AST_DEBUG(...) LOG_DEBUG(LOG_CAT_AST, __VA_ARGS__)
#define LOG_VISITOR_DEBUG(...) LOG_DEBUG(LOG_CAT_VISITOR, __VA_ARGS__)
#define LOG_MEMORY_DEBUG(...) LOG_DEBUG(LOG_CAT_MEMORY, __VA_ARGS__)
#define LOG_VALUES_DEBUG(...) LOG_DEBUG(LOG_CAT_VALUES, __VA_ARGS__)
#define LOG_STDLIB_DEBUG(...) LOG_DEBUG(LOG_CAT_STDLIB, __VA_ARGS__)

#endif // ZEN_CORE_LOGGER_H