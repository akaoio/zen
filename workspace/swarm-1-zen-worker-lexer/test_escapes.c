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
    printf("=== Testing String Escape Sequences ===\n");
    
    // Test various escape sequences
    char* input = "\"hello\\nworld\\t!\\\"\"";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: %s\n", input);
    
    token_T* token = lexer_get_next_token(lexer);
    printf("Token type: %d (expected TOKEN_STRING which is %d)\n", token->type, TOKEN_STRING);
    printf("Token value: '%s'\n", token->value);
    printf("Token value length: %zu\n", strlen(token->value));
    
    // Check if escape sequences were processed correctly
    if (token->type == TOKEN_STRING && 
        strstr(token->value, "\n") != NULL && 
        strstr(token->value, "\t") != NULL &&
        strstr(token->value, "\"") != NULL) {
        printf("✓ PASS - Escape sequences working!\n");
    } else {
        printf("✗ FAIL - Escape sequences broken\n");
    }
    
    return 0;
}