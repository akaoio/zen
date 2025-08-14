#include "zen/core/ast.h"

#include "zen/core/ast_memory_pool.h"
#include "zen/core/memory.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Create new AST node
 * @param type The AST node type
 * @return Pointer to newly allocated AST node
 */
AST_T *ast_new(int type)
{
    // TEMPORARY FIX: Disable pooling entirely to fix import memory corruption
    // The AST pool is global and causes issues when modules are imported
    // as their AST nodes get mixed with the main program's nodes in the pool
    // TODO: Implement per-parser-context pools or reference counting
    #if 1  // Set to 0 to re-enable pooling
        AST_T *ast = memory_alloc(sizeof(AST_T));
        if (!ast) {
            return NULL;  // Allocation failed
        }
        memset(ast, 0, sizeof(AST_T));
        ast->type = type;
        ast->pooled = false;  // Mark as not pooled
    #else
    // Try to allocate from memory pool first for better performance
    AST_T *ast = ast_pool_alloc_node(type);
    if (!ast) {
        // Fallback to direct allocation if pool fails
        ast = memory_alloc(sizeof(AST_T));
        if (!ast) {
            return NULL;  // Allocation failed
        }
        memset(ast, 0, sizeof(AST_T));
        ast->type = type;
        ast->pooled = false;  // Mark as not pooled
    }
    #endif

    ast->scope = (void *)0;

    /* AST_VARIABLE_DEFINITION */
    ast->variable_definition_variable_name = (void *)0;
    ast->variable_definition_value = (void *)0;

    /* AST_FUNCTION_DEFINITION */
    ast->function_definition_body = (void *)0;
    ast->function_definition_name = (void *)0;
    ast->function_definition_args = (void *)0;
    ast->function_definition_args_size = 0;

    /* AST_VARIABLE */
    ast->variable_name = (void *)0;

    /* AST_FUNCTION_CALL */
    ast->function_call_name = (void *)0;
    ast->function_call_arguments = (void *)0;
    ast->function_call_arguments_size = 0;

    /* AST_STRING */
    ast->string_value = (void *)0;

    /* AST_NUMBER */
    ast->number_value = 0.0;

    /* AST_BOOLEAN */
    ast->boolean_value = 0;

    /* AST_ARRAY */
    ast->array_elements = (void *)0;
    ast->array_size = 0;

    /* AST_OBJECT */
    ast->object_keys = (void *)0;
    ast->object_values = (void *)0;
    ast->object_size = 0;

    /* AST_BINARY_OP */
    ast->operator_type = 0;
    ast->left = (void *)0;
    ast->right = (void *)0;

    /* AST_UNARY_OP */
    ast->operand = (void *)0;

    /* AST_IF_STATEMENT */
    ast->condition = (void *)0;
    ast->then_branch = (void *)0;
    ast->else_branch = (void *)0;

    /* AST_WHILE_LOOP */
    ast->loop_condition = (void *)0;
    ast->loop_body = (void *)0;

    /* AST_FOR_LOOP */
    ast->iterator_variable = (void *)0;
    ast->iterable = (void *)0;
    ast->for_body = (void *)0;

    /* AST_RETURN */
    ast->return_value = (void *)0;

    /* AST_CLASS_DEFINITION */
    ast->class_name = (void *)0;
    ast->parent_class = (void *)0;
    ast->class_methods = (void *)0;
    ast->class_methods_size = 0;

    /* AST_PROPERTY_ACCESS */
    ast->object = (void *)0;
    ast->property_name = (void *)0;

    /* AST_IMPORT */
    ast->import_path = (void *)0;
    ast->import_names = (void *)0;
    ast->import_names_size = 0;

    /* AST_EXPORT */
    ast->export_name = (void *)0;
    ast->export_value = (void *)0;

    /* AST_TRY_CATCH */
    ast->try_block = (void *)0;
    ast->catch_block = (void *)0;
    ast->exception_variable = (void *)0;

    /* AST_THROW */
    ast->exception_value = (void *)0;

    /* AST_COMPOUND */
    ast->compound_statements = (void *)0;
    ast->compound_size = 0;

    /* LOGICAL AST NODES - Initialize all logical fields */
    ast->quantifier_type = 0;
    ast->quantified_variable = (void *)0;
    ast->quantified_domain = (void *)0;
    ast->quantified_body = (void *)0;

    ast->predicate_name = (void *)0;
    ast->predicate_args = (void *)0;
    ast->predicate_args_size = 0;

    ast->connective_type = 0;
    ast->connective_left = (void *)0;
    ast->connective_right = (void *)0;

    ast->logical_var_name = (void *)0;
    ast->is_bound = false;
    ast->binding_quantifier = (void *)0;

    ast->proposition_name = (void *)0;
    ast->proposition_value = false;

    ast->theorem_name = (void *)0;
    ast->theorem_hypotheses = (void *)0;
    ast->theorem_hypotheses_size = 0;
    ast->theorem_conclusion = (void *)0;
    ast->theorem_proof = (void *)0;

    ast->axiom_name = (void *)0;
    ast->axiom_statement = (void *)0;
    ast->axiom_is_consistent = true;

    ast->step_statement = (void *)0;
    ast->step_justification = (void *)0;
    ast->step_premises = (void *)0;
    ast->step_premises_size = 0;

    ast->inference_type = 0;
    ast->inference_premises = (void *)0;
    ast->inference_premises_size = 0;
    ast->inference_conclusion = (void *)0;

    ast->equation_left = (void *)0;
    ast->equation_right = (void *)0;

    ast->inequality_type = 0;
    ast->inequality_left = (void *)0;
    ast->inequality_right = (void *)0;

    ast->math_function_name = (void *)0;
    ast->math_function_args = (void *)0;
    ast->math_function_args_size = 0;

    /* AST_FILE_GET */
    ast->file_get_path = (void *)0;
    ast->file_get_property = (void *)0;

    /* AST_FILE_PUT */
    ast->file_put_path = (void *)0;
    ast->file_put_property = (void *)0;
    ast->file_put_value = (void *)0;

    return ast;
}

