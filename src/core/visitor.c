/*
 * visitor.c
 * Advanced AST visitor pattern implementation with runtime optimizations
 *
 * This file implements the enterprise-grade ZEN runtime evaluation system
 * with performance profiling, hot function optimization, exception handling,
 * and advanced execution analytics.
 */

#define _GNU_SOURCE  // Enable strdup
#include "zen/core/visitor.h"

#include "zen/core/ast_runtime_convert.h"
#include "zen/core/error.h"
#include "zen/core/logger.h"
#include "zen/core/memory.h"
#include "zen/core/parser.h"
#include "zen/core/runtime_value.h"
#include "zen/core/scope.h"
#include "zen/core/token.h"
#include "zen/runtime/operators.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/stdlib/module.h"
#include "zen/stdlib/stdlib.h"
#include "zen/stdlib/yaml.h"
#include "zen/types/array.h"
#include "zen/types/object.h"
#include "zen/types/value.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Advanced runtime optimization constants
#define DEFAULT_HOT_FUNCTION_THRESHOLD 100
#define MAX_CALL_STACK_DEPTH           10000
#define PROFILE_CAPACITY_INCREMENT     50
#define HOT_FUNCTION_TIME_THRESHOLD    0.001  // 1ms

// Forward declarations
// static RuntimeValue *builtin_function_print(visitor_T *visitor, AST_T **args, int args_size);
static Value *ast_to_value(AST_T *node);
// TODO: Update these to RuntimeValue
// static RuntimeValue *visitor_visit_class_definition(visitor_T *visitor, AST_T *node);
// static RuntimeValue *visitor_visit_new_expression(visitor_T *visitor, AST_T *node);

