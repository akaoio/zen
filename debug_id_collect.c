#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

token_T* debug_lexer_collect_id(lexer_T* lexer)
{
    printf("DEBUG: Starting ID collection\n");
    printf("First char: '%c' at position %d\n", lexer->c, lexer->i);
    
    // Use buffer-based approach for efficiency and correctness
    size_t buffer_size = 64; // Start with 64 chars for identifiers
    char* value = memory_alloc(buffer_size);
    value[0] = '\0';
    size_t value_len = 0;
    
    printf("Initial buffer size: %zu\n", buffer_size);

    // Helper macro to append a character to the buffer
    #define APPEND_CHAR(ch) do { \
        printf("Appending char: '%c' (len=%zu, buffer_size=%zu)\n", ch, value_len, buffer_size); \
        if (value_len + 1 >= buffer_size) { \
            buffer_size *= 2; \
            printf("Expanding buffer to: %zu\n", buffer_size); \
            value = memory_realloc(value, buffer_size); \
        } \
        value[value_len++] = (ch); \
        value[value_len] = '\0'; \
    } while(0)

    int char_count = 0;
    // Collect identifier characters (alphanumeric + underscore)
    while (isalnum(lexer->c) || lexer->c == '_')
    {
        APPEND_CHAR(lexer->c);
        lexer_advance(lexer);
        char_count++;
        
        if (char_count % 100 == 0) {
            printf("Processed %d characters, current length: %zu\n", char_count, value_len);
        }
    }

    printf("Final: processed %d chars, value length: %zu\n", char_count, strlen(value));
    printf("First 20 chars: %.20s\n", value);
    if (strlen(value) > 20) {
        printf("Last 20 chars: %s\n", value + strlen(value) - 20);
    }

    #undef APPEND_CHAR
    return init_token(TOKEN_ID, value);
}

int main() {
    char input[1005];
    strcpy(input, "abc");  // Start with short test
    
    printf("Testing short identifier: %s\n", input);
    lexer_T* lexer = init_lexer(input);
    token_T* token = debug_lexer_collect_id(lexer);
    printf("Result: '%s' (len: %zu)\n\n", token->value, strlen(token->value));
    lexer_free(lexer);
    
    // Now test long
    for (int i = 0; i < 999; i++) {
        input[i] = 'a' + (i % 26);
    }
    input[999] = '\0';
    
    printf("Testing long identifier (999 chars)\n");
    lexer = init_lexer(input);
    token = debug_lexer_collect_id(lexer);
    printf("Result length: %zu\n", strlen(token->value));
    lexer_free(lexer);
    
    return 0;
}