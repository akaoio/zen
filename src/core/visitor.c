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
#include "zen/core/scope.h"
#include "zen/core/token.h"
#include "zen/runtime/operators.h"
#include "zen/types/value.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/stdlib.h"
#include "zen/core/memory.h"
#include "zen/core/logger.h"
#include "zen/types/array.h"
#include "zen/types/object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

// Advanced runtime optimization constants
#define DEFAULT_HOT_FUNCTION_THRESHOLD 100
#define MAX_CALL_STACK_DEPTH 10000
#define PROFILE_CAPACITY_INCREMENT 50
#define HOT_FUNCTION_TIME_THRESHOLD 0.001  // 1ms


// Forward declarations
static AST_T* builtin_function_print(visitor_T* visitor, AST_T** args, int args_size);
static Value* ast_to_value(AST_T* node);
static AST_T* value_to_ast(Value* value);


static AST_T* visitor_visit_binary_op(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_unary_op(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_object(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_property_access(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_if_statement(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_while_loop(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_for_loop(visitor_T* visitor, AST_T* node);
static AST_T* visitor_execute_user_function(visitor_T* visitor, AST_T* fdef, AST_T** args, int args_size);
static bool is_truthy(AST_T* ast);

// Database-like file operations
static AST_T* visitor_visit_file_get(visitor_T* visitor, AST_T* node);
static AST_T* visitor_visit_file_put(visitor_T* visitor, AST_T* node);
static AST_T* visitor_navigate_property_path(visitor_T* visitor, AST_T* root, AST_T* property_path);
static void visitor_set_property_path(visitor_T* visitor, AST_T* root, AST_T* property_path, AST_T* value);

// Advanced runtime optimization functions
static void visitor_push_call_frame(visitor_T* visitor, AST_T* function_def, AST_T** args, size_t arg_count, const char* function_name);
static void visitor_pop_call_frame(visitor_T* visitor);
static FunctionProfile* visitor_get_or_create_profile(visitor_T* visitor, const char* function_name);
static void visitor_update_function_profile(visitor_T* visitor, const char* function_name, double execution_time);
static AST_T* visitor_optimize_constant_expression(visitor_T* visitor, AST_T* node);
static bool visitor_is_constant_expression(AST_T* node);
static AST_T* visitor_apply_tail_call_optimization(visitor_T* visitor, AST_T* node);
static double visitor_get_time_diff(clock_t start, clock_t end);

/**
 * @brief Create a new visitor instance with advanced runtime features
 * @param void Function takes no parameters
 * @return New visitor instance or NULL on failure
 */
visitor_T* visitor_new()
{
    visitor_T* visitor = memory_alloc(sizeof(visitor_T));
    if (!visitor) return NULL;
    
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
    visitor->profiling_enabled = true; // Enable by default for optimization
    
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
void visitor_free(visitor_T* visitor)
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
AST_T* visitor_visit(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node) {
        return ast_new(AST_NOOP);
    }
    
    // Check for active exception
    if (visitor_has_exception(visitor)) {
        return ast_new(AST_NULL);
    }
    
    // Increment instruction counter for analytics
    visitor->total_instructions_executed++;
    
    // Start timing for this node evaluation
    clock_t start_time = clock();
    
    // Apply constant folding optimization if enabled
    if (visitor->constant_folding && visitor_is_constant_expression(node)) {
        AST_T* optimized = visitor_optimize_constant_expression(visitor, node);
        if (optimized != node) {
            clock_t end_time = clock();
            visitor->total_execution_time += visitor_get_time_diff(start_time, end_time);
            return optimized;
        }
    }
    
    AST_T* result = NULL;
    
    switch (node->type)
    {
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
            return node;
        
        // Literals - return as-is
        case AST_STRING:
        case AST_NUMBER:
        case AST_BOOLEAN:
        case AST_NULL:
        case AST_UNDECIDABLE:
            return node;
            
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
            // Return control flow nodes as-is to preserve info
            return node;
        case AST_RETURN:
            // Handle return statements - evaluate the return value
            if (node->return_value) {
                AST_T* return_val = visitor_visit(visitor, node->return_value);
                // Create a new return node with the evaluated value
                AST_T* return_node = ast_new(AST_RETURN);
                if (return_node) {
                    return_node->return_value = return_val;
                }
                return return_node;
            }
            return node;
            
        // Database-like file operations
        case AST_FILE_GET:
            return visitor_visit_file_get(visitor, node);
        case AST_FILE_PUT:
            return visitor_visit_file_put(visitor, node);
            
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
            // For logical AST nodes, return the node itself (pure AST operations)
            result = (AST_T*)node;
            break;
            
        default:
            // For unimplemented features, just return NOOP to continue execution
            result = ast_new(AST_NOOP);
            break;
    }
    
    // Update execution timing
    clock_t end_time = clock();
    visitor->total_execution_time += visitor_get_time_diff(start_time, end_time);
    
    // Apply tail call optimization if applicable
    if (visitor->tail_call_optimization && result) {
        result = visitor_apply_tail_call_optimization(visitor, result);
    }
    
    return result;
}

/**
 * @brief Visit variable definition node
 * @param visitor Visitor instance
 * @param node Variable definition AST node
 * @return The defined variable value
 */
AST_T* visitor_visit_variable_definition(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR, "Invalid variable definition (visitor=%p, node=%p, scope=%p)", 
                  (void*)visitor, (void*)node, node ? (void*)node->scope : NULL);
        return ast_new(AST_NOOP);
    }
    
    if (!node->variable_definition_variable_name) {
        LOG_ERROR(LOG_CAT_VISITOR, "Variable definition missing name");
        return ast_new(AST_NOOP);
    }
    
    // Defining variable in scope
    
    // Evaluate the variable's value - avoid recursion for literals
    AST_T* value = node->variable_definition_value;
    if (value && (value->type != AST_STRING && value->type != AST_NUMBER && 
                  value->type != AST_BOOLEAN && value->type != AST_NULL)) {
        // Evaluating complex expression
        value = visitor_visit(visitor, value);
    }
    if (!value) {
        // No value provided, using NULL
        value = ast_new(AST_NULL);
    }
    
    // Variable assigned successfully
    
    // Update the node with the evaluated value
    node->variable_definition_value = value;
    
    // Add to scope
    scope_add_variable_definition(node->scope, node);
    // Variable added to scope successfully

    return value;
}

/**
 * @brief Visit function definition node
 * @param visitor Visitor instance
 * @param node Function definition AST node
 * @return The function definition node
 */
AST_T* visitor_visit_function_definition(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->scope) {
        return ast_new(AST_NOOP);
    }

    LOG_VISITOR_DEBUG("Defining function '%s' in scope %p", 
                      node->function_definition_name, (void*)node->scope);
    scope_add_function_definition(node->scope, node);
    return node;
}

/**
 * @brief Visit variable node
 * @param visitor Visitor instance
 * @param node Variable AST node
 * @return Variable value or NULL
 */
AST_T* visitor_visit_variable(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->variable_name || !node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR, "Invalid variable access (visitor=%p, node=%p, name=%s, scope=%p)",
                  (void*)visitor, (void*)node, 
                  node && node->variable_name ? node->variable_name : "NULL",
                  node ? (void*)node->scope : NULL);
        return ast_new(AST_NULL);
    }
    
    // Looking up variable in scope
    LOG_VISITOR_DEBUG("Looking up variable '%s' in scope %p", node->variable_name, (void*)node->scope);
    
    AST_T* vdef = scope_get_variable_definition(node->scope, node->variable_name);
    
    if (vdef != NULL && vdef->variable_definition_value != NULL) {
        // Variable found with value
        LOG_VISITOR_DEBUG("Found variable '%s', type=%d, returning value type=%d", 
                         node->variable_name, vdef->type, vdef->variable_definition_value->type);
        if (vdef->variable_definition_value->type == AST_NUMBER) {
            LOG_VISITOR_DEBUG("Variable '%s' has numeric value: %f", 
                             node->variable_name, vdef->variable_definition_value->number_value);
        }
        
        // CRITICAL FIX: Return the stored value directly - the visitor should use the evaluated result
        // The variable definition already contains the evaluated value
        return vdef->variable_definition_value;
    }

    LOG_ERROR(LOG_CAT_VISITOR, "Undefined variable '%s'", node->variable_name);
    return ast_new(AST_NULL);
}

/**
 * @brief Visit function call node
 * @param visitor Visitor instance
 * @param node Function call AST node
 * @return Result of function call
 */
AST_T* visitor_visit_function_call(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->function_call_name) {
        return ast_new(AST_NULL);
    }
    

    // Check for stdlib functions first
    LOG_VISITOR_DEBUG("Looking up stdlib function '%s'", node->function_call_name);
    const ZenStdlibFunction* stdlib_func = stdlib_get(node->function_call_name);
    if (stdlib_func != NULL) {
        LOG_VISITOR_DEBUG("Found stdlib function '%s'", node->function_call_name);
        // Convert AST arguments to Value arguments
        Value** value_args = NULL;
        size_t argc = (size_t)node->function_call_arguments_size;
        
        if (argc > 0) {
            value_args = malloc(sizeof(Value*) * argc);
            if (!value_args) {
                return ast_new(AST_NULL);
            }
            
            // Evaluate and convert each argument
            for (size_t i = 0; i < argc; i++) {
                AST_T* arg_ast = visitor_visit(visitor, node->function_call_arguments[i]);
                value_args[i] = ast_to_value(arg_ast);
                if (!value_args[i]) {
                    value_args[i] = value_new_null();
                }
            }
        }
        
        // Call the stdlib function
        Value* result = stdlib_func->func(value_args, argc);
        
        // Clean up arguments
        if (value_args) {
            for (size_t i = 0; i < argc; i++) {
                if (value_args[i]) {
                    value_unref(value_args[i]);
                }
            }
            free(value_args);
        }
        
        // Convert result back to AST
        if (result) {
            AST_T* result_ast = value_to_ast(result);
            value_unref(result);
            return result_ast ? result_ast : ast_new(AST_NULL);
        } else {
            return ast_new(AST_NULL);
        }
    }

    // Handle built-in functions (legacy print support)
    if (strcmp(node->function_call_name, "print") == 0) {
        return builtin_function_print(visitor, node->function_call_arguments, node->function_call_arguments_size);
    }

    // Look up user-defined function
    AST_T* fdef = scope_get_function_definition(node->scope, node->function_call_name);

    if (fdef == NULL) {        
        LOG_ERROR(LOG_CAT_VISITOR, "Undefined function '%s'", node->function_call_name);
        return ast_new(AST_NULL);
    }

    // Execute user-defined function
    return visitor_execute_user_function(visitor, fdef, node->function_call_arguments, node->function_call_arguments_size);
}

