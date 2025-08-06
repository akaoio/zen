#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>

int main() {
    char* input = "1e5";
    printf("Testing: %s\n", input);
    
    lexer_T* lexer = init_lexer(input);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    
    token_T* token = lexer_get_next_token(lexer);
    if (!token) {
        printf("Failed to get token\n");
        return 1;
    }
    
    printf("Token type: %d\n", token->type);
    printf("Token value: '%s'\n", token->value);
    
    lexer_free(lexer);
    return 0;
}