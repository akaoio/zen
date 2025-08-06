#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    // Test scientific notation
    printf("=== Testing Scientific Notation ===\n");
    char* inputs[] = {"1e5", "2.5e-3", "1.23E+10", "5e0", NULL};
    
    for (int i = 0; inputs[i]; i++) {
        printf("Input: %s\n", inputs[i]);
        lexer_T* lexer = init_lexer(inputs[i]);
        token_T* token = lexer_get_next_token(lexer);
        printf("  Token type: %d, value: '%s'\n", token->type, token->value);
        lexer_free(lexer);
        printf("\n");
    }
    
    // Test string escapes
    printf("=== Testing String Escapes ===\n");
    char* test_str = "\"\\n\\t\\\"\"";
    printf("Input: %s\n", test_str);
    lexer_T* lexer = init_lexer(test_str);
    token_T* token = lexer_get_next_token(lexer);
    printf("  Token type: %d, value: '%s'\n", token->type, token->value);
    printf("  Value length: %zu\n", strlen(token->value));
    for (size_t i = 0; i < strlen(token->value); i++) {
        printf("  [%zu]: %c (ASCII %d)\n", i, token->value[i], (int)token->value[i]);
    }
    lexer_free(lexer);
    
    return 0;
}
EOF < /dev/null
