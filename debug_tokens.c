#include <stdio.h>
#include <string.h>
#include "zen/core/lexer.h"
#include "zen/core/memory.h"

int main() {
    memory_debug_enable(true);
    
    const char* input = "print \"hello\"";
    printf("Input: %s\n", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    
    // First token
    token_T* token1 = lexer_get_next_token(lexer);
    printf("Token 1: type=%d, value=\"%s\"\n", (int)token1->type, token1->value ? token1->value : "NULL");
    
    // Second token  
    token_T* token2 = lexer_get_next_token(lexer);
    printf("Token 2: type=%d, value=\"", (int)token2->type);
    if (token2->value) {
        for (int i = 0; i < 10 && token2->value[i]; i++) {
            char c = token2->value[i];
            if (c >= 32 && c <= 126) {
                printf("%c", c);
            } else {
                printf("\\x%02x", (unsigned char)c);
            }
        }
    } else {
        printf("NULL");
    }
    printf("\"\n");
    
    token_free(token1);
    token_free(token2);
    lexer_free(lexer);
    memory_print_leak_report();
    return 0;
}
