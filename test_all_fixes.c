#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h" 
#include <stdio.h>
#include <string.h>

void test_long_identifier() {
    printf("=== Testing Long Identifier (999 chars) ===\n");
    
    char test_input[1005];
    for (int i = 0; i < 999; i++) {
        test_input[i] = 'a' + (i % 26);
    }
    test_input[999] = '\0';
    
    lexer_T* lexer = init_lexer(test_input);
    token_T* token = lexer_get_next_token(lexer);
    
    printf("Result: %s (length: %zu)\n", 
           token && token->value && strlen(token->value) == 999 ? "PASS" : "FAIL",
           token && token->value ? strlen(token->value) : 0);
    
    lexer_free(lexer);
}

void test_long_string() {
    printf("=== Testing Long String with Escapes (500 chars) ===\n");
    
    char test_input[1010];
    strcpy(test_input, "\"");
    
    // Build a long string with various escape sequences
    for (int i = 0; i < 100; i++) {
        strcat(test_input, "abc\\n\\t\\\"def");
    }
    strcat(test_input, "\"");
    
    lexer_T* lexer = init_lexer(test_input);
    token_T* token = lexer_get_next_token(lexer);
    
    printf("Result: %s (length: %zu)\n", 
           token && token->value && strlen(token->value) > 400 ? "PASS" : "FAIL",
           token && token->value ? strlen(token->value) : 0);
    
    lexer_free(lexer);
}

void test_scientific_notation() {
    printf("=== Testing Scientific Notation ===\n");
    
    char test_input[] = "1.5e10 2.3e-4 5e6";
    lexer_T* lexer = init_lexer(test_input);
    
    // Test first number
    token_T* token1 = lexer_get_next_token(lexer);
    printf("1.5e10: %s ('%s')\n", 
           token1 && token1->value && strcmp(token1->value, "1.5e10") == 0 ? "PASS" : "FAIL",
           token1 && token1->value ? token1->value : "null");
    
    // Skip whitespace
    token_T* ws1 = lexer_get_next_token(lexer); // might be whitespace
    if (ws1 && ws1->type != TOKEN_NUMBER) {
        token_T* token2 = lexer_get_next_token(lexer);
        printf("2.3e-4: %s ('%s')\n", 
               token2 && token2->value && strcmp(token2->value, "2.3e-4") == 0 ? "PASS" : "FAIL",
               token2 && token2->value ? token2->value : "null");
    }
    
    lexer_free(lexer);
}

int main() {
    printf("Testing All Lexer Fixes\n");
    printf("========================\n\n");
    
    test_long_identifier();
    printf("\n");
    
    test_long_string(); 
    printf("\n");
    
    test_scientific_notation();
    printf("\n");
    
    printf("All tests completed!\n");
    return 0;
}