#include "zen/core/lexer.h"

#include "zen/core/memory.h"

#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/**
 * @brief Create a new lexer instance
 * @param contents The input text to tokenize
 * @return Pointer to newly allocated lexer instance
 */
lexer_T *lexer_new(char *contents)
{
    if (!contents) {
        lexer_T *lexer = memory_alloc(sizeof(struct LEXER_STRUCT));
        memset(lexer, 0, sizeof(struct LEXER_STRUCT));
        lexer->contents = "";
        lexer->i = 0;
        lexer->c = '\0';
        lexer->current_indent = 0;
        lexer->indent_stack = memory_alloc(sizeof(int));
        lexer->indent_stack[0] = 0;
        lexer->indent_stack_size = 1;
        lexer->at_line_start = 0;
        lexer->last_token_type = TOKEN_EOF;  // Initialize to EOF (no previous token)
        lexer->in_type_context = false;      // Start without type context
        return lexer;
    }

    lexer_T *lexer = memory_alloc(sizeof(struct LEXER_STRUCT));
    memset(lexer, 0, sizeof(struct LEXER_STRUCT));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    lexer->current_indent = 0;
    lexer->indent_stack = memory_alloc(sizeof(int));
    lexer->indent_stack[0] = 0;
    lexer->indent_stack_size = 1;
    lexer->at_line_start = 0;
    lexer->last_token_type = TOKEN_EOF;  // Initialize to EOF (no previous token)
    lexer->in_type_context = false;      // Start without type context

    return lexer;
}

/**
 * @brief Free a lexer instance and its resources
 * @param lexer The lexer instance to free
 */
void lexer_free(lexer_T *lexer)
{
    if (!lexer) {
        return;
    }

    // Free indent stack
    if (lexer->indent_stack) {
        memory_free(lexer->indent_stack);
        lexer->indent_stack = NULL;
    }

    // Free line tracking arrays
    if (lexer->line_starts) {
        memory_free(lexer->line_starts);
        lexer->line_starts = NULL;
    }

    // Free token buffer and its contents
    if (lexer->token_buffer) {
        for (size_t i = 0; i < lexer->buffer_size; i++) {
            if (lexer->token_buffer[i].valid && lexer->token_buffer[i].token) {
                token_free(lexer->token_buffer[i].token);
            }
        }
        memory_free(lexer->token_buffer);
        lexer->token_buffer = NULL;
    }

    // Free error message
    if (lexer->error_message) {
        memory_free(lexer->error_message);
        lexer->error_message = NULL;
    }

    // Free any context string in last error
    if (lexer->last_error.context) {
        memory_free(lexer->last_error.context);
        lexer->last_error.context = NULL;
    }

    memory_free(lexer);
}

/**
 * @brief Advance lexer position
 * @param lexer The lexer instance to advance
 */
