#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Create a 999 character identifier like the test
    char long_name[1000];
    for (int i = 0; i < 999; i++) {
        long_name[i] = 'a' + (i % 26);
    }
    long_name[999] = '\0';
    
    printf("Created identifier of length: %zu\n", strlen(long_name));
    
    lexer_T* lexer = init_lexer(long_name);
    token_T* token = lexer_get_next_token(lexer);
    
    printf("Token type: %d\n", token->type);
    printf("Token value length: %zu\n", strlen(token->value));
    printf("First 50 chars: %.50s\n", token->value);
    printf("Last 50 chars: %s\n", token->value + strlen(token->value) - 50);
    printf("Expected length 999? %s\n", strlen(token->value) == 999 ? "YES" : "NO");
    
    lexer_free(lexer);
    return 0;
}