#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the actual headers and test framework
#include "zen/core/lexer.h" 
#include "zen/core/token.h"

// Include actual implementations
#include "src/core/lexer.c"
#include "src/core/token.c"
#include "src/core/memory.c"

int main() {
    printf("=== Debug String Collection Issue ===\n");
    
    // Test specifically the string collection that's failing
    char* input = "\"hello world\"";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: %s\n", input);
    
    token_T* token = lexer_get_next_token(lexer);
    printf("Token type: %d (expected TOKEN_STRING which is %d)\n", token->type, TOKEN_STRING);
    printf("Token value: '%s' (expected 'hello world')\n", token->value);
    printf("Token value length: %zu\n", strlen(token->value));
    
    if (token->type == TOKEN_STRING) {
        if (strlen(token->value) == 0) {
            printf("✗ FAIL - String is empty when it should contain 'hello world'\n");
        } else {
            printf("✓ PASS - String has content\n");
        }
    } else {
        printf("✗ FAIL - Token is not a string token\n");
    }
    
    // Test identifier collection too
    printf("\n=== Debug Identifier Collection ===\n");
    char* input2 = "set";
    lexer_T* lexer2 = init_lexer(input2);
    
    printf("Input: %s\n", input2);
    
    token_T* token2 = lexer_get_next_token(lexer2);
    printf("Token type: %d (expected TOKEN_SET which is %d)\n", token2->type, TOKEN_SET);
    printf("Token value: '%s' (expected 'set')\n", token2->value);
    printf("Token value length: %zu\n", strlen(token2->value));
    
    if (token2->type == TOKEN_SET) {
        if (strlen(token2->value) == 0) {
            printf("✗ FAIL - Identifier is empty when it should contain 'set'\n");
        } else {
            printf("✓ PASS - Identifier has content\n");
        }
    } else {
        printf("✗ FAIL - Token is not TOKEN_SET (%d), got %d instead\n", TOKEN_SET, token2->type);
    }
    
    return 0;
}