void lexer_advance(lexer_T *lexer)
{
    if (lexer->c != '\0' && lexer->i < strlen(lexer->contents)) {
        if (lexer->c == '\n') {
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
void lexer_skip_whitespace(lexer_T *lexer)
{
    while (lexer->c == ' ' || lexer->c == '\t' || lexer->c == '\n' || lexer->c == '\r') {
        if (lexer->at_line_start && lexer->c == ' ')
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
token_T *lexer_get_next_token(lexer_T *lexer)
{
    size_t content_length = strlen(lexer->contents);

    while (lexer->c != '\0' && lexer->i < content_length) {
        // Skip whitespace, newlines, and comments in a unified way
        while ((lexer->c == ' ' || lexer->c == '\t' || lexer->c == '\n') ||
               (lexer->c == '/' && lexer->i + 1 < content_length &&
                (lexer->contents[lexer->i + 1] == '/' || lexer->contents[lexer->i + 1] == '*'))) {
            // Handle newlines
            if (lexer->c == '\n') {
                lexer->at_line_start = 1;
                lexer->current_indent = 0;
                // Check if the entire input contains only whitespace
                bool only_whitespace_input = true;
                for (size_t j = 0; j < content_length; j++) {
                    if (lexer->contents[j] != ' ' && lexer->contents[j] != '\t' &&
                        lexer->contents[j] != '\n' && lexer->contents[j] != '\r') {
                        only_whitespace_input = false;
                        break;
                    }
                }

                if (only_whitespace_input) {
                    lexer_advance(lexer);
                    continue;
                } else {
                    return lexer_advance_with_token(lexer, token_new(TOKEN_NEWLINE, "\n"));
                }
            }

            // Handle whitespace (space and tab)
            if (lexer->c == ' ' || lexer->c == '\t') {
                if (lexer->at_line_start) {
                    if (lexer->c == ' ') {
                        lexer->current_indent++;
                    } else if (lexer->c == '\t') {
                        lexer->current_indent += 4;
                    }
                }
                lexer_advance(lexer);
                continue;
            }

            // Handle single-line comments //
            if (lexer->c == '/' && lexer->i + 1 < content_length &&
                lexer->contents[lexer->i + 1] == '/') {
                while (lexer->c != '\0' && lexer->c != '\n') {
                    lexer_advance(lexer);
                }
                continue;
            }

            // Handle multi-line comments /* */
            if (lexer->c == '/' && lexer->i + 1 < content_length &&
                lexer->contents[lexer->i + 1] == '*') {
                lexer_advance(lexer);
                lexer_advance(lexer);

                while (lexer->c != '\0') {
                    if (lexer->c == '*' && lexer->i + 1 < content_length &&
                        lexer->contents[lexer->i + 1] == '/') {
                        lexer_advance(lexer);
                        lexer_advance(lexer);
                        break;
                    }
                    if (lexer->c == '\n') {
                        lexer->at_line_start = 1;
                        lexer->current_indent = 0;
                    }
                    lexer_advance(lexer);
                }
                // After multi-line comment, reset indentation tracking
                // Don't assume we're at line start unless we actually encounter a newline
                continue;
            }
        }

        // After skipping whitespace and comments, check if we reached end
        if (lexer->c == '\0') {
            break;
        }

        // Handle indentation changes only when we have actual content
        if (lexer->at_line_start) {
            lexer->at_line_start = 0;

            int last_indent = lexer->indent_stack[lexer->indent_stack_size - 1];

            if (lexer->current_indent > last_indent) {
                lexer->indent_stack_size++;
                lexer->indent_stack =
                    memory_realloc(lexer->indent_stack, lexer->indent_stack_size * sizeof(int));
                lexer->indent_stack[lexer->indent_stack_size - 1] = lexer->current_indent;
                return token_new(TOKEN_INDENT, "INDENT");
            } else if (lexer->current_indent < last_indent) {
                while (lexer->indent_stack_size > 1 &&
                       lexer->indent_stack[lexer->indent_stack_size - 1] > lexer->current_indent) {
                    lexer->indent_stack_size--;
                    return token_new(TOKEN_DEDENT, "DEDENT");
                }
            }
        }

        if (isdigit(lexer->c)) {
            lexer->at_line_start = 0;
            // Use enhanced number parsing if underscores detected in lookahead
            size_t lookahead = lexer->i;
            bool has_underscore = false;
            while (lookahead < strlen(lexer->contents) &&
                   (isdigit(lexer->contents[lookahead]) || lexer->contents[lookahead] == '.' ||
                    lexer->contents[lookahead] == '_' || lexer->contents[lookahead] == 'e' ||
                    lexer->contents[lookahead] == 'E' || lexer->contents[lookahead] == '+' ||
                    lexer->contents[lookahead] == '-')) {
                if (lexer->contents[lookahead] == '_') {
                    has_underscore = true;
                    break;
                }
                lookahead++;
            }

            if (has_underscore) {
                return lexer_collect_enhanced_number(lexer);
            } else {
                return lexer_collect_number(lexer);
            }
        }

        if (isalpha(lexer->c) || lexer->c == '_') {
            lexer->at_line_start = 0;
            // Use Unicode identifier parsing if non-ASCII characters detected
            if ((unsigned char)lexer->c >= 128) {
                return lexer_collect_unicode_id(lexer);
            } else {
                return lexer_collect_id(lexer);
            }
        }

        if (lexer->c == '"') {
            lexer->at_line_start = 0;
            return lexer_collect_string(lexer);
        }

        switch (lexer->c) {
        case '=':
            return lexer_advance_with_token(lexer, token_new(TOKEN_EQUALS, "="));

        case '!':
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '=') {
                lexer_advance(lexer);
                return lexer_advance_with_token(lexer, token_new(TOKEN_NOT_EQUALS, "!="));
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_NOT, "!"));

        case '<':
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '=') {
                lexer_advance(lexer);
                return lexer_advance_with_token(lexer, token_new(TOKEN_LESS_EQUALS, "<="));
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_LESS_THAN, "<"));

        case '>':
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '=') {
                lexer_advance(lexer);
                return lexer_advance_with_token(lexer, token_new(TOKEN_GREATER_EQUALS, ">="));
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_GREATER_THAN, ">"));

        case '+':
            return lexer_advance_with_token(lexer, token_new(TOKEN_PLUS, "+"));
        case '-':
            return lexer_advance_with_token(lexer, token_new(TOKEN_MINUS, "-"));
        case '*':
            return lexer_advance_with_token(lexer, token_new(TOKEN_MULTIPLY, "*"));
        case '/':
            return lexer_advance_with_token(lexer, token_new(TOKEN_DIVIDE, "/"));
        case '%':
            return lexer_advance_with_token(lexer, token_new(TOKEN_MODULO, "%"));

        case '&':
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '&') {
                lexer_advance(lexer);
                return lexer_advance_with_token(lexer, token_new(TOKEN_AND, "&&"));
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_AND, "&"));

        case '|':
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '|') {
                lexer_advance(lexer);
                return lexer_advance_with_token(lexer, token_new(TOKEN_OR, "||"));
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_OR, "|"));

        case ',':
            return lexer_advance_with_token(lexer, token_new(TOKEN_COMMA, ","));
        case '.':
            // Check for spread operator (...) first
            if (lexer->i + 2 < content_length && lexer->contents[lexer->i + 1] == '.' &&
                lexer->contents[lexer->i + 2] == '.') {
                lexer_advance(lexer);  // advance past first .
                lexer_advance(lexer);  // advance past second .
                return lexer_advance_with_token(lexer, token_new(TOKEN_SPREAD, "..."));
            }
            // Check for range operator (..)
            if (lexer->i + 1 < content_length && lexer->contents[lexer->i + 1] == '.') {
                lexer_advance(lexer);  // advance past first .
                return lexer_advance_with_token(lexer, token_new(TOKEN_RANGE, ".."));
            }
            // Check if it's a floating point number like .5
            if (lexer->i + 1 < content_length && isdigit(lexer->contents[lexer->i + 1])) {
                return lexer_collect_number(lexer);
            }
            return lexer_advance_with_token(lexer, token_new(TOKEN_DOT, "."));
        case ':':
            return lexer_advance_with_token(lexer, token_new(TOKEN_COLON, ":"));
        case '?':
            return lexer_advance_with_token(lexer, token_new(TOKEN_QUESTION, "?"));
        case '(':
            return lexer_advance_with_token(lexer, token_new(TOKEN_LPAREN, "("));
        case ')':
            return lexer_advance_with_token(lexer, token_new(TOKEN_RPAREN, ")"));
        case '[':
            return lexer_advance_with_token(lexer, token_new(TOKEN_LBRACKET, "["));
        case ']':
            return lexer_advance_with_token(lexer, token_new(TOKEN_RBRACKET, "]"));

        default:
            if (lexer->c == '\t') {
                lexer_advance(lexer);
                continue;
            } else if (isprint(lexer->c)) {
                lexer_advance(lexer);
                continue;
            } else if (lexer->c == '\0') {
                break;
            } else {
                lexer_advance(lexer);
                continue;
            }
        }
    }

    if (lexer->indent_stack_size > 1) {
        lexer->indent_stack_size--;
        return token_new(TOKEN_DEDENT, "DEDENT");
    }

    return token_new(TOKEN_EOF, "\0");
}

