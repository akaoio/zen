#include "zen/core/lexer.h"
#include <stdio.h>

int main() {
    const char* input = "set x 5";
    lexer_T* lexer = init_lexer(input);
    
    printf("Debugging tokens for: '%s'\n", input);
    
    token_T* token;
    int count = 0;
    while (count < 10) {  // Limit to prevent infinite loops
        token = lexer_get_next_token(lexer);
        if (!token) break;
        
        printf("Token %d: type=%d, value='%s'\n", count, token->type, token->value ? token->value : "(null)");
        
        if (token->type == TOKEN_EOF) break;
        count++;
    }
    
    return 0;
}