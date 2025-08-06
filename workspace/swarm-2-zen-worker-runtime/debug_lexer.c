#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    char* test = "set sum x + y";
    printf("Testing lexer with: '%s'\n", test);
    
    lexer_T* lexer = init_lexer(test);
    token_T* token;
    
    while ((token = lexer_get_next_token(lexer))->type != TOKEN_EOF)
    {
        printf("Token: type=%d, value='%s'\n", token->type, token->value);
    }
    printf("Token: type=%d, value='%s'\n", token->type, token->value);
    
    return 0;
}