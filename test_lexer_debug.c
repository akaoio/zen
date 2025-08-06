#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h" 
#include <stdio.h>

int main() {
    char* input = "x + y";
    lexer_T* lexer = init_lexer(input);
    
    while (1) {
        token_T* token = lexer_get_next_token(lexer);
        printf("Token: type=%d, value='%s'\n", token->type, token->value);
        if (token->type == TOKEN_EOF) break;
    }
    
    return 0;
}