/**
 * @brief Create a new number AST node
 * @param value The numeric value
 * @return Pointer to newly allocated AST node of type AST_NUMBER
 */
AST_T *ast_new_number(double value)
{
    AST_T *ast = ast_new(AST_NUMBER);
    ast->number_value = value;
    return ast;
}

/**
 * @brief Create a new boolean AST node
 * @param value The boolean value (0 for false, non-zero for true)
 * @return Pointer to newly allocated AST node of type AST_BOOLEAN
 */
AST_T *ast_new_boolean(int value)
{
    AST_T *ast = ast_new(AST_BOOLEAN);
    ast->boolean_value = value;
    return ast;
}

/**
 * @brief Create a new null AST node
 * @return Pointer to newly allocated AST node of type AST_NULL
 */
AST_T *ast_new_null(void) { return ast_new(AST_NULL); }

/**
 * @brief Create a new undecidable AST node
 * @return Pointer to newly allocated AST node of type AST_UNDECIDABLE
 */
AST_T *ast_new_undecidable(void) { return ast_new(AST_UNDECIDABLE); }

/**
 * @brief Create a new string AST node
 * @param value The string value (will be copied)
 * @return Pointer to newly allocated AST node of type AST_STRING
 */
AST_T *ast_new_string(const char *value)
{
    AST_T *ast = ast_new(AST_STRING);
    if (value) {
        ast->string_value = memory_strdup(value);
    }
    return ast;
}

/**
 * @brief Create a new binary operation AST node
 * @param operator_type The operator type (from token types)
 * @param left Left operand AST node
 * @param right Right operand AST node
 * @return Pointer to newly allocated AST node of type AST_BINARY_OP
 */
AST_T *ast_new_binary_op(int operator_type, AST_T *left, AST_T *right)
{
    AST_T *ast = ast_new(AST_BINARY_OP);
    ast->operator_type = operator_type;
    ast->left = left;
    ast->right = right;
    return ast;
}

/**
 * @brief Create a new unary operation AST node
 * @param operator_type The operator type (from token types)
 * @param operand The operand AST node
 * @return Pointer to newly allocated AST node of type AST_UNARY_OP
 */
AST_T *ast_new_unary_op(int operator_type, AST_T *operand)
{
    AST_T *ast = ast_new(AST_UNARY_OP);
    ast->operator_type = operator_type;
    ast->operand = operand;
    return ast;
}

/**
 * @brief Create a new array AST node
 * @param elements Array of AST nodes representing array elements
 * @param size Number of elements in the array
 * @return Pointer to newly allocated AST node of type AST_ARRAY
 */
AST_T *ast_new_array(AST_T **elements, size_t size)
{
    AST_T *ast = ast_new(AST_ARRAY);
    ast->array_size = size;
    if (size > 0 && elements) {
        ast->array_elements = memory_alloc(size * sizeof(AST_T *));
        for (size_t i = 0; i < size; i++) {
            ast->array_elements[i] = elements[i];
        }
    }
    return ast;
}

/**
 * @brief Create a new object AST node
 * @param keys Array of string keys
 * @param values Array of AST nodes representing values
 * @param size Number of key-value pairs
 * @return Pointer to newly allocated AST node of type AST_OBJECT
 */
AST_T *ast_new_object(char **keys, AST_T **values, size_t size)
{
    AST_T *ast = ast_new(AST_OBJECT);
    ast->object_size = size;
    if (size > 0 && keys && values) {
        ast->object_keys = memory_alloc(size * sizeof(char *));
        ast->object_values = memory_alloc(size * sizeof(AST_T *));
        for (size_t i = 0; i < size; i++) {
            // Copy keys
            if (keys[i]) {
                ast->object_keys[i] = memory_strdup(keys[i]);
            } else {
                ast->object_keys[i] = NULL;
            }
            ast->object_values[i] = values[i];
        }
    }
    return ast;
}

/**
 * @brief Free an AST node and all its children recursively
 * @param ast The AST node to free
 */
