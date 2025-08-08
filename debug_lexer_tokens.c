#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>

int main() {
    const char* input = "name \"Alice\", age 30";
    lexer_T* lexer = lexer_new(input);
    
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    
    printf("Tokenizing: %s\n", input);
    
    token_T* token;
    int count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF && count < 10) {
        printf("Token %d: Type=%d, Value='%s'\n", count++, token->type, token->value ? token->value : "(null)");
        token_free(token);
    }
    
    if (token) token_free(token);
    lexer_free(lexer);
    
    return 0;
}