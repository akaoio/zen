#include "zen/core/scope.h"
#include "zen/core/ast.h"
#include "zen/core/memory.h"
#include <string.h>
#include <stdio.h>


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

    scope->function_definitions = NULL;
    scope->function_definitions_size = 0;

    scope->variable_definitions = NULL;
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
    
    if (scope->function_definitions != NULL && scope->function_definitions != (void*)0) {
        memory_free(scope->function_definitions);
        scope->function_definitions = NULL;
    }
    
    if (scope->variable_definitions != NULL && scope->variable_definitions != (void*)0) {
        // CRITICAL FIX: Don't free the AST nodes here - they may be referenced elsewhere
        // The AST nodes will be freed when the main AST is freed
        // Just free the array of pointers, not the nodes themselves
        memory_free(scope->variable_definitions);
        scope->variable_definitions = NULL;
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

    if (scope->function_definitions == NULL)
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

    return NULL;
}

/**
 * @brief Add variable definition to scope
 * @param scope The scope to add to
 * @param vdef The variable definition AST node
 * @return The variable definition AST node
 */
AST_T* scope_add_variable_definition(scope_T* scope, AST_T* vdef)
{
    // Validate parameters
    if (!scope || !vdef || !vdef->variable_definition_variable_name) {
        return NULL;
    }
    
    
    // CRITICAL FIX: Don't create copies of AST nodes - just store references
    // The original AST tree owns the memory, scope just references it
    // This prevents double-free issues entirely

    // First check if variable already exists and update it
    for (size_t i = 0; i < scope->variable_definitions_size; i++)
    {
        AST_T* existing = scope->variable_definitions[i];
        if (existing && existing->variable_definition_variable_name && 
            strcmp(existing->variable_definition_variable_name, vdef->variable_definition_variable_name) == 0)
        {
            // Just replace the reference - don't free anything
            scope->variable_definitions[i] = vdef;
            return vdef;
        }
    }
    
    // Variable doesn't exist, add new reference
    if (scope->variable_definitions == NULL)
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
    // Validate parameters
    if (!scope || !name) {
        return NULL;
    }

    // Check if variable_definitions array is NULL
    if (!scope->variable_definitions) {
        return NULL;
    }

    for (size_t i = 0; i < scope->variable_definitions_size; i++)
    {
        // Check if the array element is NULL before dereferencing
        if (scope->variable_definitions[i] == NULL) {
            continue; // Skip NULL entries
        }
        
        AST_T* vdef = scope->variable_definitions[i];

        // Double check the pointer is valid and has a name
        if (vdef->variable_definition_variable_name != NULL && 
            strcmp(vdef->variable_definition_variable_name, name) == 0)
        {
            return vdef;
        }
    }

    return NULL;
}