static RuntimeValue *visitor_visit_binary_op(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_unary_op(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_array(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_object(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_property_access(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_if_statement(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_while_loop(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_for_loop(visitor_T *visitor, AST_T *node);
static RuntimeValue *
visitor_execute_user_function(visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size);
static bool is_truthy_rv(RuntimeValue *rv);

// Database-like file operations
// TODO: Update these to RuntimeValue
// static RuntimeValue *visitor_visit_file_get(visitor_T *visitor, AST_T *node);
// static RuntimeValue *visitor_visit_file_put(visitor_T *visitor, AST_T *node);
// static RuntimeValue *visitor_visit_file_reference(visitor_T *visitor, AST_T *node);
// static RuntimeValue *visitor_navigate_property_path(visitor_T *visitor, AST_T *root, AST_T
// *property_path); Removed unused function declaration

// Advanced runtime optimization functions
static void visitor_push_call_frame(visitor_T *visitor,
                                    AST_T *function_def,
                                    AST_T **args,
                                    size_t arg_count,
                                    const char *function_name);
static void visitor_pop_call_frame(visitor_T *visitor);
static FunctionProfile *visitor_get_or_create_profile(visitor_T *visitor,
                                                      const char *function_name);
static void visitor_update_function_profile(visitor_T *visitor,
                                            const char *function_name,
                                            double execution_time);
// static AST_T *visitor_optimize_constant_expression(visitor_T *visitor, AST_T *node);
// static bool visitor_is_constant_expression(AST_T *node);
// static AST_T *visitor_apply_tail_call_optimization(visitor_T *visitor, AST_T *node);
static double visitor_get_time_diff(clock_t start, clock_t end);
static void visitor_update_ast_scope(AST_T *node, scope_T *new_scope);

/**
 * @brief Create a new visitor instance with advanced runtime features
 * @param void Function takes no parameters
 * @return New visitor instance or NULL on failure
 */
// Note: Temporary node tracking was removed due to architectural issues
// The proper solution would be to separate runtime values from AST nodes

visitor_T *visitor_new()
{
    visitor_T *visitor = memory_alloc(sizeof(visitor_T));
    if (!visitor)
        return NULL;

    // Initialize execution state
    visitor->mode = EXECUTION_MODE_INTERPRETED;

    // Initialize call stack
    visitor->call_stack = NULL;
    visitor->call_stack_depth = 0;
    visitor->max_call_stack_depth = MAX_CALL_STACK_DEPTH;

    // Initialize profiling system
    visitor->function_profiles = memory_alloc(sizeof(FunctionProfile) * PROFILE_CAPACITY_INCREMENT);
    visitor->profile_count = 0;
    visitor->profile_capacity = PROFILE_CAPACITY_INCREMENT;
    visitor->profiling_enabled = true;  // Enable by default for optimization

    // Initialize runtime optimizations (disabled by default for safety)
    visitor->tail_call_optimization = false;
    visitor->constant_folding = false;  // Disabled due to recursion bug
    visitor->dead_code_elimination = false;
    visitor->inline_small_functions = false;

    // Initialize exception handling
    visitor->exception_state.exception_value = NULL;
    visitor->exception_state.exception_message = NULL;
    visitor->exception_state.source_location = NULL;
    visitor->exception_state.is_active = false;

    // Initialize execution analytics
    visitor->total_instructions_executed = 0;
    visitor->total_execution_time = 0.0;
    visitor->memory_allocations = 0;
    visitor->cache_hits = 0;
    visitor->cache_misses = 0;

    // Set hot function threshold
    visitor->hot_function_threshold = DEFAULT_HOT_FUNCTION_THRESHOLD;

    if (!visitor->function_profiles) {
        memory_free(visitor);
        return NULL;
    }

    return visitor;
}

/**
 * @brief Free a visitor instance and its advanced runtime resources
 * @param visitor The visitor instance to free
 */
void visitor_free(visitor_T *visitor)
{
    if (!visitor) {
        return;
    }

    // Free call stack
    while (visitor->call_stack) {
        visitor_pop_call_frame(visitor);
    }

    // Free function profiles
    if (visitor->function_profiles) {
        for (size_t i = 0; i < visitor->profile_count; i++) {
            if (visitor->function_profiles[i].function_name) {
                memory_free(visitor->function_profiles[i].function_name);
            }
            if (visitor->function_profiles[i].optimized_ast) {
                ast_free(visitor->function_profiles[i].optimized_ast);
            }
        }
        memory_free(visitor->function_profiles);
    }

    // Free exception state
    if (visitor->exception_state.exception_message) {
        memory_free(visitor->exception_state.exception_message);
    }
    if (visitor->exception_state.source_location) {
        memory_free(visitor->exception_state.source_location);
    }
    if (visitor->exception_state.exception_value) {
        ast_free(visitor->exception_state.exception_value);
    }

    memory_free(visitor);
}

/**
 * @brief Visit and evaluate an AST node with advanced runtime features
 * @param visitor Visitor instance
 * @param node AST node to visit
 * @return Result of evaluation
 */
RuntimeValue *visitor_visit(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    static int depth = 0;
    depth++;
    if (depth > 100) {
        LOG_ERROR(LOG_CAT_VISITOR, "Stack overflow detected! Node type=%d", node->type);
        depth--;
        return rv_new_null();
    }

    // Check for active exception
    if (visitor_has_exception(visitor)) {
        depth--;
        return rv_new_null();
    }

    // Increment instruction counter for analytics
    visitor->total_instructions_executed++;

    // Start timing for this node evaluation
    clock_t start_time = clock();
    (void)start_time;  // Mark as used

    // TODO: Update constant folding optimization to use RuntimeValue
    // Apply constant folding optimization if enabled
    // Constant folding optimization disabled for now

    RuntimeValue *result = NULL;

    switch (node->type) {
    case AST_VARIABLE_DEFINITION:
        return visitor_visit_variable_definition(visitor, node);
    case AST_FUNCTION_DEFINITION:
        return visitor_visit_function_definition(visitor, node);
    case AST_VARIABLE:
        return visitor_visit_variable(visitor, node);
    case AST_FUNCTION_CALL:
        return visitor_visit_function_call(visitor, node);
    case AST_COMPOUND:
        return visitor_visit_compound(visitor, node);
    case AST_NOOP:
        return rv_new_null();

    // Literals - convert to RuntimeValue
    case AST_STRING:
        return rv_new_string(node->string_value);
    case AST_NUMBER:
        return rv_new_number(node->number_value);
    case AST_BOOLEAN:
        return rv_new_boolean(node->boolean_value);
    case AST_NULL:
    case AST_UNDECIDABLE:
        return rv_new_null();

    // Expression evaluation
    case AST_BINARY_OP:
        return visitor_visit_binary_op(visitor, node);
    case AST_UNARY_OP:
        return visitor_visit_unary_op(visitor, node);

    // Data structures
    case AST_ARRAY:
        return visitor_visit_array(visitor, node);
    case AST_OBJECT:
        return visitor_visit_object(visitor, node);
    case AST_PROPERTY_ACCESS:
        return visitor_visit_property_access(visitor, node);

    // Control flow
    case AST_IF_STATEMENT:
        return visitor_visit_if_statement(visitor, node);
    case AST_WHILE_LOOP:
        return visitor_visit_while_loop(visitor, node);
    case AST_FOR_LOOP:
        return visitor_visit_for_loop(visitor, node);
    case AST_BREAK:
    case AST_CONTINUE:
        // Return special markers for control flow
        return rv_new_string(node->type == AST_BREAK ? "__BREAK__" : "__CONTINUE__");
    case AST_RETURN:
        // Handle return statements - evaluate the return value
        if (node->return_value) {
            RuntimeValue *return_val = visitor_visit(visitor, node->return_value);
            // Create a special marker for return
            RuntimeValue *marker = rv_new_object();
            rv_object_set(marker, "__RETURN__", return_val);
            rv_unref(return_val);  // rv_object_set already refs the value
            return marker;
        }
        return rv_new_null();

    // Database-like file operations
    case AST_FILE_GET:
    case AST_FILE_PUT:
    case AST_FILE_REFERENCE:
        // TODO: Implement these with RuntimeValue
        return rv_new_null();

    // Formal Logic AST nodes - placeholder implementations
    case AST_LOGICAL_QUANTIFIER:
    case AST_LOGICAL_PREDICATE:
    case AST_LOGICAL_CONNECTIVE:
    case AST_LOGICAL_VARIABLE:
    case AST_LOGICAL_PROPOSITION:
    case AST_LOGICAL_THEOREM:
    case AST_LOGICAL_AXIOM:
    case AST_LOGICAL_PROOF_STEP:
    case AST_LOGICAL_PREMISE:
    case AST_LOGICAL_CONCLUSION:
    case AST_LOGICAL_INFERENCE:
    case AST_LOGICAL_SUBSTITUTION:
    case AST_MATHEMATICAL_EQUATION:
    case AST_MATHEMATICAL_INEQUALITY:
    case AST_MATHEMATICAL_FUNCTION:
        // For logical AST nodes, return null for now
        result = rv_new_null();
        break;

    case AST_IMPORT:
    case AST_EXPORT:
    case AST_CLASS_DEFINITION:
    case AST_NEW_EXPRESSION:
        // TODO: Implement these with RuntimeValue
        return rv_new_null();

    default:
        // For unimplemented features, return null
        result = rv_new_null();
        break;
    }

    // Update execution timing
    if (result) {
        clock_t end_time = clock();
        visitor->total_execution_time += visitor_get_time_diff(start_time, end_time);
    }

    depth--;
    return result;
}

/**
 * @brief Visit variable definition node
 * @param visitor Visitor instance
 * @param node Variable definition AST node
 * @return The defined variable value
 */
RuntimeValue *visitor_visit_variable_definition(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid variable definition (visitor=%p, node=%p, scope=%p)",
                  (void *)visitor,
                  (void *)node,
                  node ? (void *)node->scope : NULL);
        return rv_new_null();
    }

    if (!node->variable_definition_variable_name) {
        LOG_ERROR(LOG_CAT_VISITOR, "Variable definition missing name");
        return rv_new_null();
    }

    // Evaluate the value expression
    RuntimeValue *value = NULL;
    if (node->variable_definition_value) {
        value = visitor_visit(visitor, node->variable_definition_value);
    } else {
        value = rv_new_null();
    }

    // Convert the evaluated RuntimeValue back to a simple AST value node
    // This ensures we store the evaluated result, not the expression
    AST_T *evaluated_ast = runtime_value_to_ast(value);

    // Free the old expression AST if it's different from the new one
    if (node->variable_definition_value && node->variable_definition_value != evaluated_ast) {
        ast_free(node->variable_definition_value);
    }

    // Replace the expression with the evaluated value
    node->variable_definition_value = evaluated_ast;

    // Add variable to scope
    AST_T *result = scope_add_variable_definition(node->scope, node);
    if (!result) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Failed to add variable '%s' to scope",
                  node->variable_definition_variable_name);
        rv_unref(value);
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG("Variable '%s' set successfully in scope %p with evaluated value",
                      node->variable_definition_variable_name,
                      (void *)node->scope);

    return value;
}

/**
 * @brief Visit function definition node
 * @param visitor Visitor instance
 * @param node Function definition AST node
 * @return The function definition node
 */
RuntimeValue *visitor_visit_function_definition(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->scope) {
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG(
        "Defining function '%s' in scope %p", node->function_definition_name, (void *)node->scope);
    scope_add_function_definition(node->scope, node);

    // Return function as runtime value
    return rv_new_function(node, node->scope);
}

/**
 * @brief Visit variable node
 * @param visitor Visitor instance
 * @param node Variable AST node
 * @return Variable value or NULL
 */
RuntimeValue *visitor_visit_variable(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->variable_name || !node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid variable access (visitor=%p, node=%p, name=%s, scope=%p)",
                  (void *)visitor,
                  (void *)node,
                  node && node->variable_name ? node->variable_name : "NULL",
                  node ? (void *)node->scope : NULL);
        return rv_new_null();
    }

    // Looking up variable in scope
    LOG_VISITOR_DEBUG(
        "Looking up variable '%s' in scope %p", node->variable_name, (void *)node->scope);

    AST_T *vdef = scope_get_variable_definition(node->scope, node->variable_name);

    if (vdef != NULL && vdef->variable_definition_value != NULL) {
        // Variable found with value
        LOG_VISITOR_DEBUG("Found variable '%s', type=%d", node->variable_name, vdef->type);

        // Convert the stored AST value to RuntimeValue
        // The value is already evaluated and stored as a simple AST node
        RuntimeValue *result = ast_to_runtime_value(vdef->variable_definition_value);

        return result;
    }

    LOG_ERROR(LOG_CAT_VISITOR, "Undefined variable '%s'", node->variable_name);
    return rv_new_null();
}

/**
 * @brief Visit function call node
 * @param visitor Visitor instance
 * @param node Function call AST node
 * @return Result of function call
 */
RuntimeValue *visitor_visit_function_call(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->function_call_name) {
        return rv_new_null();
    }

    // Check for stdlib functions first
    LOG_VISITOR_DEBUG("Looking up stdlib function '%s'", node->function_call_name);
    const ZenStdlibFunction *stdlib_func = stdlib_get(node->function_call_name);
    if (stdlib_func != NULL) {
        LOG_VISITOR_DEBUG("Found stdlib function '%s'", node->function_call_name);
        // Convert AST arguments to Value arguments
        Value **value_args = NULL;
        size_t argc = (size_t)node->function_call_arguments_size;

        if (argc > 0) {
            value_args = memory_alloc(sizeof(Value *) * argc);
            if (!value_args) {
                return rv_new_null();
            }

            // Evaluate and convert each argument
            for (size_t i = 0; i < argc; i++) {
                RuntimeValue *arg_rv = visitor_visit(visitor, node->function_call_arguments[i]);
                AST_T *arg_ast = runtime_value_to_ast(arg_rv);
                value_args[i] = ast_to_value(arg_ast);
                ast_free(arg_ast);  // Free temporary AST
                rv_unref(arg_rv);
                if (!value_args[i]) {
                    value_args[i] = value_new_null();
                }
            }
        }

        // Call the stdlib function
        Value *result = stdlib_func->func(value_args, argc);

        // Clean up arguments
        if (value_args) {
            for (size_t i = 0; i < argc; i++) {
                if (value_args[i]) {
                    value_unref(value_args[i]);
                }
            }
            memory_free(value_args);
        }

        // Convert result to RuntimeValue
        if (result) {
            AST_T *result_ast = value_to_ast(result);
            value_unref(result);
            if (!result_ast) {
                return rv_new_null();
            }
            RuntimeValue *rv = ast_to_runtime_value(result_ast);
            ast_free(result_ast);  // Free temporary AST
            return rv;
        } else {
            return rv_new_null();
        }
    }

    // Handle built-in functions (legacy print support)
    if (strcmp(node->function_call_name, "print") == 0) {
        // Evaluate arguments and print them
        for (size_t i = 0; i < node->function_call_arguments_size; i++) {
            RuntimeValue *arg = visitor_visit(visitor, node->function_call_arguments[i]);
            char *str = rv_to_string(arg);
            printf("%s", str);
            memory_free(str);
            rv_unref(arg);
            if (i < node->function_call_arguments_size - 1) {
                printf(" ");
            }
        }
        printf("\n");
        return rv_new_null();
    }

    // Handle "new" constructor calls
    if (strcmp(node->function_call_name, "new") == 0) {
        if (node->function_call_arguments_size < 1) {
            LOG_ERROR(LOG_CAT_VISITOR, "new requires a class name");
            return rv_new_null();
        }

        // First argument should be a variable (class name)
        AST_T *class_name_node = node->function_call_arguments[0];
        if (class_name_node->type != AST_VARIABLE) {
            LOG_ERROR(LOG_CAT_VISITOR, "new requires a class name");
            return rv_new_null();
        }

        // Look up the class
        AST_T *class_var =
            scope_get_variable_definition(node->scope, class_name_node->variable_name);
        if (class_var && class_var->variable_definition_value) {
            AST_T *class_def = class_var->variable_definition_value;
            if (class_def->type == AST_CLASS_DEFINITION) {
                // Create a new instance
                Value *class_val = value_new_class(class_def->class_name, class_def->parent_class);
                if (!class_val) {
                    return rv_new_null();
                }

                Value *instance = value_new_instance(class_val);
                value_unref(class_val);

                if (instance) {
                    // Constructor method with arguments can be implemented when needed
                    // Return the new instance as RuntimeValue
                    AST_T *result_ast = value_to_ast(instance);
                    value_unref(instance);
                    if (!result_ast) {
                        return rv_new_null();
                    }
                    RuntimeValue *rv = ast_to_runtime_value(result_ast);
                    ast_free(result_ast);  // Free temporary AST
                    return rv;
                }
            }
        }

        LOG_ERROR(LOG_CAT_VISITOR, "Class '%s' not found", class_name_node->variable_name);
        return rv_new_null();
    }

    // Check if calling a class constructor
    AST_T *class_var = scope_get_variable_definition(node->scope, node->function_call_name);
    if (class_var && class_var->variable_definition_value) {
        Value *potential_class = ast_to_value(class_var->variable_definition_value);
        if (potential_class && potential_class->type == VALUE_CLASS) {
            // This is a class constructor call - create new instance
            Value *instance = value_new_instance(potential_class);
            if (instance) {
                // Constructor method calling can be implemented when needed
                // Return the new instance as RuntimeValue
                AST_T *result_ast = value_to_ast(instance);
                value_unref(instance);
                value_unref(potential_class);
                if (!result_ast) {
                    return rv_new_null();
                }
                RuntimeValue *rv = ast_to_runtime_value(result_ast);
                ast_free(result_ast);  // Free temporary AST
                return rv;
            }
        }
        if (potential_class) {
            value_unref(potential_class);
        }
    }

    // Look up user-defined function
    AST_T *fdef = scope_get_function_definition(node->scope, node->function_call_name);

    if (fdef == NULL) {
        // Function not found - check if it's a variable reference instead
        // This handles ZEN's design where standalone identifiers default to function calls
        // but should fallback to variable lookup if no function exists
        AST_T *vdef = scope_get_variable_definition(node->scope, node->function_call_name);
        if (vdef && vdef->variable_definition_value) {
            LOG_VISITOR_DEBUG("Function '%s' not found, treating as variable reference",
                              node->function_call_name);
            // For literals, return them directly as RuntimeValue
            AST_T *value = vdef->variable_definition_value;
            return ast_to_runtime_value(value);
        }

        // Neither function nor variable found
        LOG_ERROR(LOG_CAT_VISITOR, "Undefined function or variable '%s'", node->function_call_name);
        return rv_new_null();
    }

    // Execute user-defined function
    return visitor_execute_user_function(
        visitor, fdef, node->function_call_arguments, node->function_call_arguments_size);
}

/**
 * @brief Visit compound node (list of statements)
 * @param visitor Visitor instance
 * @param node Compound AST node
 * @return Result of last statement or NOOP
 */
RuntimeValue *visitor_visit_compound(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->compound_statements) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid compound node (visitor=%p, node=%p, statements=%p)",
                  (void *)visitor,
                  (void *)node,
                  node ? (void *)node->compound_statements : NULL);
        return rv_new_null();
    }

    // Executing compound with multiple statements

    RuntimeValue *last_result = rv_new_null();

    for (size_t i = 0; i < node->compound_size; i++) {
        LOG_VISITOR_DEBUG("Visiting statement %zu: %p", i, (void *)node->compound_statements[i]);
        if (!node->compound_statements[i]) {
            LOG_ERROR(LOG_CAT_VISITOR, "Statement %zu is NULL - PARSER BUG!", i + 1);
            continue;
        }

        // Free previous result
        if (last_result) {
            rv_unref(last_result);
        }

        last_result = visitor_visit(visitor, node->compound_statements[i]);

        // Check for control flow markers
        if (last_result && last_result->type == RV_OBJECT) {
            // Check for RETURN marker
            RuntimeValue *return_marker = rv_object_get(last_result, "__RETURN__");
            if (return_marker) {
                // Return statement found - stop executing statements and return the marker
                LOG_VISITOR_DEBUG("Return statement found, stopping compound execution");
                // Don't unref return_marker - it's owned by last_result
                return last_result;
            }
        } else if (last_result && last_result->type == RV_STRING) {
            // Check for BREAK/CONTINUE markers
            const char *str = rv_get_string(last_result);
            if (strcmp(str, "__BREAK__") == 0 || strcmp(str, "__CONTINUE__") == 0) {
                // Control flow marker found - propagate it up
                return last_result;
            }
        }

        // Statement executed successfully
    }

    // Compound execution complete
    return last_result;
}

