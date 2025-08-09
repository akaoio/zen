/*
 * stdlib_minimal.c
 * Minimal standard library for RuntimeValue system
 * Provides core built-in functions needed for basic ZEN operation
 */

#include "zen/core/runtime_value.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/stdlib.h"

#include <string.h>

// Forward declarations for built-in functions
static RuntimeValue *builtin_print(RuntimeValue **args, size_t argc);

// Built-in function registry
static const ZenStdlibFunction builtin_functions[] = {
    {"print", builtin_print, "Print values to stdout with newline"}, {NULL, NULL, NULL}};

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