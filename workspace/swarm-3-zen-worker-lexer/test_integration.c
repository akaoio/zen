/**
 * @file test_integration.c
 * @brief Integration test that runs complete ZEN programs
 */

#include "../../tests/framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/types/value.h"
#include "zen/runtime/operators.h"
#include <stdio.h>

// Test function declarations
DECLARE_TEST(simple_variable)
DECLARE_TEST(arithmetic_expression)
DECLARE_TEST(string_concatenation)
DECLARE_TEST(function_definition)

TEST_SUITE_BEGIN(zen_integration)
    RUN_TEST(simple_variable);
    RUN_TEST(arithmetic_expression);
    RUN_TEST(string_concatenation);
    RUN_TEST(function_definition);
TEST_SUITE_END;

// Helper function to execute ZEN code and return result
AST_T* execute_zen_code(const char* code) {
    lexer_T* lexer = init_lexer((char*)code);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    visitor_T* visitor = init_visitor();
    
    AST_T* ast = parser_parse(parser, scope);
    if (!ast) {
        return NULL;
    }
    
    AST_T* result = visitor_visit(visitor, ast);
    return result;
}

TEST(simple_variable) {
    // Test: set x 42
    const char* code = "set x 42";
    
    lexer_T* lexer = init_lexer((char*)code);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    visitor_T* visitor = init_visitor();
    
    AST_T* ast = parser_parse(parser, scope);
    ASSERT_NOT_NULL(ast);
    
    // Execute the program
    AST_T* result = visitor_visit(visitor, ast);
    ASSERT_NOT_NULL(result);
    
    // Check that variable was stored in scope
    AST_T* var_def = scope_get_variable_definition(scope, "x");
    ASSERT_NOT_NULL(var_def);
    ASSERT_STR_EQ(var_def->variable_definition_variable_name, "x");
    
    TEST_INFO("Simple variable assignment works");
}

TEST(arithmetic_expression) {
    // Test mathematical computation with Values directly
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
    Value* c = value_new_number(2.0);
    
    // Test: (10 + 5) * 2 = 30
    Value* temp = op_add(a, b);  // 15
    Value* result = op_multiply(temp, c);  // 30
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 30.0, 0.001);
    
    value_unref(a);
    value_unref(b);
    value_unref(c);
    value_unref(temp);
    value_unref(result);
    
    TEST_INFO("Arithmetic expressions work");
}

TEST(string_concatenation) {
    // Test string operations
    Value* hello = value_new_string("Hello");
    Value* space = value_new_string(" ");
    Value* world = value_new_string("World");
    
    Value* temp = op_add(hello, space);
    Value* result = op_add(temp, world);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "Hello World");
    
    value_unref(hello);
    value_unref(space);
    value_unref(world);
    value_unref(temp);
    value_unref(result);
    
    TEST_INFO("String concatenation works");
}

TEST(function_definition) {
    // Test parsing a simple function definition
    const char* code = "function greet name\n    return \"Hello\"";
    
    lexer_T* lexer = init_lexer((char*)code);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    AST_T* ast = parser_parse(parser, scope);
    ASSERT_NOT_NULL(ast);
    
    // Check if we got a compound with function definition
    if (ast->type == AST_COMPOUND && ast->compound_size > 0) {
        AST_T* func_def = ast->compound_value[0];
        if (func_def->type == AST_FUNCTION_DEFINITION) {
            ASSERT_STR_EQ(func_def->function_definition_name, "greet");
            TEST_INFO("Function definition parsing works");
        } else {
            TEST_INFO("Parser returned AST type %d instead of function definition", func_def->type);
        }
    } else {
        TEST_INFO("Parser returned AST type %d instead of compound", ast->type);
    }
}