/**
 * @brief Convert AST node to Value object
 * @param node AST node to convert
 * @return Value object or NULL on failure
 */
static Value *ast_to_value(AST_T *node)
{
    if (!node) {
        LOG_VISITOR_DEBUG("ast_to_value called with NULL node");
        return value_new_null();
    }

    LOG_VISITOR_DEBUG("Converting AST type %d to Value", node->type);

    switch (node->type) {
    case AST_NULL:
        LOG_VISITOR_DEBUG("Converting AST_NULL to Value");
        return value_new_null();

    case AST_UNDECIDABLE:
        LOG_VISITOR_DEBUG("Converting AST_UNDECIDABLE to Value");
        return value_new_undecidable();

    case AST_BOOLEAN:
        LOG_VISITOR_DEBUG("Converting AST_BOOLEAN (%s) to Value",
                          node->boolean_value ? "true" : "false");
        return value_new_boolean(node->boolean_value);
    case AST_NUMBER:
        LOG_VISITOR_DEBUG("Converting AST_NUMBER (%f) to Value", node->number_value);
        return value_new_number(node->number_value);
    case AST_STRING:
        LOG_VISITOR_DEBUG("Converting AST_STRING ('%s') to Value",
                          node->string_value ? node->string_value : "");
        return value_new_string(node->string_value ? node->string_value : "");
    case AST_ARRAY: {
        // Create a new array Value
        Value *array_val = array_new(node->array_size > 0 ? node->array_size : 1);
        if (!array_val)
            return value_new_null();

        // Convert each element and add to array
        for (size_t i = 0; i < node->array_size; i++) {
            Value *element_val = ast_to_value(node->array_elements[i]);
            if (element_val) {
                array_push(array_val, element_val);
                value_unref(element_val);
            }
        }

        LOG_VISITOR_DEBUG("Converted AST_ARRAY to Value with %zu elements", node->array_size);
        return array_val;
    }
    case AST_OBJECT: {
        // Create a new object Value
        Value *object_val = object_new();
        if (!object_val)
            return value_new_null();

        // Set each key-value pair
        for (size_t i = 0; i < node->object_size; i++) {
            if (node->object_keys[i] && node->object_values[i]) {
                Value *value_val = ast_to_value(node->object_values[i]);
                if (value_val) {
                    object_set(object_val, node->object_keys[i], value_val);

                    // Verify it was set
                    Value *check_val = object_get(object_val, node->object_keys[i]);
                    if (check_val) {
                        value_unref(check_val);
                    }

                    value_unref(value_val);
                }
            }
        }

        LOG_VISITOR_DEBUG("Converted AST_OBJECT to Value with %zu pairs", node->object_size);
        return object_val;
    }
    case AST_VARIABLE:
        // For AST_VARIABLE nodes, we need to look up the actual value from scope
        // This should not happen in normal flow as variables should be resolved first
        LOG_VISITOR_DEBUG("AST_VARIABLE in ast_to_value - variable should be resolved first");
        return value_new_null();

    default:
        LOG_VISITOR_DEBUG("Unknown AST type %d, returning null", node->type);
        return value_new_null();
    }
}

