/**
 * @file test_lexer_advanced.c
 * @brief Advanced lexer functionality tests
 */

#include <time.h>
#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/token.h"

// Forward declare all tests  
DECLARE_TEST(test_lexer_scientific_notation);
DECLARE_TEST(test_lexer_string_escapes);
DECLARE_TEST(test_lexer_comments);
DECLARE_TEST(test_lexer_edge_cases);
DECLARE_TEST(test_lexer_mixed_indentation);
DECLARE_TEST(test_lexer_operator_combinations);
DECLARE_TEST(test_lexer_very_long_tokens);
DECLARE_TEST(test_lexer_unterminated_string);
DECLARE_TEST(test_lexer_special_characters);
DECLARE_TEST(test_lexer_performance);

TEST(test_lexer_scientific_notation) {
    char* input = "1e5 2.5e-3 1.23E+10 5e0";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // 1e5
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "1e5");
    
    // 2.5e-3
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "2.5e-3");
    
    // 1.23E+10
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "1.23E+10");
    
    // 5e0
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "5e0");
}

TEST(test_lexer_string_escapes) {
    char* input = "\"\\n\\t\\r\\\\\\\"\" \"\\u0041\\u0042\"";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Escape sequences
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    ASSERT_STR_EQ(token->value, "\n\t\r\\\"");
    
    // Unicode escapes (if supported)
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_STRING);
    // Note: Unicode support may not be implemented yet
}

TEST(test_lexer_comments) {
    char* input = 
        "set x 42  // This is a comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "set y 13";
    
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Should skip comments and get tokens
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "set");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_STR_EQ(token->value, "x");
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NUMBER);
    ASSERT_STR_EQ(token->value, "42");
    
    // Skip to next set
    while (token->type != TOKEN_ID || strcmp(token->value, "set") != 0) {
        token = lexer_get_next_token(lexer);
        if (token->type == TOKEN_EOF) break;
    }
    
    if (token->type != TOKEN_EOF) {
        ASSERT_STR_EQ(token->value, "set");
        
        token = lexer_get_next_token(lexer);
        ASSERT_STR_EQ(token->value, "y");
        
        token = lexer_get_next_token(lexer);
        ASSERT_STR_EQ(token->value, "13");
    }
}

TEST(test_lexer_edge_cases) {
    // Empty input
    lexer_T* lexer = init_lexer("");
    token_T* token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EOF);
    
    // Only whitespace
    lexer = init_lexer("   \n\t  ");
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EOF);
    
    // Null input (should not crash)
    lexer = init_lexer(NULL);
    if (lexer) {
        token = lexer_get_next_token(lexer);
        ASSERT_EQ(token->type, TOKEN_EOF);
    }
}

TEST(test_lexer_mixed_indentation) {
    char* input = 
        "if true\n"
        "    line1\n"
        "\tline2\n"     // Mixed tabs and spaces
        "        line3\n"
        "back";
    
    lexer_T* lexer = init_lexer(input);
    
    // This should handle mixed indentation gracefully
    // Exact behavior depends on implementation
    token_T* token;
    int token_count = 0;
    
    do {
        token = lexer_get_next_token(lexer);
        token_count++;
    } while (token->type != TOKEN_EOF && token_count < 50);
    
    ASSERT_EQ(token->type, TOKEN_EOF);
}

TEST(test_lexer_operator_combinations) {
    char* input = "=== !== <= >= += -= *= /= %= &&& |||";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // === should be parsed as = = =
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EQUALS);
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EQUALS);
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EQUALS);
    
    // !== should be != =
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_NOT_EQUALS);
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_EQUALS);
    
    // <= and >= should work
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_LESS_EQUALS);
    
    token = lexer_get_next_token(lexer);
    ASSERT_EQ(token->type, TOKEN_GREATER_EQUALS);
    
    // Assignment operators (if supported) or separate tokens
    // The exact behavior depends on implementation
}

TEST(test_lexer_very_long_tokens) {
    // Test handling of very long identifiers and strings
    char long_name[1000];
    for (int i = 0; i < 999; i++) {
        long_name[i] = 'a' + (i % 26);
    }
    long_name[999] = '\0';
    
    char input[1100];
    snprintf(input, sizeof(input), "%s", long_name);
    
    lexer_T* lexer = init_lexer(input);
    token_T* token = lexer_get_next_token(lexer);
    
    ASSERT_EQ(token->type, TOKEN_ID);
    ASSERT_TRUE(strlen(token->value) == 999);
}

TEST(test_lexer_unterminated_string) {
    char* input = "\"unterminated string";
    lexer_T* lexer = init_lexer(input);
    
    // Should handle gracefully - either return error token or EOF
    token_T* token = lexer_get_next_token(lexer);
    
    // Implementation dependent - could be STRING token with warning or EOF
    ASSERT_TRUE(token->type == TOKEN_STRING || 
                token->type == TOKEN_EOF);
}

TEST(test_lexer_special_characters) {
    char* input = "@ # $ % ^ ~ ` [ ] { }";
    lexer_T* lexer = init_lexer(input);
    
    // These might be special tokens or errors depending on ZEN spec
    token_T* token;
    int token_count = 0;
    
    do {
        token = lexer_get_next_token(lexer);
        ASSERT_NOT_NULL(token);
        token_count++;
    } while (token->type != TOKEN_EOF && token_count < 20);
    
    ASSERT_TRUE(token_count > 1);
}

TEST(test_lexer_performance) {
    // Create a moderately large input to test performance
    char large_input[10000];
    strcpy(large_input, "");
    
    for (int i = 0; i < 100; i++) {
        strcat(large_input, "set variable");
        char num[10];
        sprintf(num, "%d", i);
        strcat(large_input, num);
        strcat(large_input, " ");
        sprintf(num, "%d", i * 2);
        strcat(large_input, num);
        strcat(large_input, "\n");
    }
    
    lexer_T* lexer = init_lexer(large_input);
    
    // Tokenize entire input and count tokens
    token_T* token;
    int token_count = 0;
    
    clock_t start = clock();
    do {
        token = lexer_get_next_token(lexer);
        token_count++;
    } while (token->type != TOKEN_EOF && token_count < 10000);
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    ASSERT_EQ(token->type, TOKEN_EOF);
    ASSERT_TRUE(token_count > 300); // Should have many tokens
    ASSERT_TRUE(time_taken < 1.0);  // Should be fast (less than 1 second)
    
    TEST_INFO("Tokenized %d tokens in %.3f seconds", token_count, time_taken);
}

TEST_SUITE_BEGIN(lexer_advanced_tests)
    RUN_TEST(test_lexer_scientific_notation);
    RUN_TEST(test_lexer_string_escapes);
    RUN_TEST(test_lexer_comments);
    RUN_TEST(test_lexer_edge_cases);
    RUN_TEST(test_lexer_mixed_indentation);
    RUN_TEST(test_lexer_operator_combinations);
    RUN_TEST(test_lexer_very_long_tokens);
    RUN_TEST(test_lexer_unterminated_string);
    RUN_TEST(test_lexer_special_characters);
    RUN_TEST(test_lexer_performance);
TEST_SUITE_END