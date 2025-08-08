#ifndef ZEN_CORE_TOKEN_H
#define ZEN_CORE_TOKEN_H
typedef struct TOKEN_STRUCT {
    enum {
        // Basic tokens
        TOKEN_EOF,
        TOKEN_NEWLINE,
        TOKEN_INDENT,
        TOKEN_DEDENT,
        TOKEN_ID,
        TOKEN_NUMBER,
        TOKEN_STRING,

        // Literals
        TOKEN_TRUE,
        TOKEN_FALSE,
        TOKEN_NULL,
        TOKEN_UNDECIDABLE,

        // Keywords
        TOKEN_SET,
        TOKEN_FUNCTION,
        TOKEN_RETURN,
        TOKEN_IF,
        TOKEN_ELIF,
        TOKEN_ELSE,
        TOKEN_THEN,
        TOKEN_WHILE,
        TOKEN_FOR,
        TOKEN_IN,
        TOKEN_BREAK,
        TOKEN_CONTINUE,
        TOKEN_CLASS,
        TOKEN_NEW,
        TOKEN_EXTENDS,
        TOKEN_IMPORT,
        TOKEN_EXPORT,
        TOKEN_FROM,
        TOKEN_AS,
        TOKEN_TRY,
        TOKEN_CATCH,
        TOKEN_THROW,
        TOKEN_GET,
        TOKEN_PUT,

        // Phase 2: Natural Language Keywords
        TOKEN_WHEN,        // Alternative to `if` for more natural expression
        TOKEN_UNLESS,      // Negative conditional (if not)
        TOKEN_WHENEVER,    // Event-driven conditional
        TOKEN_UNTIL,       // Loop continuation condition
        TOKEN_DURING,      // Time/context-based condition
        TOKEN_THROUGHOUT,  // Continuous condition
        TOKEN_OTHERWISE,   // Alternative to `else`

        // Operators
        TOKEN_EQUALS,
        TOKEN_NOT_EQUALS,
        TOKEN_LESS_THAN,
        TOKEN_GREATER_THAN,
        TOKEN_LESS_EQUALS,
        TOKEN_GREATER_EQUALS,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_MULTIPLY,
        TOKEN_DIVIDE,
        TOKEN_MODULO,
        TOKEN_AND,
        TOKEN_OR,
        TOKEN_NOT,

        // Compound assignment operators
        TOKEN_PLUS_EQUALS,
        TOKEN_MINUS_EQUALS,
        TOKEN_MULTIPLY_EQUALS,
        TOKEN_DIVIDE_EQUALS,
        TOKEN_MODULO_EQUALS,

        // Advanced operators
        TOKEN_RANGE,            // ..
        TOKEN_SPREAD,           // ...
        TOKEN_NULL_COALESCING,  // ??
        TOKEN_OPTIONAL_CHAIN,   // ?.

        // Punctuation
        TOKEN_COMMA,
        TOKEN_DOT,
        TOKEN_COLON,
        TOKEN_QUESTION,
        TOKEN_LPAREN,
        TOKEN_RPAREN,
        TOKEN_LBRACKET,
        TOKEN_RBRACKET,
        TOKEN_LBRACE,
        TOKEN_RBRACE,
        TOKEN_SEMICOLON,
        TOKEN_ARROW,        // =>
        TOKEN_DOUBLE_ARROW  // =>
    } type;

    char *value;
} token_T;

/**
 * @brief Create new token
 * @param type The token type
 * @param value The token value string
 * @return Pointer to newly allocated token
 */
token_T *token_new(int type, char *value);

/**
 * @brief Free a token and its value
 * @param token Token to free
 */
void token_free(token_T *token);
#endif  // ZEN_CORE_TOKEN_H
