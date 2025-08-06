#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the actual headers
#include "src/include/zen/core/lexer.h" 
#include "src/include/zen/core/token.h"

// Include actual implementations
#include "src/core/lexer.c"
#include "src/core/token.c"
#include "src/core/memory.c"

int main() {
    printf("=== Debug Operator Combinations ===\n");
    
    char* input = "=== !== <= >= += -= *= /= %= &&& |||";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    int count = 0;
    
    printf("Input: %s\n", input);
    printf("Tokenizing:\n");
    
    while ((token = lexer_get_next_token(lexer))->type != TOKEN_EOF && count < 40) {
        printf("%d. Type: %d, Value: '%s'\n", count + 1, token->type, token->value);
        count++;
    }
    
    printf("Total tokens: %d (test expects 31)\n", count);
    
    return 0;
}