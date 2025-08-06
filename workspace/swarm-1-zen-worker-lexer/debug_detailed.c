#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    char* input = "1e5";
    printf("Debugging input: %s (length: %zu)\n", input, strlen(input));
    
    lexer_T* lexer = init_lexer(input);
    
    printf("Initial state: i=%d, c='%c'\n", lexer->i, lexer->c);
    
    // Let's trace through lexer_collect_number manually
    printf("Calling lexer_collect_number...\n");
    
    char* value = memory_alloc(1);
    value[0] = '\0';
    int has_dot = 0;
    
    printf("Collecting digits and dots...\n");
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot))
    {
        printf("Found digit: '%c' at position %d\n", lexer->c, lexer->i);
        if (lexer->c == '.') {
            if (lexer->i + 1 >= strlen(lexer->contents) || !isdigit(lexer->contents[lexer->i + 1])) {
                break;
            }
            has_dot = 1;
        }
        
        char* s = lexer_get_current_char_as_string(lexer);
        value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);
        printf("Current value: '%s'\n", value);
        memory_free(s);
        
        lexer_advance(lexer);
        printf("After advance: i=%d, c='%c'\n", lexer->i, lexer->c);
    }
    
    printf("After digits collection: value='%s', current char='%c'\n", value, lexer->c);
    
    // Check scientific notation
    if (lexer->c == 'e' || lexer->c == 'E') {
        printf("Found e/E at position %d\n", lexer->i);
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            printf("Next character after e/E: '%c'\n", next_char);
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                printf("Valid scientific notation detected!\n");
                
                char* s = lexer_get_current_char_as_string(lexer);
                value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                strcat(value, s);
                printf("Added e/E, value now: '%s'\n", value);
                memory_free(s);
                lexer_advance(lexer);
                
                // Handle optional +/- after e/E
                if (lexer->c == '+' || lexer->c == '-') {
                    s = lexer_get_current_char_as_string(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Added +/-, value now: '%s'\n", value);
                    memory_free(s);
                    lexer_advance(lexer);
                }
                
                // Collect exponent digits
                while (isdigit(lexer->c)) {
                    printf("Adding exponent digit: '%c'\n", lexer->c);
                    s = lexer_get_current_char_as_string(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Exponent value now: '%s'\n", value);
                    memory_free(s);
                    lexer_advance(lexer);
                }
            }
        }
    }
    
    printf("Final value: '%s'\n", value);
    
    return 0;
}