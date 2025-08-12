#include "zen/core/scope.h"

#include "zen/core/ast.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Create new scope instance
 * @param void Function takes no parameters
 * @return Pointer to newly allocated scope
 */
scope_T *scope_new()
{
    scope_T *scope = memory_alloc(sizeof(struct SCOPE_STRUCT));
    if (!scope) {
        return NULL;
    }

    scope->function_definitions = NULL;
    scope->function_definitions_size = 0;

    scope->variable_definitions = NULL;
    scope->variable_definitions_size = 0;

    // Initialize new variables array
    scope->variables = NULL;
    scope->variables_size = 0;

    return scope;
}

/**
 * @brief Free a scope instance and its resources
 * @param scope The scope instance to free
 */
void scope_free(scope_T *scope)
{
    if (!scope) {
        return;
    }

    if (scope->function_definitions != NULL && scope->function_definitions != (void *)0) {
        // CRITICAL FIX: Don't free the AST nodes here - they are owned by the main AST tree
        // Function definition nodes will be freed when ast_free() is called on the root
        // Only free the array of pointers, never the nodes themselves to prevent double-free
        memory_free(scope->function_definitions);
        scope->function_definitions = NULL;
    }

    if (scope->variable_definitions != NULL && scope->variable_definitions != (void *)0) {
        // CRITICAL FIX: Don't free the AST nodes here - they are owned by the main AST tree
        // The AST nodes will be freed when ast_free() is called on the root
        // Only free the array of pointers, never the nodes themselves to prevent double-free
        memory_free(scope->variable_definitions);
        scope->variable_definitions = NULL;
    }

    // Free new variables array with proper cleanup
    if (scope->variables != NULL) {
        for (size_t i = 0; i < scope->variables_size; i++) {
            if (scope->variables[i]) {
                // Free variable name
                if (scope->variables[i]->name) {
                    memory_free(scope->variables[i]->name);
                }
                // Unref the RuntimeValue
                if (scope->variables[i]->value) {
                    rv_unref(scope->variables[i]->value);
                }
                // Free the variable struct
                memory_free(scope->variables[i]);
            }
        }
        memory_free(scope->variables);
        scope->variables = NULL;
    }

    memory_free(scope);
}

/**
 * @brief Add function definition to scope
 * @param scope The scope to add to
 * @param fdef The function definition AST node
 * @return The function definition AST node
 */
AST_T *scope_add_function_definition(scope_T *scope, AST_T *fdef)
{
    // Check if function already exists in scope to avoid duplicates
    if (fdef && fdef->function_definition_name) {
        for (size_t i = 0; i < scope->function_definitions_size; i++) {
            AST_T *existing = scope->function_definitions[i];
            if (existing && existing->function_definition_name &&
                strcmp(existing->function_definition_name, fdef->function_definition_name) == 0) {
                // Function already exists - just return the existing one
                return existing;
            }
        }
    }

    // Function doesn't exist, add it
    scope->function_definitions_size += 1;

    if (scope->function_definitions == NULL) {
        scope->function_definitions = memory_alloc(sizeof(struct AST_STRUCT *));
    } else {
        scope->function_definitions =
            memory_realloc(scope->function_definitions,
                           scope->function_definitions_size * sizeof(struct AST_STRUCT **));
    }

    scope->function_definitions[scope->function_definitions_size - 1] = fdef;

    return fdef;
}

/**
 * @brief Get function definition from scope
 * @param scope The scope to search in
 * @param fname The function name to find
 * @return The function definition AST node or NULL if not found
 */
