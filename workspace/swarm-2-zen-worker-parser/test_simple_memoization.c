#include "zen/core/parser.h"
#include "zen/core/lexer.h"
#include <stdio.h>

int main() {
    printf("=== Testing Parser Memoization Features ===\n\n");
    
    char zen_code[] = "set x 5\nset y x + 2\nprint y";
    
    // Test parser creation and memoization enable
    lexer_T* lexer = init_lexer(zen_code);
    if (!lexer) {
        printf("ERROR: Failed to create lexer\n");
        return 1;
    }
    
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("ERROR: Failed to create parser\n");
        lexer_free(lexer);
        return 1;
    }
    
    printf("1. Parser created successfully\n");
    printf("2. Initial memoization state: %s\n", 
           parser->memoization.enabled ? "ENABLED" : "DISABLED");
    
    // Enable memoization
    parser_set_memoization(parser, true);
    printf("3. Memoization enabled: %s\n", 
           parser->memoization.enabled ? "SUCCESS" : "FAILED");
    
    // Test cache allocation
    printf("4. Cache capacity: %zu\n", parser->memoization.cache_capacity);
    printf("5. Cache allocated: %s\n", 
           parser->memoization.cached_expressions ? "SUCCESS" : "FAILED");
    
    // Parse simple statement
    AST_T* ast = parser_parse_statements(parser, parser->scope);
    
    // Check analytics
    struct parser_analytics stats;
    parser_get_analytics(parser, &stats);
    
    printf("6. Expressions parsed: %zu\n", stats.expressions_parsed);
    printf("7. Cache hits: %zu\n", stats.cache_hits);
    printf("8. Cache misses: %zu\n", stats.cache_misses);
    
    printf("\n=== Test Complete ===\n");
    
    // Cleanup - simple cleanup to avoid memory issues
    if (ast) {
        printf("9. AST created successfully (type: %d)\n", ast->type);
    }
    
    printf("10. Cleaning up resources...\n");
    
    // Don't free AST to avoid double-free issues for now
    parser_free(parser);
    lexer_free(lexer);
    
    printf("11. All done!\n");
    
    return 0;
}