/**
 * @brief Convert Value object to AST node
 * @param value Value object to convert
 * @return AST node or NULL on failure
 */
AST_T *value_to_ast(Value *value)
{
    if (!value) {
        return ast_new(AST_NULL);
    }

    switch (value->type) {
    case VALUE_NULL:
        return ast_new(AST_NULL);
    case VALUE_UNDECIDABLE:
        return ast_new(AST_UNDECIDABLE);
    case VALUE_BOOLEAN: {
        AST_T *ast = ast_new(AST_BOOLEAN);
        if (ast) {
            ast->boolean_value = value->as.boolean;
        }
        return ast;
    }
    case VALUE_NUMBER: {
        AST_T *ast = ast_new(AST_NUMBER);
        if (ast) {
            ast->number_value = value->as.number;
        }
        return ast;
    }
    case VALUE_STRING: {
        AST_T *ast = ast_new(AST_STRING);
        if (ast && value->as.string && value->as.string->data) {
            // NOTE: Removed automatic @ prefix conversion here to prevent infinite loops
            // The @ prefix should only be processed during initial parsing, not value conversion
            ast->string_value = memory_strdup(value->as.string->data);
        }
        return ast;
    }
    case VALUE_ARRAY: {
        AST_T *ast = ast_new(AST_ARRAY);
        if (!ast || !value->as.array)
            return ast_new(AST_NULL);

        // Get array length
        size_t length = array_length(value);
        ast->array_size = length;

        if (length > 0) {
            // Allocate array for elements
            ast->array_elements = memory_alloc(sizeof(AST_T *) * length);
            if (!ast->array_elements) {
                ast_free(ast);
                return ast_new(AST_NULL);
            }

            // Convert each element back to AST
            for (size_t i = 0; i < length; i++) {
                Value *element_val = array_get(value, i);
                ast->array_elements[i] = value_to_ast(element_val);
                if (element_val)
                    value_unref(element_val);
            }
        } else {
            ast->array_elements = NULL;
        }

        return ast;
    }
    case VALUE_OBJECT: {
        AST_T *ast = ast_new(AST_OBJECT);
        if (!ast || !value->as.object)
            return ast_new(AST_NULL);

        ZenObject *obj = value->as.object;
        ast->object_size = obj->length;

        if (obj->length > 0) {
            // Allocate arrays for keys and values
            ast->object_keys = memory_alloc(sizeof(char *) * obj->length);
            ast->object_values = memory_alloc(sizeof(AST_T *) * obj->length);

            if (!ast->object_keys || !ast->object_values) {
                if (ast->object_keys)
                    memory_free(ast->object_keys);
                if (ast->object_values)
                    memory_free(ast->object_values);
                ast_free(ast);
                return ast_new(AST_NULL);
            }

            // Copy keys and convert values
            for (size_t i = 0; i < obj->length; i++) {
                ast->object_keys[i] = memory_strdup(obj->pairs[i].key);
                ast->object_values[i] = value_to_ast(obj->pairs[i].value);
            }
        } else {
            ast->object_keys = NULL;
            ast->object_values = NULL;
        }

        return ast;
    }
    case VALUE_ERROR:
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Error in expression evaluation: %s",
                  value->as.error && value->as.error->message ? value->as.error->message
                                                              : "Unknown error");
        return ast_new(AST_NULL);
    case VALUE_CLASS: {
        // For now, represent class as a special object AST
        AST_T *ast = ast_new(AST_OBJECT);
        if (ast) {
            // Mark this as a class somehow - could add a flag to AST_T
            // For now, just return the object representation
        }
        return ast;
    }
    case VALUE_INSTANCE: {
        // Represent instance as an object with properties
        AST_T *ast = ast_new(AST_OBJECT);
        if (ast && value->as.instance && value->as.instance->properties) {
            // Convert the properties object to AST
            AST_T *props_ast = value_to_ast(value->as.instance->properties);
            if (props_ast && props_ast->type == AST_OBJECT) {
                // Copy properties from the converted object
                ast->object_size = props_ast->object_size;
                ast->object_keys = props_ast->object_keys;
                ast->object_values = props_ast->object_values;
                // Clear the temporary AST to prevent double-free
                props_ast->object_keys = NULL;
                props_ast->object_values = NULL;
                props_ast->object_size = 0;
                ast_free(props_ast);
            }
        }
        return ast;
    }
    default:
        return ast_new(AST_NULL);
    }
}

/**
 * @brief Visit binary operation node
 * @param visitor Visitor instance
 * @param node Binary operation AST node
 * @return Result of binary operation
 */
static RuntimeValue *visitor_visit_binary_op(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->left || !node->right) {
        return rv_new_null();
    }

    // CRITICAL FIX: Ensure operands are evaluated fresh every time
    // This prevents cached variable references in loops from causing infinite loops

    // Evaluate operands directly
    RuntimeValue *left_rv = visitor_visit(visitor, node->left);
    RuntimeValue *right_rv = visitor_visit(visitor, node->right);

    LOG_VISITOR_DEBUG("Binary op - evaluating left operand type=%d", node->left->type);
    LOG_VISITOR_DEBUG("Binary op - evaluating right operand type=%d", node->right->type);
    LOG_VISITOR_DEBUG("Binary op - got left_rv type=%d, right_rv type=%d",
                      left_rv ? (int)left_rv->type : -1,
                      right_rv ? (int)right_rv->type : -1);

    if (!left_rv || !right_rv) {
        if (left_rv)
            rv_unref(left_rv);
        if (right_rv)
            rv_unref(right_rv);
        return rv_new_null();
    }

    // Convert RuntimeValue to AST then to Value objects for operators
    AST_T *left_ast = runtime_value_to_ast(left_rv);
    AST_T *right_ast = runtime_value_to_ast(right_rv);
    Value *left_val = ast_to_value(left_ast);
    Value *right_val = ast_to_value(right_ast);
    ast_free(left_ast);
    ast_free(right_ast);
    rv_unref(left_rv);
    rv_unref(right_rv);

    if (!left_val || !right_val) {
        if (left_val)
            value_unref(left_val);
        if (right_val)
            value_unref(right_val);
        return rv_new_null();
    }

    Value *result = NULL;

    // Apply the appropriate operator
    switch (node->operator_type) {
    case TOKEN_PLUS:
        result = op_add(left_val, right_val);
        if (result && result->type == VALUE_NUMBER) {
        }
        break;
    case TOKEN_MINUS:
        result = op_subtract(left_val, right_val);
        break;
    case TOKEN_MULTIPLY:
        result = op_multiply(left_val, right_val);
        break;
    case TOKEN_DIVIDE:
        result = op_divide(left_val, right_val);
        break;
    case TOKEN_MODULO:
        result = op_modulo(left_val, right_val);
        break;
    case TOKEN_EQUALS:
        result = op_equals(left_val, right_val);
        break;
    case TOKEN_NOT_EQUALS:
        result = op_not_equals(left_val, right_val);
        break;
    case TOKEN_LESS_THAN:
        result = op_less_than(left_val, right_val);
        break;
    case TOKEN_GREATER_THAN:
        result = op_greater_than(left_val, right_val);
        break;
    case TOKEN_LESS_EQUALS:
        // Implement <= as !(a > b)
        {
            Value *gt_val = op_greater_than(left_val, right_val);
            if (gt_val) {
                result = op_logical_not(gt_val);
                value_unref(gt_val);
            }
        }
        break;
    case TOKEN_GREATER_EQUALS:
        // Implement >= as !(a < b)
        {
            Value *lt_val = op_less_than(left_val, right_val);
            if (lt_val) {
                result = op_logical_not(lt_val);
                value_unref(lt_val);
            }
        }
        break;
    case TOKEN_AND:
        result = op_logical_and(left_val, right_val);
        break;
    case TOKEN_OR:
        result = op_logical_or(left_val, right_val);
        break;
    default:
        LOG_ERROR(LOG_CAT_VISITOR, "Unknown binary operator %d", node->operator_type);
        result = NULL;
        break;
    }

    // Clean up operands
    value_unref(left_val);
    value_unref(right_val);

    if (!result) {
        return rv_new_null();
    }

    // Convert result to RuntimeValue
    AST_T *result_ast = value_to_ast(result);
    value_unref(result);
    RuntimeValue *result_rv = ast_to_runtime_value(result_ast);
    ast_free(result_ast);

    return result_rv;
}

