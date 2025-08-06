#include <stdio.h>

// Just test the token constant values
#include "zen/core/token.h"

int main() {
    printf("=== Token constant values ===\n");
    printf("TOKEN_EOF = %d\n", TOKEN_EOF);
    printf("TOKEN_NEWLINE = %d\n", TOKEN_NEWLINE);
    printf("TOKEN_INDENT = %d\n", TOKEN_INDENT);
    printf("TOKEN_DEDENT = %d\n", TOKEN_DEDENT);
    printf("TOKEN_ID = %d\n", TOKEN_ID);
    printf("TOKEN_NUMBER = %d\n", TOKEN_NUMBER);
    printf("TOKEN_STRING = %d\n", TOKEN_STRING);
    printf("TOKEN_TRUE = %d\n", TOKEN_TRUE);
    printf("TOKEN_FALSE = %d\n", TOKEN_FALSE);
    printf("TOKEN_NULL = %d\n", TOKEN_NULL);
    printf("TOKEN_SET = %d\n", TOKEN_SET);
    
    printf("\nTest expectation: Expected 10, got 4\n");
    printf("Value 4 corresponds to: TOKEN_ID\n");
    printf("Value 10 would correspond to: ??? (let's see what that would be)\n");
    
    // Let's see what token type 10 would be by checking the enum order
    if (TOKEN_SET == 21) {
        printf("TOKEN_SET is indeed 21, so expecting 10 seems wrong\n");
    }
    
    return 0;
}