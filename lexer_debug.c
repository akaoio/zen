#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Get current character as string (from original code)
 */
char* lexer_get_current_char_as_string_debug(lexer_T* lexer)
{
    char* str = memory_alloc(2);
    str[0] = lexer->c;
    str[1] = '\0';
    return str;
}

/**
 * @brief Debug version of lexer_collect_number
 */
token_T* lexer_collect_number_debug(lexer_T* lexer)
{
    printf("=== DEBUG lexer_collect_number ===\n");
    printf("Starting with character: '%c' at position %d\n", lexer->c, lexer->i);
    
    char* value = memory_alloc(1);
    value[0] = '\0';
    int has_dot = 0;
    printf("Initial value: '%s'\n", value);

    // Collect digits and at most one decimal point
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot))
    {
        printf("Processing digit: '%c'\n", lexer->c);
        
        if (lexer->c == '.') {
            // Check if next character is a digit (to avoid treating "42.foo" as a number)
            if (lexer->i + 1 >= strlen(lexer->contents) || !isdigit(lexer->contents[lexer->i + 1])) {
                printf("Breaking on dot (next char not digit)\n");
                break;
            }
            has_dot = 1;
        }
        
        char* s = lexer_get_current_char_as_string_debug(lexer);
        printf("Got character string: '%s'\n", s);
        printf("Current value before realloc: '%s' (len: %zu)\n", value, strlen(value));
        
        size_t old_len = strlen(value);
        size_t new_len = old_len + strlen(s) + 1;
        printf("Reallocating to size: %zu\n", new_len);
        
        value = memory_realloc(value, new_len * sizeof(char));
        printf("After realloc, value: '%s'\n", value);
        
        strcat(value, s);
        printf("After strcat, value: '%s'\n", value);
        
        memory_free(s);
        lexer_advance(lexer);
        printf("Advanced, now at: '%c' (pos %d)\n", lexer->c, lexer->i);
    }

    printf("After digit collection, value: '%s', current char: '%c'\n", value, lexer->c);

    // Handle scientific notation (e/E)
    if (lexer->c == 'e' || lexer->c == 'E') {
        printf("Found scientific notation marker: '%c'\n", lexer->c);
        
        // Check if this is actually scientific notation (followed by digit or +/-)
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            printf("Next character: '%c'\n", next_char);
            
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                printf("Valid scientific notation detected!\n");
                
                char* s = lexer_get_current_char_as_string_debug(lexer);
                printf("Adding e/E: '%s'\n", s);
                printf("Value before e/E: '%s'\n", value);
                
                value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                strcat(value, s);
                printf("Value after e/E: '%s'\n", value);
                
                memory_free(s);
                lexer_advance(lexer);
                
                // Handle optional +/- after e/E
                if (lexer->c == '+' || lexer->c == '-') {
                    printf("Found sign after e/E: '%c'\n", lexer->c);
                    s = lexer_get_current_char_as_string_debug(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Value after sign: '%s'\n", value);
                    memory_free(s);
                    lexer_advance(lexer);
                }
                
                // Collect exponent digits
                while (isdigit(lexer->c)) {
                    printf("Adding exponent digit: '%c'\n", lexer->c);
                    s = lexer_get_current_char_as_string_debug(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Value after digit: '%s'\n", value);
                    memory_free(s);
                    lexer_advance(lexer);
                }
            }
        }
    }

    printf("Final value: '%s'\n", value);
    return init_token(TOKEN_NUMBER, value);
}

int main() {
    char* input = "1e5";
    printf("Testing: %s\n", input);
    
    lexer_T* lexer = init_lexer(input);
    token_T* token = lexer_collect_number_debug(lexer);
    
    printf("\nResult:\n");
    printf("Token type: %d\n", token->type);
    printf("Token value: '%s'\n", token->value);
    
    lexer_free(lexer);
    return 0;
}