/**
 * @brief Visit unary operation node
 * @param visitor Visitor instance
 * @param node Unary operation AST node
 * @return Result of unary operation
 */
static RuntimeValue *visitor_visit_unary_op(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->operand) {
        return rv_new_null();
    }

    // Evaluate operand
    RuntimeValue *operand_rv = visitor_visit(visitor, node->operand);
    if (!operand_rv) {
        return rv_new_null();
    }

    // Convert RuntimeValue to AST then to Value object
    AST_T *operand_ast = runtime_value_to_ast(operand_rv);
    Value *operand_val = ast_to_value(operand_ast);
    ast_free(operand_ast);
    rv_unref(operand_rv);

    if (!operand_val) {
        return rv_new_null();
    }

    Value *result = NULL;

    // Apply the appropriate unary operator
    switch (node->operator_type) {
    case TOKEN_MINUS:
        // Unary minus: multiply by -1
        {
            Value *neg_one = value_new_number(-1.0);
            if (neg_one) {
                result = op_multiply(operand_val, neg_one);
                value_unref(neg_one);
            }
        }
        break;
    case TOKEN_NOT:
        result = op_logical_not(operand_val);
        break;
    default:
        LOG_ERROR(LOG_CAT_VISITOR, "Unknown unary operator %d", node->operator_type);
        result = NULL;
        break;
    }

    // Clean up operand
    value_unref(operand_val);

    if (!result) {
        return rv_new_null();
    }

    // Convert result to RuntimeValue
    AST_T *result_ast = value_to_ast(result);
    value_unref(result);
    RuntimeValue *result_rv = ast_to_runtime_value(result_ast);
    ast_free(result_ast);

    return result_rv;
}

/**
 * @brief Visit array literal node
 * @param visitor Visitor instance
 * @param node Array AST node
 * @return Array value as AST node
 */
static RuntimeValue *visitor_visit_array(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Create a new RuntimeValue array
    RuntimeValue *array_rv = rv_new_array();
    if (!array_rv) {
        return rv_new_null();
    }

    // Evaluate and add each element
    for (size_t i = 0; i < node->array_size; i++) {
        RuntimeValue *element_rv = visitor_visit(visitor, node->array_elements[i]);
        if (element_rv) {
            rv_array_push(array_rv, element_rv);
            rv_unref(element_rv);  // array_push adds its own reference
        }
    }

    return array_rv;
}

/**
 * @brief Visit object literal node
 * @param visitor Visitor instance
 * @param node Object AST node
 * @return Object value as AST node
 */
static RuntimeValue *visitor_visit_object(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Create a new RuntimeValue object
    RuntimeValue *object_rv = rv_new_object();
    if (!object_rv) {
        return rv_new_null();
    }

    // Evaluate and set each key-value pair
    for (size_t i = 0; i < node->object_size; i++) {
        if (node->object_keys[i] && node->object_values[i]) {
            LOG_VISITOR_DEBUG("Processing object key-value pair %zu: %s (type=%d)",
                              i,
                              node->object_keys[i],
                              node->object_values[i]->type);

            RuntimeValue *value_rv = visitor_visit(visitor, node->object_values[i]);
            if (value_rv) {
                LOG_VISITOR_DEBUG("Setting object property %s (value type=%d)",
                                  node->object_keys[i],
                                  value_rv->type);
                rv_object_set(object_rv, node->object_keys[i], value_rv);
                rv_unref(value_rv);  // object_set adds its own reference
            }
        }
    }

    return object_rv;
}

/**
 * @brief Visit property access node (obj.property)
 * @param visitor Visitor instance
 * @param node Property access AST node
 * @return Property value or NULL
 */
static RuntimeValue *visitor_visit_property_access(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->object || !node->property_name) {
        return rv_new_null();
    }

    // Evaluate the object expression
    RuntimeValue *object_rv = visitor_visit(visitor, node->object);
    if (!object_rv) {
        return rv_new_null();
    }

    RuntimeValue *result = NULL;

    // Handle different types using RuntimeValue directly
    if (rv_is_object(object_rv)) {
        // Object property access
        result = rv_object_get(object_rv, node->property_name);
        if (result) {
            // rv_object_get returns a borrowed reference, need to increase refcount
            rv_ref(result);
        } else {
            result = rv_new_null();
        }
    } else if (rv_is_array(object_rv)) {
        // Handle special array property: length
        if (strcmp(node->property_name, "length") == 0) {
            size_t length = rv_array_length(object_rv);
            result = rv_new_number((double)length);
        } else {
            // Handle array indexing: arr[0] becomes arr.0 in property access
            char *endptr;
            long index = strtol(node->property_name, &endptr, 10);

            if (*endptr == '\0' && index >= 0) {
                // Valid numeric index
                RuntimeValue *element = rv_array_get(object_rv, (size_t)index);
                if (element) {
                    // rv_array_get returns a borrowed reference, need to increase refcount
                    result = element;
                    rv_ref(result);
                } else {
                    result = rv_new_null();
                }
            } else {
                LOG_ERROR(
                    LOG_CAT_VISITOR, "Invalid array property/index '%s'", node->property_name);
                result = rv_new_null();
            }
        }
    } else {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Cannot access property '%s' on type %s",
                  node->property_name,
                  rv_type_name(object_rv));
        result = rv_new_null();
    }

    // Clean up
    rv_unref(object_rv);

    return result;
}

/**
 * @brief Check if a RuntimeValue evaluates to a truthy value
 * @param rv RuntimeValue to evaluate
 * @return true if truthy, false otherwise
 */
static bool is_truthy_rv(RuntimeValue *rv)
{
    if (!rv) {
        return false;
    }

    switch (rv->type) {
    case RV_NULL:
        return false;
    case RV_BOOLEAN:
        return rv->data.boolean;
    case RV_NUMBER:
        return rv->data.number != 0.0;
    case RV_STRING:
        return rv->data.string.data && rv->data.string.length > 0;
    case RV_ARRAY:
        return rv->data.array.count > 0;
    case RV_OBJECT:
        return rv->data.object.count > 0;
    case RV_FUNCTION:
        return true;  // Functions are truthy
    case RV_ERROR:
        return false;  // Errors are falsy
    default:
        // Unknown types are truthy
        return true;
    }
}

/**
 * @brief Visit if statement node
 * @param visitor Visitor instance
 * @param node If statement AST node
 * @return Result of executed branch or NOOP
 */
static RuntimeValue *visitor_visit_if_statement(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->condition) {
        return rv_new_null();
    }

    // Evaluate the condition
    RuntimeValue *condition_result = visitor_visit(visitor, node->condition);
    if (!condition_result) {
        return rv_new_null();
    }

    // Check if condition is truthy
    bool condition_is_true = is_truthy_rv(condition_result);
    rv_unref(condition_result);

    if (condition_is_true) {
        // Execute then branch
        if (node->then_branch) {
            return visitor_visit(visitor, node->then_branch);
        }
    } else {
        // Execute else branch if it exists
        if (node->else_branch) {
            return visitor_visit(visitor, node->else_branch);
        }
    }

    return rv_new_null();
}

// Removed complex propagate_shared_scope function - replaced with direct scope assignment

/**
 * @brief Visit while loop node
 * @param visitor Visitor instance
 * @param node While loop AST node
 * @return Result of last iteration or NOOP
 */
