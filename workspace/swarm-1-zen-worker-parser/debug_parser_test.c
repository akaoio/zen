#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/ast.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"

void debug_ast_node(AST_T* node, int depth) {
    if (!node) {
        printf("%*sNULL\n", depth * 2, "");
        return;
    }
    
    printf("%*sAST Type: %d", depth * 2, "", node->type);
    
    switch (node->type) {
        case AST_COMPOUND:
            printf(" (COMPOUND, size=%zu)\n", node->compound_size);
            for (size_t i = 0; i < node->compound_size; i++) {
                printf("%*sStatement %zu:\n", depth * 2, "", i);
                debug_ast_node(node->compound_statements[i], depth + 1);
            }
            break;
            
        case AST_VARIABLE_DEFINITION:
            printf(" (VARIABLE_DEFINITION)\n");
            printf("%*sName: %s\n", (depth + 1) * 2, "", 
                   node->variable_definition_variable_name ? node->variable_definition_variable_name : "NULL");
            printf("%*sValue:\n", (depth + 1) * 2, "");
            debug_ast_node(node->variable_definition_value, depth + 2);
            break;
            
        case AST_NUMBER:
            printf(" (NUMBER, value=%f)\n", node->number_value);
            break;
            
        case AST_STRING:
            printf(" (STRING, value=%s)\n", 
                   node->string_value ? node->string_value : "NULL");
            break;
            
        case AST_VARIABLE:
            printf(" (VARIABLE, name=%s)\n", 
                   node->variable_name ? node->variable_name : "NULL");
            break;
            
        default:
            printf(" (OTHER)\n");
            break;
    }
}

int main() {
    // Initialize memory system
    memory_debug_enable(true);
    
    printf("=== Testing Parser Variable Definition ===\n");
    
    // Test simple variable definition
    const char* test_input = "set x 5\n";
    printf("Input: %s", test_input);
    
    // Create lexer
    lexer_T* lexer = init_lexer(test_input);
    if (!lexer) {
        printf("ERROR: Failed to create lexer\n");
        return 1;
    }
    
    // Create parser
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("ERROR: Failed to create parser\n");
        lexer_free(lexer);
        return 1;
    }
    
    // Create scope
    scope_T* scope = init_scope();
    if (!scope) {
        printf("ERROR: Failed to create scope\n");
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    // Parse the input
    printf("\n=== Starting Parse ===\n");
    AST_T* ast = parser_parse(parser, scope);
    
    if (!ast) {
        printf("ERROR: Parser returned NULL\n");
    } else {
        printf("SUCCESS: Parser created AST\n");
        printf("\n=== AST Structure ===\n");
        debug_ast_node(ast, 0);
    }
    
    // Clean up
    if (ast) ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    // Check for memory leaks
    printf("\n=== Memory Check ===\n");
    if (memory_check_leaks()) {
        printf("MEMORY LEAKS DETECTED!\n");
        memory_print_leak_report();
    } else {
        printf("No memory leaks detected.\n");
    }
    
    return 0;
}