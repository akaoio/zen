#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing scientific notation parsing:\n\n");
    
    char* input = "1e5 2.5e-3 1.23E+10";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    int count = 0;
    
    while ((token = lexer_get_next_token(lexer))->type != TOKEN_EOF && count < 10) {
        printf("Token %d: type=%d, value='%s'\n", count, token->type, token->value);
        count++;
    }
    
    lexer_free(lexer);
    return 0;
}