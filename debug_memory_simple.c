#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Simulate the exact same scenario as the parser
    void** statements = calloc(1, sizeof(void*));
    printf("Initial calloc: %p\n", (void*)statements);
    
    // Store a pointer (simulate statement storage)
    statements[0] = malloc(64); // Simulate an AST node
    printf("Stored pointer at [0]: %p\n", statements[0]);
    
    // Check memory before realloc
    printf("Before realloc, statements[0] = %p\n", statements[0]);
    printf("Memory content: ");
    unsigned char* mem = (unsigned char*)statements;
    for (int i = 0; i < 8; i++) {
        printf("%02x ", mem[i]);
    }
    printf("\n");
    
    // Realloc to expand (same as parser does)
    void** old_statements = statements;
    statements = realloc(statements, 2 * sizeof(void*));
    printf("Realloc from %p to %p\n", (void*)old_statements, (void*)statements);
    
    // Check memory after realloc
    printf("After realloc, statements[0] = %p\n", statements[0]);
    printf("Memory content: ");
    mem = (unsigned char*)statements;
    for (int i = 0; i < 16; i++) {
        printf("%02x ", mem[i]);
    }
    printf("\n");
    
    free(statements[0]);
    free(statements);
    return 0;
}