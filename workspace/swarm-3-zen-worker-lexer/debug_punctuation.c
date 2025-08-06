#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>

int main() {
    char* input = "( ) , \n";
    printf("Testing input: \"%s\"\n", input);
    printf("Input bytes: ");
    for (int i = 0; input[i] != '\0'; i++) {
        printf("%d ", (int)input[i]);
    }
    printf("\n");
    
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    int token_count = 0;
    
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF && token_count < 10) {
        printf("Token %d: type=%d, value=\"%s\"\n", token_count + 1, token->type, token->value);
        token_count++;
    }
    
    printf("Final token: type=%d, value=\"%s\"\n", token->type, token->value);
    printf("Total tokens: %d\n", token_count);
    
    return 0;
}