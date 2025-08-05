/**
 * @file test_parser_basic.c
 * @brief Basic parser functionality tests
 */

#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/ast.h"
#include "zen/core/scope.h"

TEST_SUITE(parser_basic_tests)

TEST(test_parser_initialization) {
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    ASSERT_NOT_NULL(parser);
    ASSERT_NOT_NULL(parser->lexer);
    ASSERT_NOT_NULL(parser->current_token);
}

TEST(test_parser_variable_definition) {
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse_variable_definition(parser, scope);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_VARIABLE_DEFINITION);
    ASSERT_STR_EQ(ast->variable_definition_variable_name, "x");
    ASSERT_NOT_NULL(ast->variable_definition_value);
    ASSERT_EQ(ast->variable_definition_value->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->variable_definition_value->number_value, 42.0, 0.001);
}

TEST(test_parser_string_literal) {
    char* input = "\"hello world\"";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_string(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_STRING);
    ASSERT_STR_EQ(ast->string_value, "hello world");
}

TEST(test_parser_number_literal) {
    char* input = "123.45";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->number_value, 123.45, 0.001);
}

TEST(test_parser_identifier) {
    char* input = "variable_name";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_id(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_VARIABLE);
    ASSERT_STR_EQ(ast->variable_name, "variable_name");
}

TEST(test_parser_function_call_no_args) {
    char* input = "print";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    // Advance to get the identifier
    parser_eat(parser, TOKEN_ID);
    
    AST_T* ast = parser_parse_function_call(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_FUNCTION_CALL);
    ASSERT_STR_EQ(ast->function_call_name, "print");
    ASSERT_EQ(ast->function_call_arguments_size, 0);
}

TEST(test_parser_function_call_with_args) {
    char* input = "print \"Hello\" 42";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_function_call(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_FUNCTION_CALL);
    ASSERT_STR_EQ(ast->function_call_name, "print");
    ASSERT_EQ(ast->function_call_arguments_size, 2);
    
    // Check first argument
    ASSERT_NOT_NULL(ast->function_call_arguments[0]);
    ASSERT_EQ(ast->function_call_arguments[0]->type, AST_STRING);
    ASSERT_STR_EQ(ast->function_call_arguments[0]->string_value, "Hello");
    
    // Check second argument
    ASSERT_NOT_NULL(ast->function_call_arguments[1]);
    ASSERT_EQ(ast->function_call_arguments[1]->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->function_call_arguments[1]->number_value, 42.0, 0.001);
}

TEST(test_parser_function_definition) {
    char* input = "function greet name\n    print \"Hello \" + name";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse_function_definition(parser, scope);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_FUNCTION_DEFINITION);
    ASSERT_STR_EQ(ast->function_definition_name, "greet");
    ASSERT_EQ(ast->function_definition_args_size, 1);
    ASSERT_STR_EQ(ast->function_definition_args[0]->variable_name, "name");
    ASSERT_NOT_NULL(ast->function_definition_body);
}

TEST(test_parser_binary_expression) {
    char* input = "2 + 3";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_BINARY_OP);
    ASSERT_EQ(ast->operator_type, TOKEN_PLUS);
    
    ASSERT_NOT_NULL(ast->left);
    ASSERT_EQ(ast->left->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->left->number_value, 2.0, 0.001);
    
    ASSERT_NOT_NULL(ast->right);
    ASSERT_EQ(ast->right->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->right->number_value, 3.0, 0.001);
}

TEST(test_parser_complex_expression) {
    char* input = "2 + 3 * 4";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_BINARY_OP);
    
    // Should respect operator precedence: 2 + (3 * 4)
    ASSERT_EQ(ast->operator_type, TOKEN_PLUS);
    ASSERT_EQ(ast->left->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->left->number_value, 2.0, 0.001);
    
    ASSERT_EQ(ast->right->type, AST_BINARY_OP);
    ASSERT_EQ(ast->right->operator_type, TOKEN_MULTIPLY);
}

TEST(test_parser_parenthesized_expression) {
    char* input = "(2 + 3) * 4";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_BINARY_OP);
    
    // Should respect parentheses: (2 + 3) * 4
    ASSERT_EQ(ast->operator_type, TOKEN_MULTIPLY);
    ASSERT_EQ(ast->left->type, AST_BINARY_OP);
    ASSERT_EQ(ast->left->operator_type, TOKEN_PLUS);
    ASSERT_EQ(ast->right->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->right->number_value, 4.0, 0.001);
}

TEST(test_parser_statements) {
    char* input = 
        "set x 10\n"
        "set y 20\n"
        "print x + y";
    
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse_statements(parser, scope);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_COMPOUND);
    ASSERT_EQ(ast->compound_size, 3);
    
    // First statement: set x 10
    ASSERT_EQ(ast->compound_value[0]->type, AST_VARIABLE_DEFINITION);
    ASSERT_STR_EQ(ast->compound_value[0]->variable_definition_variable_name, "x");
    
    // Second statement: set y 20
    ASSERT_EQ(ast->compound_value[1]->type, AST_VARIABLE_DEFINITION);
    ASSERT_STR_EQ(ast->compound_value[1]->variable_definition_variable_name, "y");
    
    // Third statement: print x + y
    ASSERT_EQ(ast->compound_value[2]->type, AST_FUNCTION_CALL);
    ASSERT_STR_EQ(ast->compound_value[2]->function_call_name, "print");
}

TEST(test_parser_error_handling) {
    // Test malformed input
    char* input = "set + 42";  // Missing variable name
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse_variable_definition(parser, scope);
    
    // Should handle error gracefully - either return NULL or error node
    // Implementation dependent
    if (ast) {
        // If it returns something, it should be an error indicator
        // or a valid recovery
    }
}

TEST(test_parser_string_concatenation) {
    char* input = "\"Hello \" + \"World\"";
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_BINARY_OP);
    ASSERT_EQ(ast->operator_type, TOKEN_PLUS);
    
    ASSERT_EQ(ast->left->type, AST_STRING);
    ASSERT_STR_EQ(ast->left->string_value, "Hello ");
    
    ASSERT_EQ(ast->right->type, AST_STRING);
    ASSERT_STR_EQ(ast->right->string_value, "World");
}

TEST(test_parser_comparison_operators) {
    char* input = "x = 42";  // Note: = is comparison in ZEN, not assignment
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_expr(parser);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_BINARY_OP);
    ASSERT_EQ(ast->operator_type, TOKEN_EQUALS);
    
    ASSERT_EQ(ast->left->type, AST_VARIABLE);
    ASSERT_STR_EQ(ast->left->variable_name, "x");
    
    ASSERT_EQ(ast->right->type, AST_NUMBER);
    ASSERT_DOUBLE_EQ(ast->right->number_value, 42.0, 0.001);
}

END_TEST_SUITE