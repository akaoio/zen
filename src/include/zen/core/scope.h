#ifndef SCOPE_H
#define SCOPE_H
#include "zen/core/ast.h"
#include "zen/core/runtime_value.h"

typedef struct SCOPE_VARIABLE {
    char *name;
    RuntimeValue *value;
} scope_variable_T;

typedef struct SCOPE_STRUCT {
    AST_T **function_definitions;
    size_t function_definitions_size;

    // DEPRECATED: Direct AST storage causes memory issues
    AST_T **variable_definitions;
    size_t variable_definitions_size;
    
    // NEW: Proper variable storage with RuntimeValues
    scope_variable_T **variables;
    size_t variables_size;
} scope_T;

/**
 * @brief Create new scope instance
 * @return Pointer to newly allocated scope
 */
scope_T *scope_new();

/**
 * @brief Free a scope instance and its resources
 * @param scope The scope instance to free
 */
void scope_free(scope_T *scope);

/**
 * @brief Add function definition to scope
 * @param scope The scope to add to
 * @param fdef The function definition AST node
 * @return The function definition AST node
 */
AST_T *scope_add_function_definition(scope_T *scope, AST_T *fdef);

/**
 * @brief Get function definition from scope
 * @param scope The scope to search in
 * @param fname The function name to find
 * @return The function definition AST node or NULL if not found
 */
AST_T *scope_get_function_definition(scope_T *scope, const char *fname);

/**
 * @brief Add variable definition to scope
 * @param scope The scope to add to
 * @param vdef The variable definition AST node
 * @return The variable definition AST node
 */
AST_T *scope_add_variable_definition(scope_T *scope, AST_T *vdef);

/**
 * @brief Get variable definition from scope
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The variable definition AST node or NULL if not found
 */
AST_T *scope_get_variable_definition(scope_T *scope, const char *name);

/**
 * @brief Set variable value in scope (new proper implementation)
 * @param scope The scope to add to
 * @param name The variable name
 * @param value The RuntimeValue to store (will be referenced)
 * @return 1 on success, 0 on failure
 */
int scope_set_variable(scope_T *scope, const char *name, RuntimeValue *value);

/**
 * @brief Get variable value from scope (new proper implementation)
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The RuntimeValue (referenced) or NULL if not found
 */
RuntimeValue *scope_get_variable(scope_T *scope, const char *name);

#endif
