#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/token.h"

int main() {
    printf("Testing lexer with 'set x 42'...\n");
    
    lexer_T* lexer = init_lexer("set x 42");
    if (!lexer) {
        printf("ERROR: Failed to initialize lexer\n");
        return 1;
    }
    
    printf("Lexer initialized successfully\n");
    
    token_T* token;
    int count = 0;
    
    while ((token = lexer_get_next_token(lexer)) != NULL && count < 10) {
        printf("Token %d: type=%d, value='%s'\n", 
               count, token->type, token->value ? token->value : "(null)");
        
        if (token->type == TOKEN_EOF) {
            printf("Reached EOF token\n");
            break;
        }
        
        count++;
    }
    
    printf("Tokenization complete, processed %d tokens\n", count);
    
    // Cleanup (note: lexer cleanup depends on implementation)
    // free(lexer); // Don't free until we check the implementation
    
    return 0;
}