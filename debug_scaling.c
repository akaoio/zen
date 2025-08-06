#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Test with increasing identifier lengths
    char test_input[1050];
    int test_lengths[] = {10, 50, 100, 500, 999, 0};
    
    for (int i = 0; test_lengths[i] > 0; i++) {
        int len = test_lengths[i];
        
        // Create identifier of specified length
        for (int j = 0; j < len; j++) {
            test_input[j] = 'a' + (j % 26);
        }
        test_input[len] = '\0';
        
        printf("=== Testing %d char identifier ===\n", len);
        
        lexer_T* lexer = init_lexer(test_input);
        token_T* token = lexer_get_next_token(lexer);
        
        printf("Token received:\n");
        printf("  Type: %d\n", token->type);
        printf("  Value length: %zu\n", token->value ? strlen(token->value) : 0);
        printf("  Expected: %d - %s\n", len, 
               (token->value && strlen(token->value) == len) ? "PASS" : "FAIL");
        
        if (token->value && strlen(token->value) > 0) {
            printf("  First 10: %.10s\n", token->value);
            if (strlen(token->value) > 10) {
                printf("  Last 10: %s\n", token->value + strlen(token->value) - 10);
            }
        }
        
        lexer_free(lexer);
        printf("\n");
    }
    
    return 0;
}