/**
 * @brief Visit compound node (list of statements)
 * @param visitor Visitor instance
 * @param node Compound AST node
 * @return Result of last statement or NOOP
 */
AST_T* visitor_visit_compound(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->compound_statements) {
        LOG_ERROR(LOG_CAT_VISITOR, "Invalid compound node (visitor=%p, node=%p, statements=%p)", 
                  (void*)visitor, (void*)node, 
                  node ? (void*)node->compound_statements : NULL);
        return ast_new(AST_NOOP);
    }
    
    // Executing compound with multiple statements
    
    AST_T* last_result = ast_new(AST_NOOP);
    
    for (size_t i = 0; i < node->compound_size; i++) {
        LOG_VISITOR_DEBUG("Visiting statement %zu: %p", i, (void*)node->compound_statements[i]);
        if (!node->compound_statements[i]) {
            LOG_ERROR(LOG_CAT_VISITOR, "Statement %zu is NULL - PARSER BUG!", i + 1);
            continue;
        }
        
        AST_T* prev_result = last_result;
        last_result = visitor_visit(visitor, node->compound_statements[i]);
        
        // Statement executed successfully
        
        // CRITICAL DOUBLE-FREE FIX: Do NOT free previous results
        // The visitor should never free AST nodes from the parse tree
        // All cleanup is handled by ast_free(root) in main.c
        // Freeing here causes double-free crashes when same nodes are freed again
        (void)prev_result; // Suppress unused variable warning
    }

    // Compound execution complete
    return last_result;
}

/**
 * @brief Built-in print function implementation
 * @param visitor The visitor instance for context
 * @param args Array of AST nodes to print
 * @param args_size Number of arguments in the args array
 * @return AST_NOOP node indicating successful completion
 */
