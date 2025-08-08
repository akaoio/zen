#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>

int main() {
    const char* input = "a 1, b 2";
    lexer_T* lexer = lexer_new(input);
    
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    
    printf("=== TOKENIZING: %s ===\n", input);
    
    token_T* token;
    int count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF && count < 10) {
        printf("Token %d: Type=%d, Value='%s'\n", 
               count++, (int)token->type, token->value ? token->value : "(null)");
        token_free(token);
    }
    
    if (token && token->type == TOKEN_EOF) {
        printf("Token %d: Type=%d (EOF)\n", count, (int)token->type);
        token_free(token);
    }
    
    lexer_free(lexer);
    return 0;
}