/**
 * @file test_lexer_simple.c
 * @brief Simple working test for lexer functionality
 */

#include "../../tests/framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/token.h"

// Test function declarations
DECLARE_TEST(lexer_initialization)
DECLARE_TEST(lexer_advance)
DECLARE_TEST(lexer_skip_whitespace)
DECLARE_TEST(lexer_basic_tokens)

TEST_SUITE_BEGIN(lexer_simple)
    RUN_TEST(lexer_initialization);
    RUN_TEST(lexer_advance);
    RUN_TEST(lexer_skip_whitespace);
    RUN_TEST(lexer_basic_tokens);
TEST_SUITE_END;

TEST(lexer_initialization) {
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    
    ASSERT_NOT_NULL(lexer);
    ASSERT_STR_EQ(lexer->contents, input);
    ASSERT_EQ(lexer->i, 0);
    ASSERT_EQ(lexer->c, 's');
}

TEST(lexer_advance) {
    char* input = "abc";
    lexer_T* lexer = init_lexer(input);
    
    ASSERT_EQ(lexer->c, 'a');
    lexer_advance(lexer);
    ASSERT_EQ(lexer->c, 'b');
    lexer_advance(lexer);
    ASSERT_EQ(lexer->c, 'c');
    lexer_advance(lexer);
    ASSERT_EQ(lexer->c, '\0');
}

TEST(lexer_skip_whitespace) {
    char* input = "   hello";
    lexer_T* lexer = init_lexer(input);
    
    lexer_skip_whitespace(lexer);
    ASSERT_EQ(lexer->c, 'h');
}

TEST(lexer_basic_tokens) {
    char* input = "set x";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token1 = lexer_get_next_token(lexer);
    ASSERT_NOT_NULL(token1);
    ASSERT_EQ(token1->type, TOKEN_SET);  // "set" is a keyword
    ASSERT_STR_EQ(token1->value, "set");
    
    token_T* token2 = lexer_get_next_token(lexer);
    ASSERT_NOT_NULL(token2);
    ASSERT_EQ(token2->type, TOKEN_ID);   // "x" is an identifier
    ASSERT_STR_EQ(token2->value, "x");
}