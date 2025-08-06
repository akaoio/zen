#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* input = "set x 5";
    lexer_T* lexer = init_lexer(input);
    
    printf("Initial state: at_line_start=%d, current_indent=%d\n", 
           lexer->at_line_start, lexer->current_indent);
    
    token_T* token;
    int count = 0;
    while (count < 10) {  // Safety limit
        printf("Before token %d: at_line_start=%d, current_indent=%d, c='%c'\n", 
               count, lexer->at_line_start, lexer->current_indent, lexer->c);
        
        token = lexer_get_next_token(lexer);
        if (!token) break;
        
        printf("Token %d: type=%d, value='%s'\n", count, token->type, token->value ? token->value : "NULL");
        
        if (token->type == TOKEN_EOF) break;
        count++;
    }
    
    return 0;
}