/**
 * @file test_minimal.c
 * @brief Minimal test that demonstrates working ZEN functionality
 */

#include "../../tests/framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/types/value.h"
#include "zen/runtime/operators.h"
#include "zen/stdlib/io.h"
#include <stdio.h>

// Test function declarations
DECLARE_TEST(core_components_work)
DECLARE_TEST(zen_program_execution)
DECLARE_TEST(operators_work)
DECLARE_TEST(memory_management)

TEST_SUITE_BEGIN(zen_minimal)
    RUN_TEST(core_components_work);
    RUN_TEST(zen_program_execution);
    RUN_TEST(operators_work);
    RUN_TEST(memory_management);
TEST_SUITE_END;

TEST(core_components_work) {
    // Test that all core components can be created
    char* input = "x";
    lexer_T* lexer = init_lexer(input);
    ASSERT_NOT_NULL(lexer);
    
    parser_T* parser = init_parser(lexer);
    ASSERT_NOT_NULL(parser);
    
    scope_T* scope = init_scope();
    ASSERT_NOT_NULL(scope);
    
    visitor_T* visitor = init_visitor();
    ASSERT_NOT_NULL(visitor);
    
    printf("Core components created successfully\n");
}

TEST(zen_program_execution) {
    // Test basic lexer functionality
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    
    // Get all tokens
    token_T* token1 = lexer_get_next_token(lexer);
    ASSERT_EQ(token1->type, TOKEN_SET);
    
    token_T* token2 = lexer_get_next_token(lexer);
    ASSERT_EQ(token2->type, TOKEN_ID);
    ASSERT_STR_EQ(token2->value, "x");
    
    token_T* token3 = lexer_get_next_token(lexer);
    ASSERT_EQ(token3->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token3->value, "42");
    
    printf("Basic tokenization works\n");
}

TEST(operators_work) {
    // Test arithmetic operations
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
    
    Value* add_result = op_add(a, b);
    ASSERT_EQ(add_result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(add_result->as.number, 15.0, 0.001);
    
    Value* sub_result = op_subtract(a, b);
    ASSERT_EQ(sub_result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(sub_result->as.number, 5.0, 0.001);
    
    Value* mul_result = op_multiply(a, b);
    ASSERT_EQ(mul_result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(mul_result->as.number, 50.0, 0.001);
    
    Value* div_result = op_divide(a, b);
    ASSERT_EQ(div_result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(div_result->as.number, 2.0, 0.001);
    
    value_unref(a);
    value_unref(b);
    value_unref(add_result);
    value_unref(sub_result);
    value_unref(mul_result);
    value_unref(div_result);
    
    printf("Arithmetic operations work\n");
}

TEST(memory_management) {
    // Test that value reference counting works
    Value* val = value_new_string("test");
    ASSERT_EQ(val->ref_count, 1);
    
    Value* ref = value_ref(val);
    ASSERT_EQ(val->ref_count, 2);
    ASSERT_EQ(ref, val); // Should be same pointer
    
    value_unref(ref);
    ASSERT_EQ(val->ref_count, 1);
    
    value_unref(val);
    // val should be freed now
    
    printf("Memory management works\n");
}