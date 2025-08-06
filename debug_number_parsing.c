#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Debug version of lexer_collect_number to see what's happening
token_T* debug_lexer_collect_number(lexer_T* lexer)
{
    printf("Starting number collection, c='%c', i=%d\n", lexer->c, lexer->i);
    
    char* value = malloc(1);
    value[0] = '\0';
    int has_dot = 0;

    printf("Initial value: '%s'\n", value);

    // Collect digits and at most one decimal point
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot))
    {
        printf("Processing character: '%c' at i=%d\n", lexer->c, lexer->i);
        
        if (lexer->c == '.') {
            // Check if next character is a digit (to avoid treating "42.foo" as a number)
            if (lexer->i + 1 >= strlen(lexer->contents) || !isdigit(lexer->contents[lexer->i + 1])) {
                printf("Breaking on dot not followed by digit\n");
                break;
            }
            has_dot = 1;
        }
        
        char* s = malloc(2);
        s[0] = lexer->c;
        s[1] = '\0';
        printf("Adding '%s' to value\n", s);
        
        value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);
        printf("Value now: '%s'\n", value);
        free(s);
        
        lexer->i++;
        if (lexer->i < strlen(lexer->contents)) {
            lexer->c = lexer->contents[lexer->i];
        } else {
            lexer->c = '\0';
        }
        printf("Advanced to c='%c', i=%d\n", lexer->c, lexer->i);
    }

    printf("After digit collection: value='%s', c='%c', i=%d\n", value, lexer->c, lexer->i);

    // Handle scientific notation (e/E)
    if (lexer->c == 'e' || lexer->c == 'E') {
        printf("Found e/E at position %d\n", lexer->i);
        // Check if this is actually scientific notation (followed by digit or +/-)
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            printf("Next character after e/E: '%c'\n", next_char);
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                printf("Valid scientific notation pattern found!\n");
                char* s = malloc(2);
                s[0] = lexer->c;
                s[1] = '\0';
                printf("Adding e/E: '%s'\n", s);
                value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                strcat(value, s);
                printf("Value after adding e/E: '%s'\n", value);
                free(s);
                
                lexer->i++;
                if (lexer->i < strlen(lexer->contents)) {
                    lexer->c = lexer->contents[lexer->i];
                } else {
                    lexer->c = '\0';
                }
                printf("Advanced to c='%c', i=%d\n", lexer->c, lexer->i);
                
                // Handle optional +/- after e/E
                if (lexer->c == '+' || lexer->c == '-') {
                    printf("Found sign after e/E: '%c'\n", lexer->c);
                    s = malloc(2);
                    s[0] = lexer->c;
                    s[1] = '\0';
                    value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Value after adding sign: '%s'\n", value);
                    free(s);
                    lexer->i++;
                    if (lexer->i < strlen(lexer->contents)) {
                        lexer->c = lexer->contents[lexer->i];
                    } else {
                        lexer->c = '\0';
                    }
                    printf("Advanced to c='%c', i=%d\n", lexer->c, lexer->i);
                }
                
                // Collect exponent digits
                printf("Collecting exponent digits...\n");
                while (isdigit(lexer->c)) {
                    printf("Adding exponent digit: '%c'\n", lexer->c);
                    s = malloc(2);
                    s[0] = lexer->c;
                    s[1] = '\0';
                    value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    printf("Value: '%s'\n", value);
                    free(s);
                    lexer->i++;
                    if (lexer->i < strlen(lexer->contents)) {
                        lexer->c = lexer->contents[lexer->i];
                    } else {
                        lexer->c = '\0';
                    }
                    printf("Advanced to c='%c', i=%d\n", lexer->c, lexer->i);
                }
            } else {
                printf("Not valid scientific notation, next char is '%c'\n", next_char);
            }
        } else {
            printf("At end of input, not scientific notation\n");
        }
    } else {
        printf("No e/E found, c='%c'\n", lexer->c);
    }

    printf("Final value: '%s'\n", value);
    
    token_T* token = malloc(sizeof(token_T));
    token->type = 5; // TOKEN_NUMBER
    token->value = value;
    return token;
}

int main() {
    printf("Debug testing scientific notation parsing:\n\n");
    
    char* input = "1e5";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: '%s', length: %zu\n", input, strlen(input));
    printf("Initial state: c='%c', i=%d\n\n", lexer->c, lexer->i);
    
    token_T* token = debug_lexer_collect_number(lexer);
    printf("\nFinal result: value='%s'\n", token->value);
    
    return 0;
}