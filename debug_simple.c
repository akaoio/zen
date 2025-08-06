#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/memory.h"

int main() {
    memory_debug_enable(true);
    
    const char* input = "print \"hello\"";
    printf("Input: %s\n", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    printf("Lexer initialized\n");
    
    token_T* token = lexer_get_next_token(lexer);
    printf("Got first token, type: %d\n", (int)token->type);
    
    if (token->value) {
        printf("Token value length: %zu\n", strlen(token->value));
        printf("First few chars: ");
        for (int i = 0; i < 5 && token->value[i]; i++) {
            printf("%02x ", (unsigned char)token->value[i]);
        }
        printf("\n");
    }
    
    token_free(token);
    lexer_free(lexer);
    memory_print_leak_report();
    return 0;
}