void ast_free(AST_T *ast)
{
    if (!ast)
        return;

    // Prevent double-free by checking if already freed
    if (ast->type < 0) {
        return;  // Already freed
    }

    // Free string values
    if (ast->string_value)
        memory_free(ast->string_value);
    if (ast->variable_definition_variable_name)
        memory_free(ast->variable_definition_variable_name);
    if (ast->function_definition_name)
        memory_free(ast->function_definition_name);
    if (ast->variable_name)
        memory_free(ast->variable_name);
    if (ast->function_call_name)
        memory_free(ast->function_call_name);
    if (ast->iterator_variable)
        memory_free(ast->iterator_variable);
    if (ast->class_name)
        memory_free(ast->class_name);
    if (ast->parent_class)
        memory_free(ast->parent_class);
    if (ast->new_class_name)
        memory_free(ast->new_class_name);
    if (ast->property_name)
        memory_free(ast->property_name);
    if (ast->import_path)
        memory_free(ast->import_path);
    if (ast->exception_variable)
        memory_free(ast->exception_variable);
    if (ast->file_ref_target_file)
        memory_free(ast->file_ref_target_file);
    if (ast->file_ref_property_path)
        memory_free(ast->file_ref_property_path);
    if (ast->export_name)
        memory_free(ast->export_name);

    // Free object keys
    if (ast->object_keys) {
        for (size_t i = 0; i < ast->object_size; i++) {
            if (ast->object_keys[i])
                memory_free(ast->object_keys[i]);
        }
        memory_free(ast->object_keys);
    }

    // Free import names
    if (ast->import_names) {
        for (size_t i = 0; i < ast->import_names_size; i++) {
            if (ast->import_names[i])
                memory_free(ast->import_names[i]);
        }
        memory_free(ast->import_names);
    }

    // Recursively free child AST nodes
    if (ast->variable_definition_value)
        ast_free(ast->variable_definition_value);
    if (ast->function_definition_body)
        ast_free(ast->function_definition_body);
    if (ast->left)
        ast_free(ast->left);
    if (ast->right)
        ast_free(ast->right);
    if (ast->operand)
        ast_free(ast->operand);
    if (ast->condition)
        ast_free(ast->condition);
    if (ast->then_branch)
        ast_free(ast->then_branch);
    if (ast->else_branch)
        ast_free(ast->else_branch);
    if (ast->loop_condition)
        ast_free(ast->loop_condition);
    if (ast->loop_body)
        ast_free(ast->loop_body);
    if (ast->iterable)
        ast_free(ast->iterable);
    if (ast->for_body)
        ast_free(ast->for_body);
    if (ast->return_value)
        ast_free(ast->return_value);
    if (ast->object)
        ast_free(ast->object);
    if (ast->try_block)
        ast_free(ast->try_block);
    if (ast->catch_block)
        ast_free(ast->catch_block);
    if (ast->exception_value)
        ast_free(ast->exception_value);
    if (ast->lambda_body)
        ast_free(ast->lambda_body);
    if (ast->export_value)
        ast_free(ast->export_value);
    
    // Free FILE_GET and FILE_PUT nodes
    if (ast->file_get_path)
        ast_free(ast->file_get_path);
    if (ast->file_get_property)
        ast_free(ast->file_get_property);
    if (ast->file_put_path)
        ast_free(ast->file_put_path);
    if (ast->file_put_property)
        ast_free(ast->file_put_property);
    if (ast->file_put_value)
        ast_free(ast->file_put_value);

    // Free arrays of AST nodes
    if (ast->function_definition_args) {
        for (size_t i = 0; i < ast->function_definition_args_size; i++) {
            if (ast->function_definition_args[i])
                ast_free(ast->function_definition_args[i]);
        }
        memory_free(ast->function_definition_args);
    }

    if (ast->function_call_arguments) {
        for (size_t i = 0; i < ast->function_call_arguments_size; i++) {
            if (ast->function_call_arguments[i])
                ast_free(ast->function_call_arguments[i]);
        }
        memory_free(ast->function_call_arguments);
    }

    if (ast->new_arguments) {
        for (size_t i = 0; i < ast->new_arguments_size; i++) {
            if (ast->new_arguments[i])
                ast_free(ast->new_arguments[i]);
        }
        memory_free(ast->new_arguments);
    }

    if (ast->array_elements) {
        for (size_t i = 0; i < ast->array_size; i++) {
            if (ast->array_elements[i])
                ast_free(ast->array_elements[i]);
        }
        memory_free(ast->array_elements);
    }

    if (ast->object_values) {
        for (size_t i = 0; i < ast->object_size; i++) {
            if (ast->object_values[i])
                ast_free(ast->object_values[i]);
        }
        memory_free(ast->object_values);
    }

    if (ast->compound_statements) {
        for (size_t i = 0; i < ast->compound_size; i++) {
            if (ast->compound_statements[i])
                ast_free(ast->compound_statements[i]);
        }
        memory_free(ast->compound_statements);
    }

    if (ast->class_methods) {
        // CRITICAL: Don't free method AST nodes - they may be referenced by RuntimeValues
        // This is a temporary fix until proper AST lifecycle management is implemented
        // TODO: Implement reference counting or deep copying for AST nodes used in functions
        /*
        for (size_t i = 0; i < ast->class_methods_size; i++) {
            if (ast->class_methods[i])
                ast_free(ast->class_methods[i]);
        }
        */
        memory_free(ast->class_methods);
    }

    if (ast->lambda_args) {
        for (size_t i = 0; i < ast->lambda_args_size; i++) {
            if (ast->lambda_args[i])
                ast_free(ast->lambda_args[i]);
        }
        memory_free(ast->lambda_args);
    }

    // Free runtime value cache if present
    if (ast->runtime_value) {
        // Need to include runtime_value.h to use rv_unref
        void rv_unref(void *);  // Forward declaration
        rv_unref(ast->runtime_value);
    }

    // Mark as freed to help detect use-after-free
    ast->type = -1;

    // Free the AST node itself (use pool if it was pool-allocated)
    if (ast->pooled) {
        ast_pool_free_node(ast);
    } else {
        memory_free(ast);
    }
}

/* ============================================================================
 * CONVENIENCE FUNCTIONS FOR COMMON AST PATTERNS
 * ============================================================================ */