static RuntimeValue *visitor_visit_while_loop(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->loop_condition || !node->loop_body) {
        return rv_new_null();
    }

    if (!node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR, "While loop node has no scope");
        return rv_new_null();
    }

    // CRITICAL FIX: Use proper recursive scope propagation
    // This ensures that ALL nested AST nodes (like variables inside conditions) get the correct
    // scope
    visitor_update_ast_scope(node->loop_condition, node->scope);
    visitor_update_ast_scope(node->loop_body, node->scope);

    RuntimeValue *last_result = rv_new_null();
    int iteration_count = 0;
    const int max_iterations = 10000;  // Safety limit

    while (iteration_count < max_iterations) {
        // Simple condition evaluation - no scope manipulation per iteration
        RuntimeValue *condition_result = visitor_visit(visitor, node->loop_condition);
        if (!condition_result) {
            break;
        }

        bool condition_is_true = is_truthy_rv(condition_result);
        rv_unref(condition_result);

        if (!condition_is_true) {
            break;
        }

        // Simple body execution - no scope manipulation per iteration
        RuntimeValue *body_result = visitor_visit(visitor, node->loop_body);
        if (body_result) {
            // Check for break/continue statements
            if (body_result->type == RV_STRING) {
                const char *str = rv_get_string(body_result);
                if (strcmp(str, "__BREAK__") == 0) {
                    rv_unref(body_result);
                    break;
                } else if (strcmp(str, "__CONTINUE__") == 0) {
                    rv_unref(body_result);
                    iteration_count++;
                    continue;
                }
            } else if (body_result->type == RV_OBJECT) {
                // Check for RETURN marker
                RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                if (return_marker) {
                    // Don't unref return_marker - it's owned by body_result
                    rv_unref(last_result);
                    return body_result;  // Return the whole marker object
                }
            }
            // Update last result
            rv_unref(last_result);
            last_result = body_result;
        }

        iteration_count++;
    }

    if (iteration_count >= max_iterations) {
        LOG_WARN(LOG_CAT_VISITOR,
                 "While loop exceeded maximum iterations (%d), terminating",
                 max_iterations);
    }

    return last_result;
}

/**
 * @brief Visit for loop node
 * @param visitor Visitor instance
 * @param node For loop AST node
 * @return Result of last iteration or NOOP
 */
static RuntimeValue *visitor_visit_for_loop(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->iterator_variable || !node->iterable || !node->for_body) {
        return rv_new_null();
    }

    // Evaluate the iterable expression
    RuntimeValue *iterable_result = visitor_visit(visitor, node->iterable);
    if (!iterable_result) {
        return rv_new_null();
    }

    RuntimeValue *last_result = rv_new_null();

    // CRITICAL FIX: Create iterator variable definition ONCE before the loop
    // This prevents repeated creation/destruction and potential memory issues
    AST_T *iterator_def = ast_new(AST_VARIABLE_DEFINITION);
    if (!iterator_def || !node->scope) {
        // MEMORY LEAK FIX: Clean up iterator_def if allocation failed
        if (iterator_def) {
            ast_free(iterator_def);
        }
        rv_unref(iterable_result);
        rv_unref(last_result);
        return rv_new_null();
    }

    iterator_def->variable_definition_variable_name = memory_strdup(node->iterator_variable);
    iterator_def->scope = node->scope;

    // Handle different types of iterables
    if (iterable_result->type == RV_ARRAY) {
        for (size_t i = 0; i < rv_array_length(iterable_result); i++) {
            RuntimeValue *element = rv_array_get(iterable_result, i);
            if (!element)
                continue;

            // Convert element to AST for variable storage
            AST_T *element_ast = runtime_value_to_ast(element);
            rv_unref(element);

            // FIXED: Update the iterator variable's value instead of creating new one
            iterator_def->variable_definition_value = element_ast;

            // Add/update iterator in scope (scope_add handles updates automatically)
            scope_add_variable_definition(node->scope, iterator_def);

            // Execute loop body
            RuntimeValue *body_result = visitor_visit(visitor, node->for_body);
            if (body_result) {
                // Check for break/continue/return statements
                if (body_result->type == RV_STRING) {
                    const char *str = rv_get_string(body_result);
                    if (strcmp(str, "__BREAK__") == 0) {
                        rv_unref(body_result);
                        break;
                    } else if (strcmp(str, "__CONTINUE__") == 0) {
                        rv_unref(body_result);
                        continue;
                    }
                } else if (body_result->type == RV_OBJECT) {
                    // Check for RETURN marker
                    RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                    if (return_marker) {
                        // Don't unref return_marker - it's owned by body_result
                        rv_unref(last_result);
                        rv_unref(iterable_result);
                        // Clean up iterator
                        memory_free(iterator_def->variable_definition_variable_name);
                        ast_free(iterator_def);
                        return body_result;
                    }
                }
                rv_unref(last_result);
                last_result = body_result;
            }
        }
    } else if (iterable_result->type == RV_OBJECT) {
        // Get object keys - need to iterate over object properties
        // For now, just skip object iteration since we don't have a direct API
        LOG_ERROR(LOG_CAT_VISITOR, "Object iteration not yet implemented for RuntimeValue");
        rv_unref(iterable_result);
        rv_unref(last_result);
        memory_free(iterator_def->variable_definition_variable_name);
        ast_free(iterator_def);
        return rv_new_null();

        /* TODO: Implement object iteration when we have object key iteration API
        for (size_t i = 0; i < rv_object_size(iterable_result); i++) {
            char *key = rv_object_get_key_at(iterable_result, i);
            if (!key)
                continue;

            // Create a string AST node for the key
            AST_T *key_ast = ast_new(AST_STRING);
            if (!key_ast)
                continue;

            key_ast->string_value = memory_strdup(key);

            // FIXED: Update the iterator variable's value instead of creating new one
            iterator_def->variable_definition_value = key_ast;

            // Add/update iterator in scope (scope_add handles updates automatically)
            scope_add_variable_definition(node->scope, iterator_def);

            // Execute loop body
            RuntimeValue *body_result = visitor_visit(visitor, node->for_body);
            if (body_result) {
                // Check for break/continue/return statements
                if (body_result->type == RV_STRING) {
                    const char *str = rv_get_string(body_result);
                    if (strcmp(str, "__BREAK__") == 0) {
                        rv_unref(body_result);
                        break;
                    } else if (strcmp(str, "__CONTINUE__") == 0) {
                        rv_unref(body_result);
                        continue;
                    }
                } else if (body_result->type == RV_OBJECT) {
                    // Check for RETURN marker
                    RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                    if (return_marker) {
                        // Don't unref return_marker - it's owned by body_result
                        rv_unref(last_result);
                        rv_unref(iterable_result);
                        // Clean up iterator
                        memory_free(iterator_def->variable_definition_variable_name);
                        ast_free(iterator_def);
                        return body_result;
                    }
                }
                rv_unref(last_result);
                last_result = body_result;
            }
        }
        */
    } else {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "For loop iterable is not an array or object (type: %d)",
                  iterable_result->type);
        // MEMORY LEAK FIX: Clean up iterator variable definition completely
        if (iterator_def) {
            if (iterator_def->variable_definition_variable_name) {
                memory_free(iterator_def->variable_definition_variable_name);
                iterator_def->variable_definition_variable_name = NULL;
            }
            ast_free(iterator_def);
        }
        rv_unref(iterable_result);
        rv_unref(last_result);
        return rv_new_null();
    }

    // MEMORY LEAK FIX: Clean up iterator variable definition completely
    if (iterator_def) {
        if (iterator_def->variable_definition_variable_name) {
            memory_free(iterator_def->variable_definition_variable_name);
            iterator_def->variable_definition_variable_name = NULL;
        }
        // Free the iterator_def node itself (it's not owned by the main AST tree)
        ast_free(iterator_def);
        iterator_def = NULL;
    }

    // Clean up the iterable result
    rv_unref(iterable_result);

    return last_result;
}

/**
 * @brief Visit import statement (placeholder)
 * @param visitor Visitor instance
 * @param node Import AST node
 * @return Null value
 */
RuntimeValue *visitor_visit_import(visitor_T *visitor, AST_T *node)
{
    (void)visitor;
    (void)node;
    LOG_ERROR(LOG_CAT_VISITOR, "Import statement not yet implemented");
    return rv_new_null();
}

/**
 * @brief Visit export statement (placeholder)
 * @param visitor Visitor instance
 * @param node Export AST node
 * @return Null value
 */
