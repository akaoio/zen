#include "zen/core/lexer.h"
#include "zen/core/token.h" 
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* input = 
        "set x 42  // This is a comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "set y 13";
    
    lexer_T* lexer = init_lexer(input);
    token_T* token;
    int count = 0;
    
    printf("Debugging lexer tokens:\n");
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: type=%d, value='%s'\n", count++, token->type, token->value);
        if (count > 20) break;  // Safety limit
    }
    
    return 0;
}