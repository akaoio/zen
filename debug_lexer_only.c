/**
 * Debug lexer-only memory leaks
 */
#include <stdio.h>
#include <stdlib.h>

#include "zen/core/lexer.h"
#include "zen/core/memory.h"

int main() {
    printf("=== Lexer-Only Memory Debug ===\n");
    
    memory_debug_enable(true);
    
    const char* code = "set x 42\n";
    
    printf("Creating lexer with code: %s", code);
    
    lexer_T* lexer = lexer_new((char*)code);
    if (!lexer) {
        printf("ERROR: Failed to create lexer\n");
        return 1;
    }
    
    printf("Lexer created successfully\n");
    
    // Get a few tokens to see if that causes more leaks
    token_T* token1 = lexer_get_next_token(lexer);
    printf("Token 1: type=%d\n", token1 ? token1->type : -1);
    
    token_T* token2 = lexer_get_next_token(lexer);
    printf("Token 2: type=%d\n", token2 ? token2->type : -1);
    
    token_T* token3 = lexer_get_next_token(lexer);
    printf("Token 3: type=%d\n", token3 ? token3->type : -1);
    
    // Free tokens
    if (token1) token_free(token1);
    if (token2) token_free(token2);
    if (token3) token_free(token3);
    
    printf("Freeing lexer...\n");
    lexer_free(lexer);
    
    printf("Checking for leaks...\n");
    if (memory_check_leaks()) {
        printf("MEMORY LEAKS:\n");
        memory_print_leak_report();
    } else {
        printf("No memory leaks detected\n");
    }
    
    memory_debug_cleanup();
    
    return 0;
}