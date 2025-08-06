#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>

int main() {
    char* input = "1e5";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token = lexer_get_next_token(lexer);
    printf("Input: %s\n", input);
    printf("Token type: %d\n", token->type);
    printf("Token value: '%s'\n", token->value);
    
    lexer_free(lexer);
    return 0;
}