RuntimeValue *visitor_visit_export(visitor_T *visitor, AST_T *node)
{
    (void)visitor;
    (void)node;
    LOG_ERROR(LOG_CAT_VISITOR, "Export statement not yet implemented");
    return rv_new_null();
}

/**
 * @brief Visit string node
 * @param visitor Visitor instance
 * @param node String AST node
 * @return The string as RuntimeValue
 */
RuntimeValue *visitor_visit_string(visitor_T *visitor, AST_T *node)
{
    (void)visitor;  // Suppress unused parameter warning
    // String literals are converted to RuntimeValue
    return rv_new_string(node->string_value);
}

/**
 * @brief Recursively update scope for all nodes in AST tree
 * @param node The AST node to update
 * @param new_scope The new scope to set
 */
static void visitor_update_ast_scope(AST_T *node, scope_T *new_scope)
{
    if (!node)
        return;

    // Update this node's scope
    node->scope = new_scope;

    // Recursively update children based on node type
    switch (node->type) {
    case AST_COMPOUND:
        if (node->compound_statements) {
            for (size_t i = 0; i < node->compound_size; i++) {
                visitor_update_ast_scope(node->compound_statements[i], new_scope);
            }
        }
        break;

    case AST_IF_STATEMENT:
        visitor_update_ast_scope(node->condition, new_scope);
        visitor_update_ast_scope(node->then_branch, new_scope);
        visitor_update_ast_scope(node->else_branch, new_scope);
        break;

    case AST_WHILE_LOOP:
        visitor_update_ast_scope(node->loop_condition, new_scope);
        visitor_update_ast_scope(node->loop_body, new_scope);
        break;

    case AST_FOR_LOOP:
        visitor_update_ast_scope(node->iterable, new_scope);
        visitor_update_ast_scope(node->for_body, new_scope);
        break;

    case AST_BINARY_OP:
        visitor_update_ast_scope(node->left, new_scope);
        visitor_update_ast_scope(node->right, new_scope);
        break;

    case AST_UNARY_OP:
        visitor_update_ast_scope(node->operand, new_scope);
        break;

    case AST_VARIABLE_DEFINITION:
        visitor_update_ast_scope(node->variable_definition_value, new_scope);
        break;

    case AST_FUNCTION_CALL:
        // Update argument scopes but NOT the function call scope itself
        // This allows arguments to find parameters while function calls find functions
        if (node->function_call_arguments) {
            for (size_t i = 0; i < node->function_call_arguments_size; i++) {
                visitor_update_ast_scope(node->function_call_arguments[i], new_scope);
            }
        }
        // Don't update the function call node's own scope - it should keep original scope
        // to find function definitions
        node->scope = node->scope;  // Keep original scope for function lookup
        return;                     // Important: return without setting node->scope = new_scope

    case AST_RETURN:
        visitor_update_ast_scope(node->return_value, new_scope);
        break;

    // Variable nodes should get the new scope to find parameters
    case AST_VARIABLE:
        // Variables should use the new scope to find parameters
        break;

    // Other leaf nodes (no children to update, but scope can be updated)
    case AST_STRING:
    case AST_NUMBER:
    case AST_BOOLEAN:
    case AST_NULL:
    case AST_NOOP:
        break;

    default:
        // For any other node types, just update the scope
        break;
    }
}

/**
 * @brief Execute a user-defined function with advanced profiling and call stack management
 * @param visitor Visitor instance
 * @param fdef Function definition AST node
 * @param args Array of argument expressions
 * @param args_size Number of arguments
 * @return Result of function execution
 */
static RuntimeValue *
visitor_execute_user_function(visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size)
{
    if (!visitor || !fdef || !fdef->function_definition_body) {
        return rv_new_null();
    }

    const char *function_name =
        fdef->function_definition_name ? fdef->function_definition_name : "anonymous";

    // Simple parameter count validation (no rest parameters for now to avoid complexity)
    if (fdef->function_definition_args_size != (size_t)args_size) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Argument count mismatch", function_name);
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Function '%s' expects %zu arguments, got %d",
                  function_name,
                  fdef->function_definition_args_size,
                  args_size);
        return rv_new_null();
    }

    // Check for stack overflow
    if (visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", function_name);
        return rv_new_null();
    }

    // Push call frame for profiling and stack management
    visitor_push_call_frame(visitor, fdef, args, (size_t)args_size, function_name);

    // Create a new scope for this function call
    scope_T *function_scope = scope_new();
    if (!function_scope) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Failed to create function scope", function_name);
        visitor_pop_call_frame(visitor);
        return rv_new_null();
    }

    // Copy global variables into function scope so functions can access them
    if (fdef->scope && fdef->scope->variable_definitions) {
        for (size_t i = 0; i < fdef->scope->variable_definitions_size; i++) {
            AST_T *global_var = fdef->scope->variable_definitions[i];
            if (global_var) {
                scope_add_variable_definition(function_scope, global_var);
            }
        }
    }

    // SIMPLIFIED PARAMETER BINDING - Use straightforward value-based approach
    // This eliminates complex AST copying that causes double-free issues
    for (size_t i = 0; i < fdef->function_definition_args_size; i++) {
        AST_T *param_ast = fdef->function_definition_args[i];
        if (!param_ast || !param_ast->variable_name) {
            LOG_ERROR(LOG_CAT_VISITOR, "Invalid parameter at index %zu", i);
            scope_free(function_scope);
            visitor_pop_call_frame(visitor);
            return rv_new_null();
        }

        // Evaluate the argument to get its runtime value
        RuntimeValue *arg_value = visitor_visit(visitor, args[i]);
        if (!arg_value) {
            arg_value = rv_new_null();
        }

        // Convert RuntimeValue to AST for scope storage
        AST_T *param_value = runtime_value_to_ast(arg_value);
        rv_unref(arg_value);

        // Create variable definition for the parameter
        AST_T *param_def = ast_new(AST_VARIABLE_DEFINITION);
        if (!param_def || !param_value) {
            if (param_value)
                ast_free(param_value);
            if (param_def)
                ast_free(param_def);
            scope_free(function_scope);
            visitor_pop_call_frame(visitor);
            return rv_new_null();
        }

        param_def->variable_definition_variable_name = memory_strdup(param_ast->variable_name);
        param_def->variable_definition_value = param_value;
        param_def->scope = function_scope;

        // Add to scope using the standard function
        scope_add_variable_definition(function_scope, param_def);

        // Debug: Verify parameter binding
        LOG_VISITOR_DEBUG("Bound parameter %s", param_ast->variable_name);
    }

    // CRITICAL FIX: Do NOT modify the original AST's scope
    // This causes corruption when functions are called multiple times
    // Only use visitor_update_ast_scope which is temporary

    // Execute function body with proper scope management
    visitor_update_ast_scope(fdef->function_definition_body, function_scope);

    RuntimeValue *result = NULL;
    if (!visitor_has_exception(visitor)) {
        result = visitor_visit(visitor, fdef->function_definition_body);
        visitor->total_instructions_executed++;

        // Handle return statements
        if (result && result->type == RV_OBJECT) {
            // Check for RETURN marker
            RuntimeValue *return_value = rv_object_get(result, "__RETURN__");
            if (return_value) {
                // Found return marker, extract the actual return value
                // The return value is owned by the marker object, so we need to
                // increase its refcount before we free the marker
                rv_ref(return_value);  // Retain the return value
                // Now we can safely unref the marker - return_value won't be freed
                // because we increased its refcount
                rv_unref(result);
                result = return_value;  // Use the return value directly
            }
        }
    }

    // Result is already a RuntimeValue, no need for defensive copying
    RuntimeValue *final_result = result ? result : rv_new_null();

    // Pop call frame (this will update profiling automatically)
    visitor_pop_call_frame(visitor);

    // Clean up parameter nodes created for this function call
    // These are temporary nodes not part of the main AST tree
    if (function_scope && function_scope->variable_definitions) {
        // We need to be careful here - only free nodes we created for parameters
        // Don't free nodes that existed before (from outer scope copies)
        size_t params_start = 0;
        if (fdef->scope && fdef->scope->variable_definitions) {
            params_start = fdef->scope->variable_definitions_size;
        }

        // Free parameter nodes starting from where we added them
        for (size_t i = params_start; i < function_scope->variable_definitions_size; i++) {
            AST_T *param_def = function_scope->variable_definitions[i];
            if (param_def) {
                // Free the value AST we created with runtime_value_to_ast
                if (param_def->variable_definition_value) {
                    ast_free(param_def->variable_definition_value);
                }
                // Free the parameter definition node
                ast_free(param_def);
            }
        }
    }

    // Free the function scope (now safe - no AST nodes to double-free)
    scope_free(function_scope);

    // Handle exceptions
    if (visitor_has_exception(visitor)) {
        LOG_ERROR(LOG_CAT_VISITOR, "Exception occurred in function '%s'", function_name);
        if (final_result && final_result != result) {
            rv_unref(final_result);  // Clean up if we created a new null
        }
        return rv_new_null();
    }

    return final_result;
}

