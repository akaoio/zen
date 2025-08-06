#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_lexer_collect_string() {
    printf("Testing lexer_collect_string...\n");
    
    // Test 1: Simple string
    char* input1 = "\"hello\"";
    lexer_T* lexer1 = init_lexer(input1);
    lexer1->c = '"'; // Position at quote
    token_T* token1 = lexer_collect_string(lexer1);
    assert(token1 != NULL);
    assert(token1->type == TOKEN_STRING);
    assert(strcmp(token1->value, "hello") == 0);
    printf("✓ Simple string test passed\n");
    
    // Test 2: String with escape sequences
    char* input2 = "\"hello\\nworld\\t!\"";
    lexer_T* lexer2 = init_lexer(input2);
    lexer2->c = '"'; // Position at quote
    token_T* token2 = lexer_collect_string(lexer2);
    assert(token2 != NULL);
    assert(token2->type == TOKEN_STRING);
    printf("✓ Escape sequences test passed\n");
    
    // Test 3: Long string (>999 characters)
    char long_input[1100] = "\"";
    for (int i = 0; i < 1000; i++) {
        strcat(long_input, "a");
    }
    strcat(long_input, "\"");
    
    lexer_T* lexer3 = init_lexer(long_input);
    lexer3->c = '"'; // Position at quote
    token_T* token3 = lexer_collect_string(lexer3);
    assert(token3 != NULL);
    assert(token3->type == TOKEN_STRING);
    assert(strlen(token3->value) == 1000);
    printf("✓ Long string test passed\n");
    
    printf("lexer_collect_string: ALL TESTS PASSED\n");
}

void test_lexer_collect_number() {
    printf("Testing lexer_collect_number...\n");
    
    // Test 1: Simple integer
    char* input1 = "42";
    lexer_T* lexer1 = init_lexer(input1);
    lexer1->c = '4'; // Position at digit
    token_T* token1 = lexer_collect_number(lexer1);
    assert(token1 != NULL);
    assert(token1->type == TOKEN_NUMBER);
    assert(strcmp(token1->value, "42") == 0);
    printf("✓ Simple integer test passed\n");
    
    // Test 2: Float
    char* input2 = "3.14159";
    lexer_T* lexer2 = init_lexer(input2);
    lexer2->c = '3'; // Position at digit
    token_T* token2 = lexer_collect_number(lexer2);
    assert(token2 != NULL);
    assert(token2->type == TOKEN_NUMBER);
    assert(strcmp(token2->value, "3.14159") == 0);
    printf("✓ Float test passed\n");
    
    // Test 3: Scientific notation
    char* input3 = "1.5e10";
    lexer_T* lexer3 = init_lexer(input3);
    lexer3->c = '1'; // Position at digit
    token_T* token3 = lexer_collect_number(lexer3);
    assert(token3 != NULL);
    assert(token3->type == TOKEN_NUMBER);
    assert(strcmp(token3->value, "1.5e10") == 0);
    printf("✓ Scientific notation test passed\n");
    
    // Test 4: Negative scientific notation
    char* input4 = "2.3e-4";
    lexer_T* lexer4 = init_lexer(input4);
    lexer4->c = '2'; // Position at digit
    token_T* token4 = lexer_collect_number(lexer4);
    assert(token4 != NULL);
    assert(token4->type == TOKEN_NUMBER);
    assert(strcmp(token4->value, "2.3e-4") == 0);
    printf("✓ Negative scientific notation test passed\n");
    
    printf("lexer_collect_number: ALL TESTS PASSED\n");
}

void test_lexer_collect_id() {
    printf("Testing lexer_collect_id...\n");
    
    // Test 1: Simple identifier
    char* input1 = "variable";
    lexer_T* lexer1 = init_lexer(input1);
    lexer1->c = 'v'; // Position at letter
    token_T* token1 = lexer_collect_id(lexer1);
    assert(token1 != NULL);
    assert(token1->type == TOKEN_ID);
    assert(strcmp(token1->value, "variable") == 0);
    printf("✓ Simple identifier test passed\n");
    
    // Test 2: Keyword recognition
    char* input2 = "function";
    lexer_T* lexer2 = init_lexer(input2);
    lexer2->c = 'f'; // Position at letter
    token_T* token2 = lexer_collect_id(lexer2);
    assert(token2 != NULL);
    assert(token2->type == TOKEN_FUNCTION);
    assert(strcmp(token2->value, "function") == 0);
    printf("✓ Keyword recognition test passed\n");
    
    // Test 3: Long identifier
    char long_id[1100];
    strcpy(long_id, "very_long_identifier_");
    for (int i = 0; i < 50; i++) {
        strcat(long_id, "abcdefghij");
    }
    
    lexer_T* lexer3 = init_lexer(long_id);
    lexer3->c = 'v'; // Position at letter
    token_T* token3 = lexer_collect_id(lexer3);
    assert(token3 != NULL);
    assert(token3->type == TOKEN_ID);
    assert(strlen(token3->value) == strlen(long_id));
    printf("✓ Long identifier test passed\n");
    
    printf("lexer_collect_id: ALL TESTS PASSED\n");
}

void test_lexer_advance_with_token() {
    printf("Testing lexer_advance_with_token...\n");
    
    char* input = "abc";
    lexer_T* lexer = init_lexer(input);
    lexer->c = 'a';
    lexer->i = 0;
    
    token_T* test_token = init_token(TOKEN_ID, "test");
    token_T* result = lexer_advance_with_token(lexer, test_token);
    
    assert(result == test_token); // Should return the same token
    assert(lexer->i == 1); // Should have advanced position
    assert(lexer->c == 'b'); // Should be at next character
    
    printf("✓ lexer_advance_with_token: ALL TESTS PASSED\n");
}

int main() {
    printf("=== TESTING LEXER TOKEN COLLECTION FUNCTIONS ===\n\n");
    
    test_lexer_collect_string();
    printf("\n");
    
    test_lexer_collect_number();
    printf("\n");
    
    test_lexer_collect_id();
    printf("\n");
    
    test_lexer_advance_with_token();
    printf("\n");
    
    printf("=== ALL LEXER FUNCTION TESTS COMPLETED SUCCESSFULLY ===\n");
    return 0;
}