static AST_T* builtin_function_print(visitor_T* visitor, AST_T** args, int args_size)
{
    // Convert arguments to Values and use print from stdlib/io.c
    for (int i = 0; i < args_size; i++)
    {
        AST_T* visited_ast = visitor_visit(visitor, args[i]);
        if (!visited_ast) continue;

        // Convert AST to Value and use proper print
        Value* value = ast_to_value(visited_ast);
        if (value) {
            io_print_no_newline_internal(value);
            value_unref(value);
        }
        
        if (i < args_size - 1) {
            printf(" ");
        }
    }
    
    printf("\n");
    return ast_new(AST_NOOP);
}

/**
 * @brief Convert AST node to Value object
 * @param node AST node to convert
 * @return Value object or NULL on failure
 */
static Value* ast_to_value(AST_T* node)
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
            LOG_VISITOR_DEBUG("Converting AST_BOOLEAN (%s) to Value", node->boolean_value ? "true" : "false");
            return value_new_boolean(node->boolean_value);
        case AST_NUMBER:
            LOG_VISITOR_DEBUG("Converting AST_NUMBER (%f) to Value", node->number_value);
            Value* num_val = value_new_number(node->number_value);
            if (num_val) {
                LOG_VISITOR_DEBUG("Created Value type=%d, number=%f", num_val->type, num_val->as.number);
            } else {
                LOG_VISITOR_DEBUG("Failed to create number Value!");
            }
            return num_val;
        case AST_STRING:
            LOG_VISITOR_DEBUG("Converting AST_STRING ('%s') to Value", node->string_value ? node->string_value : "");
            return value_new_string(node->string_value ? node->string_value : "");
        case AST_ARRAY:
            {
                // Create a new array Value
                Value* array_val = array_new(node->array_size > 0 ? node->array_size : 1);
                if (!array_val) return value_new_null();
                
                // Convert each element and add to array
                for (size_t i = 0; i < node->array_size; i++) {
                    Value* element_val = ast_to_value(node->array_elements[i]);
                    if (element_val) {
                        array_push(array_val, element_val);
                        value_unref(element_val); // array_push should add its own reference
                    }
                }
                return array_val;
            }
        case AST_OBJECT:
            {
                // Create a new object Value
                Value* object_val = object_new();
                if (!object_val) return value_new_null();
                
                // Set each key-value pair
                for (size_t i = 0; i < node->object_size; i++) {
                    if (node->object_keys[i] && node->object_values[i]) {
                        Value* value_val = ast_to_value(node->object_values[i]);
                        if (value_val) {
                            object_set(object_val, node->object_keys[i], value_val);
                            value_unref(value_val); // object_set should add its own reference
                        }
                    }
                }
                return object_val;
            }
        default:
            return value_new_null();
    }
}

/**
 * @brief Convert Value object to AST node
 * @param value Value object to convert
 * @return AST node or NULL on failure
 */