/**
 * @brief Create a new compound AST node with multiple statements
 * @param statements Array of AST statement nodes
 * @param count Number of statements
 * @return Pointer to newly allocated compound AST node
 */
AST_T *ast_new_compound(AST_T **statements, size_t count)
{
    AST_T *ast = ast_new(AST_COMPOUND);
    ast->compound_size = count;
    if (count > 0 && statements) {
        ast->compound_statements = memory_alloc(count * sizeof(AST_T *));
        for (size_t i = 0; i < count; i++) {
            ast->compound_statements[i] = statements[i];
        }
    }
    return ast;
}

/**
 * @brief Create a new spread operator AST node for rest parameters
 * @param expression The expression to spread (typically a variable name)
 * @return Pointer to newly allocated AST node of type AST_SPREAD
 */
AST_T *ast_new_spread(AST_T *expression)
{
    AST_T *ast = ast_new(AST_SPREAD);
    ast->spread_expression = expression;
    return ast;
}

/**
 * @brief Create a new variable definition AST node
 * @param name Variable name
 * @param value Initial value AST node
 * @return Pointer to newly allocated variable definition AST node
 */
AST_T *ast_new_variable_definition(const char *name, AST_T *value)
{
    AST_T *ast = ast_new(AST_VARIABLE_DEFINITION);
    if (name) {
        ast->variable_definition_variable_name = memory_strdup(name);
    }
    ast->variable_definition_value = value;
    return ast;
}

/**
 * @brief Create a new variable reference AST node
 * @param name Variable name
 * @return Pointer to newly allocated variable AST node
 */
AST_T *ast_new_variable(const char *name)
{
    AST_T *ast = ast_new(AST_VARIABLE);
    if (name) {
        ast->variable_name = memory_strdup(name);
    }
    return ast;
}

/**
 * @brief Create a new function call AST node
 * @param name Function name
 * @param args Array of argument AST nodes
 * @param arg_count Number of arguments
 * @return Pointer to newly allocated function call AST node
 */
AST_T *ast_new_function_call(const char *name, AST_T **args, size_t arg_count)
{
    AST_T *ast = ast_new(AST_FUNCTION_CALL);
    if (name) {
        ast->function_call_name = memory_strdup(name);
    }
    ast->function_call_arguments_size = arg_count;
    if (arg_count > 0 && args) {
        ast->function_call_arguments = memory_alloc(arg_count * sizeof(AST_T *));
        for (size_t i = 0; i < arg_count; i++) {
            ast->function_call_arguments[i] = args[i];
        }
    }
    return ast;
}

/**
 * @brief Create a new function definition AST node
 * @param name Function name
 * @param args Array of parameter AST nodes
 * @param arg_count Number of parameters
 * @param body Function body AST node
 * @return Pointer to newly allocated function definition AST node
 */
AST_T *ast_new_function_definition(const char *name, AST_T **args, size_t arg_count, AST_T *body)
{
    AST_T *ast = ast_new(AST_FUNCTION_DEFINITION);
    if (name) {
        ast->function_definition_name = memory_strdup(name);
    }
    ast->function_definition_args_size = arg_count;
    if (arg_count > 0 && args) {
        ast->function_definition_args = memory_alloc(arg_count * sizeof(AST_T *));
        for (size_t i = 0; i < arg_count; i++) {
            ast->function_definition_args[i] = args[i];
        }
    }
    ast->function_definition_body = body;
    return ast;
}

/**
 * @brief Create a new if statement AST node
 * @param condition Condition AST node
 * @param then_branch Then branch AST node
 * @param else_branch Else branch AST node (can be NULL)
 * @return Pointer to newly allocated if statement AST node
 */
AST_T *ast_new_if_statement(AST_T *condition, AST_T *then_branch, AST_T *else_branch)
{
    AST_T *ast = ast_new(AST_IF_STATEMENT);
    ast->condition = condition;
    ast->then_branch = then_branch;
    ast->else_branch = else_branch;
    return ast;
}

/**
 * @brief Create a new while loop AST node
 * @param condition Loop condition AST node
 * @param body Loop body AST node
 * @return Pointer to newly allocated while loop AST node
 */
AST_T *ast_new_while_loop(AST_T *condition, AST_T *body)
{
    AST_T *ast = ast_new(AST_WHILE_LOOP);
    ast->loop_condition = condition;
    ast->loop_body = body;
    return ast;
}

/**
 * @brief Create a new for loop AST node
 * @param iterator Iterator variable name
 * @param iterable Iterable expression AST node
 * @param body Loop body AST node
 * @return Pointer to newly allocated for loop AST node
 */
AST_T *ast_new_for_loop(const char *iterator, AST_T *iterable, AST_T *body)
{
    AST_T *ast = ast_new(AST_FOR_LOOP);
    if (iterator) {
        ast->iterator_variable = memory_strdup(iterator);
    }
    ast->iterable = iterable;
    ast->for_body = body;
    return ast;
}

/**
 * @brief Create a new return statement AST node
 * @param value Return value AST node (can be NULL for empty return)
 * @return Pointer to newly allocated return statement AST node
 */
AST_T *ast_new_return_statement(AST_T *value)
{
    AST_T *ast = ast_new(AST_RETURN);
    ast->return_value = value;
    return ast;
}

/**
 * @brief Create a new property access AST node
 * @param object Object AST node
 * @param property Property name
 * @return Pointer to newly allocated property access AST node
 */
