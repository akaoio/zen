#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the actual headers and test framework
#include "src/include/zen/core/lexer.h" 
#include "src/include/zen/core/token.h"

// Include actual implementations with our fixes
#include "src/core/lexer.c"
#include "src/core/token.c"
#include "src/core/memory.c"

int main() {
    printf("=== Testing Fixed String Collection ===\n");
    
    // Test string collection
    char* input1 = "\"hello world\"";
    lexer_T* lexer1 = init_lexer(input1);
    
    printf("Input: %s\n", input1);
    
    token_T* token1 = lexer_get_next_token(lexer1);
    printf("Token type: %d (expected TOKEN_STRING which is %d)\n", token1->type, TOKEN_STRING);
    printf("Token value: '%s' (expected 'hello world')\n", token1->value);
    printf("Token value length: %zu\n", strlen(token1->value));
    
    if (token1->type == TOKEN_STRING && strcmp(token1->value, "hello world") == 0) {
        printf("✓ PASS - String collection fixed!\n");
    } else {
        printf("✗ FAIL - String collection still broken\n");
    }
    
    // Test identifier collection
    printf("\n=== Testing Fixed Identifier Collection ===\n");
    char* input2 = "set";
    lexer_T* lexer2 = init_lexer(input2);
    
    printf("Input: %s\n", input2);
    
    token_T* token2 = lexer_get_next_token(lexer2);
    printf("Token type: %d (expected TOKEN_SET which is %d)\n", token2->type, TOKEN_SET);
    printf("Token value: '%s' (expected 'set')\n", token2->value);
    printf("Token value length: %zu\n", strlen(token2->value));
    
    if (token2->type == TOKEN_SET && strcmp(token2->value, "set") == 0) {
        printf("✓ PASS - Identifier collection fixed!\n");
    } else {
        printf("✗ FAIL - Identifier collection still broken\n");
    }
    
    // Test decimal number starting with '.'
    printf("\n=== Testing Decimal Numbers ===\n");
    char* input3 = ".5 5. 3.14";
    lexer_T* lexer3 = init_lexer(input3);
    
    printf("Input: %s\n", input3);
    
    token_T* token3a = lexer_get_next_token(lexer3);
    printf("Token 1 type: %d, value: '%s' (expected TOKEN_NUMBER, '.5')\n", token3a->type, token3a->value);
    
    token_T* token3b = lexer_get_next_token(lexer3);
    printf("Token 2 type: %d, value: '%s' (expected TOKEN_NUMBER, '5.')\n", token3b->type, token3b->value);
    
    token_T* token3c = lexer_get_next_token(lexer3);
    printf("Token 3 type: %d, value: '%s' (expected TOKEN_NUMBER, '3.14')\n", token3c->type, token3c->value);
    
    if (token3a->type == TOKEN_NUMBER && strcmp(token3a->value, ".5") == 0 &&
        token3b->type == TOKEN_NUMBER && strcmp(token3b->value, "5.") == 0 &&
        token3c->type == TOKEN_NUMBER && strcmp(token3c->value, "3.14") == 0) {
        printf("✓ PASS - Decimal number parsing fixed!\n");
    } else {
        printf("✗ FAIL - Decimal number parsing still broken\n");
    }
    
    // Test scientific notation
    printf("\n=== Testing Scientific Notation ===\n");
    char* input4 = "1e5 2.5e-3 1.23E+10";
    lexer_T* lexer4 = init_lexer(input4);
    
    printf("Input: %s\n", input4);
    
    token_T* token4a = lexer_get_next_token(lexer4);
    printf("Token 1 type: %d, value: '%s' (expected TOKEN_NUMBER, '1e5')\n", token4a->type, token4a->value);
    
    token_T* token4b = lexer_get_next_token(lexer4);
    printf("Token 2 type: %d, value: '%s' (expected TOKEN_NUMBER, '2.5e-3')\n", token4b->type, token4b->value);
    
    token_T* token4c = lexer_get_next_token(lexer4);
    printf("Token 3 type: %d, value: '%s' (expected TOKEN_NUMBER, '1.23E+10')\n", token4c->type, token4c->value);
    
    if (token4a->type == TOKEN_NUMBER && strcmp(token4a->value, "1e5") == 0 &&
        token4b->type == TOKEN_NUMBER && strcmp(token4b->value, "2.5e-3") == 0 &&
        token4c->type == TOKEN_NUMBER && strcmp(token4c->value, "1.23E+10") == 0) {
        printf("✓ PASS - Scientific notation parsing working!\n");
    } else {
        printf("✗ FAIL - Scientific notation parsing broken\n");
    }
    
    return 0;
}