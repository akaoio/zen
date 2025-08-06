/*
 * visitor.c
 * AST visitor pattern implementation - SIMPLIFIED VERSION FOR VARIABLE DEBUGGING
 * 
 * This file implements the core ZEN runtime evaluation system
 * focused ONLY on basic variable operations to debug the variable system.
 */

#define _GNU_SOURCE  // Enable strdup
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/token.h"
#include "zen/runtime/operators.h"
#include "zen/types/value.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Array and object function forward declarations (from MANIFEST.json)
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);
Value* array_get(Value* array, size_t index);
size_t array_length(const Value* array);
Value* object_new(void);
void object_set(Value* object, const char* key, Value* value);
Value* object_get(Value* object, const char* key);

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

/**
 * @brief Create a new visitor instance
 * @return New visitor instance or NULL on failure
 */
visitor_T* init_visitor()
{
    visitor_T* visitor = malloc(sizeof(visitor_T));
    if (!visitor) return NULL;
    
    return visitor;
}

/**
 * @brief Visit and evaluate an AST node
 * @param visitor Visitor instance
 * @param node AST node to visit
 * @return Result of evaluation
 */
AST_T* visitor_visit(visitor_T* visitor, AST_T* node)
{
    if (!visitor || !node) {
        return init_ast(AST_NOOP);
    }


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
                AST_T* return_node = init_ast(AST_RETURN);
                if (return_node) {
                    return_node->return_value = return_val;
                }
                return return_node;
            }
            return node;
            
        default:
            // For unimplemented features, just return NOOP to continue execution
            return init_ast(AST_NOOP);
    }
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
        printf("Error: Invalid variable definition (visitor=%p, node=%p, scope=%p)\n", 
               (void*)visitor, (void*)node, node ? (void*)node->scope : NULL);
        return init_ast(AST_NOOP);
    }
    
    if (!node->variable_definition_variable_name) {
        printf("Error: Variable definition missing name\n");
        return init_ast(AST_NOOP);
    }
    
    
    // Evaluate the variable's value - avoid recursion for literals
    AST_T* value = node->variable_definition_value;
    if (value && (value->type != AST_STRING && value->type != AST_NUMBER && 
                  value->type != AST_BOOLEAN && value->type != AST_NULL)) {
        value = visitor_visit(visitor, value);
    }
    if (!value) {
        value = init_ast(AST_NULL);
    }
    
    // Update the node with the evaluated value
    node->variable_definition_value = value;
    
    // Add to scope
    scope_add_variable_definition(node->scope, node);

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
        return init_ast(AST_NOOP);
    }

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
        printf("Error: Invalid variable access (visitor=%p, node=%p, name=%s, scope=%p)\n",
               (void*)visitor, (void*)node, 
               node && node->variable_name ? node->variable_name : "NULL",
               node ? (void*)node->scope : NULL);
        return init_ast(AST_NULL);
    }
    
    
    AST_T* vdef = scope_get_variable_definition(node->scope, node->variable_name);
    
    if (vdef != NULL) {
        // Return the stored value directly
        return vdef->variable_definition_value;
    }

    printf("Error: Undefined variable '%s'\n", node->variable_name);
    return init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }

    // Check for stdlib functions first
    const ZenStdlibFunction* stdlib_func = zen_stdlib_get(node->function_call_name);
    if (stdlib_func != NULL) {
        // Convert AST arguments to Value arguments
        Value** value_args = NULL;
        size_t argc = (size_t)node->function_call_arguments_size;
        
        if (argc > 0) {
            value_args = malloc(sizeof(Value*) * argc);
            if (!value_args) {
                return init_ast(AST_NULL);
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
            return result_ast ? result_ast : init_ast(AST_NULL);
        } else {
            return init_ast(AST_NULL);
        }
    }

    // Handle built-in functions (legacy print support)
    if (strcmp(node->function_call_name, "print") == 0) {
        return builtin_function_print(visitor, node->function_call_arguments, node->function_call_arguments_size);
    }

    // Look up user-defined function
    AST_T* fdef = scope_get_function_definition(node->scope, node->function_call_name);

    if (fdef == NULL) {        
        printf("Error: Undefined function '%s'\n", node->function_call_name);
        return init_ast(AST_NULL);
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
    if (!visitor || !node || !node->compound_value) {
        return init_ast(AST_NOOP);
    }
    
    
    AST_T* last_result = init_ast(AST_NOOP);
    
    for (size_t i = 0; i < node->compound_size; i++) {
        last_result = visitor_visit(visitor, node->compound_value[i]);
    }

    return last_result;
}

/**
 * @brief Built-in print function implementation
 */
static AST_T* builtin_function_print(visitor_T* visitor, AST_T** args, int args_size)
{
    
    for (int i = 0; i < args_size; i++)
    {
        AST_T* visited_ast = visitor_visit(visitor, args[i]);
        if (!visited_ast) continue;


        switch (visited_ast->type)
        {
            case AST_STRING: 
                if (visited_ast->string_value) {
                    printf("%s", visited_ast->string_value);
                } else {
                    printf("(null string)");
                }
                break;
            case AST_NUMBER:
                printf("%.15g", visited_ast->number_value);
                break;
            case AST_BOOLEAN:
                printf("%s", visited_ast->boolean_value ? "true" : "false");
                break;
            case AST_NULL:
                printf("null");
                break;
            case AST_ARRAY:
                printf("[");
                for (size_t j = 0; j < visited_ast->array_size; j++) {
                    AST_T* element = visited_ast->array_elements[j];
                    if (element) {
                        switch (element->type) {
                            case AST_STRING:
                                printf("\"%s\"", element->string_value ? element->string_value : "");
                                break;
                            case AST_NUMBER:
                                printf("%.15g", element->number_value);
                                break;
                            case AST_BOOLEAN:
                                printf("%s", element->boolean_value ? "true" : "false");
                                break;
                            case AST_NULL:
                                printf("null");
                                break;
                            default:
                                printf("[nested]");
                                break;
                        }
                    }
                    if (j < visited_ast->array_size - 1) {
                        printf(", ");
                    }
                }
                printf("]");
                break;
            case AST_OBJECT:
                printf("{");
                for (size_t j = 0; j < visited_ast->object_size; j++) {
                    if (visited_ast->object_keys[j] && visited_ast->object_values[j]) {
                        printf("\"%s\": ", visited_ast->object_keys[j]);
                        AST_T* value = visited_ast->object_values[j];
                        switch (value->type) {
                            case AST_STRING:
                                printf("\"%s\"", value->string_value ? value->string_value : "");
                                break;
                            case AST_NUMBER:
                                printf("%.15g", value->number_value);
                                break;
                            case AST_BOOLEAN:
                                printf("%s", value->boolean_value ? "true" : "false");
                                break;
                            case AST_NULL:
                                printf("null");
                                break;
                            default:
                                printf("[nested]");
                                break;
                        }
                    }
                    if (j < visited_ast->object_size - 1) {
                        printf(", ");
                    }
                }
                printf("}");
                break;
            default:
                printf("[object type %d]", visited_ast->type);
                break;
        }
        
        if (i < args_size - 1) {
            printf(" ");
        }
    }
    
    printf("\n");
    return init_ast(AST_NOOP);
}

/**
 * @brief Convert AST node to Value object
 * @param node AST node to convert
 * @return Value object or NULL on failure
 */
static Value* ast_to_value(AST_T* node)
{
    if (!node) {
        return value_new_null();
    }
    
    switch (node->type) {
        case AST_NULL:
            return value_new_null();
        case AST_BOOLEAN:
            return value_new_boolean(node->boolean_value);
        case AST_NUMBER:
            return value_new_number(node->number_value);
        case AST_STRING:
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
        return init_ast(AST_NULL);
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return init_ast(AST_NULL);
        case VALUE_BOOLEAN:
            {
                AST_T* ast = init_ast(AST_BOOLEAN);
                if (ast) {
                    ast->boolean_value = value->as.boolean;
                }
                return ast;
            }
        case VALUE_NUMBER:
            {
                AST_T* ast = init_ast(AST_NUMBER);
                if (ast) {
                    ast->number_value = value->as.number;
                }
                return ast;
            }
        case VALUE_STRING:
            {
                AST_T* ast = init_ast(AST_STRING);
                if (ast && value->as.string && value->as.string->data) {
                    ast->string_value = strdup(value->as.string->data);
                }
                return ast;
            }
        case VALUE_ARRAY:
            {
                AST_T* ast = init_ast(AST_ARRAY);
                if (!ast || !value->as.array) return init_ast(AST_NULL);
                
                // Get array length
                size_t length = array_length(value);
                ast->array_size = length;
                
                if (length > 0) {
                    // Allocate array for elements
                    ast->array_elements = malloc(sizeof(AST_T*) * length);
                    if (!ast->array_elements) {
                        ast_free(ast);
                        return init_ast(AST_NULL);
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
                AST_T* ast = init_ast(AST_OBJECT);
                if (!ast || !value->as.object) return init_ast(AST_NULL);
                
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
                        return init_ast(AST_NULL);
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
            printf("Error in expression evaluation: %s\n", 
                   value->as.error && value->as.error->message ? value->as.error->message : "Unknown error");
            return init_ast(AST_NULL);
        default:
            return init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }
    
    // Evaluate operands
    AST_T* left_ast = visitor_visit(visitor, node->left);
    AST_T* right_ast = visitor_visit(visitor, node->right);
    
    if (!left_ast || !right_ast) {
        return init_ast(AST_NULL);
    }
    
    // Convert AST nodes to Value objects
    Value* left_val = ast_to_value(left_ast);
    Value* right_val = ast_to_value(right_ast);
    
    if (!left_val || !right_val) {
        if (left_val) value_unref(left_val);
        if (right_val) value_unref(right_val);
        return init_ast(AST_NULL);
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
            printf("Error: Unknown binary operator %d\n", node->operator_type);
            result = NULL;
            break;
    }
    
    // Clean up operands
    value_unref(left_val);
    value_unref(right_val);
    
    if (!result) {
        return init_ast(AST_NULL);
    }
    
    // Convert result back to AST
    AST_T* result_ast = value_to_ast(result);
    value_unref(result);
    
    return result_ast ? result_ast : init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }
    
    // Evaluate operand
    AST_T* operand_ast = visitor_visit(visitor, node->operand);
    if (!operand_ast) {
        return init_ast(AST_NULL);
    }
    
    // Convert AST node to Value object
    Value* operand_val = ast_to_value(operand_ast);
    if (!operand_val) {
        return init_ast(AST_NULL);
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
            printf("Error: Unknown unary operator %d\n", node->operator_type);
            result = NULL;
            break;
    }
    
    // Clean up operand
    value_unref(operand_val);
    
    if (!result) {
        return init_ast(AST_NULL);
    }
    
    // Convert result back to AST
    AST_T* result_ast = value_to_ast(result);
    value_unref(result);
    
    return result_ast ? result_ast : init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }
    
    // Create a new array Value
    Value* array_val = array_new(node->array_size > 0 ? node->array_size : 1);
    if (!array_val) {
        return init_ast(AST_NULL);
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
    
    return result_ast ? result_ast : init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }
    
    // Create a new object Value
    Value* object_val = object_new();
    if (!object_val) {
        return init_ast(AST_NULL);
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
    
    return result_ast ? result_ast : init_ast(AST_NULL);
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
        return init_ast(AST_NULL);
    }
    
    // Evaluate the object expression
    AST_T* object_ast = visitor_visit(visitor, node->object);
    if (!object_ast) {
        return init_ast(AST_NULL);
    }
    
    // Convert to Value for property access
    Value* object_val = ast_to_value(object_ast);
    if (!object_val) {
        return init_ast(AST_NULL);
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
            printf("Error: Invalid array index '%s'\n", node->property_name);
            result_val = value_new_null();
        }
    } else {
        printf("Error: Cannot access property '%s' on non-object/array type\n", node->property_name);
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
    
    return result_ast ? result_ast : init_ast(AST_NULL);
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
        return init_ast(AST_NOOP);
    }
    
    
    // Evaluate the condition
    AST_T* condition_result = visitor_visit(visitor, node->condition);
    if (!condition_result) {
        return init_ast(AST_NOOP);
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
    
    return init_ast(AST_NOOP);
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
        return init_ast(AST_NOOP);
    }
    
    // CRITICAL FIX FOR WHILE LOOP SCOPE SHARING
    // The issue is that variable updates in the loop body must be visible to the condition
    // in subsequent iterations. We need to ensure both use the exact same scope instance.
    
    if (!node->scope) {
        // If no scope is set, we can't fix the issue
        printf("ERROR: While loop node has no scope - cannot ensure variable persistence\n");
        return init_ast(AST_NOOP);
    }
    
    // Recursive function to ensure all nodes use the same shared scope
    void propagate_shared_scope(AST_T* ast_node, scope_T* shared_scope) {
        if (!ast_node || !shared_scope) return;
        
        // CRITICAL: Set the scope on this node
        ast_node->scope = shared_scope;
        
        // Recursively apply to all child nodes
        switch (ast_node->type) {
            case AST_COMPOUND:
                // Compound nodes (like loop bodies) must use shared scope
                for (size_t i = 0; i < ast_node->compound_size; i++) {
                    propagate_shared_scope(ast_node->compound_value[i], shared_scope);
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
    
    // Apply shared scope to both condition and body - this is the key fix
    propagate_shared_scope(node->loop_condition, node->scope);
    propagate_shared_scope(node->loop_body, node->scope);
    
    AST_T* last_result = init_ast(AST_NOOP);
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
        printf("WARNING: While loop exceeded maximum iterations (%d), terminating\n", max_iterations);
    }
    
    return last_result ? last_result : init_ast(AST_NOOP);
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
        return init_ast(AST_NOOP);
    }
    
    
    // Evaluate the iterable expression
    AST_T* iterable_result = visitor_visit(visitor, node->iterable);
    if (!iterable_result) {
        return init_ast(AST_NOOP);
    }
    
    AST_T* last_result = init_ast(AST_NOOP);
    
    // Handle different types of iterables
    if (iterable_result->type == AST_ARRAY) {
        
        for (size_t i = 0; i < iterable_result->array_size; i++) {
            AST_T* element = iterable_result->array_elements[i];
            if (!element) continue;
            
            
            // Create a temporary variable definition for the iterator
            AST_T* iterator_def = init_ast(AST_VARIABLE_DEFINITION);
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
            AST_T* key_ast = init_ast(AST_STRING);
            if (key_ast) {
                key_ast->string_value = strdup(key);
                
                // Create iterator variable definition
                AST_T* iterator_def = init_ast(AST_VARIABLE_DEFINITION);
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
        printf("ERROR: For loop iterable is not an array or object (type: %d)\n", iterable_result->type);
        return init_ast(AST_NULL);
    }
    
    return last_result ? last_result : init_ast(AST_NOOP);
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
 * @brief Execute a user-defined function
 * @param visitor Visitor instance
 * @param fdef Function definition AST node
 * @param args Array of argument expressions
 * @param args_size Number of arguments
 * @return Result of function execution
 */
static AST_T* visitor_execute_user_function(visitor_T* visitor, AST_T* fdef, AST_T** args, int args_size)
{
    if (!visitor || !fdef || !fdef->function_definition_body) {
        return init_ast(AST_NULL);
    }
    
    // Check parameter count matches argument count
    if (fdef->function_definition_args_size != (size_t)args_size) {
        printf("Error: Function '%s' expects %zu arguments, got %d\n", 
               fdef->function_definition_name ? fdef->function_definition_name : "unknown",
               fdef->function_definition_args_size, args_size);
        return init_ast(AST_NULL);
    }
    
    // Create a new scope for the function (inheriting from the function's definition scope)
    scope_T* function_scope = fdef->scope;
    if (!function_scope) {
        printf("Error: Function has no scope\n");
        return init_ast(AST_NULL);
    }
    
    // Bind parameters to arguments by creating temporary variable definitions
    AST_T** param_bindings = NULL;
    if (args_size > 0) {
        param_bindings = malloc(sizeof(AST_T*) * args_size);
        if (!param_bindings) {
            return init_ast(AST_NULL);
        }
        
        for (int i = 0; i < args_size; i++) {
            // Evaluate the argument
            AST_T* arg_value = visitor_visit(visitor, args[i]);
            if (!arg_value) {
                arg_value = init_ast(AST_NULL);
            }
            
            // Create a variable definition for the parameter
            AST_T* param_def = init_ast(AST_VARIABLE_DEFINITION);
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
    
    // Execute function body with correct scope but don't modify original AST
    AST_T* result = NULL;
    if (fdef->function_definition_body) {
        // Save original scope
        scope_T* original_scope = fdef->function_definition_body->scope;
        
        // Temporarily set function scope
        fdef->function_definition_body->scope = function_scope;
        
        // Execute function body
        result = visitor_visit(visitor, fdef->function_definition_body);
        
        // Restore original scope to prevent corruption
        fdef->function_definition_body->scope = original_scope;
    } else {
        result = init_ast(AST_NULL);
    }
    
    // Handle return statements
    if (result && result->type == AST_RETURN) {
        // If it's a return statement, get the return value
        if (result->return_value) {
            AST_T* return_val = visitor_visit(visitor, result->return_value);
            result = return_val ? return_val : init_ast(AST_NULL);
        } else {
            result = init_ast(AST_NULL);
        }
    }
    
    // Clean up parameter bindings (in a real implementation, we'd remove them from scope)
    if (param_bindings) {
        free(param_bindings);
    }
    
    return result ? result : init_ast(AST_NULL);
}