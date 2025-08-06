#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* input = "set x 5\nset y 10\nset result x and y\nprint result";
    lexer_T* lexer = init_lexer(input);
    
    printf("Testing multiline lexer with input:\n%s\n\n", input);
    
    token_T* token;
    int count = 0;
    while (count < 20) {  // Safety limit
        token = lexer_get_next_token(lexer);
        if (!token) break;
        
        printf("Token %d: type=%d, value='%s'\n", count, token->type, token->value ? token->value : "NULL");
        
        if (token->type == TOKEN_EOF) break;
        count++;
    }
    
    return 0;
}