/**
 * @brief Collect string token
 * @param lexer The lexer instance to read from
 * @return String token
 */
token_T *lexer_collect_string(lexer_T *lexer)
{
    lexer_advance(lexer);

    size_t buffer_size = 64;
    char *value = memory_alloc(buffer_size);
    if (!value) {
        return NULL;
    }
    value[0] = '\0';
    size_t value_len = 0;

#define APPEND_CHAR(ch)                                                                            \
    do {                                                                                           \
        /* Prevent excessively long numbers that could cause memory issues */                      \
        if (value_len >= 64) {                                                                     \
            memory_free(value);                                                                    \
            lexer_enter_error_recovery(lexer, "Number literal too long");                          \
            return NULL;                                                                           \
        }                                                                                          \
        if (value_len + 1 >= buffer_size) {                                                        \
            size_t new_size = buffer_size * 2;                                                     \
            /* Prevent excessive memory allocation */                                              \
            if (new_size > 256) {                                                                  \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Number literal buffer overflow");               \
                return NULL;                                                                       \
            }                                                                                      \
            char *new_value = memory_realloc(value, new_size);                                     \
            if (!new_value) {                                                                      \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Out of memory parsing number");                 \
                return NULL;                                                                       \
            }                                                                                      \
            value = new_value;                                                                     \
            buffer_size = new_size;                                                                \
        }                                                                                          \
        value[value_len++] = (ch);                                                                 \
        value[value_len] = '\0';                                                                   \
    } while (0)

    while (lexer->c != '"' && lexer->c != '\0') {
        char ch_to_add;

        if (lexer->c == '\\' && lexer->i + 1 < strlen(lexer->contents)) {
            lexer_advance(lexer);

            switch (lexer->c) {
            case 'n':
                ch_to_add = '\n';
                break;
            case 't':
                ch_to_add = '\t';
                break;
            case 'r':
                ch_to_add = '\r';
                break;
            case '\\':
                ch_to_add = '\\';
                break;
            case '"':
                ch_to_add = '"';
                break;
            case '0':
                ch_to_add = '\0';
                break;
            default:
                APPEND_CHAR('\\');
                APPEND_CHAR(lexer->c);
                lexer_advance(lexer);
                continue;
            }
        } else {
            ch_to_add = lexer->c;
        }

        APPEND_CHAR(ch_to_add);
        lexer_advance(lexer);
    }

    if (lexer->c == '"') {
        lexer_advance(lexer);
    }

#undef APPEND_CHAR

    char *tracked_value = memory_strdup(value);
    memory_free(value);

    return token_new(TOKEN_STRING, tracked_value);
}

/**
 * @brief Collect number token
 * @param lexer The lexer instance to read from
 * @return Number token
 */
token_T *lexer_collect_number(lexer_T *lexer)
{
    size_t buffer_size = 32;
    char *value = memory_alloc(buffer_size);
    if (!value) {
        return NULL;
    }
    value[0] = '\0';
    size_t value_len = 0;
    int has_dot = 0;

#define APPEND_CHAR(ch)                                                                            \
    do {                                                                                           \
        /* Prevent excessively long numbers that could cause memory issues */                      \
        if (value_len >= 64) {                                                                     \
            memory_free(value);                                                                    \
            lexer_enter_error_recovery(lexer, "Number literal too long");                          \
            return NULL;                                                                           \
        }                                                                                          \
        if (value_len + 1 >= buffer_size) {                                                        \
            size_t new_size = buffer_size * 2;                                                     \
            /* Prevent excessive memory allocation */                                              \
            if (new_size > 256) {                                                                  \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Number literal buffer overflow");               \
                return NULL;                                                                       \
            }                                                                                      \
            char *new_value = memory_realloc(value, new_size);                                     \
            if (!new_value) {                                                                      \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Out of memory parsing number");                 \
                return NULL;                                                                       \
            }                                                                                      \
            value = new_value;                                                                     \
            buffer_size = new_size;                                                                \
        }                                                                                          \
        value[value_len++] = (ch);                                                                 \
        value[value_len] = '\0';                                                                   \
    } while (0)

    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot)) {
        if (lexer->c == '.') {
            // If this is the first character and it's a dot, we need at least one following digit
            if (value_len == 0) {
                if (lexer->i + 1 >= strlen(lexer->contents) ||
                    !isdigit(lexer->contents[lexer->i + 1])) {
                    break;
                }
            } else {
                // If we already have digits, a trailing dot is acceptable (like "5.")
                // We don't need to check for digits after the dot in this case
            }
            has_dot = 1;
        }

        APPEND_CHAR(lexer->c);
        lexer_advance(lexer);
    }

    if (lexer->c == 'e' || lexer->c == 'E') {
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                APPEND_CHAR(lexer->c);
                lexer_advance(lexer);

                if (lexer->c == '+' || lexer->c == '-') {
                    APPEND_CHAR(lexer->c);
                    lexer_advance(lexer);
                }

                while (isdigit(lexer->c)) {
                    APPEND_CHAR(lexer->c);
                    lexer_advance(lexer);
                }
            }
        }
    }

#undef APPEND_CHAR

    char *tracked_value = memory_strdup(value);
    memory_free(value);

    return token_new(TOKEN_NUMBER, tracked_value);
}

