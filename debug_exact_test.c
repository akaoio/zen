#include <stdio.h>
#include <stdlib.h>

// Include the actual headers and test framework
#include "zen/core/lexer.h" 
#include "zen/core/token.h"

// Simple memory functions for testing
void* memory_alloc(size_t size) { return calloc(1, size); }
void* memory_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
void memory_free(void* ptr) { if (ptr) free(ptr); }

// Include actual implementations
#include "src/core/lexer.c"
#include "src/core/token.c"
#include "src/core/memory.c"

int main() {
    printf("=== Exact test replication ===\n");
    
    // This exactly mirrors the failing test
    char* input = "set function if else while for return";
    lexer_T* lexer = init_lexer(input);
    
    token_T* token;
    
    // Test "set" - this is what failed
    printf("Getting first token from input: '%s'\n", input);
    token = lexer_get_next_token(lexer);
    printf("Token type: %d (expected some specific value)\n", token->type);
    printf("Token value: '%s' (expected 'set')\n", token->value);
    printf("TOKEN_SET constant value: %d\n", TOKEN_SET);
    
    if (token->type == TOKEN_SET) {
        printf("✓ PASS - Token type matches TOKEN_SET\n");
    } else {
        printf("✗ FAIL - Token type %d does not match TOKEN_SET %d\n", token->type, TOKEN_SET);
    }
    
    return 0;
}