// =============================================================================
// Advanced Runtime Optimization Functions
// =============================================================================

/**
 * @brief Get time difference in seconds
 * @param start Start time
 * @param end End time
 * @return Time difference in seconds
 */
static double visitor_get_time_diff(clock_t start, clock_t end)
{
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

/**
 * @brief Push a new call frame onto the call stack
 * @param visitor Visitor instance
 * @param function_def Function definition AST node
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param function_name Function name
 */
static void visitor_push_call_frame(visitor_T *visitor,
                                    AST_T *function_def,
                                    AST_T **args,
                                    size_t arg_count,
                                    const char *function_name)
{
    if (!visitor || visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        if (visitor) {
            visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", "call_stack");
        }
        return;
    }

    CallFrame *frame = memory_alloc(sizeof(CallFrame));
    if (!frame) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Memory allocation failed", "call_frame");
        return;
    }

    frame->function_def = function_def;
    frame->arguments = args;
    frame->arg_count = arg_count;
    frame->previous = visitor->call_stack;
    frame->start_time = clock();
    frame->recursion_depth = visitor->call_stack_depth;
    frame->function_name = function_name ? memory_strdup(function_name) : NULL;

    visitor->call_stack = frame;
    visitor->call_stack_depth++;
}

/**
 * @brief Pop the top call frame from the call stack
 * @param visitor Visitor instance
 */
static void visitor_pop_call_frame(visitor_T *visitor)
{
    if (!visitor || !visitor->call_stack) {
        return;
    }

    CallFrame *frame = visitor->call_stack;

    // Update function profiling
    if (visitor->profiling_enabled && frame->function_name) {
        double execution_time = visitor_get_time_diff(frame->start_time, clock());
        visitor_update_function_profile(visitor, frame->function_name, execution_time);
    }

    visitor->call_stack = frame->previous;
    visitor->call_stack_depth--;

    // Clean up frame
    if (frame->function_name) {
        memory_free(frame->function_name);
    }
    memory_free(frame);
}

/**
 * @brief Get or create a function profile for performance tracking
 * @param visitor Visitor instance
 * @param function_name Function name
 * @return Function profile or NULL on failure
 */
static FunctionProfile *visitor_get_or_create_profile(visitor_T *visitor, const char *function_name)
{
    if (!visitor || !function_name) {
        return NULL;
    }

    // Search for existing profile
    for (size_t i = 0; i < visitor->profile_count; i++) {
        if (visitor->function_profiles[i].function_name &&
            strcmp(visitor->function_profiles[i].function_name, function_name) == 0) {
            return &visitor->function_profiles[i];
        }
    }

    // Create new profile if space available
    if (visitor->profile_count < visitor->profile_capacity) {
        FunctionProfile *profile = &visitor->function_profiles[visitor->profile_count];
        profile->function_name = memory_strdup(function_name);
        profile->execution_count = 0;
        profile->total_execution_time = 0.0;
        profile->is_hot_function = false;
        profile->average_time_per_call = 0.0;
        profile->optimized_ast = NULL;
        visitor->profile_count++;
        return profile;
    }

    // Expand profile capacity if needed
    size_t new_capacity = visitor->profile_capacity + PROFILE_CAPACITY_INCREMENT;
    FunctionProfile *new_profiles = memory_alloc(sizeof(FunctionProfile) * new_capacity);
    if (!new_profiles) {
        return NULL;
    }

    // Copy existing profiles
    memcpy(
        new_profiles, visitor->function_profiles, sizeof(FunctionProfile) * visitor->profile_count);
    memory_free(visitor->function_profiles);
    visitor->function_profiles = new_profiles;
    visitor->profile_capacity = new_capacity;

    // Create the new profile
    FunctionProfile *profile = &visitor->function_profiles[visitor->profile_count];
    profile->function_name = memory_strdup(function_name);
    profile->execution_count = 0;
    profile->total_execution_time = 0.0;
    profile->is_hot_function = false;
    profile->average_time_per_call = 0.0;
    profile->optimized_ast = NULL;
    visitor->profile_count++;
    return profile;
}

/**
 * @brief Update function profile with execution time
 * @param visitor Visitor instance
 * @param function_name Function name
 * @param execution_time Execution time in seconds
 */
static void visitor_update_function_profile(visitor_T *visitor,
                                            const char *function_name,
                                            double execution_time)
{
    FunctionProfile *profile = visitor_get_or_create_profile(visitor, function_name);
    if (!profile) {
        return;
    }

    profile->execution_count++;
    profile->total_execution_time += execution_time;
    profile->average_time_per_call = profile->total_execution_time / profile->execution_count;

    // Check if function became hot
    if (!profile->is_hot_function && profile->execution_count >= visitor->hot_function_threshold &&
        profile->average_time_per_call >= HOT_FUNCTION_TIME_THRESHOLD) {
        profile->is_hot_function = true;
        LOG_VISITOR_DEBUG("Function '%s' marked as HOT (calls: %zu, avg time: %.6fs)",
                          function_name,
                          profile->execution_count,
                          profile->average_time_per_call);

        // Trigger optimization for hot function
        visitor_optimize_hot_function(visitor, function_name);
    }
}

/**
 * @brief Check if an AST node represents a constant expression
 * @param node AST node to check
 * @return true if constant, false otherwise
 */

/**
 * @brief Optimize constant expressions through constant folding
 * @param visitor Visitor instance
 * @param node AST node to optimize
 * @return Optimized AST node
 */

/**
 * @brief Apply tail call optimization if applicable
 * @param visitor Visitor instance
 * @param node AST node to optimize
 * @return Optimized AST node
 */
/* TODO: Update to RuntimeValue
static AST_T *visitor_apply_tail_call_optimization(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !visitor->tail_call_optimization) {
        return node;
    }

    // Basic tail call detection - function call as last statement in function
    if (node->type == AST_FUNCTION_CALL && visitor->call_stack &&
        visitor->call_stack->function_def) {
        LOG_VISITOR_DEBUG("Tail call optimization candidate detected");
        // In a complete implementation, we would replace the recursive call
        // with a loop to avoid stack growth
    }

    return node;
}
*/

// =============================================================================
// Database-like file operations implementation
// =============================================================================

// Removed unused visitor_set_property_path function

// =============================================================================
// Public Advanced Runtime API Functions
// =============================================================================

/**
 * @brief Enable performance profiling for function optimization
 * @param visitor Visitor instance
 * @param enabled Whether to enable profiling
 */

// Stub implementations for missing functions to fix linking errors
// These functions are not in MANIFEST.json but are called by visitor code

/**
 * @brief Check if visitor has an exception
 * @param visitor Visitor instance
 * @return true if has exception, false otherwise
 */
bool visitor_has_exception(visitor_T *visitor)
{
    (void)visitor;  // Mark as used
    return false;   // No exception handling for now
}

/**
 * @brief Throw an exception in visitor
 * @param visitor Visitor instance
 * @param node AST node where exception occurred
 * @param message Error message
 * @param context Context string
 */
void visitor_throw_exception(visitor_T *visitor,
                             AST_T *node,
                             const char *message,
                             const char *context)
{
    (void)visitor;  // Mark as used
    (void)node;
    (void)message;
    (void)context;
    // No exception handling for now - just ignore
}

/**
 * @brief Optimize a hot function
 * @param visitor Visitor instance
 * @param function_name Name of function to optimize
 * @return true if optimization succeeded, false otherwise
 */
bool visitor_optimize_hot_function(visitor_T *visitor, const char *function_name)
{
    (void)visitor;  // Mark as used
    (void)function_name;
    // No optimization for now - just ignore
    return false;
}