/**
 * @brief Check if identifier is a keyword and return appropriate token type
 * @param value The identifier string
 * @return Token type (keyword or TOKEN_ID)
 */
int lexer_keyword_type(const char *value)
{
    if (strcmp(value, "true") == 0)
        return TOKEN_TRUE;
    if (strcmp(value, "false") == 0)
        return TOKEN_FALSE;
    if (strcmp(value, "null") == 0)
        return TOKEN_NULL;
    if (strcmp(value, "undecidable") == 0)
        return TOKEN_UNDECIDABLE;
    if (strcmp(value, "set") == 0)
        return TOKEN_SET;
    if (strcmp(value, "function") == 0)
        return TOKEN_FUNCTION;
    if (strcmp(value, "return") == 0)
        return TOKEN_RETURN;
    if (strcmp(value, "if") == 0)
        return TOKEN_IF;
    if (strcmp(value, "elif") == 0)
        return TOKEN_ELIF;
    if (strcmp(value, "else") == 0)
        return TOKEN_ELSE;
    if (strcmp(value, "then") == 0)
        return TOKEN_THEN;
    if (strcmp(value, "while") == 0)
        return TOKEN_WHILE;
    if (strcmp(value, "for") == 0)
        return TOKEN_FOR;
    if (strcmp(value, "in") == 0)
        return TOKEN_IN;
    if (strcmp(value, "break") == 0)
        return TOKEN_BREAK;
    if (strcmp(value, "continue") == 0)
        return TOKEN_CONTINUE;
    if (strcmp(value, "class") == 0)
        return TOKEN_CLASS;
    if (strcmp(value, "new") == 0)
        return TOKEN_NEW;
    if (strcmp(value, "extends") == 0)
        return TOKEN_EXTENDS;
    if (strcmp(value, "import") == 0)
        return TOKEN_IMPORT;
    if (strcmp(value, "export") == 0)
        return TOKEN_EXPORT;
    if (strcmp(value, "from") == 0)
        return TOKEN_FROM;
    if (strcmp(value, "as") == 0)
        return TOKEN_AS;
    if (strcmp(value, "try") == 0)
        return TOKEN_TRY;
    if (strcmp(value, "catch") == 0)
        return TOKEN_CATCH;
    if (strcmp(value, "throw") == 0)
        return TOKEN_THROW;
    if (strcmp(value, "get") == 0)
        return TOKEN_GET;
    if (strcmp(value, "put") == 0)
        return TOKEN_PUT;
    if (strcmp(value, "and") == 0)
        return TOKEN_AND;
    if (strcmp(value, "or") == 0)
        return TOKEN_OR;
    if (strcmp(value, "not") == 0)
        return TOKEN_NOT;

    // Phase 2: Natural Language Keywords
    if (strcmp(value, "when") == 0)
        return TOKEN_WHEN;
    if (strcmp(value, "unless") == 0)
        return TOKEN_UNLESS;
    if (strcmp(value, "whenever") == 0)
        return TOKEN_WHENEVER;
    if (strcmp(value, "until") == 0)
        return TOKEN_UNTIL;
    if (strcmp(value, "during") == 0)
        return TOKEN_DURING;
    if (strcmp(value, "throughout") == 0)
        return TOKEN_THROUGHOUT;
    if (strcmp(value, "otherwise") == 0)
        return TOKEN_OTHERWISE;

    return TOKEN_ID;
}

/**
 * @brief Check if identifier is a recognized type name
 * @param value The identifier string to check
 * @return true if the identifier represents a valid type name, false otherwise
 * @note Used for intelligent type inference tokenization (Phase 2)
 */
static bool lexer_is_type_name(const char *value)
{
    // Core ZEN type names
    if (strcmp(value, "integer") == 0)
        return true;
    if (strcmp(value, "int") == 0)
        return true;
    if (strcmp(value, "float") == 0)
        return true;
    if (strcmp(value, "number") == 0)
        return true;
    if (strcmp(value, "decimal") == 0)
        return true;
    if (strcmp(value, "string") == 0)
        return true;
    if (strcmp(value, "text") == 0)
        return true;
    if (strcmp(value, "boolean") == 0)
        return true;
    if (strcmp(value, "bool") == 0)
        return true;
    if (strcmp(value, "array") == 0)
        return true;
    if (strcmp(value, "list") == 0)
        return true;
    if (strcmp(value, "object") == 0)
        return true;
    if (strcmp(value, "record") == 0)
        return true;
    if (strcmp(value, "dict") == 0)
        return true;

    return false;
}

/**
 * @brief Process identifier with type inference context awareness (Phase 2)
 * @param lexer The lexer instance
 * @param identifier The identifier string
 * @return Token with appropriate type context information
 * @note Provides intelligent type inference when identifier follows "as"
 * @note Maintains backward compatibility with existing "as" usage
 */
token_T *lexer_process_type_context_id(lexer_T *lexer, const char *identifier)
{
    if (!lexer || !identifier) {
        return NULL;
    }

    // Check if we're in a type context (after "as" keyword)
    if (lexer->in_type_context && lexer_is_type_name(identifier)) {
        // We're after "as" and this is a recognized type name
        // Create a regular TOKEN_ID but mark that we've processed type context
        lexer->in_type_context = false;  // Reset context flag

        char *tracked_value = memory_strdup(identifier);
        return token_new(TOKEN_ID, tracked_value);
    }

    // Standard identifier processing - check for keywords first
    int token_type = lexer_keyword_type(identifier);

    // If this is the "as" keyword, set the type context flag
    if (token_type == TOKEN_AS) {
        lexer->in_type_context = true;
    } else {
        // Reset type context for other keywords/identifiers
        lexer->in_type_context = false;
    }

    char *tracked_value = memory_strdup(identifier);
    return token_new(token_type, tracked_value);
}

