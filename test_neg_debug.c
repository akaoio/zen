#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/visitor.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include "src/include/zen/core/runtime_value.h"

int main() {
    memory_debug_enable(true);
    
    const char* code = "set negative_int -42\nset negative_float -3.14";
    
    lexer_T* lexer = lexer_new((char*)code);
    if (!lexer) return 1;
    
    parser_T* parser = parser_new(lexer);
    if (!parser) {
        lexer_free(lexer);
        return 1;
    }
    
    scope_T* scope = scope_new();
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
    
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    // Clean up
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    memory_print_leak_report();
    memory_debug_cleanup();
    
    return 0;
}