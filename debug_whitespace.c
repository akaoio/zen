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
    printf("=== Debug Whitespace Edge Cases ===\n");
    
    // Test empty string
    char* input1 = "";
    lexer_T* lexer1 = init_lexer(input1);
    token_T* token1 = lexer_get_next_token(lexer1);
    printf("Empty string -> Token type: %d (TOKEN_EOF=%d)\n", token1->type, TOKEN_EOF);
    
    // Test whitespace only
    char* input2 = "   \n\t  ";
    lexer_T* lexer2 = init_lexer(input2);
    token_T* token2 = lexer_get_next_token(lexer2);
    printf("Whitespace only -> Token type: %d (TOKEN_EOF=%d, TOKEN_NEWLINE=%d)\n", 
           token2->type, TOKEN_EOF, TOKEN_NEWLINE);
    
    // Test if we get more tokens
    if (token2->type != TOKEN_EOF) {
        token_T* token3 = lexer_get_next_token(lexer2);
        printf("Next token -> Token type: %d\n", token3->type);
    }
    
    return 0;
}