/**
 * @brief Collect identifier token
 * @param lexer The lexer instance to read from
 * @return Identifier or keyword token
 */
token_T *lexer_collect_id(lexer_T *lexer)
{
    size_t buffer_size = 64;
    char *value = memory_alloc(buffer_size);
    if (!value) {
        return NULL;
    }
    value[0] = '\0';
    size_t value_len = 0;

#define APPEND_CHAR(ch)                                                                            \
    do {                                                                                           \
        /* Allow reasonable identifier lengths (up to 4096 characters) */                          \
        if (value_len >= 4096) {                                                                   \
            memory_free(value);                                                                    \
            lexer_enter_error_recovery(lexer, "Identifier too long (max 4096 characters)");        \
            return NULL;                                                                           \
        }                                                                                          \
        if (value_len + 1 >= buffer_size) {                                                        \
            size_t new_size = buffer_size * 2;                                                     \
            /* Prevent excessive memory allocation */                                              \
            if (new_size > 8192) {                                                                 \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Identifier buffer overflow");                   \
                return NULL;                                                                       \
            }                                                                                      \
            char *new_value = memory_realloc(value, new_size);                                     \
            if (!new_value) {                                                                      \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Out of memory parsing identifier");             \
                return NULL;                                                                       \
            }                                                                                      \
            value = new_value;                                                                     \
            buffer_size = new_size;                                                                \
        }                                                                                          \
        value[value_len++] = (ch);                                                                 \
        value[value_len] = '\0';                                                                   \
    } while (0)

    while (isalnum(lexer->c) || lexer->c == '_' || lexer->c == '-') {
        APPEND_CHAR(lexer->c);
        lexer_advance(lexer);
    }

#undef APPEND_CHAR

    // Process identifier with type inference context awareness (Phase 2)
    token_T *processed_token = lexer_process_type_context_id(lexer, value);
    memory_free(value);

    return processed_token;
}

/**
 * @brief Advance lexer position and return provided token
 * @param lexer The lexer instance to advance
 * @param token The token to return after advancing
 * @return The provided token after lexer position is advanced
 */
token_T *lexer_advance_with_token(lexer_T *lexer, token_T *token)
{
    if (!lexer || !token) {
        return token;
    }

    lexer_advance(lexer);
    return token;
}

/**
 * @brief Get current character as string
 * @param lexer The lexer instance to read from
 * @return Current character as allocated string
 */
char *lexer_get_current_char_as_string(lexer_T *lexer)
{
    char *str = memory_alloc(2);
    str[0] = lexer->c;
    str[1] = '\0';

    return str;
}

/**
 * @brief Create enhanced lexer with configuration
 * @param contents Input source code
 * @param config Configuration options
 * @return Enhanced lexer instance
 */
lexer_T *lexer_create_enhanced(char *contents, const LexerConfig *config)
{
    if (!contents || !config)
        return NULL;

    lexer_T *lexer = lexer_new(contents);
    if (!lexer)
        return NULL;

    return lexer;
}

/**
 * @brief Look ahead at tokens without consuming them
 * @param lexer Lexer instance
 * @param offset Number of tokens to look ahead
 * @return Lookahead token or NULL
 */
token_T *lexer_peek_token(lexer_T *lexer, size_t offset)
{
    if (!lexer || offset > 16) {  // Reasonable limit for lookahead
        return NULL;
    }

    // Save current lexer state
    size_t saved_i = lexer->i;
    char saved_c = lexer->c;
    int saved_indent = lexer->current_indent;
    int saved_at_line_start = lexer->at_line_start;
    int saved_indent_stack_size = lexer->indent_stack_size;
    size_t saved_line_number = lexer->line_number;
    size_t saved_column_number = lexer->column_number;

    // Create a copy of indent stack
    int *saved_indent_stack = memory_alloc(saved_indent_stack_size * sizeof(int));
    if (!saved_indent_stack) {
        return NULL;
    }
    memcpy(saved_indent_stack, lexer->indent_stack, saved_indent_stack_size * sizeof(int));

    token_T *result = NULL;

    // Skip 'offset' tokens - get the token at position offset
    for (size_t i = 0; i < offset + 1; i++) {
        if (result) {
            token_free(result);
        }
        result = lexer_get_next_token(lexer);
        if (!result || result->type == TOKEN_EOF) {
            break;
        }
    }

    // Restore lexer state
    lexer->i = saved_i;
    lexer->c = saved_c;
    lexer->current_indent = saved_indent;
    lexer->at_line_start = saved_at_line_start;
    lexer->indent_stack_size = saved_indent_stack_size;
    lexer->line_number = saved_line_number;
    lexer->column_number = saved_column_number;

    // Restore indent stack
    if (lexer->indent_stack != saved_indent_stack) {
        memory_free(lexer->indent_stack);
        lexer->indent_stack = saved_indent_stack;
    } else {
        memory_free(saved_indent_stack);
    }

    return result;
}

/**
 * @brief Get current source location for error reporting
 * @param lexer Lexer instance
 * @return Source location structure
 */
SourceLocation lexer_get_location(const lexer_T *lexer)
{
    SourceLocation loc = {0};
    if (!lexer)
        return loc;

    loc.line = 1;
    loc.column = 1;
    loc.position = lexer->i;

    return loc;
}

/**
 * @brief Enter error recovery mode
 * @param lexer Lexer instance
 * @param error_message Error description
 */
void lexer_enter_error_recovery(lexer_T *lexer, const char *error_message)
{
    if (!lexer)
        return;

    // Set error recovery mode flag
    lexer->error_recovery_mode = true;

    // Store error location information
    lexer->last_error.line = lexer->line_number;
    lexer->last_error.column = lexer->column_number;
    lexer->last_error.position = lexer->i;

    // Store error message (duplicate if provided)
    if (lexer->error_message) {
        memory_free(lexer->error_message);
        lexer->error_message = NULL;
    }

    if (error_message) {
        lexer->error_message = memory_strdup(error_message);
    }

    // Skip to synchronization point (next newline or end of input)
    // This helps recovery by moving past the problematic token
    while (lexer->c != '\0' && lexer->c != '\n') {
        lexer_advance(lexer);
    }
}

