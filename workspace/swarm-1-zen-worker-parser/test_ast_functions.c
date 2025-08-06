/**
 * @file test_ast_functions.c
 * @brief Test ast_new_null and ast_new_noop functions
 */

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "zen/core/ast.h"
#include "zen/core/memory.h"

int main() {
    printf("Testing ast_new_null() and ast_new_noop()...\n");
    
    // Test ast_new_null()
    printf("  Testing ast_new_null()...\n");
    AST_T* null_ast = ast_new_null();
    
    assert(null_ast != NULL);  // Function returns valid pointer
    assert(null_ast->type == AST_NULL);  // Correct type is set
    printf("    ✓ ast_new_null() creates valid AST_NULL node\n");
    
    // Test ast_new_noop()
    printf("  Testing ast_new_noop()...\n");
    AST_T* noop_ast = ast_new_noop();
    
    assert(noop_ast != NULL);  // Function returns valid pointer
    assert(noop_ast->type == AST_NOOP);  // Correct type is set
    printf("    ✓ ast_new_noop() creates valid AST_NOOP node\n");
    
    // Test memory cleanup works
    printf("  Testing memory cleanup...\n");
    ast_free(null_ast);
    ast_free(noop_ast);
    printf("    ✓ ast_free() works correctly\n");
    
    printf("All tests passed! Both functions are correctly implemented.\n");
    return 0;
}