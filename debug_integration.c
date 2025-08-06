#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Test with a simple 10 char identifier first
    char test_input[20] = "abcdefghij";
    
    printf("=== Testing simple 10 char identifier: '%s' ===\n", test_input);
    
    lexer_T* lexer = init_lexer(test_input);
    printf("Lexer initialized, first char: '%c' at pos %d\n", lexer->c, lexer->i);
    
    // Call the main entry point
    token_T* token = lexer_get_next_token(lexer);
    
    printf("Token received:\n");
    printf("  Type: %d\n", token->type);
    printf("  Value: '%s'\n", token->value ? token->value : "(null)");
    printf("  Value length: %zu\n", token->value ? strlen(token->value) : 0);
    
    lexer_free(lexer);
    
    return 0;
}