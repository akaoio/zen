#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"

int main() {
    char* input = "set a true";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: '%s'\n", input);
    printf("All tokens:\n");
    
    token_T* token;
    int count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != 0 && count < 10) { // TOKEN_EOF = 0
        printf("Token %d: Type=%d, Value='%s'\n", count++, token->type, token->value ? token->value : "(null)");
    }
    
    return 0;
}