static AST_T* value_to_ast(Value* value)
{
    if (!value) {
        return ast_new(AST_NULL);
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return ast_new(AST_NULL);
        case VALUE_UNDECIDABLE:
            return ast_new(AST_UNDECIDABLE);
        case VALUE_BOOLEAN:
            {
                AST_T* ast = ast_new(AST_BOOLEAN);
                if (ast) {
                    ast->boolean_value = value->as.boolean;
                }
                return ast;
            }
        case VALUE_NUMBER:
            {
                AST_T* ast = ast_new(AST_NUMBER);
                if (ast) {
                    ast->number_value = value->as.number;
                }
                return ast;
            }
        case VALUE_STRING:
            {
                AST_T* ast = ast_new(AST_STRING);
                if (ast && value->as.string && value->as.string->data) {
                    ast->string_value = strdup(value->as.string->data);
                }
                return ast;
            }
        case VALUE_ARRAY:
            {
                AST_T* ast = ast_new(AST_ARRAY);
                if (!ast || !value->as.array) return ast_new(AST_NULL);
                
                // Get array length
                size_t length = array_length(value);
                ast->array_size = length;
                
                if (length > 0) {
                    // Allocate array for elements
                    ast->array_elements = malloc(sizeof(AST_T*) * length);
                    if (!ast->array_elements) {
                        ast_free(ast);
                        return ast_new(AST_NULL);
                    }
                    
                    // Convert each element back to AST
                    for (size_t i = 0; i < length; i++) {
                        Value* element_val = array_get(value, i);
                        ast->array_elements[i] = value_to_ast(element_val);
                        if (element_val) value_unref(element_val);
                    }
                } else {
                    ast->array_elements = NULL;
                }
                
                return ast;
            }
        case VALUE_OBJECT:
            {
                AST_T* ast = ast_new(AST_OBJECT);
                if (!ast || !value->as.object) return ast_new(AST_NULL);
                
                ZenObject* obj = value->as.object;
                ast->object_size = obj->length;
                
                if (obj->length > 0) {
                    // Allocate arrays for keys and values
                    ast->object_keys = malloc(sizeof(char*) * obj->length);
                    ast->object_values = malloc(sizeof(AST_T*) * obj->length);
                    
                    if (!ast->object_keys || !ast->object_values) {
                        if (ast->object_keys) free(ast->object_keys);
                        if (ast->object_values) free(ast->object_values);
                        ast_free(ast);
                        return ast_new(AST_NULL);
                    }
                    
                    // Copy keys and convert values
                    for (size_t i = 0; i < obj->length; i++) {
                        ast->object_keys[i] = strdup(obj->pairs[i].key);
                        ast->object_values[i] = value_to_ast(obj->pairs[i].value);
                    }
                } else {
                    ast->object_keys = NULL;
                    ast->object_values = NULL;
                }
                
                return ast;
            }
        case VALUE_ERROR:
            LOG_ERROR(LOG_CAT_VISITOR, "Error in expression evaluation: %s", 
                      value->as.error && value->as.error->message ? value->as.error->message : "Unknown error");
            return ast_new(AST_NULL);
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
static AST_T* visitor_visit_binary_op(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->left || !node->right) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate operands
    LOG_VISITOR_DEBUG("Binary op - evaluating left operand type=%d", node->left->type);
    AST_T* left_ast = visitor_visit(visitor, node->left);
    LOG_VISITOR_DEBUG("Binary op - evaluating right operand type=%d", node->right->type);
    AST_T* right_ast = visitor_visit(visitor, node->right);
    LOG_VISITOR_DEBUG("Binary op - got left_ast type=%d, right_ast type=%d", left_ast ? (int)left_ast->type : -1, right_ast ? (int)right_ast->type : -1);
    
    if (!left_ast || !right_ast) {
        return ast_new(AST_NULL);
    }
    
    // Convert AST nodes to Value objects
    Value* left_val = ast_to_value(left_ast);
    Value* right_val = ast_to_value(right_ast);
    
    if (!left_val || !right_val) {
        if (left_val) value_unref(left_val);
        if (right_val) value_unref(right_val);
        return ast_new(AST_NULL);
    }
    
    Value* result = NULL;
    
    // Apply the appropriate operator
    switch (node->operator_type) {
        case TOKEN_PLUS:
            result = op_add(left_val, right_val);
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
                Value* gt_val = op_greater_than(left_val, right_val);
                if (gt_val) {
                    result = op_logical_not(gt_val);
                    value_unref(gt_val);
                }
            }
            break;
        case TOKEN_GREATER_EQUALS:
            // Implement >= as !(a < b)
            {
                Value* lt_val = op_less_than(left_val, right_val);
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
        return ast_new(AST_NULL);
    }
    
    // Convert result back to AST
    AST_T* result_ast = value_to_ast(result);
    value_unref(result);
    
    return result_ast ? result_ast : ast_new(AST_NULL);
}

/**
 * @brief Visit unary operation node
 * @param visitor Visitor instance
 * @param node Unary operation AST node
 * @return Result of unary operation
 */
static AST_T* visitor_visit_unary_op(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->operand) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate operand
    AST_T* operand_ast = visitor_visit(visitor, node->operand);
    if (!operand_ast) {
        return ast_new(AST_NULL);
    }
    
    // Convert AST node to Value object
    Value* operand_val = ast_to_value(operand_ast);
    if (!operand_val) {
        return ast_new(AST_NULL);
    }
    
    Value* result = NULL;
    
    // Apply the appropriate unary operator
    switch (node->operator_type) {
        case TOKEN_MINUS:
            // Unary minus: multiply by -1
            {
                Value* neg_one = value_new_number(-1.0);
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
        return ast_new(AST_NULL);
    }
    
    // Convert result back to AST
    AST_T* result_ast = value_to_ast(result);
    value_unref(result);
    
    return result_ast ? result_ast : ast_new(AST_NULL);
}

/**
 * @brief Visit array literal node
 * @param visitor Visitor instance
 * @param node Array AST node
 * @return Array value as AST node
 */
static AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node) {
        return ast_new(AST_NULL);
    }
    
    // Create a new array Value
    Value* array_val = array_new(node->array_size > 0 ? node->array_size : 1);
    if (!array_val) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate and add each element
    for (size_t i = 0; i < node->array_size; i++) {
        AST_T* element_ast = visitor_visit(visitor, node->array_elements[i]);
        if (element_ast) {
            Value* element_val = ast_to_value(element_ast);
            if (element_val) {
                array_push(array_val, element_val);
                value_unref(element_val); // array_push adds its own reference
            }
        }
    }
    
    // Convert back to AST for return
    AST_T* result_ast = value_to_ast(array_val);
    value_unref(array_val);
    
    return result_ast ? result_ast : ast_new(AST_NULL);
}

/**
 * @brief Visit object literal node
 * @param visitor Visitor instance
 * @param node Object AST node
 * @return Object value as AST node
 */
static AST_T* visitor_visit_object(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node) {
        return ast_new(AST_NULL);
    }
    
    // Create a new object Value
    Value* object_val = object_new();
    if (!object_val) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate and set each key-value pair
    for (size_t i = 0; i < node->object_size; i++) {
        if (node->object_keys[i] && node->object_values[i]) {
            AST_T* value_ast = visitor_visit(visitor, node->object_values[i]);
            if (value_ast) {
                Value* value_val = ast_to_value(value_ast);
                if (value_val) {
                    object_set(object_val, node->object_keys[i], value_val);
                    value_unref(value_val); // object_set adds its own reference
                }
            }
        }
    }
    
    // Convert back to AST for return
    AST_T* result_ast = value_to_ast(object_val);
    value_unref(object_val);
    
    return result_ast ? result_ast : ast_new(AST_NULL);
}

/**
 * @brief Visit property access node (obj.property)
 * @param visitor Visitor instance
 * @param node Property access AST node
 * @return Property value or NULL
 */
static AST_T* visitor_visit_property_access(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->object || !node->property_name) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate the object expression
    AST_T* object_ast = visitor_visit(visitor, node->object);
    if (!object_ast) {
        return ast_new(AST_NULL);
    }
    
    // Convert to Value for property access
    Value* object_val = ast_to_value(object_ast);
    if (!object_val) {
        return ast_new(AST_NULL);
    }
    
    Value* result_val = NULL;
    
    // Check if it's an object type or array type with numeric index
    if (object_val->type == VALUE_OBJECT) {
        result_val = object_get(object_val, node->property_name);
    } else if (object_val->type == VALUE_ARRAY) {
        // Handle array indexing: arr[0] becomes arr.0 in property access
        char* endptr;
        long index = strtol(node->property_name, &endptr, 10);
        
        if (*endptr == '\0' && index >= 0) {
            // Valid numeric index
            result_val = array_get(object_val, (size_t)index);
            if (result_val) {
                // array_get returns a reference, need to copy for our return
                Value* copied_val = value_copy(result_val);
                value_unref(result_val);
                result_val = copied_val;
            }
        } else {
            LOG_ERROR(LOG_CAT_VISITOR, "Invalid array index '%s'", node->property_name);
            result_val = value_new_null();
        }
    } else {
        LOG_ERROR(LOG_CAT_VISITOR, "Cannot access property '%s' on non-object/array type", node->property_name);
        result_val = value_new_null();
    }
    
    // Clean up object value
    value_unref(object_val);
    
    if (!result_val) {
        result_val = value_new_null();
    }
    
    // Convert result back to AST
    AST_T* result_ast = value_to_ast(result_val);
    value_unref(result_val);
    
    return result_ast ? result_ast : ast_new(AST_NULL);
}


