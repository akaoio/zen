#include "zen/core/lexer.h"
#include "zen/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


/**
 * @brief Create a new lexer instance
 * @param contents The input text to tokenize
 * @return Pointer to newly allocated lexer instance
 */
lexer_T* init_lexer(char* contents)
{
    if (!contents) {
        lexer_T* lexer = memory_alloc(sizeof(struct LEXER_STRUCT));
        lexer->contents = "";
        lexer->i = 0;
        lexer->c = '\0';
        lexer->current_indent = 0;
        lexer->indent_stack = memory_alloc(sizeof(int));
        lexer->indent_stack[0] = 0;
        lexer->indent_stack_size = 1;
        lexer->at_line_start = 0;
        return lexer;
    }
    
    lexer_T* lexer = memory_alloc(sizeof(struct LEXER_STRUCT));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    lexer->current_indent = 0;
    lexer->indent_stack = memory_alloc(sizeof(int));
    lexer->indent_stack[0] = 0;
    lexer->indent_stack_size = 1;
    lexer->at_line_start = 0;  // Start as not at line start for single-line expressions

    return lexer;
}

/**
 * @brief Free a lexer instance and its resources
 * @param lexer The lexer instance to free
 */
void lexer_free(lexer_T* lexer)
{
    if (!lexer) {
        return;
    }
    
    if (lexer->indent_stack) {
        memory_free(lexer->indent_stack);
    }
    
    // Note: contents is not freed here as it's usually owned by caller
    memory_free(lexer);
}

/**
 * @brief Advance lexer position
 * @param lexer The lexer instance to advance
 */
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

/**
 * @brief Skip whitespace and track indentation
 * @param lexer The lexer instance to process
 */
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

/**
 * @brief Get next token
 * @param lexer The lexer instance to read from
 * @return Pointer to next token
 */
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

        // Skip spaces and tabs but track indentation only at line start (and only after newlines)
        if (lexer->c == ' ' || lexer->c == '\t')
        {
            if (lexer->at_line_start && lexer->i > 0)  // Only track indentation after we've seen content
            {
                // We're at the start of a line after a newline, track indentation changes
                // Handle mixed tabs and spaces by treating tabs as 4 spaces
                while (lexer->c == ' ' || lexer->c == '\t') {
                    if (lexer->c == ' ') {
                        lexer->current_indent++;
                    } else if (lexer->c == '\t') {
                        lexer->current_indent += 4;  // Treat tab as 4 spaces
                    }
                    lexer_advance(lexer);
                }
                lexer->at_line_start = 0;
                
                // Check indentation changes only if there's actual content after spaces
                if (lexer->c != '\n' && lexer->c != '\0')
                {
                    int last_indent = lexer->indent_stack[lexer->indent_stack_size - 1];
                    
                    if (lexer->current_indent > last_indent)
                    {
                        // Push new indent level
                        lexer->indent_stack_size++;
                        lexer->indent_stack = memory_realloc(lexer->indent_stack, lexer->indent_stack_size * sizeof(int));
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
                    // If indentation is the same, don't generate any tokens
                }
            }
            else
            {
                // Just skip spaces and tabs without tracking indentation
                if (lexer->at_line_start) {
                    lexer->at_line_start = 0;  // No longer at line start
                }
                lexer_advance(lexer);
            }
            continue;
        }

        // Numbers (digits)
        if (isdigit(lexer->c)) {
            lexer->at_line_start = 0;  // No longer at line start after parsing content
            return lexer_collect_number(lexer);
        }

        // Identifiers and keywords (letters and underscore)
        if (isalpha(lexer->c) || lexer->c == '_') {
            lexer->at_line_start = 0;  // No longer at line start after parsing content
            return lexer_collect_id(lexer);
        }

        // String literals
        if (lexer->c == '"') {
            lexer->at_line_start = 0;  // No longer at line start after parsing content
            return lexer_collect_string(lexer);
        }
        
        // Single-line comments (//)
        if (lexer->c == '/' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '/') {
            // Skip the rest of the line
            while (lexer->c != '\n' && lexer->c != '\0') {
                lexer_advance(lexer);
            }
            continue;
        }
        
        // Multi-line comments (/* */)
        if (lexer->c == '/' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '*') {
            lexer_advance(lexer); // Skip '/'
            lexer_advance(lexer); // Skip '*'
            
            // Look for closing */
            while (lexer->c != '\0') {
                if (lexer->c == '*' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '/') {
                    lexer_advance(lexer); // Skip '*'
                    lexer_advance(lexer); // Skip '/'
                    break;
                }
                if (lexer->c == '\n') {
                    lexer->at_line_start = 1;
                    lexer->current_indent = 0;
                }
                lexer_advance(lexer);
            }
            continue;
        }

        switch (lexer->c)
        {
            // Operators
            case '=':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '=')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_EQUALS, "=="));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_EQUALS, "="));
            
            case '!':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '=')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_NOT_EQUALS, "!="));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_NOT, "!"));
            
            case '<':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '=')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_LESS_EQUALS, "<="));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_LESS_THAN, "<"));
            
            case '>':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '=')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_GREATER_EQUALS, ">="));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_GREATER_THAN, ">"));
            
            case '+':
                return lexer_advance_with_token(lexer, init_token(TOKEN_PLUS, "+"));
            case '-':
                return lexer_advance_with_token(lexer, init_token(TOKEN_MINUS, "-"));
            case '*':
                return lexer_advance_with_token(lexer, init_token(TOKEN_MULTIPLY, "*"));
            case '/':
                return lexer_advance_with_token(lexer, init_token(TOKEN_DIVIDE, "/"));
            case '%':
                return lexer_advance_with_token(lexer, init_token(TOKEN_MODULO, "%"));
            
            case '&':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '&')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_AND, "&&"));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_AND, "&"));
            
            case '|':
                if (lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '|')
                {
                    lexer_advance(lexer);
                    return lexer_advance_with_token(lexer, init_token(TOKEN_OR, "||"));
                }
                return lexer_advance_with_token(lexer, init_token(TOKEN_OR, "|"));
            
            // Punctuation
            case ',':
                return lexer_advance_with_token(lexer, init_token(TOKEN_COMMA, ","));
            case '.':
                return lexer_advance_with_token(lexer, init_token(TOKEN_DOT, "."));
            case ':':
                return lexer_advance_with_token(lexer, init_token(TOKEN_COLON, ":"));
            case '?':
                return lexer_advance_with_token(lexer, init_token(TOKEN_QUESTION, "?"));
            case '(':
                return lexer_advance_with_token(lexer, init_token(TOKEN_LPAREN, "("));
            case ')':
                return lexer_advance_with_token(lexer, init_token(TOKEN_RPAREN, ")"));
            case '[':
                return lexer_advance_with_token(lexer, init_token(TOKEN_LBRACKET, "["));
            case ']':
                return lexer_advance_with_token(lexer, init_token(TOKEN_RBRACKET, "]"));
            
            default:
                // Handle unexpected characters more gracefully
                if (lexer->c == '\t') {
                    // Handle tabs in content (not indentation) - treat as whitespace
                    lexer_advance(lexer);
                    continue;
                } else if (isprint(lexer->c)) {
                    // For now, skip unexpected printable characters 
                    // This allows the lexer to continue instead of crashing
                    lexer_advance(lexer);
                    continue;
                } else if (lexer->c == '\0') {
                    // End of input reached - don't print error for null terminator
                    break;
                } else {
                    // Only print error for truly unexpected characters
                    printf("Unexpected character: %c (ASCII %d)\n", lexer->c, (int)lexer->c);
                    lexer_advance(lexer);
                    continue;
                }
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

