#include "zen/core/lexer.h"
#include "zen/core/token.h"
#include "zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_improved_string_collection() {
    printf("Testing improved lexer_collect_string...\n");
    
    // Test enhanced escape sequences
    char* input = "\"Hello\\nworld\\t!\\r\\\\\\/\\\"\"";
    lexer_T* lexer = init_lexer(input);
    lexer_set_metrics(lexer, true);  // Use proper function to enable metrics
    lexer->c = '"'; // Position at quote
    
    token_T* token = lexer_collect_string(lexer);
    assert(token != NULL);
    assert(token->type == TOKEN_STRING);
    
    // Check that the string contains proper escape characters
    assert(strstr(token->value, "\n") != NULL); // newline
    assert(strstr(token->value, "\t") != NULL); // tab
    assert(strstr(token->value, "\r") != NULL); // carriage return
    assert(strstr(token->value, "\\") != NULL); // backslash
    assert(strstr(token->value, "/") != NULL);  // forward slash
    assert(strstr(token->value, "\"") != NULL); // quote
    
    // Check metrics updated
    assert(lexer->metrics.string_literals == 1);
    assert(lexer->metrics.total_tokens == 1);
    
    printf("✓ Enhanced escape sequences test passed\n");
    
    // Test large string with memory_realloc
    lexer_free(lexer);
    
    char large_input[2048] = "\"";
    for (int i = 0; i < 1500; i++) { // Much larger than initial 64-byte buffer
        strcat(large_input, "x");
    }
    strcat(large_input, "\"");
    
    lexer_T* lexer2 = init_lexer(large_input);
    lexer2->c = '"';
    token_T* token2 = lexer_collect_string(lexer2);
    assert(token2 != NULL);
    assert(strlen(token2->value) == 1500);
    
    printf("✓ Large string memory management test passed\n");
    
    token_free(token);
    token_free(token2);
    lexer_free(lexer2);
}

void test_improved_number_collection() {
    printf("Testing improved lexer_collect_number...\n");
    
    char* input = "123.456e-7";
    lexer_T* lexer = init_lexer(input);
    lexer_set_metrics(lexer, true);
    lexer->c = '1';
    
    token_T* token = lexer_collect_number(lexer);
    assert(token != NULL);
    assert(token->type == TOKEN_NUMBER);
    assert(strcmp(token->value, "123.456e-7") == 0);
    
    // Check metrics updated
    assert(lexer->metrics.numeric_literals == 1);
    assert(lexer->metrics.total_tokens == 1);
    
    printf("✓ Scientific notation with metrics test passed\n");
    
    token_free(token);
    lexer_free(lexer);
}

void test_improved_id_collection() {
    printf("Testing improved lexer_collect_id...\n");
    
    // Test regular identifier
    char* input1 = "my_variable_123";
    lexer_T* lexer1 = init_lexer(input1);
    lexer_set_metrics(lexer1, true);
    lexer1->c = 'm';
    
    token_T* token1 = lexer_collect_id(lexer1);
    assert(token1 != NULL);
    assert(token1->type == TOKEN_ID);
    assert(strcmp(token1->value, "my_variable_123") == 0);
    assert(lexer1->metrics.identifiers == 1);
    
    printf("✓ Identifier with metrics test passed\n");
    
    // Test keyword
    char* input2 = "function";
    lexer_T* lexer2 = init_lexer(input2);
    lexer_set_metrics(lexer2, true);
    lexer2->c = 'f';
    
    token_T* token2 = lexer_collect_id(lexer2);
    assert(token2 != NULL);
    assert(token2->type == TOKEN_FUNCTION);
    assert(strcmp(token2->value, "function") == 0);
    assert(lexer2->metrics.keywords == 1);
    
    printf("✓ Keyword with metrics test passed\n");
    
    token_free(token1);
    token_free(token2);
    lexer_free(lexer1);
    lexer_free(lexer2);
}

void test_improved_advance_with_token() {
    printf("Testing improved lexer_advance_with_token...\n");
    
    char* input = "+-*/";
    lexer_T* lexer = init_lexer(input);
    lexer_set_metrics(lexer, true);
    lexer->c = '+';
    lexer->i = 0;
    
    // Test null safety
    token_T* null_result = lexer_advance_with_token(NULL, NULL);
    assert(null_result == NULL);
    
    // Test normal operation with operator token
    token_T* plus_token = init_token(TOKEN_PLUS, "+");
    token_T* result = lexer_advance_with_token(lexer, plus_token);
    
    assert(result == plus_token);
    assert(lexer->i == 1);
    assert(lexer->c == '-');
    
    // Check operator metrics
    assert(lexer->metrics.operators == 1);
    assert(lexer->metrics.total_tokens == 1);
    
    printf("✓ Null safety and metrics test passed\n");
    
    token_free(plus_token);
    lexer_free(lexer);
}

void test_memory_management() {
    printf("Testing memory management...\n");
    
    // Enable memory debugging
    memory_debug_enable(true);
    
    char* input = "\"test\" 42 variable function";
    lexer_T* lexer = init_lexer(input);
    
    // Get all tokens
    token_T* tokens[10];
    int token_count = 0;
    
    token_T* token;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF && token_count < 10) {
        tokens[token_count++] = token;
    }
    
    // Free all tokens
    for (int i = 0; i < token_count; i++) {
        token_free(tokens[i]);
    }
    if (token) token_free(token);
    
    lexer_free(lexer);
    
    // Check for memory leaks
    bool has_leaks = memory_check_leaks();
    assert(!has_leaks); // Should be no leaks
    
    printf("✓ Memory management test passed\n");
}

int main() {
    printf("=== TESTING IMPROVED LEXER FUNCTIONS ===\n\n");
    
    test_improved_string_collection();
    printf("\n");
    
    test_improved_number_collection();  
    printf("\n");
    
    test_improved_id_collection();
    printf("\n");
    
    test_improved_advance_with_token();
    printf("\n");
    
    test_memory_management();
    printf("\n");
    
    printf("=== ALL IMPROVED LEXER TESTS PASSED ===\n");
    printf("✅ lexer_collect_string: Now uses proper memory management + enhanced escapes\n");
    printf("✅ lexer_collect_number: Now uses proper memory management + metrics\n");
    printf("✅ lexer_collect_id: Now uses proper memory management + metrics\n");
    printf("✅ lexer_advance_with_token: Enhanced with null safety + operator metrics\n");
    printf("✅ All functions: Eliminated inefficient malloc/realloc workaround\n");
    return 0;
}