AST_T *scope_get_function_definition(scope_T *scope, const char *fname)
{
    // Add NULL checks to prevent segfault
    if (!scope || !fname || !scope->function_definitions) {
        return NULL;
    }

    for (size_t i = 0; i < scope->function_definitions_size; i++) {
        AST_T *fdef = scope->function_definitions[i];

        // Check that fdef and its name are valid before comparing
        if (fdef && fdef->function_definition_name &&
            strcmp(fdef->function_definition_name, fname) == 0) {
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
AST_T *scope_add_variable_definition(scope_T *scope, AST_T *vdef)
{
    // Validate parameters
    if (!scope || !vdef || !vdef->variable_definition_variable_name) {
        return NULL;
    }

    // CRITICAL FIX: Don't create copies of AST nodes - just store references
    // The original AST tree owns the memory, scope just references it
    // This prevents double-free issues entirely

    // First check if variable already exists and update it
    for (size_t i = 0; i < scope->variable_definitions_size; i++) {
        AST_T *existing = scope->variable_definitions[i];
        if (existing && existing->variable_definition_variable_name &&
            strcmp(existing->variable_definition_variable_name,
                   vdef->variable_definition_variable_name) == 0) {
            // CRITICAL FIX: Always update the reference, even for same node
            // This ensures that variable values are properly updated in loops

            // IMPORTANT: When replacing a variable definition, we need to preserve
            // any runtime_value from the old definition if the new one doesn't have one yet
            // This prevents "null" values during variable updates like "set x x + 5"
            if (existing->runtime_value && !vdef->runtime_value) {
                vdef->runtime_value = rv_ref((RuntimeValue *)existing->runtime_value);
            }

            scope->variable_definitions[i] = vdef;
            return vdef;
        }
    }

    // Variable doesn't exist, add new reference
    if (scope->variable_definitions == NULL) {
        scope->variable_definitions = memory_alloc(sizeof(struct AST_STRUCT *));
        scope->variable_definitions[0] = vdef;
        scope->variable_definitions_size += 1;
    } else {
        scope->variable_definitions_size += 1;
        scope->variable_definitions =
            memory_realloc(scope->variable_definitions,
                           scope->variable_definitions_size * sizeof(struct AST_STRUCT *));
        scope->variable_definitions[scope->variable_definitions_size - 1] = vdef;
    }

    return vdef;
}

/**
 * @brief Get variable definition from scope
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The variable definition AST node or NULL if not found
 */
AST_T *scope_get_variable_definition(scope_T *scope, const char *name)
{
    // Validate parameters
    if (!scope || !name) {
        return NULL;
    }

    // Check if variable_definitions array is NULL
    if (!scope->variable_definitions) {
        return NULL;
    }

    for (size_t i = 0; i < scope->variable_definitions_size; i++) {
        // Check if the array element is NULL before dereferencing
        if (scope->variable_definitions[i] == NULL) {
            continue;  // Skip NULL entries
        }

        AST_T *vdef = scope->variable_definitions[i];

        // Double check the pointer is valid and has a name
        if (vdef->variable_definition_variable_name != NULL) {
            if (strcmp(vdef->variable_definition_variable_name, name) == 0) {
                return vdef;
            }
        }
    }

    return NULL;
}

/**
 * @brief Set variable value in scope (new proper implementation)
 * @param scope The scope to add to
 * @param name The variable name
 * @param value The RuntimeValue to store (will be referenced)
 * @return 1 on success, 0 on failure
 */
int scope_set_variable(scope_T *scope, const char *name, RuntimeValue *value)
{
    if (!scope || !name || !value) {
        return 0;
    }

    // Check if variable already exists
    for (size_t i = 0; i < scope->variables_size; i++) {
        if (scope->variables[i] && scope->variables[i]->name &&
            strcmp(scope->variables[i]->name, name) == 0) {
            // Update existing variable
            if (scope->variables[i]->value) {
                rv_unref(scope->variables[i]->value);
            }
            scope->variables[i]->value = rv_ref(value);
            return 1;
        }
    }

    // Add new variable
    scope_variable_T *new_var = memory_alloc(sizeof(scope_variable_T));
    if (!new_var) {
        return 0;
    }

    new_var->name = memory_alloc(strlen(name) + 1);
    if (!new_var->name) {
        memory_free(new_var);
        return 0;
    }
    strcpy(new_var->name, name);
    new_var->value = rv_ref(value);

    // Expand variables array
    if (scope->variables == NULL) {
        scope->variables = memory_alloc(sizeof(scope_variable_T *));
        scope->variables[0] = new_var;
        scope->variables_size = 1;
    } else {
        scope->variables_size++;
        scope->variables =
            memory_realloc(scope->variables, scope->variables_size * sizeof(scope_variable_T *));
        scope->variables[scope->variables_size - 1] = new_var;
    }

    return 1;
}

/**
 * @brief Get variable value from scope (new proper implementation)
 * @param scope The scope to search in
 * @param name The variable name to find
 * @return The RuntimeValue (referenced) or NULL if not found
 */
RuntimeValue *scope_get_variable(scope_T *scope, const char *name)
{
    if (!scope || !name) {
        return NULL;
    }

    // Search in new variables array
    for (size_t i = 0; i < scope->variables_size; i++) {
        if (scope->variables[i] && scope->variables[i]->name &&
            strcmp(scope->variables[i]->name, name) == 0) {
            if (scope->variables[i]->value) {
                return rv_ref(scope->variables[i]->value);
            }
        }
    }

    return NULL;
}
