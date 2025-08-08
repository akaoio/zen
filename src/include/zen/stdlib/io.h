#ifndef ZEN_STDLIB_IO_H
#define ZEN_STDLIB_IO_H

#include "zen/types/value.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Read entire file to string
 * @param filepath Path to the file to read
 * @return Newly allocated string containing file contents, or NULL on error
 */
char *io_read_file_internal(const char *filepath);

// Additional I/O helper functions for the stdlib (internal use)

/**
 * @brief Print Value to stdout with newline
 * @param value Value to print
 */
void io_print_internal(const Value *value);

/**
 * @brief Print Value to stdout without newline
 * @param value Value to print
 */
void io_print_no_newline_internal(const Value *value);

/**
 * @brief Read a line from stdin
 * @return Newly allocated string containing user input, or NULL on error
 */
char *io_input_internal(void);

/**
 * @brief Read a line from stdin with prompt
 * @param prompt Prompt string to display
 * @return Newly allocated string containing user input, or NULL on error
 */
char *io_input_prompt_internal(const char *prompt);

/**
 * @brief Write string to file
 * @param filepath Path to file to write
 * @param content Content to write
 * @return true on success, false on failure
 */
bool io_write_file_internal(const char *filepath, const char *content);

/**
 * @brief Append string to file
 * @param filepath Path to file to append to
 * @param content Content to append
 * @return true on success, false on failure
 */
bool io_append_file_internal(const char *filepath, const char *content);

/**
 * @brief Check if file exists
 * @param filepath Path to check
 * @return true if file exists, false otherwise
 */
bool io_file_exists_internal(const char *filepath);

/**
 * @brief Load JSON file as Value object
 * @param filepath Path to JSON file
 * @return Value object representing the JSON data, or NULL on error
 */
Value *io_load_json_file_internal(const char *filepath);

/**
 * @brief Resolve module path with extensions
 * @param module_path Base path for the module
 * @return Newly allocated string with resolved path, or NULL if not found
 */
char *io_resolve_module_path_internal(const char *module_path);

#endif
