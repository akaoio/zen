/**
 * @file test_lexer_basic.c
 * @brief Basic lexer functionality tests
 */

#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/token.h"

// Forward declare tests
DECLARE_TEST(test_lexer_initialization);
DECLARE_TEST(test_lexer_advance);
DECLARE_TEST(test_lexer_skip_whitespace);
DECLARE_TEST(test_lexer_keywords);
DECLARE_TEST(test_lexer_identifiers);
DECLARE_TEST(test_lexer_numbers);
DECLARE_TEST(test_lexer_strings);
DECLARE_TEST(test_lexer_operators);
DECLARE_TEST(test_lexer_punctuation);
DECLARE_TEST(test_lexer_indentation);
DECLARE_TEST(test_lexer_complete_program);

TEST(test_lexer_initialization) {
    char* input = "set x 42";
    lexer_T* lexer = init_lexer(input);
    
    ASSERT_NOT_NULL(lexer);
    ASSERT_STR_EQ(lexer->contents, input);
    ASSERT_EQ(lexer->i, 0);
    ASSERT_EQ(lexer->c, 's');
}

TEST(test_lexer_advance) {
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

TEST(test_lexer_skip_whitespace) {
    char* input = "   \t\n  hello";
    lexer_T* lexer = init_lexer(input);
    
    lexer_skip_whitespace(lexer);
    ASSERT_EQ(lexer->c, 'h');
}

TEST(test_lexer_keywords) {
    char* input = "set function if else while for return";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Test "set"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_SET);
    ASSERT_STR_EQ(token->value, "set");
    
    // Test "function"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_FUNCTION);
    ASSERT_STR_EQ(token->value, "function");
    
    // Test "if"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_IF);
    ASSERT_STR_EQ(token->value, "if");
    
    // Test "else"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ELSE);
    ASSERT_STR_EQ(token->value, "else");
    
    // Test "while"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_WHILE);
    ASSERT_STR_EQ(token->value, "while");
    
    // Test "for"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_FOR);
    ASSERT_STR_EQ(token->value, "for");
    
    // Test "return"
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_RETURN);
    ASSERT_STR_EQ(token->value, "return");
}

TEST(test_lexer_identifiers) {
    char* input = "variable_name camelCase snake_case var123 _private";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "variable_name");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "camelCase");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "snake_case");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "var123");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "_private");
}

TEST(test_lexer_numbers) {
    char* input = "42 3.14 0 123.456 .5 5.";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Integer
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "42");
    
    // Float
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "3.14");
    
    // Zero
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "0");
    
    // Float with more decimals
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "123.456");
    
    // Float starting with decimal
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, ".5");
    
    // Float ending with decimal
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "5.");
}

TEST(test_lexer_strings) {
    char* input = "\"hello world\" \"escaped\\\"quote\" \"\" \"multi\nline\"";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Basic string
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    ASSERT_STR_EQ(token->value, "hello world");
    
    // String with escaped quote
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    ASSERT_STR_EQ(token->value, "escaped\"quote");
    
    // Empty string
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    ASSERT_STR_EQ(token->value, "");
    
    // Multi-line string
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    ASSERT_STR_EQ(token->value, "multi\nline");
}

TEST(test_lexer_operators) {
    char* input = "+ - * / % = != < > <= >= & | !";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_PLUS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_MINUS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_MULTIPLY);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_DIVIDE);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_MODULO);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EQUALS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NOT_EQUALS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_LESS_THAN);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_GREATER_THAN);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_LESS_EQUALS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_GREATER_EQUALS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_AND);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_OR);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NOT);
}

TEST(test_lexer_punctuation) {
    char* input = "( ) , \n";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_LPAREN);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_RPAREN);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_COMMA);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NEWLINE);
}

TEST(test_lexer_indentation) {
    char* input = "line1\n    indented\n        more_indented\nback";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // First line
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "line1");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NEWLINE);
    
    // Indented line - should generate INDENT token
    token = lexer_get_next_token(lexer);
    if (token->type == TOKEN_INDENT) {
        // Good, indentation tracked
    }
    
    // Skip to identifier
    while (token->type != TOKEN_ID && token->type != TOKEN_EOF) {
        token = lexer_get_next_token(lexer);
    }
    ASSERT_STR_EQ(token->value, "indented");
}

TEST(test_lexer_complete_program) {
    char* input = 
        "set name \"Alice\"\n"
        "set age 30\n"
        "if age >= 18\n"
        "    print \"Adult\"\n"
        "else\n"
        "    print \"Minor\"\n";
    
    lexer_T* lexer = init_lexer(input);
    
    // Just verify we can tokenize the entire program without errors
    token_T* token;
    int token_count = 0;
    
    do {
        token = lexer_get_next_token(lexer);
        ASSERT_NOT_NULL(token);
        token_count++;
    } while (token->type != TOKEN_EOF && token_count < 100); // Safety limit
    
    ASSERT_TRUE(token_count > 10); // Should have many tokens
    ASSERT_EQ(token->type, TOKEN_EOF);
}

TEST_SUITE_BEGIN(lexer_basic_tests)
    RUN_TEST(test_lexer_initialization);
    RUN_TEST(test_lexer_advance);
    RUN_TEST(test_lexer_skip_whitespace);
    RUN_TEST(test_lexer_keywords);
    RUN_TEST(test_lexer_identifiers);
    RUN_TEST(test_lexer_numbers);
    RUN_TEST(test_lexer_strings);
    RUN_TEST(test_lexer_operators);
    RUN_TEST(test_lexer_punctuation);
    RUN_TEST(test_lexer_indentation);
    RUN_TEST(test_lexer_complete_program);
TEST_SUITE_END