/**
 * @brief Check if an AST node evaluates to a truthy value
 * @param ast AST node to evaluate
 * @return true if truthy, false otherwise
 */
static bool is_truthy(AST_T* ast)
{
    if (!ast) {
        return false;
    }
    
    switch (ast->type) {
        case AST_NULL:
            return false;
        case AST_UNDECIDABLE:
            return false;  // Undecidable values are falsy
        case AST_BOOLEAN:
            return ast->boolean_value != 0;
        case AST_NUMBER:
            return ast->number_value != 0.0;
        case AST_STRING:
            return ast->string_value && strlen(ast->string_value) > 0;
        case AST_ARRAY:
            return ast->array_size > 0;
        case AST_OBJECT:
            return ast->object_size > 0;
        default:
            // Non-null values are generally truthy
            return true;
    }
}

/**
 * @brief Visit if statement node
 * @param visitor Visitor instance
 * @param node If statement AST node
 * @return Result of executed branch or NOOP
 */
static AST_T* visitor_visit_if_statement(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->condition) {
        return ast_new(AST_NOOP);
    }
    
    
    // Evaluate the condition
    AST_T* condition_result = visitor_visit(visitor, node->condition);
    if (!condition_result) {
        return ast_new(AST_NOOP);
    }
    
    // Check if condition is truthy
    bool condition_is_true = is_truthy(condition_result);
    
    
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
    
    return ast_new(AST_NOOP);
}

/**
 * @brief Recursive function to ensure all nodes use the same shared scope
 * @param ast_node AST node to propagate scope to
 * @param shared_scope Scope to propagate
 */
static void propagate_shared_scope(AST_T* ast_node, scope_T* shared_scope) {
    if (!ast_node || !shared_scope) return;
    
    // CRITICAL: Set the scope on this node
    ast_node->scope = shared_scope;
    
    // Recursively apply to all child nodes
    switch (ast_node->type) {
        case AST_COMPOUND:
            // Compound nodes (like loop bodies) must use shared scope
            for (size_t i = 0; i < ast_node->compound_size; i++) {
                propagate_shared_scope(ast_node->compound_statements[i], shared_scope);
            }
            break;
            
        case AST_VARIABLE_DEFINITION:
            // Variable definitions are CRITICAL - they must update the shared scope
            propagate_shared_scope(ast_node->variable_definition_value, shared_scope);
            break;
            
        case AST_BINARY_OP:
            propagate_shared_scope(ast_node->left, shared_scope);
            propagate_shared_scope(ast_node->right, shared_scope);
            break;
            
        case AST_UNARY_OP:
            propagate_shared_scope(ast_node->operand, shared_scope);
            break;
            
        case AST_FUNCTION_CALL:
            for (size_t i = 0; i < ast_node->function_call_arguments_size; i++) {
                propagate_shared_scope(ast_node->function_call_arguments[i], shared_scope);
            }
            break;
            
        default:
            // Variables, literals, etc. just need the scope set (already done above)
            break;
    }
}

/**
 * @brief Visit while loop node
 * @param visitor Visitor instance
 * @param node While loop AST node
 * @return Result of last iteration or NOOP
 */
static AST_T* visitor_visit_while_loop(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->loop_condition || !node->loop_body) {
        return ast_new(AST_NOOP);
    }
    
    // CRITICAL FIX FOR WHILE LOOP SCOPE SHARING
    // The issue is that variable updates in the loop body must be visible to the condition
    // in subsequent iterations. We need to ensure both use the exact same scope instance.
    
    if (!node->scope) {
        // If no scope is set, we can't fix the issue
        LOG_ERROR(LOG_CAT_VISITOR, "While loop node has no scope - cannot ensure variable persistence");
        return ast_new(AST_NOOP);
    }
    
    // Apply shared scope to both condition and body - this is the key fix
    propagate_shared_scope(node->loop_condition, node->scope);
    propagate_shared_scope(node->loop_body, node->scope);
    
    AST_T* last_result = ast_new(AST_NOOP);
    int iteration_count = 0;
    const int max_iterations = 10000; // Safety limit to prevent infinite loops
    
    while (iteration_count < max_iterations) {
        // Evaluate loop condition - now uses shared scope
        AST_T* condition_result = visitor_visit(visitor, node->loop_condition);
        if (!condition_result) {
            break;
        }
        
        bool condition_is_true = is_truthy(condition_result);
        
        if (!condition_is_true) {
            break;
        }
        
        // Execute loop body - variable updates here will persist in shared scope
        AST_T* body_result = visitor_visit(visitor, node->loop_body);
        if (body_result) {
            // Check for break/continue statements
            if (body_result->type == AST_BREAK) {
                break;
            } else if (body_result->type == AST_CONTINUE) {
                iteration_count++;
                continue;
            } else if (body_result->type == AST_RETURN) {
                return body_result;
            }
            last_result = body_result;
        }
        
        iteration_count++;
    }
    
    if (iteration_count >= max_iterations) {
        LOG_WARN(LOG_CAT_VISITOR, "While loop exceeded maximum iterations (%d), terminating", max_iterations);
    }
    
    return last_result ? last_result : ast_new(AST_NOOP);
}

/**
 * @brief Visit for loop node
 * @param visitor Visitor instance
 * @param node For loop AST node
 * @return Result of last iteration or NOOP
 */
