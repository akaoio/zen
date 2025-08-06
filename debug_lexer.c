#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zen/core/lexer.h"
#include "zen/core/memory.h"

int main() {
    // Enable memory debugging
    memory_debug_enable(true);
    
    const char* input = "print \"hello\"";
    printf("Input: %s\n", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    printf("Lexer initialized\n");
    
    token_T* token;
    int count = 0;
    
    while (count < 10) {
        printf("\n=== Getting token %d ===\n", count);
        token = lexer_get_next_token(lexer);
        
        if (\!token) {
            printf("Token is NULL\n");
            break;
        }
        
        printf("Token type: %d\n", (int)token->type);
        if (token->value) {
            printf("Token value: '");
            for (int i = 0; i < 10 && token->value[i] \!= '\0'; i++) {
                char c = token->value[i];
                if (c >= 32 && c <= 126) {
                    printf("%c", c);
                } else {
                    printf("\\x%02x", (unsigned char)c);
                }
            }
            printf("'\n");
        } else {
            printf("Token value: NULL\n");
        }
        
        if (token->type == TOKEN_EOF) {
            token_free(token);
            break;
        }
        
        token_free(token);
        count++;
    }
    
    lexer_free(lexer);
    
    // Check for leaks
    memory_print_leak_report();
    memory_debug_cleanup();
    
    return 0;
}
EOF < /dev/null
