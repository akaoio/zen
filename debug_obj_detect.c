#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/memory.h"
#include <stdio.h>

int main(void) {
    
    printf("Testing lexer_peek_token functionality...\n");
    
    char* input = "name \"Alice\", age 30";
    lexer_T* lexer = lexer_new(input);
    
    printf("Created lexer for input: %s\n", input);
    
    token_T* tok0 = lexer_peek_token(lexer, 0);
    if (tok0) {
        printf("Token 0: type=%d, value='%s'\n", tok0->type, tok0->value);
        token_free(tok0);
    } else {
        printf("Token 0: NULL\n");
    }
    
    token_T* tok1 = lexer_peek_token(lexer, 1);
    if (tok1) {
        printf("Token 1: type=%d, value='%s'\n", tok1->type, tok1->value);
        token_free(tok1);
    } else {
        printf("Token 1: NULL\n");
    }
    
    printf("Testing completed successfully\n");
    
    lexer_free(lexer);
    return 0;
}