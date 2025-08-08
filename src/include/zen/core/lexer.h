#ifndef ZEN_CORE_LEXER_H
#define ZEN_CORE_LEXER_H
#include "zen/core/token.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Token lookahead buffer for performance optimization
 */
typedef struct {
    token_T *token;  /**< Buffered token */
    size_t position; /**< Position in source when token was created */
    bool valid;      /**< Whether this buffer entry is valid */
} TokenBuffer;

/**
 * @brief Lexical complexity metrics
 */
typedef struct {
    size_t total_tokens;     /**< Total tokens processed */
    size_t token_density;    /**< Average tokens per line */
    size_t max_nesting;      /**< Maximum indentation nesting */
    size_t string_literals;  /**< Number of string literals */
    size_t numeric_literals; /**< Number of numeric literals */
    size_t identifiers;      /**< Number of identifiers */
    size_t keywords;         /**< Number of keywords */
    size_t operators;        /**< Number of operators */
} LexerMetrics;

/**
 * @brief Error location information
 */
typedef struct {
    size_t line;     /**< Line number (1-based) */
    size_t column;   /**< Column number (1-based) */
    size_t position; /**< Absolute position in source */
    char *context;   /**< Surrounding context for error */
} SourceLocation;

/**
 * @brief Lexer configuration for advanced features
 */
typedef struct {
    bool enable_lookahead;      /**< Enable token lookahead buffering */
    bool enable_metrics;        /**< Enable complexity metrics tracking */
    bool enable_error_recovery; /**< Enable lexical error recovery */
    bool preserve_whitespace;   /**< Preserve whitespace tokens for formatting */
    bool unicode_identifiers;   /**< Allow Unicode in identifiers */
    size_t buffer_size;         /**< Token buffer size (0 = disabled) */
} LexerConfig;

/**
 * @brief Lexer structure for tokenizing ZEN language source code
 *
 * The lexer handles indentation-based block structure, string literals,
 * identifiers, and various operators. It maintains an indentation stack
 * to emit appropriate INDENT/DEDENT tokens for ZEN's Python-like syntax.
 */
typedef struct LEXER_STRUCT {
    // Core lexer state
    char c;                /**< Current character being processed */
    unsigned int i;        /**< Current position in input string */
    char *contents;        /**< Input source code string */
    size_t content_length; /**< Length of content string (cached) */
    int current_indent;    /**< Current indentation level (spaces) */
    int *indent_stack;     /**< Stack of indentation levels */
    int indent_stack_size; /**< Size of indentation stack */
    int at_line_start;     /**< Flag indicating if at start of line */

    // Type inference context tracking (Phase 2)
    int last_token_type;  /**< Type of the last token for context-aware parsing */
    bool in_type_context; /**< Flag indicating if we're in a type declaration context */

    // Source location tracking
    size_t line_number;   /**< Current line number (1-based) */
    size_t column_number; /**< Current column number (1-based) */
    size_t *line_starts;  /**< Array of line start positions */
    size_t line_count;    /**< Total number of lines */
    size_t line_capacity; /**< Allocated capacity for line_starts */

    // Performance optimizations
    TokenBuffer *token_buffer; /**< Lookahead token buffer */
    size_t buffer_size;        /**< Size of token buffer */
    size_t buffer_head;        /**< Head pointer in circular buffer */
    size_t buffer_tail;        /**< Tail pointer in circular buffer */
    size_t buffer_count;       /**< Number of tokens in buffer */

    // Metrics and analytics
    LexerMetrics metrics; /**< Complexity metrics */
    bool metrics_enabled; /**< Whether to collect metrics */

    // Error handling
    bool error_recovery_mode;  /**< Whether in error recovery mode */
    SourceLocation last_error; /**< Last error location */
    char *error_message;       /**< Current error message */

    // Configuration
    LexerConfig config; /**< Lexer configuration */
} lexer_T;

/**
 * @brief Create a new lexer instance
 * @param contents Input source code string to tokenize
 * @return Pointer to newly allocated lexer_T structure
 * @note The caller is responsible for freeing the returned lexer
 * @note The contents string should remain valid for the lifetime of the lexer
 */
lexer_T *lexer_new(char *contents);

/**
 * @brief Free a lexer instance and its resources
 * @param lexer The lexer instance to free
 */
void lexer_free(lexer_T *lexer);

/**
 * @brief Advance lexer position
 * @param lexer Pointer to lexer instance
 * @note Advances the current position and updates current character
 * @note Handles newline detection and resets line start flag
 */
void lexer_advance(lexer_T *lexer);