AST_T *ast_new_property_access(AST_T *object, const char *property)
{
    AST_T *ast = ast_new(AST_PROPERTY_ACCESS);
    ast->object = object;
    if (property) {
        ast->property_name = memory_strdup(property);
    }
    return ast;
}

/**
 * @brief Create a new assignment AST node
 * @param left Left-hand side AST node (variable or property access)
 * @param right Right-hand side value AST node
 * @return Pointer to newly allocated assignment AST node
 */
AST_T *ast_new_assignment(AST_T *left, AST_T *right)
{
    AST_T *ast = ast_new(AST_ASSIGNMENT);
    ast->left = left;
    ast->right = right;
    return ast;
}

/**
 * @brief Create a new NOOP AST node (no operation)
 * @return Pointer to newly allocated NOOP AST node
 */
AST_T *ast_new_noop(void) { return ast_new(AST_NOOP); }

/* ============================================================================
 * FORMAL LOGIC AST CONSTRUCTOR FUNCTIONS - PURE AST ONLY!
 * ============================================================================ */

/**
 * @brief Create a new logical quantifier AST node
 * @param type QUANTIFIER_UNIVERSAL or QUANTIFIER_EXISTENTIAL
 * @param variable Name of quantified variable
 * @param domain Domain of quantification (can be NULL)
 * @param body Body of quantified expression
 * @return Pointer to AST_LOGICAL_QUANTIFIER node
 */
AST_T *ast_new_logical_quantifier(int type, const char *variable, AST_T *domain, AST_T *body)
{
    AST_T *ast = ast_new(AST_LOGICAL_QUANTIFIER);
    ast->quantifier_type = type;
    if (variable) {
        ast->quantified_variable = memory_strdup(variable);
    }
    ast->quantified_domain = domain;
    ast->quantified_body = body;
    return ast;
}

/**
 * @brief Create a new logical predicate AST node
 * @param name Name of predicate (P, Q, etc.)
 * @param args Array of argument ASTs
 * @param args_size Number of arguments
 * @return Pointer to AST_LOGICAL_PREDICATE node
 */
AST_T *ast_new_logical_predicate(const char *name, AST_T **args, size_t args_size)
{
    AST_T *ast = ast_new(AST_LOGICAL_PREDICATE);
    if (name) {
        ast->predicate_name = memory_strdup(name);
    }
    ast->predicate_args_size = args_size;
    if (args_size > 0 && args) {
        ast->predicate_args = memory_alloc(args_size * sizeof(AST_T *));
        for (size_t i = 0; i < args_size; i++) {
            ast->predicate_args[i] = args[i];
        }
    }
    return ast;
}

/**
 * @brief Create a new logical connective AST node
 * @param type CONNECTIVE_AND, CONNECTIVE_OR, CONNECTIVE_IMPLIES, CONNECTIVE_IFF, CONNECTIVE_NOT
 * @param left Left operand (NULL for NOT)
 * @param right Right operand
 * @return Pointer to AST_LOGICAL_CONNECTIVE node
 */
AST_T *ast_new_logical_connective(int type, AST_T *left, AST_T *right)
{
    AST_T *ast = ast_new(AST_LOGICAL_CONNECTIVE);
    ast->connective_type = type;
    ast->connective_left = left;
    ast->connective_right = right;
    return ast;
}

/**
 * @brief Create a new logical variable AST node
 * @param name Variable name
 * @param is_bound Whether variable is bound by quantifier
 * @return Pointer to AST_LOGICAL_VARIABLE node
 */
AST_T *ast_new_logical_variable(const char *name, bool is_bound)
{
    AST_T *ast = ast_new(AST_LOGICAL_VARIABLE);
    if (name) {
        ast->logical_var_name = memory_strdup(name);
    }
    ast->is_bound = is_bound;
    return ast;
}

/**
 * @brief Create a new logical proposition AST node
 * @param name Proposition name
 * @return Pointer to AST_LOGICAL_PROPOSITION node
 */
AST_T *ast_new_logical_proposition(const char *name)
{
    AST_T *ast = ast_new(AST_LOGICAL_PROPOSITION);
    if (name) {
        ast->proposition_name = memory_strdup(name);
    }
    return ast;
}

/**
 * @brief Create a new mathematical equation AST node
 * @param left Left side expression
 * @param right Right side expression
 * @return Pointer to AST_MATHEMATICAL_EQUATION node
 */
AST_T *ast_new_mathematical_equation(AST_T *left, AST_T *right)
{
    AST_T *ast = ast_new(AST_MATHEMATICAL_EQUATION);
    ast->equation_left = left;
    ast->equation_right = right;
    return ast;
}

/**
 * @brief Create a new mathematical inequality AST node
 * @param type INEQUALITY_LT, INEQUALITY_LE, INEQUALITY_GT, INEQUALITY_GE
 * @param left Left side expression
 * @param right Right side expression
 * @return Pointer to AST_MATHEMATICAL_INEQUALITY node
 */
AST_T *ast_new_mathematical_inequality(int type, AST_T *left, AST_T *right)
{
    AST_T *ast = ast_new(AST_MATHEMATICAL_INEQUALITY);
    ast->inequality_type = type;
    ast->inequality_left = left;
    ast->inequality_right = right;
    return ast;
}

/**
 * @brief Create a new mathematical function AST node
 * @param name Function name
 * @param args Array of argument ASTs
 * @param args_size Number of arguments
 * @return Pointer to AST_MATHEMATICAL_FUNCTION node
 */