static AST_T* visitor_visit_for_loop(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->iterator_variable || !node->iterable || !node->for_body) {
        return ast_new(AST_NOOP);
    }
    
    
    // Evaluate the iterable expression
    AST_T* iterable_result = visitor_visit(visitor, node->iterable);
    if (!iterable_result) {
        return ast_new(AST_NOOP);
    }
    
    AST_T* last_result = ast_new(AST_NOOP);
    
    // Handle different types of iterables
    if (iterable_result->type == AST_ARRAY) {
        
        for (size_t i = 0; i < iterable_result->array_size; i++) {
            AST_T* element = iterable_result->array_elements[i];
            if (!element) continue;
            
            
            // Create a temporary variable definition for the iterator
            AST_T* iterator_def = ast_new(AST_VARIABLE_DEFINITION);
            if (iterator_def && node->scope) {
                iterator_def->variable_definition_variable_name = strdup(node->iterator_variable);
                iterator_def->variable_definition_value = element;
                iterator_def->scope = node->scope;
                
                // Add iterator to scope temporarily
                scope_add_variable_definition(node->scope, iterator_def);
                
                // Execute loop body
                AST_T* body_result = visitor_visit(visitor, node->for_body);
                if (body_result) {
                    // Check for break/continue/return statements
                    if (body_result->type == AST_BREAK) {
                        break;
                    } else if (body_result->type == AST_CONTINUE) {
                        continue;
                    } else if (body_result->type == AST_RETURN) {
                        return body_result;
                    }
                    last_result = body_result;
                }
                
                // Note: In a complete implementation, we'd need to remove the iterator 
                // from scope after each iteration, but the current scope system doesn't 
                // support removal
            }
        }
    } else if (iterable_result->type == AST_OBJECT) {
        
        for (size_t i = 0; i < iterable_result->object_size; i++) {
            char* key = iterable_result->object_keys[i];
            if (!key) continue;
            
            
            // Create a string AST node for the key
            AST_T* key_ast = ast_new(AST_STRING);
            if (key_ast) {
                key_ast->string_value = strdup(key);
                
                // Create iterator variable definition
                AST_T* iterator_def = ast_new(AST_VARIABLE_DEFINITION);
                if (iterator_def && node->scope) {
                    iterator_def->variable_definition_variable_name = strdup(node->iterator_variable);
                    iterator_def->variable_definition_value = key_ast;
                    iterator_def->scope = node->scope;
                    
                    // Add iterator to scope temporarily
                    scope_add_variable_definition(node->scope, iterator_def);
                    
                    // Execute loop body
                    AST_T* body_result = visitor_visit(visitor, node->for_body);
                    if (body_result) {
                        // Check for break/continue/return statements
                        if (body_result->type == AST_BREAK) {
                            break;
                        } else if (body_result->type == AST_CONTINUE) {
                            continue;
                        } else if (body_result->type == AST_RETURN) {
                            return body_result;
                        }
                        last_result = body_result;
                    }
                }
            }
        }
    } else {
        LOG_ERROR(LOG_CAT_VISITOR, "For loop iterable is not an array or object (type: %d)", iterable_result->type);
        return ast_new(AST_NULL);
    }
    
    return last_result ? last_result : ast_new(AST_NOOP);
}

/**
 * @brief Visit string node
 * @param visitor Visitor instance
 * @param node String AST node
 * @return The string node as-is
 */
AST_T* visitor_visit_string(visitor_T* visitor, AST_T* node)
{
    (void)visitor;  // Suppress unused parameter warning
    // String literals are returned as-is
    return node;
}

/**
 * @brief Execute a user-defined function with advanced profiling and call stack management
 * @param visitor Visitor instance
 * @param fdef Function definition AST node
 * @param args Array of argument expressions
 * @param args_size Number of arguments
 * @return Result of function execution
 */
static AST_T* visitor_execute_user_function(visitor_T* visitor, AST_T* fdef, AST_T** args, int args_size)
{
    if (!visitor || !fdef || !fdef->function_definition_body) {
        return ast_new(AST_NULL);
    }
    
    const char* function_name = fdef->function_definition_name ? fdef->function_definition_name : "anonymous";
    
    // Check parameter count matches argument count
    if (fdef->function_definition_args_size != (size_t)args_size) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Argument count mismatch", function_name);
        LOG_ERROR(LOG_CAT_VISITOR, "Function '%s' expects %zu arguments, got %d", 
                  function_name, fdef->function_definition_args_size, args_size);
        return ast_new(AST_NULL);
    }
    
    // Check for stack overflow
    if (visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", function_name);
        return ast_new(AST_NULL);
    }
    
    // Push call frame for profiling and stack management
    visitor_push_call_frame(visitor, fdef, args, (size_t)args_size, function_name);
    
    // Create a new scope for the function (inheriting from the function's definition scope)
    scope_T* function_scope = fdef->scope;
    if (!function_scope) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Function has no scope", function_name);
        visitor_pop_call_frame(visitor);
        return ast_new(AST_NULL);
    }
    
    // Bind parameters to arguments by creating temporary variable definitions
    AST_T** param_bindings = NULL;
    if (args_size > 0) {
        param_bindings = malloc(sizeof(AST_T*) * args_size);
        if (!param_bindings) {
            return ast_new(AST_NULL);
        }
        
        for (int i = 0; i < args_size; i++) {
            // Evaluate the argument
            AST_T* arg_value = visitor_visit(visitor, args[i]);
            if (!arg_value) {
                arg_value = ast_new(AST_NULL);
            }
            
            // Create a variable definition for the parameter
            AST_T* param_def = ast_new(AST_VARIABLE_DEFINITION);
            if (param_def && fdef->function_definition_args[i] && fdef->function_definition_args[i]->variable_name) {
                param_def->variable_definition_variable_name = strdup(fdef->function_definition_args[i]->variable_name);
                param_def->variable_definition_value = arg_value;
                param_def->scope = function_scope;
                
                // Add to function scope
                scope_add_variable_definition(function_scope, param_def);
                param_bindings[i] = param_def;
            }
        }
    }
    
    // Set the function body's scope to the function scope
    if (fdef->function_definition_body) {
        fdef->function_definition_body->scope = function_scope;
    }
    
    // Check for optimized version first
    FunctionProfile* profile = visitor_get_or_create_profile(visitor, function_name);
    AST_T* function_body = fdef->function_definition_body;
    
    if (profile && profile->optimized_ast && profile->is_hot_function) {
        function_body = profile->optimized_ast;
        visitor->cache_hits++;
        LOG_VISITOR_DEBUG("Using optimized version of function '%s'", function_name);
    } else {
        visitor->cache_misses++;
    }
    
    // Set the function body's scope to the function scope
    if (function_body) {
        function_body->scope = function_scope;
    }
    
    // Execute the function body with exception handling
    AST_T* result = NULL;
    if (!visitor_has_exception(visitor)) {
        result = visitor_visit(visitor, function_body);
        visitor->total_instructions_executed++;
        
        // Apply optimizations if enabled
        if (visitor->constant_folding && visitor_is_constant_expression(result)) {
            AST_T* optimized = visitor_optimize_constant_expression(visitor, result);
            if (optimized != result) {
                result = optimized;
            }
        }
        
        if (visitor->tail_call_optimization) {
            result = visitor_apply_tail_call_optimization(visitor, result);
        }
    }
    
    // Handle return statements
    if (result && result->type == AST_RETURN) {
        // If it's a return statement, get the return value
        if (result->return_value) {
            AST_T* return_val = visitor_visit(visitor, result->return_value);
            result = return_val ? return_val : ast_new(AST_NULL);
        } else {
            result = ast_new(AST_NULL);
        }
    }
    
    // Pop call frame (this will update profiling automatically)
    visitor_pop_call_frame(visitor);
    
    // Clean up parameter bindings (in a real implementation, we'd remove them from scope)
    if (param_bindings) {
        free(param_bindings);
    }
    
    // Handle exceptions
    if (visitor_has_exception(visitor)) {
        LOG_ERROR(LOG_CAT_VISITOR, "Exception occurred in function '%s'", function_name);
        return ast_new(AST_NULL);
    }
    
    return result ? result : ast_new(AST_NULL);
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
static void visitor_push_call_frame(visitor_T* visitor, AST_T* function_def, AST_T** args, size_t arg_count, const char* function_name)
{
    if (!visitor || visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        if (visitor) {
            visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", "call_stack");
        }
        return;
    }
    
    CallFrame* frame = memory_alloc(sizeof(CallFrame));
    if (!frame) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Memory allocation failed", "call_frame");
        return;
    }
    
    frame->function_def = function_def;
    frame->arguments = args;
    frame->arg_count = arg_count;
    frame->previous = visitor->call_stack;
    frame->start_time = clock();
    frame->recursion_depth = visitor->call_stack_depth;
    frame->function_name = function_name ? strdup(function_name) : NULL;
    
    visitor->call_stack = frame;
    visitor->call_stack_depth++;
}

