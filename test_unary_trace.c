#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/visitor.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include "src/include/zen/core/runtime_value.h"

int main() {
    memory_debug_enable(true);
    memory_reset_stats();
    
    const char* code1 = "set x -42";
    const char* code2 = "set y -3.14";
    
    printf("=== Processing: %s ===\n", code1);
    lexer_T* lexer = lexer_new((char*)code1);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    AST_T* ast = parser_parse_statements(parser, scope);
    visitor_T* visitor = visitor_new();
    
    MemoryStats before;
    memory_get_stats(&before);
    
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    MemoryStats after;
    memory_get_stats(&after);
    printf("After first visit: +%zu bytes\n", after.current_allocated - before.current_allocated);
    
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    printf("\n=== Processing: %s ===\n", code2);
    lexer = lexer_new((char*)code2);
    parser = parser_new(lexer);
    scope = scope_new();
    ast = parser_parse_statements(parser, scope);
    visitor = visitor_new();
    
    memory_get_stats(&before);
    
    result = visitor_visit(visitor, ast);
    
    memory_get_stats(&after);
    printf("After second visit: +%zu bytes\n", after.current_allocated - before.current_allocated);
    
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    printf("\n=== Final memory report ===\n");
    memory_print_leak_report();
    
    return 0;
}