AST_T *ast_new_mathematical_function(const char *name, AST_T **args, size_t args_size)
{
    AST_T *ast = ast_new(AST_MATHEMATICAL_FUNCTION);
    if (name) {
        ast->math_function_name = memory_strdup(name);
    }
    ast->math_function_args_size = args_size;
    if (args_size > 0 && args) {
        ast->math_function_args = memory_alloc(args_size * sizeof(AST_T *));
        for (size_t i = 0; i < args_size; i++) {
            ast->math_function_args[i] = args[i];
        }
    }
    return ast;
}

/**
 * @brief Create a new file get AST node
 * @param file_path File path expression (string or variable)
 * @param property_path Property path expression for accessing nested data
 * @return Pointer to AST_FILE_GET node
 */
AST_T *ast_new_file_get(AST_T *file_path, AST_T *property_path)
{
    AST_T *ast = ast_new(AST_FILE_GET);
    ast->file_get_path = file_path;
    ast->file_get_property = property_path;
    return ast;
}

/**
 * @brief Create a new file put AST node
 * @param file_path File path expression (string or variable)
 * @param property_path Property path expression for nested data
 * @param value Value to store
 * @return Pointer to AST_FILE_PUT node
 */
AST_T *ast_new_file_put(AST_T *file_path, AST_T *property_path, AST_T *value)
{
    AST_T *ast = ast_new(AST_FILE_PUT);
    ast->file_put_path = file_path;
    ast->file_put_property = property_path;
    ast->file_put_value = value;
    return ast;
}

/**
 * @brief Create a new file reference AST node for @ prefix cross-file references
 * @param target_file Target file path (e.g., "../addresses.json")
 * @param property_path Property path in target file (e.g., "office.alice")
 * @return Pointer to AST_FILE_REFERENCE node
 */
AST_T *ast_new_file_reference(const char *target_file, const char *property_path)
{
    AST_T *ast = ast_new(AST_FILE_REFERENCE);
    ast->file_ref_target_file = target_file ? memory_strdup(target_file) : NULL;
    ast->file_ref_property_path = property_path ? memory_strdup(property_path) : NULL;
    return ast;
}

/**
 * @brief Create a new class definition AST node
 * @param class_name Name of the class
 * @param parent_class Name of parent class (can be NULL)
 * @param methods Array of method definition AST nodes
 * @param methods_count Number of methods
 * @return Pointer to newly allocated class definition AST node
 */
AST_T *ast_new_class_definition(const char *class_name,
                                const char *parent_class,
                                AST_T **methods,
                                size_t methods_count)
{
    AST_T *ast = ast_new(AST_CLASS_DEFINITION);

    // Set class name
    if (class_name) {
        ast->class_name = memory_strdup(class_name);
    }

    // Set parent class if provided
    if (parent_class) {
        ast->parent_class = memory_strdup(parent_class);
    }

    // Set methods
    ast->class_methods_size = methods_count;
    if (methods_count > 0 && methods) {
        ast->class_methods = memory_alloc(methods_count * sizeof(AST_T *));
        for (size_t i = 0; i < methods_count; i++) {
            ast->class_methods[i] = methods[i];
        }
    }

    return ast;
}

/* ============================================================================
 * CRITICAL: AST COPY WITH CYCLE DETECTION
 * ============================================================================ */

// Maximum recursion depth to prevent stack overflow
#define AST_COPY_MAX_DEPTH 1000

// Structure for tracking visited nodes during copy
typedef struct {
    AST_T **visited_original;  // Original nodes we've seen
    AST_T **visited_copies;    // Corresponding copies
    size_t visited_count;      // Number of entries
    size_t visited_capacity;   // Allocated capacity
} VisitedNodes;

/**
 * @brief Initialize visited nodes tracker
 * @param visited Pointer to VisitedNodes structure
 * @return true on success, false on allocation failure
 */
static bool ast_visited_nodes_init(VisitedNodes *visited)
{
    if (!visited)
        return false;

    visited->visited_capacity = 64;  // Start with reasonable size
    visited->visited_original = memory_alloc(visited->visited_capacity * sizeof(AST_T *));
    visited->visited_copies = memory_alloc(visited->visited_capacity * sizeof(AST_T *));
    visited->visited_count = 0;

    if (!visited->visited_original || !visited->visited_copies) {
        if (visited->visited_original)
            memory_free(visited->visited_original);
        if (visited->visited_copies)
            memory_free(visited->visited_copies);
        return false;
    }

    return true;
}

/**
 * @brief Clean up visited nodes tracker
 * @param visited Pointer to VisitedNodes structure
 */
static void ast_visited_nodes_cleanup(VisitedNodes *visited)
{
    if (!visited)
        return;

    if (visited->visited_original)
        memory_free(visited->visited_original);
    if (visited->visited_copies)
        memory_free(visited->visited_copies);

    visited->visited_original = NULL;
    visited->visited_copies = NULL;
    visited->visited_count = 0;
    visited->visited_capacity = 0;
}

/**
 * @brief Check if we've already seen this node
 * @param visited Visited nodes tracker
 * @param original Original node to check
 * @return Copy of node if found, NULL otherwise
 */
static AST_T *ast_visited_nodes_find(const VisitedNodes *visited, const AST_T *original)
{
    if (!visited || !original)
        return NULL;

    for (size_t i = 0; i < visited->visited_count; i++) {
        if (visited->visited_original[i] == original) {
            return visited->visited_copies[i];
        }
    }

    return NULL;
}