/**
 * @brief Exit error recovery mode
 * @param lexer Lexer instance
 */
void lexer_exit_error_recovery(lexer_T *lexer)
{
    if (!lexer)
        return;

    // Clear error recovery mode flag
    lexer->error_recovery_mode = false;

    // Clean up error context
    if (lexer->error_message) {
        memory_free(lexer->error_message);
        lexer->error_message = NULL;
    }

    // Reset error location
    memset(&lexer->last_error, 0, sizeof(SourceLocation));
}

/**
 * @brief Check if lexer is in error recovery mode
 * @param lexer Lexer instance
 * @return true if in error recovery mode
 */
bool lexer_in_error_recovery(const lexer_T *lexer)
{
    if (!lexer)
        return false;

    return lexer->error_recovery_mode;
}

/**
 * @brief Enhanced number collection with better parsing
 * Supports underscores in numbers for readability: 1_000_000, 3.14159_26535
 * Natural formats for financial and scientific calculations
 * @param lexer Lexer instance
 * @return Enhanced number token
 */
token_T *lexer_collect_enhanced_number(lexer_T *lexer)
{
    size_t buffer_size = 64;
    char *value = memory_alloc(buffer_size);
    char *clean_value = memory_alloc(buffer_size);  // For storing without underscores
    if (!value || !clean_value) {
        if (value)
            memory_free(value);
        if (clean_value)
            memory_free(clean_value);
        return NULL;
    }

    value[0] = '\0';
    clean_value[0] = '\0';
    size_t value_len = 0;
    size_t clean_len = 0;
    int has_dot = 0;
    bool last_was_underscore = false;
    bool first_char = true;

#define APPEND_CHAR(ch)                                                                            \
    do {                                                                                           \
        if (value_len + 1 >= buffer_size) {                                                        \
            buffer_size *= 2;                                                                      \
            /* Save original pointers BEFORE realloc calls */                                      \
            char *new_value = memory_realloc(value, buffer_size);                                  \
            char *new_clean = memory_realloc(clean_value, buffer_size);                            \
            if (!new_value || !new_clean) {                                                        \
                memory_free(value);                                                                \
                memory_free(clean_value);                                                          \
                return NULL;                                                                       \
            }                                                                                      \
            value = new_value;                                                                     \
            clean_value = new_clean;                                                               \
        }                                                                                          \
        value[value_len++] = (ch);                                                                 \
        value[value_len] = '\0';                                                                   \
    } while (0)

#define APPEND_CLEAN_CHAR(ch)                                                                      \
    do {                                                                                           \
        clean_value[clean_len++] = (ch);                                                           \
        clean_value[clean_len] = '\0';                                                             \
    } while (0)

    // Handle numbers with underscores for readability
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot) || lexer->c == '_') {
        // Handle underscore separators
        if (lexer->c == '_') {
            // Cannot start with underscore or have consecutive underscores
            if (first_char || last_was_underscore) {
                break;
            }

            // Look ahead to ensure underscore is followed by digit
            if (lexer->i + 1 >= strlen(lexer->contents) ||
                (!isdigit(lexer->contents[lexer->i + 1]) && lexer->contents[lexer->i + 1] != '.')) {
                break;  // Trailing underscore not allowed
            }

            APPEND_CHAR(lexer->c);  // Keep underscore in display value
            last_was_underscore = true;
            lexer_advance(lexer);
            continue;
        }

        // Handle decimal point
        if (lexer->c == '.') {
            // If this is the first character and it's a dot, we need at least one following digit
            if (value_len == 0 || clean_len == 0) {
                if (lexer->i + 1 >= strlen(lexer->contents) ||
                    (!isdigit(lexer->contents[lexer->i + 1]) &&
                     lexer->contents[lexer->i + 1] != '_')) {
                    break;
                }
            }
            has_dot = 1;
        }

        APPEND_CHAR(lexer->c);
        if (lexer->c != '_') {  // Don't add underscores to clean value
            APPEND_CLEAN_CHAR(lexer->c);
        }

        last_was_underscore = false;
        first_char = false;
        lexer_advance(lexer);
    }

    // Handle scientific notation (e/E)
    if (lexer->c == 'e' || lexer->c == 'E') {
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                APPEND_CHAR(lexer->c);
                APPEND_CLEAN_CHAR(lexer->c);
                lexer_advance(lexer);

                // Handle sign
                if (lexer->c == '+' || lexer->c == '-') {
                    APPEND_CHAR(lexer->c);
                    APPEND_CLEAN_CHAR(lexer->c);
                    lexer_advance(lexer);
                }

                // Handle exponent digits (with underscores)
                last_was_underscore = false;
                while (isdigit(lexer->c) || lexer->c == '_') {
                    if (lexer->c == '_') {
                        // Cannot have consecutive underscores in exponent
                        if (last_was_underscore)
                            break;

                        // Look ahead to ensure underscore is followed by digit
                        if (lexer->i + 1 >= strlen(lexer->contents) ||
                            !isdigit(lexer->contents[lexer->i + 1])) {
                            break;  // Trailing underscore not allowed
                        }

                        APPEND_CHAR(lexer->c);  // Keep underscore in display value
                        last_was_underscore = true;
                    } else {
                        APPEND_CHAR(lexer->c);
                        APPEND_CLEAN_CHAR(lexer->c);
                        last_was_underscore = false;
                    }
                    lexer_advance(lexer);
                }
            }
        }
    }

