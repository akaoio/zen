/**
 * @file test_zen_basic.c
 * @brief Basic ZEN language functionality tests
 */

#include "../../tests/framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/types/value.h"
#include "zen/runtime/operators.h"

// Test function declarations
DECLARE_TEST(parse_variable_assignment)
DECLARE_TEST(parse_function_definition)
DECLARE_TEST(evaluate_arithmetic)
DECLARE_TEST(evaluate_string_operations)
DECLARE_TEST(variable_scoping)

TEST_SUITE_BEGIN(zen_basic)
    RUN_TEST(parse_variable_assignment);
    RUN_TEST(parse_function_definition);
    RUN_TEST(evaluate_arithmetic);
    RUN_TEST(evaluate_string_operations);
    RUN_TEST(variable_scoping);
TEST_SUITE_END;

TEST(parse_variable_assignment) {
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse(parser, scope);
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_COMPOUND);
    ASSERT_TRUE(ast->compound_size > 0);
    
    // First statement should be variable definition
    AST_T* var_def = ast->compound_value[0];
    ASSERT_EQ(var_def->type, AST_VARIABLE_DEFINITION);
    ASSERT_STR_EQ(var_def->variable_definition_variable_name, "x");
    ASSERT_NOT_NULL(var_def->variable_definition_value);
}

TEST(parse_function_definition) {
    char* input = "function greet name\n    return \"Hello\"";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse(parser, scope);
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_COMPOUND);
    
    // Should contain function definition
    AST_T* func_def = ast->compound_value[0];
    ASSERT_EQ(func_def->type, AST_FUNCTION_DEFINITION);
    ASSERT_STR_EQ(func_def->function_definition_name, "greet");
    ASSERT_NOT_NULL(func_def->function_definition_body);
}

TEST(evaluate_arithmetic) {
    // Test basic arithmetic: 5 + 3
    Value* val1 = value_new_number(5.0);
    Value* val2 = value_new_number(3.0);
    
    Value* result = op_add(val1, val2);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 8.0, 0.001);
    
    value_unref(val1);
    value_unref(val2);
    value_unref(result);
}

TEST(evaluate_string_operations) {
    // Test string concatenation
    Value* str1 = value_new_string("Hello");
    Value* str2 = value_new_string(" World");
    
    Value* result = op_add(str1, str2);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "Hello World");
    
    value_unref(str1);
    value_unref(str2);
    value_unref(result);
}

TEST(variable_scoping) {
    scope_T* scope = init_scope();
    
    // Create a variable definition AST node
    AST_T* var_def = init_ast(AST_VARIABLE_DEFINITION);
    var_def->variable_definition_variable_name = "test_var";
    AST_T* value_node = init_ast(AST_STRING);
    value_node->string_value = "test_value";
    var_def->variable_definition_value = value_node;
    var_def->scope = scope;
    
    // Add to scope
    scope_add_variable_definition(scope, var_def);
    
    // Retrieve from scope
    AST_T* retrieved = scope_get_variable_definition(scope, "test_var");
    ASSERT_NOT_NULL(retrieved);
    ASSERT_STR_EQ(retrieved->variable_definition_variable_name, "test_var");
}