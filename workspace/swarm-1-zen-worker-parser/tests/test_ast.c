#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../src/include/zen/core/ast.h"

/**
 * @brief Test AST node creation and basic functionality
 */
void test_ast_node_creation(void)
{
    printf("Testing AST node creation...\n");
    
    // Test creating different node types
    AST_Node* literal = ast_node_new(AST_LITERAL_STRING);
    assert(literal != NULL);
    assert(literal->type == AST_LITERAL_STRING);
    assert(literal->child_count == 0);
    assert(literal->parent == NULL);
    
    AST_Node* identifier = ast_node_new(AST_IDENTIFIER);
    assert(identifier != NULL);
    assert(identifier->type == AST_IDENTIFIER);
    
    AST_Node* binary = ast_node_new(AST_BINARY_ADD);
    assert(binary != NULL);
    assert(binary->type == AST_BINARY_ADD);
    
    printf("✓ Node creation tests passed\n");
    
    // Clean up
    ast_node_free(literal);
    ast_node_free(identifier);
    ast_node_free(binary);
}

/**
 * @brief Test parent-child relationships
 */
void test_ast_parent_child(void)
{
    printf("Testing parent-child relationships...\n");
    
    AST_Node* parent = ast_node_new(AST_PROGRAM);
    assert(parent != NULL);
    
    AST_Node* child1 = ast_node_new(AST_ASSIGNMENT);
    AST_Node* child2 = ast_node_new(AST_FUNCTION_CALL);
    AST_Node* child3 = ast_node_new(AST_IF_STATEMENT);
    
    assert(child1 != NULL && child2 != NULL && child3 != NULL);
    
    // Add children
    ast_node_add_child(parent, child1);
    assert(parent->child_count == 1);
    assert(parent->children[0] == child1);
    assert(child1->parent == parent);
    
    ast_node_add_child(parent, child2);
    assert(parent->child_count == 2);
    assert(parent->children[1] == child2);
    assert(child2->parent == parent);
    
    ast_node_add_child(parent, child3);
    assert(parent->child_count == 3);
    assert(parent->children[2] == child3);
    assert(child3->parent == parent);
    
    printf("✓ Parent-child relationship tests passed\n");
    
    // Test null safety
    ast_node_add_child(NULL, child1);  // Should not crash
    ast_node_add_child(parent, NULL);  // Should not crash
    assert(parent->child_count == 3);  // Count should remain unchanged
    
    printf("✓ Null safety tests passed\n");
    
    // Clean up (should free all children recursively)
    ast_node_free(parent);
}

/**
 * @brief Test capacity expansion
 */
void test_ast_capacity_expansion(void)
{
    printf("Testing capacity expansion...\n");
    
    AST_Node* parent = ast_node_new(AST_BLOCK);
    assert(parent != NULL);
    
    // Add more children than initial capacity (4)
    for (int i = 0; i < 10; i++) {
        AST_Node* child = ast_node_new(AST_LITERAL_NUMBER);
        assert(child != NULL);
        ast_node_add_child(parent, child);
        assert(parent->child_count == (size_t)(i + 1));
    }
    
    // Verify all children are properly stored
    assert(parent->child_count == 10);
    assert(parent->child_capacity >= 10);  // Should have expanded
    
    for (size_t i = 0; i < parent->child_count; i++) {
        assert(parent->children[i] != NULL);
        assert(parent->children[i]->type == AST_LITERAL_NUMBER);
        assert(parent->children[i]->parent == parent);
    }
    
    printf("✓ Capacity expansion tests passed\n");
    
    // Clean up
    ast_node_free(parent);
}

/**
 * @brief Test node type names
 */
void test_ast_node_type_names(void)
{
    printf("Testing node type names...\n");
    
    assert(strcmp(ast_node_type_name(AST_LITERAL_STRING), "LITERAL_STRING") == 0);
    assert(strcmp(ast_node_type_name(AST_BINARY_ADD), "BINARY_ADD") == 0);
    assert(strcmp(ast_node_type_name(AST_FUNCTION_DEF), "FUNCTION_DEF") == 0);
    assert(strcmp(ast_node_type_name(AST_PROGRAM), "PROGRAM") == 0);
    
    printf("✓ Node type name tests passed\n");
}

/**
 * @brief Test recursive freeing
 */
void test_ast_recursive_free(void)
{
    printf("Testing recursive freeing...\n");
    
    // Create a tree structure:
    //   program
    //   ├── assignment
    //   │   └── literal_string
    //   └── function_call
    //       ├── identifier
    //       └── literal_number
    
    AST_Node* program = ast_node_new(AST_PROGRAM);
    AST_Node* assignment = ast_node_new(AST_ASSIGNMENT);
    AST_Node* literal_str = ast_node_new(AST_LITERAL_STRING);
    AST_Node* func_call = ast_node_new(AST_FUNCTION_CALL);
    AST_Node* identifier = ast_node_new(AST_IDENTIFIER);
    AST_Node* literal_num = ast_node_new(AST_LITERAL_NUMBER);
    
    assert(program && assignment && literal_str && func_call && identifier && literal_num);
    
    // Build tree
    ast_node_add_child(program, assignment);
    ast_node_add_child(assignment, literal_str);
    ast_node_add_child(program, func_call);
    ast_node_add_child(func_call, identifier);
    ast_node_add_child(func_call, literal_num);
    
    // Verify structure
    assert(program->child_count == 2);
    assert(assignment->child_count == 1);
    assert(func_call->child_count == 2);
    
    printf("✓ Tree structure created successfully\n");
    
    // Free root should free entire tree
    ast_node_free(program);
    
    printf("✓ Recursive freeing completed without errors\n");
}

int main(void)
{
    printf("Running AST tests...\n\n");
    
    test_ast_node_creation();
    test_ast_parent_child();
    test_ast_capacity_expansion();
    test_ast_node_type_names();
    test_ast_recursive_free();
    
    printf("\n✅ All AST tests passed!\n");
    return 0;
}