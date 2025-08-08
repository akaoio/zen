#ifndef SCOPE_H
#define SCOPE_H
#include "zen/core/ast.h"

typedef struct SCOPE_STRUCT
{
    AST_T** function_definitions;
    size_t function_definitions_size;

    AST_T** variable_definitions;
    size_t variable_definitions_size;
} scope_T;

/**
 * @brief Create new scope instance
 * @return Pointer to newly allocated scope
 */
scope_T* scope_new();

/**
 * @brief Free a scope instance and its resources
 * @param scope The scope instance to free
 */
void scope_free(scope_T* scope);

/**
 * @brief Add function definition to scope
 * @param scope The scope to add to
 * @param fdef The function definition AST node
 * @return The function definition AST node
 */
AST_T* scope_add_function_definition(scope_T* scope, AST_T* fdef);

/**
 * @brief Get function definition from scope
 * @param scope The scope to search in
 * @param fname The function name to find
 * @return The function definition AST node or NULL if not found
 */
AST_T* scope_get_function_definition(scope_T* scope, const char* fname);

/**
 * @brief Add variable definition to scope
 * @param scope The scope to add to
 * @param vdef The variable definition AST node
 * @return The variable definition AST node
 */
AST_T* scope_add_variable_definition(scope_T* scope, AST_T* vdef);

/**
 * @brief Get variable definition from scope
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The variable definition AST node or NULL if not found
 */
AST_T* scope_get_variable_definition(scope_T* scope, const char* name);
#endif
