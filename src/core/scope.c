#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include <string.h>


/**
 * @brief Create new scope instance
 * @param void Function takes no parameters
 * @return Pointer to newly allocated scope
 */
scope_T* scope_new()
{
    scope_T* scope = memory_alloc(sizeof(struct SCOPE_STRUCT));
    if (!scope) {
        return NULL;
    }

    scope->function_definitions = (void*) 0;
    scope->function_definitions_size = 0;

    scope->variable_definitions = (void*) 0;
    scope->variable_definitions_size = 0;

    return scope;
}

/**
 * @brief Free a scope instance and its resources
 * @param scope The scope instance to free
 */
void scope_free(scope_T* scope)
{
    if (!scope) {
        return;
    }
    
    if (scope->function_definitions) {
        memory_free(scope->function_definitions);
    }
    
    if (scope->variable_definitions) {
        memory_free(scope->variable_definitions);
    }
    
    memory_free(scope);
}

/**
 * @brief Add function definition to scope
 * @param scope The scope to add to
 * @param fdef The function definition AST node
 * @return The function definition AST node
 */
AST_T* scope_add_function_definition(scope_T* scope, AST_T* fdef)
{
    scope->function_definitions_size += 1;

    if (scope->function_definitions == (void*)0)
    {
        scope->function_definitions = memory_alloc(sizeof(struct AST_STRUCT*));
    }
    else
    {
        scope->function_definitions =
            memory_realloc(
                scope->function_definitions,
                scope->function_definitions_size * sizeof(struct AST_STRUCT**)
            );
    }

    scope->function_definitions[scope->function_definitions_size-1] =
        fdef;

    return fdef;
}

/**
 * @brief Get function definition from scope
 * @param scope The scope to search in
 * @param fname The function name to find
 * @return The function definition AST node or NULL if not found
 */
AST_T* scope_get_function_definition(scope_T* scope, const char* fname)
{
    for (size_t i = 0; i < scope->function_definitions_size; i++)
    {
        AST_T* fdef = scope->function_definitions[i];

        if (strcmp(fdef->function_definition_name, fname) == 0)
        {
            return fdef;
        }
    }

    return (void*)0;
}

/**
 * @brief Add variable definition to scope
 * @param scope The scope to add to
 * @param vdef The variable definition AST node
 * @return The variable definition AST node
 */
AST_T* scope_add_variable_definition(scope_T* scope, AST_T* vdef)
{
    // First check if variable already exists and update it
    for (size_t i = 0; i < scope->variable_definitions_size; i++)
    {
        AST_T* existing = scope->variable_definitions[i];
        if (strcmp(existing->variable_definition_variable_name, vdef->variable_definition_variable_name) == 0)
        {
            // Update existing variable
            scope->variable_definitions[i] = vdef;
            return vdef;
        }
    }
    
    // Variable doesn't exist, add new one
    if (scope->variable_definitions == (void*) 0)
    {
        scope->variable_definitions = memory_alloc(sizeof(struct AST_STRUCT*));
        scope->variable_definitions[0] = vdef;
        scope->variable_definitions_size += 1;
    }
    else
    {
        scope->variable_definitions_size += 1;
        scope->variable_definitions = memory_realloc(
            scope->variable_definitions,
            scope->variable_definitions_size * sizeof(struct AST_STRUCT*)  
        );
        scope->variable_definitions[scope->variable_definitions_size-1] = vdef;
    }

    return vdef;
}

/**
 * @brief Get variable definition from scope
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The variable definition AST node or NULL if not found
 */
AST_T* scope_get_variable_definition(scope_T* scope, const char* name)
{
    for (size_t i = 0; i < scope->variable_definitions_size; i++)
    {
        AST_T* vdef = scope->variable_definitions[i];

        if (strcmp(vdef->variable_definition_variable_name, name) == 0)
        {
            return vdef;
        }
    }

    return (void*)0;
}