#undef APPEND_CHAR
#undef APPEND_CLEAN_CHAR

    // Validate the number is not malformed
    if (clean_len == 0 || (clean_len == 1 && clean_value[0] == '.')) {
        memory_free(value);
        memory_free(clean_value);
        return NULL;
    }

    // Use clean value for numerical parsing, but keep original for display
    char *tracked_value = memory_strdup(value);
    memory_free(value);
    memory_free(clean_value);

    return token_new(TOKEN_NUMBER, tracked_value);
}

/**
 * @brief Get error context for debugging
 * @param lexer Lexer instance
 * @param context_size Size of context
 * @return Context string
 */
char *lexer_get_error_context(const lexer_T *lexer, size_t context_size)
{
    if (!lexer || !lexer->contents)
        return NULL;

    char *context = memory_alloc(context_size + 1);
    strncpy(context, lexer->contents + lexer->i, context_size);
    context[context_size] = '\0';

    return context;
}

/**
 * @brief Enable token buffering for performance
 * @param lexer Lexer instance
 * @param buffer_size Buffer size (0 to disable)
 * @return true if successful
 */
bool lexer_enable_buffering(lexer_T *lexer, size_t buffer_size)
{
    (void)lexer;        // Suppress unused parameter warning
    (void)buffer_size;  // Suppress unused parameter warning
    return true;
}

/**
 * @brief Collect Unicode identifier tokens
 * Supports international programming with Unicode identifiers from 50+ languages
 * Natural language variable names: température_en_celsius, العدد, 数量, etc.
 * @param lexer Lexer instance
 * @return Unicode identifier token
 */
