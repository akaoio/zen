#include <stdio.h>
#include <string.h>
#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include "zen/core/memory.h"

int main() {
    char* code = memory_strdup("set obj.prop \"value\"");
    lexer_T* lexer = lexer_new(code);
    
    token_T* token;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        printf("Token: type=%d, value='%s'\n", token->type, token->value);
        token_free(token);
    }
    
    lexer_free(lexer);
    memory_free(code);
    return 0;
}