/**
 * @brief Add a node pair to the visited tracker
 * @param visited Visited nodes tracker
 * @param original Original node
 * @param copy Copy node
 * @return true on success, false on failure
 */
static bool ast_visited_nodes_add(VisitedNodes *visited, AST_T *original, AST_T *copy)
{
    if (!visited || !original || !copy)
        return false;

    // Expand capacity if needed
    if (visited->visited_count >= visited->visited_capacity) {
        size_t new_capacity = visited->visited_capacity * 2;
        AST_T **new_original =
            memory_realloc(visited->visited_original, new_capacity * sizeof(AST_T *));
        AST_T **new_copies =
            memory_realloc(visited->visited_copies, new_capacity * sizeof(AST_T *));

        if (!new_original || !new_copies) {
            return false;
        }

        visited->visited_original = new_original;
        visited->visited_copies = new_copies;
        visited->visited_capacity = new_capacity;
    }

    visited->visited_original[visited->visited_count] = original;
    visited->visited_copies[visited->visited_count] = copy;
    visited->visited_count++;

    return true;
}

/**
 * @brief Internal recursive AST copy function with cycle detection
 * @param original Original AST node to copy
 * @param visited Visited nodes tracker for cycle detection
 * @param depth Current recursion depth
 * @return Copied AST node or NULL on failure
 */
