/*
 * stdlib_minimal.c
 * Minimal standard library for RuntimeValue system
 * Provides core built-in functions needed for basic ZEN operation
 */

#include "zen/core/runtime_value.h"
#include "zen/core/memory.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/stdlib.h"

#include <string.h>

// Forward declarations for built-in functions
static RuntimeValue *builtin_print(RuntimeValue **args, size_t argc);
static RuntimeValue *builtin_file_read(RuntimeValue **args, size_t argc);
static RuntimeValue *builtin_file_write(RuntimeValue **args, size_t argc);

// Built-in function registry
static const ZenStdlibFunction builtin_functions[] = {
    {"print", builtin_print, "Print values to stdout with newline"},
    {"file_read", builtin_file_read, "Read file contents"},
    {"file_write", builtin_file_write, "Write content to file"},
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
    
    bool success = io_write_file_internal(args[0]->data.string.data, 
                                         args[1]->data.string.data);
    
    return rv_new_boolean(success);
}