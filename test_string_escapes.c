#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

void test_string(const char* input, const char* expected) {
    printf("Testing: %s -> ", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    token_T* token = lexer_get_next_token(lexer);
    
    if (token && token->type == TOKEN_STRING) {
        if (strcmp(token->value, expected) == 0) {
            printf("✓ PASS (%s)\n", token->value);
        } else {
            printf("✗ FAIL (expected: ");
            for (int i = 0; expected[i]; i++) {
                if (expected[i] == '\n') printf("\\n");
                else if (expected[i] == '\t') printf("\\t");
                else if (expected[i] == '\"') printf("\\\"");
                else printf("%c", expected[i]);
            }
            printf(", got: ");
            for (int i = 0; token->value[i]; i++) {
                if (token->value[i] == '\n') printf("\\n");
                else if (token->value[i] == '\t') printf("\\t");
                else if (token->value[i] == '\"') printf("\\\"");
                else printf("%c", token->value[i]);
            }
            printf(")\n");
        }
    } else {
        printf("✗ FAIL (expected STRING token, got type %d)\n", token ? token->type : -1);
    }
    
    lexer_free(lexer);
}

int main() {
    printf("=== Testing String Escape Sequences ===\n");
    
    test_string("\"hello\"", "hello");
    test_string("\"\\n\\t\\\"\"", "\n\t\"");  // This is the failing test case
    test_string("\"hello\\nworld\"", "hello\nworld");
    test_string("\"tab\\there\"", "tab\there");
    test_string("\"quote\\\"test\"", "quote\"test");
    test_string("\"backslash\\\\test\"", "backslash\\test");
    
    return 0;
}