static AST_T *ast_copy_internal(AST_T *original, VisitedNodes *visited, int depth)
{
    if (!original)
        return NULL;

    // Check recursion depth limit
    if (depth > AST_COPY_MAX_DEPTH) {
        fprintf(stderr,
                "ERROR: AST copy recursion depth exceeded %d (infinite recursion)\n",
                AST_COPY_MAX_DEPTH);
        return NULL;
    }

    // Check if we've already copied this node (cycle detection)
    AST_T *existing_copy = ast_visited_nodes_find(visited, original);
    if (existing_copy) {
        // We've seen this node before - return the existing copy to break cycle
        return existing_copy;
    }

    // Create new node of the same type
    AST_T *copy = ast_new(original->type);
    if (!copy)
        return NULL;

    // Add to visited nodes BEFORE recursing to handle self-references
    if (!ast_visited_nodes_add(visited, original, copy)) {
        ast_free(copy);
        return NULL;
    }

    // Copy simple fields first
    copy->boolean_value = original->boolean_value;
    copy->number_value = original->number_value;
    copy->operator_type = original->operator_type;

    // Copy string fields
    if (original->string_value) {
        copy->string_value = memory_strdup(original->string_value);
    }
    if (original->variable_definition_variable_name) {
        copy->variable_definition_variable_name =
            memory_strdup(original->variable_definition_variable_name);
    }
    if (original->function_definition_name) {
        copy->function_definition_name = memory_strdup(original->function_definition_name);
    }
    if (original->variable_name) {
        copy->variable_name = memory_strdup(original->variable_name);
    }
    if (original->function_call_name) {
        copy->function_call_name = memory_strdup(original->function_call_name);
    }
    if (original->iterator_variable) {
        copy->iterator_variable = memory_strdup(original->iterator_variable);
    }
    if (original->class_name) {
        copy->class_name = memory_strdup(original->class_name);
    }
    if (original->parent_class) {
        copy->parent_class = memory_strdup(original->parent_class);
    }
    if (original->new_class_name) {
        copy->new_class_name = memory_strdup(original->new_class_name);
    }
    if (original->property_name) {
        copy->property_name = memory_strdup(original->property_name);
    }
    if (original->import_path) {
        copy->import_path = memory_strdup(original->import_path);
    }
    if (original->exception_variable) {
        copy->exception_variable = memory_strdup(original->exception_variable);
    }

    // Copy single AST node fields recursively
    copy->variable_definition_value =
        ast_copy_internal(original->variable_definition_value, visited, depth + 1);
    copy->function_definition_body =
        ast_copy_internal(original->function_definition_body, visited, depth + 1);
    copy->left = ast_copy_internal(original->left, visited, depth + 1);
    copy->right = ast_copy_internal(original->right, visited, depth + 1);
    copy->operand = ast_copy_internal(original->operand, visited, depth + 1);
    copy->condition = ast_copy_internal(original->condition, visited, depth + 1);
    copy->then_branch = ast_copy_internal(original->then_branch, visited, depth + 1);
    copy->else_branch = ast_copy_internal(original->else_branch, visited, depth + 1);
    copy->loop_condition = ast_copy_internal(original->loop_condition, visited, depth + 1);
    copy->loop_body = ast_copy_internal(original->loop_body, visited, depth + 1);
    copy->iterable = ast_copy_internal(original->iterable, visited, depth + 1);
    copy->for_body = ast_copy_internal(original->for_body, visited, depth + 1);
    copy->return_value = ast_copy_internal(original->return_value, visited, depth + 1);
    copy->object = ast_copy_internal(original->object, visited, depth + 1);
    copy->try_block = ast_copy_internal(original->try_block, visited, depth + 1);
    copy->catch_block = ast_copy_internal(original->catch_block, visited, depth + 1);
    copy->exception_value = ast_copy_internal(original->exception_value, visited, depth + 1);
    copy->lambda_body = ast_copy_internal(original->lambda_body, visited, depth + 1);

    // Copy arrays of AST nodes
    if (original->function_definition_args && original->function_definition_args_size > 0) {
        copy->function_definition_args_size = original->function_definition_args_size;
        copy->function_definition_args =
            memory_alloc(copy->function_definition_args_size * sizeof(AST_T *));
        if (copy->function_definition_args) {
            for (size_t i = 0; i < copy->function_definition_args_size; i++) {
                copy->function_definition_args[i] =
                    ast_copy_internal(original->function_definition_args[i], visited, depth + 1);
            }
        }
    }

    if (original->function_call_arguments && original->function_call_arguments_size > 0) {
        copy->function_call_arguments_size = original->function_call_arguments_size;
        copy->function_call_arguments =
            memory_alloc(copy->function_call_arguments_size * sizeof(AST_T *));
        if (copy->function_call_arguments) {
            for (size_t i = 0; i < copy->function_call_arguments_size; i++) {
                copy->function_call_arguments[i] =
                    ast_copy_internal(original->function_call_arguments[i], visited, depth + 1);
            }
        }
    }

    if (original->new_arguments && original->new_arguments_size > 0) {
        copy->new_arguments_size = original->new_arguments_size;
        copy->new_arguments = memory_alloc(copy->new_arguments_size * sizeof(AST_T *));
        if (copy->new_arguments) {
            for (size_t i = 0; i < copy->new_arguments_size; i++) {
                copy->new_arguments[i] =
                    ast_copy_internal(original->new_arguments[i], visited, depth + 1);
            }
        }
    }

    if (original->array_elements && original->array_size > 0) {
        copy->array_size = original->array_size;
        copy->array_elements = memory_alloc(copy->array_size * sizeof(AST_T *));
        if (copy->array_elements) {
            for (size_t i = 0; i < copy->array_size; i++) {
                copy->array_elements[i] =
                    ast_copy_internal(original->array_elements[i], visited, depth + 1);
            }
        }
    }

    // Copy object keys and values
    if (original->object_keys && original->object_values && original->object_size > 0) {
        copy->object_size = original->object_size;
        copy->object_keys = memory_alloc(copy->object_size * sizeof(char *));
        copy->object_values = memory_alloc(copy->object_size * sizeof(AST_T *));

        if (copy->object_keys && copy->object_values) {
            for (size_t i = 0; i < copy->object_size; i++) {
                copy->object_keys[i] =
                    original->object_keys[i] ? memory_strdup(original->object_keys[i]) : NULL;
                copy->object_values[i] =
                    ast_copy_internal(original->object_values[i], visited, depth + 1);
            }
        }
    }

    if (original->compound_statements && original->compound_size > 0) {
        copy->compound_size = original->compound_size;
        copy->compound_statements = memory_alloc(copy->compound_size * sizeof(AST_T *));
        if (copy->compound_statements) {
            for (size_t i = 0; i < copy->compound_size; i++) {
                copy->compound_statements[i] =
                    ast_copy_internal(original->compound_statements[i], visited, depth + 1);
            }
        }
    }

    if (original->class_methods && original->class_methods_size > 0) {
        copy->class_methods_size = original->class_methods_size;
        copy->class_methods = memory_alloc(copy->class_methods_size * sizeof(AST_T *));
        if (copy->class_methods) {
            for (size_t i = 0; i < copy->class_methods_size; i++) {
                copy->class_methods[i] =
                    ast_copy_internal(original->class_methods[i], visited, depth + 1);
            }
        }
    }

    if (original->lambda_args && original->lambda_args_size > 0) {
        copy->lambda_args_size = original->lambda_args_size;
        copy->lambda_args = memory_alloc(copy->lambda_args_size * sizeof(AST_T *));
        if (copy->lambda_args) {
            for (size_t i = 0; i < copy->lambda_args_size; i++) {
                copy->lambda_args[i] =
                    ast_copy_internal(original->lambda_args[i], visited, depth + 1);
            }
        }
    }

    // Copy import names array
    if (original->import_names && original->import_names_size > 0) {
        copy->import_names_size = original->import_names_size;
        copy->import_names = memory_alloc(copy->import_names_size * sizeof(char *));
        if (copy->import_names) {
            for (size_t i = 0; i < copy->import_names_size; i++) {
                copy->import_names[i] =
                    original->import_names[i] ? memory_strdup(original->import_names[i]) : NULL;
            }
        }
    }

    // Note: scope field is intentionally NOT copied to avoid circular references
    // The copy will get a new scope when needed during evaluation

    return copy;
}

/**
 * @brief Create a deep copy of an AST node with cycle detection
 * @param original Original AST node to copy
 * @return Deep copy of the AST node, or NULL on failure/infinite recursion
 *
 * This function performs a deep copy of an AST tree while detecting and handling
 * circular references that could cause infinite recursion. It uses a visited
 * nodes tracker to identify cycles and returns shared references for already
 * visited nodes, effectively converting circular structures into DAGs.
 *
 * Key features:
 * - Detects infinite recursion and circular references
 * - Limits recursion depth to prevent stack overflow
 * - Memory-safe with proper cleanup on failure
 * - Handles all AST node types comprehensively
 */
AST_T *ast_copy(AST_T *original)
{
    if (!original)
        return NULL;

    VisitedNodes visited;
    if (!ast_visited_nodes_init(&visited)) {
        return NULL;
    }

    AST_T *result = ast_copy_internal(original, &visited, 0);

    ast_visited_nodes_cleanup(&visited);

    return result;
}