/**
 * @brief Collect string token
 * @param lexer The lexer instance to read from
 * @return String token
 */
token_T* lexer_collect_string(lexer_T* lexer)
{
    lexer_advance(lexer); // Skip opening quote

    char* value = memory_alloc(1);
    value[0] = '\0';
    size_t value_len = 0;

    while (lexer->c != '"' && lexer->c != '\0')
    {
        char ch_to_add;
        
        if (lexer->c == '\\' && lexer->i + 1 < strlen(lexer->contents)) {
            // Handle escape sequences
            lexer_advance(lexer); // Skip backslash
            
            switch (lexer->c) {
                case 'n': ch_to_add = '\n'; break;
                case 't': ch_to_add = '\t'; break;
                case 'r': ch_to_add = '\r'; break;
                case '\\': ch_to_add = '\\'; break;
                case '"': ch_to_add = '"'; break;
                case '0': ch_to_add = '\0'; break;
                default: 
                    // For unrecognized escapes, include the backslash and character
                    value = memory_realloc(value, value_len + 3);
                    value[value_len] = '\\';
                    value[value_len + 1] = lexer->c;
                    value[value_len + 2] = '\0';
                    value_len += 2;
                    lexer_advance(lexer);
                    continue;
            }
        } else {
            ch_to_add = lexer->c;
        }
        
        // Add character to value
        value = memory_realloc(value, value_len + 2);
        value[value_len] = ch_to_add;
        value[value_len + 1] = '\0';
        value_len++;
        
        lexer_advance(lexer);
    }

    if (lexer->c == '"') {
        lexer_advance(lexer); // Skip closing quote
    }

    return init_token(TOKEN_STRING, value);
}

/**
 * @brief Collect number token (integer or float)
 * @param lexer The lexer instance to read from
 * @return Number token
 */
