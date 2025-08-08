/**
 * Single test memory check - simplified version
 */
#include <stdio.h>
#include <stdlib.h>

#include "zen/core/lexer.h"
#include "zen/core/parser.h" 
#include "zen/core/ast.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"

static bool execute_code(const char* code) {
    printf("Creating lexer...\n");
    lexer_T* lexer = lexer_new((char*)code);
    if (!lexer) {
        printf("ERROR: Failed to create lexer\n");
        return false;
    }
    
    printf("Creating parser...\n");
    parser_T* parser = parser_new(lexer);
    if (!parser) {
        printf("ERROR: Failed to create parser\n");
        lexer_free(lexer);
        return false;
    }
    
    printf("Creating scope...\n");
    scope_T* scope = init_scope();
    if (!scope) {
        printf("ERROR: Failed to create scope\n");
        parser_free(parser);
        lexer_free(lexer);
        return false;
    }
    
    printf("Parsing AST...\n");
    AST_T* ast = parser_parse_statements(parser, scope);
    if (!ast) {
        printf("ERROR: Failed to parse AST\n");
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return false;
    }
    
    printf("Creating visitor...\n");
    visitor_T* visitor = visitor_new();
    if (!visitor) {
        printf("ERROR: Failed to create visitor\n");
        ast_free(ast);
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return false;
    }
    
    printf("Visiting AST...\n");
    AST_T* result = visitor_visit(visitor, ast);
    
    printf("Result: %p\n", (void*)result);
    
    bool success = (result != NULL);
    
    printf("Cleaning up...\n");
    // Cleanup
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    return success;
}

int main() {
    printf("=== Single Test Memory Debug ===\n");
    
    memory_debug_enable(true);
    
    // Simple test case
    const char* code = "set x 42\n";
    
    printf("Testing code: %s", code);
    bool result = execute_code(code);
    printf("Execution result: %s\n", result ? "SUCCESS" : "FAILED");
    
    printf("Checking for memory leaks...\n");
    if (memory_check_leaks()) {
        printf("MEMORY LEAKS DETECTED:\n");
        memory_print_leak_report();
    } else {
        printf("No memory leaks detected\n");
    }
    
    memory_debug_cleanup();
    
    return result ? 0 : 1;
}