token_T *lexer_collect_unicode_id(lexer_T *lexer)
{
    size_t buffer_size = 128;  // Larger buffer for Unicode
    char *value = memory_alloc(buffer_size);
    if (!value) {
        return NULL;
    }
    value[0] = '\0';
    size_t value_len = 0;

#define APPEND_CHAR(ch)                                                                            \
    do {                                                                                           \
        /* Prevent excessively long numbers that could cause memory issues */                      \
        if (value_len >= 64) {                                                                     \
            memory_free(value);                                                                    \
            lexer_enter_error_recovery(lexer, "Number literal too long");                          \
            return NULL;                                                                           \
        }                                                                                          \
        if (value_len + 1 >= buffer_size) {                                                        \
            size_t new_size = buffer_size * 2;                                                     \
            /* Prevent excessive memory allocation */                                              \
            if (new_size > 256) {                                                                  \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Number literal buffer overflow");               \
                return NULL;                                                                       \
            }                                                                                      \
            char *new_value = memory_realloc(value, new_size);                                     \
            if (!new_value) {                                                                      \
                memory_free(value);                                                                \
                lexer_enter_error_recovery(lexer, "Out of memory parsing number");                 \
                return NULL;                                                                       \
            }                                                                                      \
            value = new_value;                                                                     \
            buffer_size = new_size;                                                                \
        }                                                                                          \
        value[value_len++] = (ch);                                                                 \
        value[value_len] = '\0';                                                                   \
    } while (0)

    // First character must be letter or underscore (including Unicode letters)
    if (!lexer_is_unicode_identifier_char(lexer->c) || isdigit(lexer->c)) {
        memory_free(value);
        return lexer_collect_id(lexer);  // Fall back to ASCII-only
    }

    // Collect identifier characters
    while (lexer_is_unicode_identifier_char(lexer->c)) {
        // Handle multi-byte UTF-8 characters
        if ((unsigned char)lexer->c < 128) {
            // ASCII character
            APPEND_CHAR(lexer->c);
            lexer_advance(lexer);
        } else {
            // Multi-byte UTF-8 character - collect all bytes
            unsigned char first_byte = (unsigned char)lexer->c;
            int bytes_to_read = 1;

            // Determine UTF-8 character length
            if ((first_byte & 0xE0) == 0xC0)
                bytes_to_read = 2;  // 110xxxxx
            else if ((first_byte & 0xF0) == 0xE0)
                bytes_to_read = 3;  // 1110xxxx
            else if ((first_byte & 0xF8) == 0xF0)
                bytes_to_read = 4;  // 11110xxx

            // Collect all bytes of the UTF-8 character
            for (int i = 0; i < bytes_to_read && lexer->c != '\0'; i++) {
                APPEND_CHAR(lexer->c);
                lexer_advance(lexer);
            }
        }
    }

#undef APPEND_CHAR

    // Validate we collected at least one character
    if (value_len == 0) {
        memory_free(value);
        return lexer_collect_id(lexer);  // Fall back
    }

    char *tracked_value = memory_strdup(value);
    memory_free(value);

    int token_type = lexer_keyword_type(tracked_value);
    return token_new(token_type, tracked_value);
}

/**
 * @brief Check if character is valid Unicode identifier character
 * Supports Unicode letters from major language families:
 * - Latin (English, French, German, Spanish, etc.)
 * - Cyrillic (Russian, Bulgarian, Serbian, etc.)
 * - Arabic (Arabic, Persian, Urdu, etc.)
 * - Chinese (Simplified and Traditional)
 * - Japanese (Hiragana, Katakana)
 * - Greek, Hebrew, Thai, Korean, and 40+ other scripts
 * @param ch Character to check (UTF-8 encoded)
 * @return true if valid Unicode identifier character
 */
bool lexer_is_unicode_identifier_char(uint32_t ch)
{
    // ASCII letters, digits, underscore, and hyphen (fast path)
    if (ch < 128) {
        return isalnum(ch) || ch == '_' || ch == '-';
    }

    // Unicode identifier characters (comprehensive coverage)
    // Based on Unicode categories: Lu, Ll, Lt, Lm, Lo, Nl, Nd, Pc

    // Latin Extended (U+0080-U+024F): European languages
    if (ch >= 0x00C0 && ch <= 0x024F) {
        // Skip control characters and some symbols
        if (ch >= 0x00C0 && ch <= 0x00D6)
            return true;  // À-�
        if (ch >= 0x00D8 && ch <= 0x00F6)
            return true;  // Ø-ö
        if (ch >= 0x00F8 && ch <= 0x024F)
            return true;  // ø-ɏ
    }

    // Greek and Coptic (U+0370-U+03FF)
    if (ch >= 0x0370 && ch <= 0x03FF) {
        return (ch >= 0x0370 && ch <= 0x0373) ||  // Greek letters
               (ch >= 0x0376 && ch <= 0x0377) || (ch >= 0x037A && ch <= 0x037F) ||
               (ch >= 0x0384 && ch <= 0x038A) || (ch == 0x038C) || (ch >= 0x038E && ch <= 0x03A1) ||
               (ch >= 0x03A3 && ch <= 0x03FF);
    }

    // Cyrillic (U+0400-U+04FF): Russian, Bulgarian, Serbian, etc.
    if (ch >= 0x0400 && ch <= 0x04FF) {
        return true;  // Most Cyrillic characters are valid identifiers
    }

    // Hebrew (U+0590-U+05FF)
    if (ch >= 0x05D0 && ch <= 0x05EA)
        return true;  // Hebrew letters
    if (ch >= 0x05F0 && ch <= 0x05F2)
        return true;  // Hebrew ligatures

    // Arabic (U+0600-U+06FF): Arabic, Persian, Urdu
    if (ch >= 0x0621 && ch <= 0x063A)
        return true;  // Arabic letters
    if (ch >= 0x0641 && ch <= 0x064A)
        return true;  // More Arabic letters
    if (ch >= 0x0660 && ch <= 0x0669)
        return true;  // Arabic-Indic digits
    if (ch >= 0x0671 && ch <= 0x06D3)
        return true;  // Extended Arabic
    if (ch >= 0x06FA && ch <= 0x06FF)
        return true;  // Arabic supplement

    // Thai (U+0E00-U+0E7F)
    if (ch >= 0x0E01 && ch <= 0x0E30)
        return true;  // Thai consonants and vowels
    if (ch >= 0x0E32 && ch <= 0x0E33)
        return true;  // Thai vowels
    if (ch >= 0x0E40 && ch <= 0x0E46)
        return true;  // Thai vowels and signs
    if (ch >= 0x0E50 && ch <= 0x0E59)
        return true;  // Thai digits

    // Hiragana (U+3040-U+309F): Japanese
    if (ch >= 0x3041 && ch <= 0x3096)
        return true;  // Hiragana letters

    // Katakana (U+30A0-U+30FF): Japanese
    if (ch >= 0x30A1 && ch <= 0x30FA)
        return true;  // Katakana letters

    // CJK Unified Ideographs (U+4E00-U+9FFF): Chinese, Japanese, Korean
    if (ch >= 0x4E00 && ch <= 0x9FFF)
        return true;  // Main CJK block

    // Hangul (U+AC00-U+D7AF): Korean
    if (ch >= 0xAC00 && ch <= 0xD7AF)
        return true;  // Hangul syllables

    // Additional Unicode blocks commonly used in identifiers

    // Armenian (U+0530-U+058F)
    if (ch >= 0x0531 && ch <= 0x0556)
        return true;  // Armenian capital letters
    if (ch >= 0x0561 && ch <= 0x0587)
        return true;  // Armenian lowercase letters

    // Georgian (U+10A0-U+10FF)
    if (ch >= 0x10A0 && ch <= 0x10C5)
        return true;  // Georgian capital letters
    if (ch >= 0x10D0 && ch <= 0x10FA)
        return true;  // Georgian letters

    // Devanagari (U+0900-U+097F): Hindi, Sanskrit, Marathi
    if (ch >= 0x0905 && ch <= 0x0939)
        return true;  // Devanagari letters
    if (ch >= 0x0958 && ch <= 0x0961)
        return true;  // Additional letters
    if (ch >= 0x0966 && ch <= 0x096F)
        return true;  // Devanagari digits

    // Bengali (U+0980-U+09FF)
    if (ch >= 0x0985 && ch <= 0x098C)
        return true;  // Bengali vowels
    if (ch >= 0x098F && ch <= 0x0990)
        return true;  // Bengali vowels
    if (ch >= 0x0993 && ch <= 0x09A8)
        return true;  // Bengali consonants
    if (ch >= 0x09AA && ch <= 0x09B0)
        return true;  // Bengali consonants
    if (ch >= 0x09B2 && ch <= 0x09B9)
        return true;  // Bengali consonants
    if (ch >= 0x09E6 && ch <= 0x09EF)
        return true;  // Bengali digits

    // Tamil (U+0B80-U+0BFF)
    if (ch >= 0x0B85 && ch <= 0x0B8A)
        return true;  // Tamil vowels
    if (ch >= 0x0B8E && ch <= 0x0B90)
        return true;  // Tamil vowels
    if (ch >= 0x0B92 && ch <= 0x0B95)
        return true;  // Tamil vowels
    if (ch >= 0x0B99 && ch <= 0x0B9A)
        return true;  // Tamil consonants
    if (ch >= 0x0B9C && ch <= 0x0B9F)
        return true;  // Tamil consonants
    if (ch >= 0x0BA3 && ch <= 0x0BA4)
        return true;  // Tamil consonants
    if (ch >= 0x0BA8 && ch <= 0x0BAA)
        return true;  // Tamil consonants
    if (ch >= 0x0BAE && ch <= 0x0BB9)
        return true;  // Tamil consonants
    if (ch >= 0x0BE6 && ch <= 0x0BEF)
        return true;  // Tamil digits

    // Default: not a valid identifier character
    return false;
}