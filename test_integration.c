#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/visitor.h"
#include "src/include/zen/core/scope.h"

int main() {
    char* code = "set x 42\nx";
    
    printf("Testing code: %s\n", code);
    
    lexer_T* lexer = lexer_new(code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    
    AST_T* ast = parser_parse_statements(parser, scope);
    if (!ast) {
        printf("Parse failed\n");
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    visitor_T* visitor = visitor_new();
    AST_T* result = visitor_visit(visitor, ast);
    
    printf("Result: %p\n", (void*)result);
    
    // Clean up
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    return 0;
}