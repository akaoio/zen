#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* input = "set x 5";
    lexer_T* lexer = init_lexer(input);
    
    printf("Testing lexer with input: '%s'\n", input);
    
    token_T* token;
    int count = 0;
    while (count < 10) {  // Safety limit
        token = lexer_get_next_token(lexer);
        if (!token) break;
        
        printf("Token %d: type=%d, value='%s'\n", count, token->type, token->value ? token->value : "NULL");
        
        if (token->type == TOKEN_EOF) break;
        count++;
    }
    
    return 0;
}