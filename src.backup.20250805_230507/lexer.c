#include "include/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


lexer_T* init_lexer(char* contents)
{
    lexer_T* lexer = calloc(1, sizeof(struct LEXER_STRUCT));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    lexer->current_indent = 0;
    lexer->indent_stack = calloc(1, sizeof(int));
    lexer->indent_stack[0] = 0;
    lexer->indent_stack_size = 1;
    lexer->at_line_start = 1;

    return lexer;
}

void lexer_advance(lexer_T* lexer)
{
    if (lexer->c != '\0' && lexer->i < strlen(lexer->contents))
    {
        if (lexer->c == '\n')
        {
            lexer->at_line_start = 1;
            lexer->current_indent = 0;
        }
        lexer->i += 1;
        lexer->c = lexer->contents[lexer->i];
    }
}

void lexer_skip_whitespace(lexer_T* lexer)
{
    while (lexer->c == ' ')
    {
        if (lexer->at_line_start)
            lexer->current_indent++;
        lexer_advance(lexer);
    }
    lexer->at_line_start = 0;
}

token_T* lexer_get_next_token(lexer_T* lexer)
{
    while (lexer->c != '\0' && lexer->i < strlen(lexer->contents))
    {
        // Handle newlines and indentation
        if (lexer->c == '\n')
        {
            lexer_advance(lexer);
            return init_token(TOKEN_NEWLINE, "\n");
        }

        // Skip spaces but track indentation
        if (lexer->c == ' ')
        {
            lexer_skip_whitespace(lexer);
            
            // If at line start, check indentation changes
            if (lexer->at_line_start && lexer->c != '\n' && lexer->c != '\0')
            {
                int last_indent = lexer->indent_stack[lexer->indent_stack_size - 1];
                
                if (lexer->current_indent > last_indent)
                {
                    // Push new indent level
                    lexer->indent_stack_size++;
                    lexer->indent_stack = realloc(lexer->indent_stack, lexer->indent_stack_size * sizeof(int));
                    lexer->indent_stack[lexer->indent_stack_size - 1] = lexer->current_indent;
                    return init_token(TOKEN_INDENT, "INDENT");
                }
                else if (lexer->current_indent < last_indent)
                {
                    // Pop indent levels
                    while (lexer->indent_stack_size > 1 && 
                           lexer->indent_stack[lexer->indent_stack_size - 1] > lexer->current_indent)
                    {
                        lexer->indent_stack_size--;
                        return init_token(TOKEN_DEDENT, "DEDENT");
                    }
                }
            }
            continue;
        }

        if (isalnum(lexer->c))
            return lexer_collect_id(lexer);

        if (lexer->c == '"')
            return lexer_collect_string(lexer);

        switch (lexer->c)
        {
            case '=': return lexer_advance_with_token(lexer, init_token(TOKEN_EQUALS, lexer_get_current_char_as_string(lexer))); break;
            case ',': return lexer_advance_with_token(lexer, init_token(TOKEN_COMMA, lexer_get_current_char_as_string(lexer))); break;
            default:
                printf("Unexpected character: %c\n", lexer->c);
                exit(1);
        }
    }

    // Emit remaining dedents at EOF
    if (lexer->indent_stack_size > 1)
    {
        lexer->indent_stack_size--;
        return init_token(TOKEN_DEDENT, "DEDENT");
    }

    return init_token(TOKEN_EOF, "\0");
}

token_T* lexer_collect_string(lexer_T* lexer)
{
    lexer_advance(lexer);

    char* value = calloc(1, sizeof(char));
    value[0] = '\0';

    while (lexer->c != '"')
    {
        char* s = lexer_get_current_char_as_string(lexer);
        value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);

        lexer_advance(lexer);
    }

    lexer_advance(lexer);

    return init_token(TOKEN_STRING, value);
}

token_T* lexer_collect_id(lexer_T* lexer)
{
    char* value = calloc(1, sizeof(char));
    value[0] = '\0';

    while (isalnum(lexer->c))
    {
        char* s = lexer_get_current_char_as_string(lexer);
        value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);

        lexer_advance(lexer);
    }

    return init_token(TOKEN_ID, value);
}

token_T* lexer_advance_with_token(lexer_T* lexer, token_T* token)
{
    lexer_advance(lexer);

    return token;
}

char* lexer_get_current_char_as_string(lexer_T* lexer)
{
    char* str = calloc(2, sizeof(char));
    str[0] = lexer->c;
    str[1] = '\0';

    return str;
}
