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
    
    const char* code = "set negative_int -42\nset negative_float -3.14";
    
    printf("Testing: %s\n", code);
    
    MemoryStats before;
    memory_get_stats(&before);
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    AST_T* ast = parser_parse_statements(parser, scope);
    visitor_T* visitor = visitor_new();
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    MemoryStats after;
    memory_get_stats(&after);
    
    printf("Allocated before: %zu bytes\n", before.current_allocated);
    printf("Allocated after: %zu bytes\n", after.current_allocated);
    printf("Difference: %zu bytes\n", after.current_allocated - before.current_allocated);
    
    memory_print_leak_report();
    
    return 0;
}