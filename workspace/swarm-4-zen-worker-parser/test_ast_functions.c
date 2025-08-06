#include "src/include/zen/core/ast.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

int main() {
    printf("Testing ast_new_null() and ast_new_noop()...\n");
    
    // Test ast_new_null()
    AST_T* null_node = ast_new_null();
    assert(null_node != NULL);
    assert(null_node->type == AST_NULL);
    printf("✓ ast_new_null() works correctly\n");
    
    // Test ast_new_noop() 
    AST_T* noop_node = ast_new_noop();
    assert(noop_node != NULL);
    assert(noop_node->type == AST_NOOP);
    printf("✓ ast_new_noop() works correctly\n");
    
    // Clean up
    ast_free(null_node);
    ast_free(noop_node);
    
    printf("✓ All tests passed! Functions are properly implemented.\n");
    return 0;
}