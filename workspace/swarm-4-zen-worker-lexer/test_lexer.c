#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test program to verify lexer functionality
int main() {
    printf("Testing ZEN Lexer Functionality...\n\n");
    
    // Test 1: Basic tokenization
    printf("=== Test 1: Basic Tokens ===\n");
    char* test1 = "set x 42\nset y \"hello\"\nif x = 42\n    print y";
    lexer_T* lexer = init_lexer(test1);
    
    token_T* token;
    int token_count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: Type=%d, Value='%s'\n", ++token_count, token->type, token->value ? token->value : "NULL");
        token_free(token);
        
        if (token_count > 20) break; // Prevent infinite loops
    }
    if (token && token->type == TOKEN_EOF) {
        printf("Token %d: EOF\n", ++token_count);
        token_free(token);
    }
    lexer_free(lexer);
    
    // Test 2: String collection
    printf("\n=== Test 2: String Handling ===\n");
    char* test2 = "\"simple string\"\n\"with\\nescapes\"\n\"with\\ttabs\"";
    lexer = init_lexer(test2);
    
    token_count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        if (token->type == TOKEN_STRING) {
            printf("String Token: '%s'\n", token->value ? token->value : "NULL");
        }
        token_free(token);
        
        if (++token_count > 10) break;
    }
    if (token && token->type == TOKEN_EOF) {
        token_free(token);
    }
    lexer_free(lexer);
    
    // Test 3: Number collection
    printf("\n=== Test 3: Number Handling ===\n");
    char* test3 = "42\n3.14\n1.23e-4\n0\n-5";
    lexer = init_lexer(test3);
    
    token_count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        if (token->type == TOKEN_NUMBER) {
            printf("Number Token: '%s'\n", token->value ? token->value : "NULL");
        }
        token_free(token);
        
        if (++token_count > 15) break;
    }
    if (token && token->type == TOKEN_EOF) {
        token_free(token);
    }
    lexer_free(lexer);
    
    // Test 4: Identifier and keyword recognition
    printf("\n=== Test 4: Identifiers and Keywords ===\n");
    char* test4 = "set function return if else while for true false null";
    lexer = init_lexer(test4);
    
    token_count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        printf("Token: Type=%d, Value='%s'\n", token->type, token->value ? token->value : "NULL");
        token_free(token);
        
        if (++token_count > 20) break;
    }
    if (token && token->type == TOKEN_EOF) {
        token_free(token);
    }
    lexer_free(lexer);
    
    // Test 5: Enhanced features
    printf("\n=== Test 5: Enhanced Features ===\n");
    LexerConfig config = {
        .enable_lookahead = true,
        .enable_metrics = true,
        .enable_error_recovery = true,
        .preserve_whitespace = false,
        .unicode_identifiers = true,
        .buffer_size = 8
    };
    
    char* test5 = "set count 100\nset name \"ZEN\"";
    lexer = lexer_create_enhanced(test5, &config);
    
    // Test location tracking
    SourceLocation loc = lexer_get_location(lexer);
    printf("Initial location: line=%zu, col=%zu, pos=%zu\n", loc.line, loc.column, loc.position);
    
    // Test metrics
    token_count = 0;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        token_count++;
        token_free(token);
        if (token_count > 15) break;
    }
    if (token && token->type == TOKEN_EOF) {
        token_free(token);
    }
    
    LexerMetrics metrics = lexer_get_metrics(lexer);
    printf("Metrics: total_tokens=%zu, string_literals=%zu, numeric_literals=%zu\n", 
           metrics.total_tokens, metrics.string_literals, metrics.numeric_literals);
    
    lexer_free(lexer);
    
    printf("\nLexer functionality test completed!\n");
    return 0;
}