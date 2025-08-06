#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <string.h>

void test_comment_parsing(const char* input, const char* expected_tokens[]) {
    printf("Testing: %s\n", input);
    
    lexer_T* lexer = init_lexer((char*)input);
    
    int i = 0;
    token_T* token;
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF && expected_tokens[i]) {
        printf("  Token %d: type=%d, value='%s' (expected: '%s') ", 
               i, token->type, token->value, expected_tokens[i]);
        
        if (strcmp(token->value, expected_tokens[i]) == 0) {
            printf("✓\n");
        } else {
            printf("✗ FAIL\n");
        }
        i++;
    }
    
    if (expected_tokens[i]) {
        printf("  MISSING TOKENS: %s\n", expected_tokens[i]);
    }
    
    lexer_free(lexer);
    printf("\n");
}

int main() {
    printf("=== Testing Comment Parsing ===\n");
    
    // Test case from failing test
    const char* test_input = 
        "set x 42  // This is a comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "set y 13";
        
    const char* expected1[] = {"set", "x", "42", "set", "y", "13", NULL};
    test_comment_parsing(test_input, expected1);
    
    // Simple comment test
    const char* expected2[] = {"set", "a", "5", NULL};
    test_comment_parsing("set a 5 // comment", expected2);
    
    // Multi-line comment test  
    const char* expected3[] = {"before", "after", NULL};
    test_comment_parsing("before /* comment */ after", expected3);
    
    return 0;
}