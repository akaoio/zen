/*
 * stdlib_minimal.c
 * Minimal standard library for RuntimeValue system
 * Provides core built-in functions needed for basic ZEN operation
 */

#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/stdlib.h"

#include <string.h>

// Forward declarations for built-in functions
static RuntimeValue *builtin_print(RuntimeValue **args, size_t argc);
static RuntimeValue *builtin_file_read(RuntimeValue **args, size_t argc);
static RuntimeValue *builtin_file_write(RuntimeValue **args, size_t argc);
static RuntimeValue *builtin_split(RuntimeValue **args, size_t argc);

// Built-in function registry
static const ZenStdlibFunction builtin_functions[] = {
    {"print", builtin_print, "Print values to stdout with newline"},
    {"file_read", builtin_file_read, "Read file contents"},
    {"file_write", builtin_file_write, "Write content to file"},
    {"split", builtin_split, "Split string by delimiter"},
    {NULL, NULL, NULL}};

/**
 * @brief Get a built-in function by name
 * @param name Function name to look up
 * @return Pointer to function info, or NULL if not found
 */
const ZenStdlibFunction *stdlib_get(const char *name)
{
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; builtin_functions[i].name != NULL; i++) {
        if (strcmp(builtin_functions[i].name, name) == 0) {
            return &builtin_functions[i];
        }
    }

    return NULL;
}

/**
 * @brief Print function implementation
 * @param args Array of arguments
 * @param argc Number of arguments
 * @return null value
 */
static RuntimeValue *builtin_print(RuntimeValue **args, size_t argc)
{
    for (size_t i = 0; i < argc; i++) {
        if (i > 0) {
            printf(" ");
        }
        io_print_no_newline_internal(args[i]);
    }
    printf("\n");

    return rv_new_null();
}

/**
 * @brief Read file contents
 * @param args Array of arguments (expects filename)
 * @param argc Number of arguments
 * @return String with file contents or error
 */
static RuntimeValue *builtin_file_read(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("file.read expects 1 argument", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("file.read expects string filename", -1);
    }

    char *content = io_read_file_internal(args[0]->data.string.data);
    if (!content) {
        return rv_new_error("Failed to read file", -1);
    }

    RuntimeValue *result = rv_new_string(content);
    memory_free(content);
    return result;
}

/**
 * @brief Write content to file
 * @param args Array of arguments (expects filename, content)
 * @param argc Number of arguments
 * @return Boolean indicating success
 */
static RuntimeValue *builtin_file_write(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("file.write expects 2 arguments", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("file.write expects string filename", -1);
    }

    if (!args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("file.write expects string content", -1);
    }

    bool success = io_write_file_internal(args[0]->data.string.data, args[1]->data.string.data);

    return rv_new_boolean(success);
}

/**
 * @brief Split string by delimiter
 * @param args Array of arguments (expects string, delimiter)
 * @param argc Number of arguments
 * @return Array of string parts
 */
static RuntimeValue *builtin_split(RuntimeValue **args, size_t argc)
{
    // Debug: Print argument count
    if (argc != 2) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "split expects 2 arguments, got %zu", argc);
        return rv_new_error(error_msg, -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("split expects string as first argument", -1);
    }

    if (!args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("split expects string delimiter as second argument", -1);
    }

    const char *str = args[0]->data.string.data;
    const char *delim = args[1]->data.string.data;

    // Create result array
    RuntimeValue *result = rv_new_array();
    if (!result) {
        return rv_new_error("Failed to create array", -1);
    }

    // Handle empty string
    if (!str || strlen(str) == 0) {
        rv_array_push(result, rv_new_string(""));
        return result;
    }

    // Handle empty delimiter
    if (!delim || strlen(delim) == 0) {
        // Split each character
        for (size_t i = 0; str[i]; i++) {
            char ch[2] = {str[i], '\0'};
            rv_array_push(result, rv_new_string(ch));
        }
        return result;
    }

    // Make a copy of the string to work with
    char *str_copy = memory_strdup(str);
    if (!str_copy) {
        rv_unref(result);
        return rv_new_error("Memory allocation failed", -1);
    }

    // Split the string
    char *current = str_copy;
    char *next;
    size_t delim_len = strlen(delim);

    while ((next = strstr(current, delim)) != NULL) {
        // Found delimiter, extract the part before it
        *next = '\0';
        rv_array_push(result, rv_new_string(current));
        current = next + delim_len;
    }

    // Add the remaining part (or the whole string if no delimiter found)
    rv_array_push(result, rv_new_string(current));

    memory_free(str_copy);
    return result;
}