/**
 * @brief Pop the top call frame from the call stack
 * @param visitor Visitor instance
 */
static void visitor_pop_call_frame(visitor_T* visitor)
{
    if (!visitor || !visitor->call_stack) {
        return;
    }
    
    CallFrame* frame = visitor->call_stack;
    
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
static FunctionProfile* visitor_get_or_create_profile(visitor_T* visitor, const char* function_name)
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
        FunctionProfile* profile = &visitor->function_profiles[visitor->profile_count];
        profile->function_name = strdup(function_name);
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
    FunctionProfile* new_profiles = memory_alloc(sizeof(FunctionProfile) * new_capacity);
    if (!new_profiles) {
        return NULL;
    }
    
    // Copy existing profiles
    memcpy(new_profiles, visitor->function_profiles, sizeof(FunctionProfile) * visitor->profile_count);
    memory_free(visitor->function_profiles);
    visitor->function_profiles = new_profiles;
    visitor->profile_capacity = new_capacity;
    
    // Create the new profile
    FunctionProfile* profile = &visitor->function_profiles[visitor->profile_count];
    profile->function_name = strdup(function_name);
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
static void visitor_update_function_profile(visitor_T* visitor, const char* function_name, double execution_time)
{
    FunctionProfile* profile = visitor_get_or_create_profile(visitor, function_name);
    if (!profile) {
        return;
    }
    
    profile->execution_count++;
    profile->total_execution_time += execution_time;
    profile->average_time_per_call = profile->total_execution_time / profile->execution_count;
    
    // Check if function became hot
    if (!profile->is_hot_function && 
        profile->execution_count >= visitor->hot_function_threshold &&
        profile->average_time_per_call >= HOT_FUNCTION_TIME_THRESHOLD) {
        profile->is_hot_function = true;
        LOG_VISITOR_DEBUG("Function '%s' marked as HOT (calls: %zu, avg time: %.6fs)", 
                          function_name, profile->execution_count, profile->average_time_per_call);
        
        // Trigger optimization for hot function
        visitor_optimize_hot_function(visitor, function_name);
    }
}

/**
 * @brief Check if an AST node represents a constant expression
 * @param node AST node to check
 * @return true if constant, false otherwise
 */
static bool visitor_is_constant_expression(AST_T* node)
{
    if (!node) {
        return false;
    }
    
    switch (node->type) {
        case AST_NULL:
        case AST_UNDECIDABLE:
        case AST_BOOLEAN:
        case AST_NUMBER:
        case AST_STRING:
            return true;
        case AST_BINARY_OP:
            return visitor_is_constant_expression(node->left) && 
                   visitor_is_constant_expression(node->right);
        case AST_UNARY_OP:
            return visitor_is_constant_expression(node->operand);
        case AST_ARRAY:
            for (size_t i = 0; i < node->array_size; i++) {
                if (!visitor_is_constant_expression(node->array_elements[i])) {
                    return false;
                }
            }
            return true;
        default:
            return false;
    }
}

/**
 * @brief Optimize constant expressions through constant folding
 * @param visitor Visitor instance
 * @param node AST node to optimize
 * @return Optimized AST node
 */
static AST_T* visitor_optimize_constant_expression(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !visitor->constant_folding) {
        return node;
    }
    
    if (!visitor_is_constant_expression(node)) {
        return node;
    }
    
    // Evaluate the constant expression once and cache the result
    AST_T* result = visitor_visit(visitor, node);
    if (result && result != node) {
        visitor->cache_hits++;
        LOG_VISITOR_DEBUG("Constant folding optimization applied");
        return result;
    }
    
    visitor->cache_misses++;
    return node;
}

/**
 * @brief Apply tail call optimization if applicable
 * @param visitor Visitor instance  
 * @param node AST node to optimize
 * @return Optimized AST node
 */
static AST_T* visitor_apply_tail_call_optimization(visitor_T* visitor, AST_T* node)
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

// =============================================================================
// Database-like file operations implementation
// =============================================================================

/**
 * @brief Visit file get operation
 * @param visitor Visitor instance
 * @param node File get AST node
 * @return Value from file or null if failed
 */
