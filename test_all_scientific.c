#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

void test_number(const char* input, const char* expected) {
    printf("Testing: %s -> ", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    token_T* token = lexer_get_next_token(lexer);
    
    if (token && token->type == TOKEN_NUMBER && strcmp(token->value, expected) == 0) {
        printf("✓ PASS (%s)\n", token->value);
    } else {
        printf("✗ FAIL (expected: %s, got: %s, type: %d)\n", 
               expected, token ? token->value : "null", token ? token->type : -1);
    }
    
    lexer_free(lexer);
}

int main() {
    printf("=== Testing Scientific Notation Fixes ===\n");
    
    test_number("1e5", "1e5");
    test_number("2.5e-3", "2.5e-3");
    test_number("1.23E+10", "1.23E+10");
    test_number("5e0", "5e0");
    test_number("42", "42");
    test_number("3.14", "3.14");
    test_number("0.001", "0.001");
    
    return 0;
}