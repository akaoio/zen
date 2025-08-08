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
    // Temporarily disable pool to debug infinite loop issue
    AST_T *ast = memory_alloc(sizeof(AST_T));
    if (!ast) {
        return NULL;  // Allocation failed
    }
    memset(ast, 0, sizeof(AST_T));
    ast->type = type;

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
        for (size_t i = 0; i < ast->class_methods_size; i++) {
            if (ast->class_methods[i])
                ast_free(ast->class_methods[i]);
        }
        memory_free(ast->class_methods);
    }

    if (ast->lambda_args) {
        for (size_t i = 0; i < ast->lambda_args_size; i++) {
            if (ast->lambda_args[i])
                ast_free(ast->lambda_args[i]);
        }
        memory_free(ast->lambda_args);
    }

    // Temporarily using direct free to debug infinite loop issue
    memory_free(ast);
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