/**
 * @brief Skip whitespace and track indentation
 * @param lexer Pointer to lexer instance
 * @note Only skips spaces, not newlines or tabs
 * @note Tracks indentation level when at start of line
 * @note Sets at_line_start flag to false after processing
 */
void lexer_skip_whitespace(lexer_T *lexer);

/**
 * @brief Get next token
 * @param lexer Pointer to lexer instance
 * @return Pointer to next token_T structure
 * @note Handles indentation changes by emitting INDENT/DEDENT tokens
 * @note Returns TOKEN_EOF when end of input is reached
 * @note The caller is responsible for freeing the returned token
 */
token_T *lexer_get_next_token(lexer_T *lexer);

/**
 * @brief Collect string token
 * @param lexer Pointer to lexer instance
 * @return Pointer to TOKEN_STRING token containing the string value
 * @note Expects lexer to be positioned at opening quote
 * @note Advances past closing quote
 * @note Does not handle escape sequences (limitation)
 * @note The caller is responsible for freeing the returned token
 */
token_T *lexer_collect_string(lexer_T *lexer);

/**
 * @brief Collect number token (integer or float)
 * @param lexer Pointer to lexer instance
 * @return Pointer to TOKEN_NUMBER token containing the numeric value
 * @note Supports integers, floats, and scientific notation (e.g., 1.23e-4)
 * @note Handles edge cases like "42.foo" (stops at dot if not followed by digit)
 * @note The caller is responsible for freeing the returned token
 */
token_T *lexer_collect_number(lexer_T *lexer);

/**
 * @brief Collect identifier token with keyword recognition
 * @param lexer Pointer to lexer instance
 * @return Pointer to TOKEN_ID or keyword token containing the identifier
 * @note Collects alphanumeric characters and underscores
 * @note Recognizes keywords and returns appropriate token types
 * @note The caller is responsible for freeing the returned token
 */
token_T *lexer_collect_id(lexer_T *lexer);

/**
 * @brief Check if identifier is a keyword and return appropriate token type
 * @param value The identifier string to check
 * @return Token type (keyword token or TOKEN_ID for regular identifiers)
 * @note Recognizes all ZEN language keywords including literals, control flow, OOP, modules, etc.
 * @note Used internally by lexer_collect_id to distinguish keywords from identifiers
 */
int lexer_keyword_type(const char *value);

/**
 * @brief Process identifier with type inference context awareness (Phase 2)
 * @param lexer The lexer instance
 * @param identifier The identifier string
 * @return Token with appropriate type context information
 * @note Provides intelligent type inference when identifier follows "as"
 * @note Maintains backward compatibility with existing "as" usage
 */
token_T *lexer_process_type_context_id(lexer_T *lexer, const char *identifier);

/**
 * @brief Advance and return token
 * @param lexer Pointer to lexer instance
 * @param token Token to return after advancing
 * @return The same token passed as parameter
 * @note Convenience function that advances lexer position then returns token
 * @note Used for single-character tokens like operators
 */
token_T *lexer_advance_with_token(lexer_T *lexer, token_T *token);

/**
 * @brief Get current character as string
 * @param lexer Pointer to lexer instance
 * @return Newly allocated string containing current character
 * @note Returns a null-terminated string with single character
 * @note The caller is responsible for freeing the returned string
 * @note Used internally by string and identifier collection functions
 */
char *lexer_get_current_char_as_string(lexer_T *lexer);

/* ========== ADVANCED LEXER FEATURES (PERFORMANCE & ENHANCEMENTS) ========== */

/**
 * @brief Initialize lexer with advanced configuration
 * @param contents Input source code string to tokenize
 * @param config Configuration for advanced features
 * @return Pointer to newly allocated lexer_T structure with enhanced features
 * @note Enables performance optimizations and metrics collection
 */
lexer_T *lexer_create_enhanced(char *contents, const LexerConfig *config);

/**
 * @brief Enable token lookahead buffering for performance
 * @param lexer Pointer to lexer instance
 * @param buffer_size Size of lookahead buffer (0 to disable)
 * @return true if buffering was successfully enabled, false otherwise
 * @note Improves performance for parsers that need lookahead
 */
bool lexer_enable_buffering(lexer_T *lexer, size_t buffer_size);

/**
 * @brief Peek at next token without consuming it
 * @param lexer Pointer to lexer instance
 * @param offset Number of tokens to look ahead (0 = next token)
 * @return Pointer to lookahead token or NULL if not available
 * @note Requires buffering to be enabled
 * @note Does not advance lexer position
 */
token_T *lexer_peek_token(lexer_T *lexer, size_t offset);

/**
 * @brief Get current source location information
 * @param lexer Pointer to lexer instance
 * @return Source location structure with line/column info
 * @note Provides precise error location reporting
 */
