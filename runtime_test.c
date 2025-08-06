#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== ZEN Advanced Runtime API Test ===\n");
    
    // Initialize memory debugging
    memory_debug_enable(true);
    
    // Create a simple ZEN program
    const char* program = 
        "set x 5\n"
        "set y 10\n"
        "function add a b\n"
        "    return a + b\n"
        "set result add x y\n"
        "print \"Result:\", result\n";
    
    printf("Program to execute:\n%s\n", program);
    
    // Initialize lexer
    lexer_T* lexer = init_lexer((char*)program);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    
    // Initialize parser
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("Failed to create parser\n");
        return 1;
    }
    
    // Parse the program
    AST_T* root = parser_parse(parser, init_scope());
    if (!root) {
        printf("Failed to parse program\n");
        return 1;
    }
    
    // Create advanced visitor with enhanced features
    visitor_T* visitor = init_visitor();
    if (!visitor) {
        printf("Failed to create visitor\n");
        return 1;
    }
    
    // Configure runtime optimizations
    printf("Configuring runtime optimizations...\n");
    visitor_enable_profiling(visitor, true);
    visitor_set_hot_function_threshold(visitor, 5);  // Low threshold for testing
    visitor_enable_optimizations(visitor, true, true, false);  // TCO + Constant Folding
    
    printf("Executing program with advanced runtime...\n");
    
    // Execute the program
    AST_T* result = visitor_visit(visitor, root);
    
    printf("Program execution completed.\n");
    
    // Get and display execution statistics
    char stats_buffer[2048];
    visitor_get_execution_stats(visitor, stats_buffer, sizeof(stats_buffer));
    printf("\n%s\n", stats_buffer);
    
    printf("Call stack depth: %zu\n", visitor_get_call_depth(visitor));
    
    // Check for exceptions
    if (visitor_has_exception(visitor)) {
        printf("Exception occurred during execution!\n");
        visitor_clear_exception(visitor);
    } else {
        printf("Execution completed without exceptions.\n");
    }
    
    // Clean up
    if (result && result != root) {
        ast_free(result);
    }
    ast_free(root);
    visitor_free(visitor);
    parser_free(parser);
    
    // Print memory statistics
    memory_print_leak_report();
    
    printf("=== Advanced Runtime API Test Complete ===\n");
    return 0;
}