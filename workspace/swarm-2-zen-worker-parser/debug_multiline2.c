#include "zen/core/lexer.h"
#include "zen/core/token.h" 
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* input = "set x 5\nset y 10";
    lexer_T* lexer = init_lexer(input);
    
    printf("Testing: %s\n\n", input);
    
    token_T* token;
    int count = 0;
    while (count < 10) {
        printf("Before token %d: i=%d, c='%c', at_line_start=%d, current_indent=%d\n",
               count, lexer->i, lexer->c, lexer->at_line_start, lexer->current_indent);
        
        token = lexer_get_next_token(lexer);
        if (!token) break;
        
        printf("Token %d: type=%d, value='%s'\n\n", count, token->type, token->value ? token->value : "NULL");
        
        if (token->type == TOKEN_EOF) break;
        count++;
    }
    
    return 0;
}