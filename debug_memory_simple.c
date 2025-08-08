/**
 * Simple memory debug test
 */
#include <stdio.h>
#include <stdlib.h>

#include "zen/core/lexer.h"
#include "zen/core/parser.h" 
#include "zen/core/ast.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"

int main() {
    printf("=== Memory Debug Test ===\n");
    
    memory_debug_enable(true);
    
    const char* code = "set x 42\n";
    
    printf("Testing: %s", code);
    
    printf("Creating lexer...\n");
    lexer_T* lexer = lexer_new((char*)code);
    if (!lexer) {
        printf("ERROR: Failed to create lexer\n");
        return 1;
    }
    printf("Lexer created, current memory stats:\n");
    memory_print_leak_report();
    
    parser_T* parser = parser_new(lexer);
    if (!parser) {
        lexer_free(lexer);
        return 1;
    }
    
    scope_T* scope = init_scope();
    if (!scope) {
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    AST_T* ast = parser_parse_statements(parser, scope);
    if (!ast) {
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    visitor_T* visitor = visitor_new();
    if (!visitor) {
        ast_free(ast);
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    AST_T* result = visitor_visit(visitor, ast);
    printf("Visitor result: %p\n", (void*)result);
    
    // Cleanup
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    if (memory_check_leaks()) {
        printf("MEMORY LEAKS:\n");
        memory_print_leak_report();
    } else {
        printf("No memory leaks\n");
    }
    
    memory_debug_cleanup();
    
    return 0;
}