#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/visitor.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include "src/include/zen/core/runtime_value.h"

void test_negative_parsing() {
    printf("\n=== Testing negative number parsing ===\n");
    memory_debug_enable(true);
    memory_reset_stats();
    
    MemoryStats before;
    memory_get_stats(&before);
    printf("Before: %zu bytes, %zu allocs\n", before.current_allocated, before.allocation_count);
    
    const char* code = "set x -42";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    AST_T* ast = parser_parse_statements(parser, scope);
    
    printf("AST created, type: %d\n", ast ? ast->type : -1);
    
    visitor_T* visitor = visitor_new();
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    printf("Result type: %d\n", result ? result->type : -1);
    
    // Clean up
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    MemoryStats after;
    memory_get_stats(&after);
    printf("After: %zu bytes, %zu allocs\n", after.current_allocated, after.allocation_count);
    printf("Leaked: %zu bytes\n", after.current_allocated - before.current_allocated);
    
    memory_print_leak_report();
}

void test_positive_parsing() {
    printf("\n=== Testing positive number parsing ===\n");
    memory_reset_stats();
    
    MemoryStats before;
    memory_get_stats(&before);
    printf("Before: %zu bytes, %zu allocs\n", before.current_allocated, before.allocation_count);
    
    const char* code = "set x 42";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    AST_T* ast = parser_parse_statements(parser, scope);
    
    visitor_T* visitor = visitor_new();
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    // Clean up
    if (result) rv_unref(result);
    visitor_free(visitor);
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    MemoryStats after;
    memory_get_stats(&after);
    printf("After: %zu bytes, %zu allocs\n", after.current_allocated, after.allocation_count);
    printf("Leaked: %zu bytes\n", after.current_allocated - before.current_allocated);
}

int main() {
    test_positive_parsing();
    test_negative_parsing();
    return 0;
}