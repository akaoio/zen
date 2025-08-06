#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Detailed debugging of '1e5':\n");
    
    char* input = "1e5";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: '%s'\n", input);
    printf("Input length: %zu\n", strlen(input));
    
    printf("\nStep by step:\n");
    printf("Initial: c='%c', i=%d\n", lexer->c, lexer->i);
    
    // Manual stepping
    if (isdigit(lexer->c)) {
        printf("Found digit, calling lexer_collect_number\n");
        token_T* token = lexer_collect_number(lexer);
        printf("Result: type=%d, value='%s'\n", token->type, token->value);
        printf("After collection: c='%c', i=%d\n", lexer->c, lexer->i);
    }
    
    printf("\nNext character analysis:\n");
    if (lexer->c \!= '\0') {
        printf("Next char: '%c' (ASCII %d)\n", lexer->c, (int)lexer->c);
        if (lexer->c == 'e' || lexer->c == 'E') {
            printf("This is 'e' - should be part of scientific notation\!\n");
            if (lexer->i + 1 < strlen(lexer->contents)) {
                char next = lexer->contents[lexer->i + 1];
                printf("Character after 'e': '%c'\n", next);
                if (isdigit(next)) {
                    printf("FOUND SCIENTIFIC NOTATION PATTERN\!\n");
                }
            }
        }
    }
    
    lexer_free(lexer);
    return 0;
}
EOF < /dev/null