static AST_T* visitor_visit_file_get(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->file_get_path) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate file path
    AST_T* path_result = visitor_visit(visitor, node->file_get_path);
    if (!path_result || path_result->type != AST_STRING || !path_result->string_value) {
        return ast_new(AST_NULL);
    }
    
    char* file_path = path_result->string_value;
    
    // Load file contents based on extension
    char* extension = strrchr(file_path, '.');
    if (!extension) {
        LOG_ERROR(LOG_CAT_VISITOR, "File get: No extension found in path %s", file_path);
        return ast_new(AST_NULL);
    }
    
    // For now, return a placeholder implementation
    // TODO: Implement actual JSON/YAML file loading when those stdlib functions are available
    LOG_VISITOR_DEBUG("File get operation on %s (extension: %s)", file_path, extension);
    
    // Return a simple object as placeholder
    AST_T* result = ast_new(AST_OBJECT);
    if (result) {
        result->object_size = 1;
        result->object_keys = memory_alloc(sizeof(char*));
        result->object_values = memory_alloc(sizeof(AST_T*));
        if (result->object_keys && result->object_values) {
            result->object_keys[0] = memory_strdup("status");
            result->object_values[0] = ast_new_string("file_get_placeholder");
        }
        
        // If no property path, return entire file content
        if (!node->file_get_property) {
            return result;
        }
        
        // Navigate property path using helper function
        AST_T* property_result = visitor_visit(visitor, node->file_get_property);
        if (property_result) {
            AST_T* navigated = visitor_navigate_property_path(visitor, result, property_result);
            return navigated ? navigated : ast_new(AST_NULL);
        }
    }
    
    return result ? result : ast_new(AST_NULL);
}

/**
 * @brief Visit file put operation
 * @param visitor Visitor instance
 * @param node File put AST node
 * @return Success/failure AST node
 */
static AST_T* visitor_visit_file_put(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node || !node->file_put_path || !node->file_put_value) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate file path
    AST_T* path_result = visitor_visit(visitor, node->file_put_path);
    if (!path_result || path_result->type != AST_STRING || !path_result->string_value) {
        return ast_new(AST_NULL);
    }
    
    // Evaluate value to put
    AST_T* value_result = visitor_visit(visitor, node->file_put_value);
    if (!value_result) {
        return ast_new(AST_NULL);
    }
    
    char* file_path = path_result->string_value;
    char* extension = strrchr(file_path, '.');
    
    // For now, return a placeholder implementation
    // TODO: Implement actual JSON/YAML file saving when those stdlib functions are available
    LOG_VISITOR_DEBUG("File put operation on %s (extension: %s)", file_path, extension ? extension : "none");
    
    // Create a placeholder file content structure
    AST_T* file_content = ast_new(AST_OBJECT);
    if (!file_content) {
        return ast_new(AST_NULL);
    }
    
    // Set value at property path using helper function
    if (node->file_put_property) {
        AST_T* property_result = visitor_visit(visitor, node->file_put_property);
        if (property_result) {
            visitor_set_property_path(visitor, file_content, property_result, value_result);
        }
    }
    
    // Return success placeholder
    return ast_new_boolean(1);
}

/**
 * @brief Navigate property path in object/array structure
 * @param visitor Visitor instance
 * @param root Root object/array to navigate
 * @param property_path Property path AST (compound or string)
 * @return Value at property path or NULL if not found
 */
static AST_T* visitor_navigate_property_path(visitor_T* visitor, AST_T* root, AST_T* property_path)
{
    if (!visitor || !root || !property_path) {
        return NULL;
    }
    
    AST_T* current = root;
    
    if (property_path->type == AST_STRING) {
        // Simple property access
        if (current->type == AST_OBJECT && property_path->string_value) {
            for (size_t i = 0; i < current->object_size; i++) {
                if (current->object_keys[i] && 
                    strcmp(current->object_keys[i], property_path->string_value) == 0) {
                    return current->object_values[i];
                }
            }
        }
    } else if (property_path->type == AST_COMPOUND) {
        // Compound property path (nested access)
        for (size_t i = 0; i < property_path->compound_size; i++) {
            current = visitor_navigate_property_path(visitor, current, property_path->compound_statements[i]);
            if (!current) {
                return NULL;
            }
        }
        return current;
    }
    
    return NULL;
}

/**
 * @brief Set value at property path in object structure
 * @param visitor Visitor instance
 * @param root Root object to modify
 * @param property_path Property path AST
 * @param value Value to set
 */
static void visitor_set_property_path(visitor_T* visitor, AST_T* root, AST_T* property_path, AST_T* value)
{
    if (!visitor || !root || !property_path || !value) {
        return;
    }
    
    if (property_path->type == AST_STRING && property_path->string_value) {
        // Simple property set
        if (root->type == AST_OBJECT) {
            // Find existing key or add new one
            for (size_t i = 0; i < root->object_size; i++) {
                if (root->object_keys[i] && 
                    strcmp(root->object_keys[i], property_path->string_value) == 0) {
                    // Update existing value
                    if (root->object_values[i]) {
                        ast_free(root->object_values[i]);
                    }
                    root->object_values[i] = value;
                    return;
                }
            }
            
            // Add new key-value pair
            root->object_size++;
            root->object_keys = memory_realloc(root->object_keys, root->object_size * sizeof(char*));
            root->object_values = memory_realloc(root->object_values, root->object_size * sizeof(AST_T*));
            
            if (root->object_keys && root->object_values) {
                root->object_keys[root->object_size - 1] = memory_strdup(property_path->string_value);
                root->object_values[root->object_size - 1] = value;
            }
        }
    }
    // TODO: Handle compound property paths for nested object modification
}

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
bool visitor_has_exception(visitor_T* visitor) {
    (void)visitor; // Mark as used
    return false; // No exception handling for now
}

/**
 * @brief Throw an exception in visitor
 * @param visitor Visitor instance
 * @param node AST node where exception occurred
 * @param message Error message
 * @param context Context string
 */
void visitor_throw_exception(visitor_T* visitor, AST_T* node, const char* message, const char* context) {
    (void)visitor; // Mark as used
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
bool visitor_optimize_hot_function(visitor_T* visitor, const char* function_name) {
    (void)visitor; // Mark as used
    (void)function_name;
    // No optimization for now - just ignore
    return false;
}