SourceLocation lexer_get_location(const lexer_T *lexer);

/**
 * @brief Get lexical complexity metrics
 * @param lexer Pointer to lexer instance
 * @return Metrics structure with analysis data
 * @note Only available when metrics are enabled
 */
LexerMetrics lexer_get_metrics(const lexer_T *lexer);

/**
 * @brief Enable or disable metrics collection
 * @param lexer Pointer to lexer instance
 * @param enabled Whether to collect metrics
 * @note Slight performance overhead when enabled
 */
void lexer_set_metrics(lexer_T *lexer, bool enabled);

/**
 * @brief Reset metrics to initial state
 * @param lexer Pointer to lexer instance
 */
void lexer_reset_metrics(lexer_T *lexer);

/**
 * @brief Enter error recovery mode
 * @param lexer Pointer to lexer instance
 * @param error_message Description of the error
 * @note Allows lexer to continue after lexical errors
 */
void lexer_enter_error_recovery(lexer_T *lexer, const char *error_message);

/**
 * @brief Exit error recovery mode
 * @param lexer Pointer to lexer instance
 * @note Resumes normal tokenization
 */
void lexer_exit_error_recovery(lexer_T *lexer);

/**
 * @brief Check if lexer is in error recovery mode
 * @param lexer Pointer to lexer instance
 * @return true if in error recovery mode, false otherwise
 */
bool lexer_in_error_recovery(const lexer_T *lexer);

/**
 * @brief Collect enhanced string with interpolation support
 * @param lexer Pointer to lexer instance
 * @return Pointer to string token with interpolation metadata
 * @note Supports ${expression} interpolation syntax
 * @note Handles nested braces and escape sequences
 */
token_T *lexer_collect_interpolated_string(lexer_T *lexer);

/**
 * @brief Collect raw string literal
 * @param lexer Pointer to lexer instance
 * @return Pointer to raw string token
 * @note Supports r"raw string" syntax with no escape processing
 */
token_T *lexer_collect_raw_string(lexer_T *lexer);

/**
 * @brief Collect enhanced number with extended formats
 * @param lexer Pointer to lexer instance
 * @return Pointer to number token with enhanced format support
 * @note Supports binary (0b), hexadecimal (0x), and scientific notation
 * @note Handles underscores in numeric literals for readability
 */
token_T *lexer_collect_enhanced_number(lexer_T *lexer);

/**
 * @brief Collect Unicode identifier
 * @param lexer Pointer to lexer instance
 * @return Pointer to identifier token supporting Unicode characters
 * @note Supports full Unicode identifier specification
 * @note Only enabled when unicode_identifiers config is true
 */
token_T *lexer_collect_unicode_id(lexer_T *lexer);

/**
 * @brief Create formatting-preserving token cache
 * @param lexer Pointer to lexer instance
 * @return Array of tokens with whitespace preservation
 * @note Useful for code formatters and syntax highlighters
 */
token_T **lexer_tokenize_with_formatting(lexer_T *lexer, size_t *token_count);

/**
 * @brief Get context around current position for error reporting
 * @param lexer Pointer to lexer instance
 * @param context_size Number of characters before/after current position
 * @return Newly allocated string with context (caller must free)
 * @note Provides rich error context for better debugging
 */
char *lexer_get_error_context(const lexer_T *lexer, size_t context_size);

/**
 * @brief Suggest corrections for lexical errors
 * @param lexer Pointer to lexer instance
 * @param error_char Character that caused the error
 * @return Newly allocated string with suggestions (caller must free)
 * @note Provides helpful suggestions for common lexical mistakes
 */
char *lexer_suggest_correction(const lexer_T *lexer, char error_char);

/**
 * @brief Optimize lexer for streaming large files
 * @param lexer Pointer to lexer instance
 * @param chunk_size Size of chunks to process at a time
 * @return true if streaming mode was enabled successfully
 * @note Reduces memory usage for very large files
 */
bool lexer_enable_streaming(lexer_T *lexer, size_t chunk_size);

/**
 * @brief Check if character is valid for Unicode identifiers
 * @param ch Character to check
 * @return true if character can be part of a Unicode identifier
 * @note Implements Unicode identifier specification
 */
bool lexer_is_unicode_identifier_char(uint32_t ch);

/**
 * @brief Get token frequency analysis
 * @param lexer Pointer to lexer instance
 * @return Array of token type frequencies (indexed by token type)
 * @note Useful for language analysis and optimization
 */
size_t *lexer_get_token_frequencies(const lexer_T *lexer);

#endif  // ZEN_CORE_LEXER_H
