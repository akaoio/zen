#include "zen/core/parser.h"
#include "zen/core/lexer.h"
#include "zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    memory_debug_enable(true);
    
    // Test program with repeated expressions
    const char* zen_code = 
        "set x 5\n"
        "set y x + 2\n"
        "set z y * 3\n"
        "set result1 x + y + z\n"
        "set result2 x + y + z\n"  // Same expression as result1
        "set result3 x + y + z\n"  // Same expression again
        "print result1\n"
        "print result2\n"
        "print result3\n";
    
    printf("=== Testing Parser Memoization ===\n\n");
    
    // Test WITHOUT memoization
    printf("--- Test 1: WITHOUT Memoization ---\n");
    lexer_T* lexer1 = init_lexer(zen_code);
    parser_T* parser1 = init_parser(lexer1);
    
    clock_t start1 = clock();
    AST_T* ast1 = parser_parse_statements(parser1, parser1->scope);
    clock_t end1 = clock();
    
    struct parser_analytics stats1;
    parser_get_analytics(parser1, &stats1);
    
    printf("Parse time: %ld microseconds\n", (end1 - start1));
    printf("Expressions parsed: %zu\n", stats1.expressions_parsed);
    printf("Cache hits: %zu\n", stats1.cache_hits);
    printf("Cache misses: %zu\n", stats1.cache_misses);
    printf("Cache hit rate: %.2f%%\n", stats1.cache_hit_rate * 100);
    
    // Test WITH memoization
    printf("\n--- Test 2: WITH Memoization ---\n");
    lexer_T* lexer2 = init_lexer(zen_code);
    parser_T* parser2 = init_parser(lexer2);
    parser_set_memoization(parser2, true);  // Enable memoization
    
    clock_t start2 = clock();
    AST_T* ast2 = parser_parse_statements(parser2, parser2->scope);
    clock_t end2 = clock();
    
    struct parser_analytics stats2;
    parser_get_analytics(parser2, &stats2);
    
    printf("Parse time: %ld microseconds\n", (end2 - start2));
    printf("Expressions parsed: %zu\n", stats2.expressions_parsed);
    printf("Cache hits: %zu\n", stats2.cache_hits);
    printf("Cache misses: %zu\n", stats2.cache_misses);
    printf("Cache hit rate: %.2f%%\n", stats2.cache_hit_rate * 100);
    
    // Performance comparison
    printf("\n--- Performance Comparison ---\n");
    long time_diff = (end1 - start1) - (end2 - start2);
    printf("Performance improvement: %ld microseconds\n", time_diff);
    if (end1 != start1) {
        double improvement = ((double)time_diff / (end1 - start1)) * 100;
        printf("Speed improvement: %.2f%%\n", improvement);
    }
    
    // Test advanced syntax features
    printf("\n=== Testing Advanced Syntax Features ===\n");
    
    const char* advanced_code = 
        "set condition true\n"
        "set value condition ? 100 : 200\n"  // Ternary operator
        "set a 10\n"
        "a += 5\n"                          // Compound assignment
        "a *= 2\n"                          // Compound assignment
        "print value\n"
        "print a\n";
    
    lexer_T* lexer3 = init_lexer(advanced_code);
    parser_T* parser3 = init_parser(lexer3);
    parser_set_memoization(parser3, true);
    
    AST_T* ast3 = parser_parse_statements(parser3, parser3->scope);
    
    printf("Advanced syntax parsing: %s\n", 
           ast3 && ast3->type == AST_COMPOUND ? "SUCCESS" : "FAILED");
    
    // Cleanup
    if (ast1) ast_free(ast1);
    if (ast2) ast_free(ast2);
    if (ast3) ast_free(ast3);
    
    parser_free(parser1);
    parser_free(parser2);
    parser_free(parser3);
    
    lexer_free(lexer1);
    lexer_free(lexer2);
    lexer_free(lexer3);
    
    printf("Memory test complete\n");
    
    return 0;
}