#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../src/include/zen/core/ast.h"

/**
 * @brief Simple test program for AST implementation
 */
int main() {
    printf("Testing AST implementation...\n");
    
    // Test 1: Create a root program node
    AST_Node* program = ast_node_new(AST_PROGRAM);
    assert(program != NULL);
    assert(program->type == AST_PROGRAM);
    assert(program->child_count == 0);
    assert(program->children == NULL);
    printf("✓ Test 1: ast_node_new() creates program node correctly\n");
    
    // Test 2: Create a function definition node
    AST_Node* func_def = ast_node_new(AST_FUNCTION_DEF);
    assert(func_def != NULL);
    assert(func_def->type == AST_FUNCTION_DEF);
    
    // Set identifier
    func_def->identifier = strdup("test_function");
    assert(func_def->identifier != NULL);
    assert(strcmp(func_def->identifier, "test_function") == 0);
    printf("✓ Test 2: Function definition node created with identifier\n");
    
    // Test 3: Add child to parent
    ast_node_add_child(program, func_def);
    assert(program->child_count == 1);
    assert(program->children != NULL);
    assert(program->children[0] == func_def);
    printf("✓ Test 3: ast_node_add_child() works correctly\n");
    
    // Test 4: Add multiple children
    AST_Node* stmt1 = ast_node_new(AST_ASSIGNMENT);
    AST_Node* stmt2 = ast_node_new(AST_RETURN_STATEMENT);
    ast_node_add_child(program, stmt1);
    ast_node_add_child(program, stmt2);
    
    assert(program->child_count == 3);
    assert(program->children[1] == stmt1);
    assert(program->children[2] == stmt2);
    printf("✓ Test 4: Multiple children added correctly\n");
    
    // Test 5: Create expression tree
    AST_Node* binary = ast_node_new(AST_BINARY_OP);
    binary->operator = strdup("+");
    AST_Node* left = ast_node_new(AST_LITERAL_NUMBER);
    AST_Node* right = ast_node_new(AST_LITERAL_NUMBER);
    
    ast_node_add_child(binary, left);
    ast_node_add_child(binary, right);
    
    assert(binary->child_count == 2);
    assert(binary->children[0] == left);
    assert(binary->children[1] == right);
    printf("✓ Test 5: Binary expression tree created correctly\n");
    
    // Test 6: Free the entire tree
    ast_node_free(program);
    ast_node_free(binary);
    printf("✓ Test 6: ast_node_free() completed without crashes\n");
    
    // Test 7: NULL safety
    ast_node_free(NULL);  // Should not crash
    ast_node_add_child(NULL, NULL);  // Should not crash
    AST_Node* null_test = ast_node_new(AST_IDENTIFIER);
    ast_node_add_child(null_test, NULL);  // Should not crash
    ast_node_add_child(NULL, null_test);  // Should not crash
    ast_node_free(null_test);
    printf("✓ Test 7: NULL safety checks passed\n");
    
    printf("\n🎉 All AST tests passed successfully!\n");
    printf("✅ AST implementation is working correctly\n");
    
    return 0;
}