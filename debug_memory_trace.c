#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Debug version of lexer_collect_id with memory tracing
token_T* debug_lexer_collect_id_trace(lexer_T* lexer)
{
    printf("DEBUG: Starting ID collection with memory tracing\n");
    
    // Use buffer-based approach for efficiency and correctness
    size_t buffer_size = 64; // Start with 64 chars for identifiers
    char* value = memory_alloc(buffer_size);
    printf("Initial allocation: ptr=%p, size=%zu\n", (void*)value, buffer_size);
    
    if (!value) {
        printf("ERROR: Initial memory allocation failed!\n");
        return NULL;
    }
    
    value[0] = '\0';
    size_t value_len = 0;
    
    int char_count = 0;
    // Collect identifier characters (alphanumeric + underscore)
    while (isalnum(lexer->c) || lexer->c == '_')
    {
        char_count++;
        
        // Check if buffer needs expansion
        if (value_len + 1 >= buffer_size) {
            size_t old_size = buffer_size;
            char* old_ptr = value;
            buffer_size *= 2;
            printf("EXPANDING: char %d, old_size=%zu, new_size=%zu, old_ptr=%p\n", 
                   char_count, old_size, buffer_size, (void*)old_ptr);
            
            value = memory_realloc(value, buffer_size);
            printf("REALLOC result: new_ptr=%p %s\n", (void*)value, 
                   value ? "SUCCESS" : "FAILED");
            
            if (!value) {
                printf("ERROR: Memory reallocation failed at char %d!\n", char_count);
                return NULL;
            }
            
            // Verify the string is still intact
            printf("String after realloc (len=%zu): '%.10s%s'\n", 
                   strlen(value), value, strlen(value) > 10 ? "...' (truncated)" : "'");
        }
        
        // Append character
        value[value_len++] = lexer->c;
        value[value_len] = '\0';
        
        if (char_count % 50 == 0) {
            printf("Processed %d chars, buffer_size=%zu, value_len=%zu\n", 
                   char_count, buffer_size, value_len);
        }
        
        lexer_advance(lexer);
    }
    
    printf("Final: processed %d chars, final length: %zu\n", char_count, strlen(value));
    printf("Final buffer: ptr=%p, size=%zu\n", (void*)value, buffer_size);
    
    // Check if this identifier is a keyword
    int token_type = lexer_keyword_type(value);
    return init_token(token_type, value);
}

int main() {
    // Test with 100 chars (which we know fails)
    char test_input[150];
    int len = 100;
    
    // Create identifier of specified length
    for (int j = 0; j < len; j++) {
        test_input[j] = 'a' + (j % 26);
    }
    test_input[len] = '\0';
    
    printf("=== Testing %d char identifier with memory tracing ===\n", len);
    
    lexer_T* lexer = init_lexer(test_input);
    token_T* token = debug_lexer_collect_id_trace(lexer);
    
    printf("\nToken result:\n");
    printf("  Type: %d\n", token ? token->type : -1);
    printf("  Value length: %zu\n", token && token->value ? strlen(token->value) : 0);
    printf("  Expected: %d - %s\n", len, 
           (token && token->value && strlen(token->value) == len) ? "PASS" : "FAIL");
    
    lexer_free(lexer);
    
    return 0;
}