token_T* lexer_collect_number(lexer_T* lexer)
{
    char* value = memory_alloc(1);
    value[0] = '\0';
    int has_dot = 0;

    // Collect digits and at most one decimal point
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot))
    {
        if (lexer->c == '.') {
            // Check if next character is a digit (to avoid treating "42.foo" as a number)
            if (lexer->i + 1 >= strlen(lexer->contents) || !isdigit(lexer->contents[lexer->i + 1])) {
                break;
            }
            has_dot = 1;
        }
        
        char* s = lexer_get_current_char_as_string(lexer);
        value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);
        memory_free(s);
        
        lexer_advance(lexer);
    }

    // Handle scientific notation (e/E)
    if (lexer->c == 'e' || lexer->c == 'E') {
        // Check if this is actually scientific notation (followed by digit or +/-)
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                char* s = lexer_get_current_char_as_string(lexer);
                value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                strcat(value, s);
                memory_free(s);
                lexer_advance(lexer);
                
                // Handle optional +/- after e/E
                if (lexer->c == '+' || lexer->c == '-') {
                    s = lexer_get_current_char_as_string(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    memory_free(s);
                    lexer_advance(lexer);
                }
                
                // Collect exponent digits
                while (isdigit(lexer->c)) {
                    s = lexer_get_current_char_as_string(lexer);
                    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
                    strcat(value, s);
                    memory_free(s);
                    lexer_advance(lexer);
                }
            }
        }
    }

    return init_token(TOKEN_NUMBER, value);
}

/**
 * @brief Check if identifier is a keyword and return appropriate token type
 * @param value The identifier string
 * @return Token type (keyword or TOKEN_ID)
 */
int lexer_keyword_type(const char* value)
{
    // Literals
    if (strcmp(value, "true") == 0) return TOKEN_TRUE;
    if (strcmp(value, "false") == 0) return TOKEN_FALSE;
    if (strcmp(value, "null") == 0) return TOKEN_NULL;
    
    // Control flow keywords
    if (strcmp(value, "set") == 0) return TOKEN_SET;
    if (strcmp(value, "function") == 0) return TOKEN_FUNCTION;
    if (strcmp(value, "return") == 0) return TOKEN_RETURN;
    if (strcmp(value, "if") == 0) return TOKEN_IF;
    if (strcmp(value, "elif") == 0) return TOKEN_ELIF;
    if (strcmp(value, "else") == 0) return TOKEN_ELSE;
    if (strcmp(value, "then") == 0) return TOKEN_THEN;
    if (strcmp(value, "while") == 0) return TOKEN_WHILE;
    if (strcmp(value, "for") == 0) return TOKEN_FOR;
    if (strcmp(value, "in") == 0) return TOKEN_IN;
    if (strcmp(value, "break") == 0) return TOKEN_BREAK;
    if (strcmp(value, "continue") == 0) return TOKEN_CONTINUE;
    
    // Object-oriented keywords
    if (strcmp(value, "class") == 0) return TOKEN_CLASS;
    if (strcmp(value, "extends") == 0) return TOKEN_EXTENDS;
    
    // Module system keywords
    if (strcmp(value, "import") == 0) return TOKEN_IMPORT;
    if (strcmp(value, "export") == 0) return TOKEN_EXPORT;
    if (strcmp(value, "from") == 0) return TOKEN_FROM;
    if (strcmp(value, "as") == 0) return TOKEN_AS;
    
    // Error handling keywords
    if (strcmp(value, "try") == 0) return TOKEN_TRY;
    if (strcmp(value, "catch") == 0) return TOKEN_CATCH;
    if (strcmp(value, "throw") == 0) return TOKEN_THROW;
    
    // Logical operators as keywords
    if (strcmp(value, "and") == 0) return TOKEN_AND;
    if (strcmp(value, "or") == 0) return TOKEN_OR;
    if (strcmp(value, "not") == 0) return TOKEN_NOT;
    
    // Not a keyword, return as identifier
    return TOKEN_ID;
}

/**
 * @brief Collect identifier token with keyword recognition
 * @param lexer The lexer instance to read from
 * @return Identifier or keyword token
 */
token_T* lexer_collect_id(lexer_T* lexer)
{
    char* value = memory_alloc(1);
    value[0] = '\0';

    // Collect identifier characters (alphanumeric + underscore)
    while (isalnum(lexer->c) || lexer->c == '_')
    {
        char* s = lexer_get_current_char_as_string(lexer);
        value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);
        memory_free(s);

        lexer_advance(lexer);
    }

    // Check if this identifier is a keyword
    int token_type = lexer_keyword_type(value);
    return init_token(token_type, value);
}

/**
 * @brief Advance and return token
 * @param lexer The lexer instance to advance
 * @param token The token to return
 * @return The provided token
 */
token_T* lexer_advance_with_token(lexer_T* lexer, token_T* token)
{
    lexer_advance(lexer);

    return token;
}

/**
 * @brief Get current character as string
 * @param lexer The lexer instance to read from
 * @return Current character as allocated string
 */
char* lexer_get_current_char_as_string(lexer_T* lexer)
{
    char* str = memory_alloc(2);
    str[0] = lexer->c;
    str[1] = '\0';

    return str;
}
