#ifndef PARSER_H
#define PARSER_H
#include "zen/core/ast.h"
#include "zen/core/lexer.h"
#include "zen/core/scope.h"

#include <stdbool.h>
#include <time.h>

typedef struct PARSER_STRUCT {
    lexer_T *lexer;
    token_T *current_token;
    token_T *prev_token;
    scope_T *scope;
    bool owns_scope;  // True if parser owns the scope and should free it

    // Parsing context flags
    struct {
        bool in_variable_assignment;
        bool in_method_body;
        bool in_function_call;
    } context;

    // Performance optimization cache
    struct {
        struct AST_STRUCT **cached_expressions;
        size_t cache_size;
        size_t cache_capacity;
        bool enabled;
    } memoization;

    // Error recovery state
    struct {
        bool in_panic_mode;
        int synchronize_points[16];  // Token types to synchronize on
        size_t sync_point_count;
        struct AST_STRUCT **partial_results;
        size_t partial_count;
    } error_recovery;

    // Analytics and profiling
    struct {
        size_t expressions_parsed;
        size_t cache_hits;
        size_t cache_misses;
        size_t errors_recovered;
        clock_t parse_start_time;
        clock_t total_parse_time;
    } analytics;
} parser_T;

/* Parser analytics structure */
struct parser_analytics {
    size_t expressions_parsed;
    size_t cache_hits;
    size_t cache_misses;
    size_t errors_recovered;
    double cache_hit_rate;
    double parse_time_ms;
    size_t memory_used;
};

/* MANIFEST.json required functions */
/**
 * @brief Create parser instance
 * @param lexer Lexical analyzer instance
 * @return parser_T* New parser instance
 */
parser_T *parser_new(lexer_T *lexer);

/**
 * @brief Free a parser instance and its resources
 * @param parser The parser instance to free
 */
void parser_free(parser_T *parser);

/**
 * @brief Consume expected token type
 * @param parser Parser instance
 * @param token_type Expected token type
 */
void parser_eat(parser_T *parser, int token_type);

/**
 * @brief Parse input and return AST
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Root AST node
 */
AST_T *parser_parse(parser_T *parser, scope_T *scope);

/**
 * @brief Parse a single statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Statement AST node
 */
AST_T *parser_parse_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse multiple statements
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Compound AST node containing statements
 */
AST_T *parser_parse_statements(parser_T *parser, scope_T *scope);

/**
 * @brief Parse expression
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Expression AST node
 */
AST_T *parser_parse_expr(parser_T *parser, scope_T *scope);

/**
 * @brief Parse function call
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function call AST node
 */
AST_T *parser_parse_function_call(parser_T *parser, scope_T *scope);

/**
 * @brief Parse variable definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable definition AST node
 */
AST_T *parser_parse_variable_definition(parser_T *parser, scope_T *scope);

/**
 * @brief Parse function definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function definition AST node
 */
AST_T *parser_parse_function_definition(parser_T *parser, scope_T *scope);

/**
 * @brief Parse variable reference
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable AST node
 */
AST_T *parser_parse_variable(parser_T *parser, scope_T *scope);

/**
 * @brief Parse string literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* String AST node
 */
AST_T *parser_parse_string(parser_T *parser, scope_T *scope);

/**
 * @brief Parse identifier
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Appropriate AST node based on identifier type
 */
AST_T *parser_parse_id(parser_T *parser, scope_T *scope);

/* Extended parsing functions for complete ZEN language support */

/**
 * @brief Parse binary expression with precedence climbing
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @param min_precedence Minimum precedence level
 * @return AST_T* Binary expression AST node
 */
AST_T *parser_parse_binary_expr(parser_T *parser, scope_T *scope, int min_precedence);

/**
 * @brief Parse ternary conditional expression (a ? b : c)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Expression AST node
 */
AST_T *parser_parse_ternary_expr(parser_T *parser, scope_T *scope);

/**
 * @brief Parse null coalescing expression (a ?? b)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Expression AST node
 */
AST_T *parser_parse_null_coalescing_expr(parser_T *parser, scope_T *scope);

/**
 * @brief Parse unary expression
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Unary expression AST node
 */
AST_T *parser_parse_unary_expr(parser_T *parser, scope_T *scope);

/**
 * @brief Parse primary expression (literals, variables, parentheses)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Primary expression AST node
 */
AST_T *parser_parse_primary_expr(parser_T *parser, scope_T *scope);

/**
 * @brief Parse identifier or object literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Appropriate AST node based on identifier type
 */
AST_T *parser_parse_id_or_object(parser_T *parser, scope_T *scope);

/* Literal parsing functions */

/**
 * @brief Parse number literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Number AST node
 */
AST_T *parser_parse_number(parser_T *parser, scope_T *scope);

/**
 * @brief Parse boolean literal (true/false)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Boolean AST node
 */
AST_T *parser_parse_boolean(parser_T *parser, scope_T *scope);

/**
 * @brief Parse null literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Null AST node
 */
AST_T *parser_parse_null(parser_T *parser, scope_T *scope);

/**
 * @brief Parse undecidable literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Undecidable AST node
 */
AST_T *parser_parse_undecidable(parser_T *parser, scope_T *scope);

/**
 * @brief Parse array literal with comma-separated elements
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Array AST node
 */
AST_T *parser_parse_array(parser_T *parser, scope_T *scope);

/**
 * @brief Parse object literal with key-value pairs
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object AST node
 */
AST_T *parser_parse_object(parser_T *parser, scope_T *scope);

/**
 * @brief Parse ZEN object literal creating AST_OBJECT nodes instead of AST_FUNCTION_CALL
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object literal AST node
 */
AST_T *parser_parse_object_literal(parser_T *parser, scope_T *scope);

/* Control flow parsing functions */

/**
 * @brief Parse if/elif/else statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* If statement AST node
 */
AST_T *parser_parse_if_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse while loop statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* While loop AST node
 */
AST_T *parser_parse_while_loop(parser_T *parser, scope_T *scope);

/**
 * @brief Parse for loop statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* For loop AST node
 */
AST_T *parser_parse_for_loop(parser_T *parser, scope_T *scope);

/**
 * @brief Parse return statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Return statement AST node
 */
AST_T *parser_parse_return_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse break statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Break statement AST node
 */
AST_T *parser_parse_break_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse continue statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Continue statement AST node
 */
AST_T *parser_parse_continue_statement(parser_T *parser, scope_T *scope);

/* Utility functions */

/**
 * @brief Get operator precedence for token type
 * @param token_type Token type of operator
 * @return int Precedence level (higher = tighter binding)
 */
int parser_get_precedence(int token_type);

/**
 * @brief Check if token type is a binary operator
 * @param token_type Token type to check
 * @return int 1 if binary operator, 0 otherwise
 */
int parser_is_binary_operator(int token_type);

/**
 * @brief Enhanced detection algorithm for object literals in ZEN syntax
 * @param parser Parser instance
 * @return int 1 if object literal detected, 0 otherwise
 */
int parser_detect_object_literal(parser_T *parser);

/**
 * @brief Look ahead to determine if parsing an object literal
 * @param parser Parser instance
 * @return int 1 if object literal detected, 0 otherwise
 */
int parser_peek_for_object_literal(parser_T *parser);

/**
 * @brief Strict look ahead for object literal detection
 * @param parser Parser instance
 * @return int 1 if object literal definitely detected, 0 otherwise
 */
int parser_peek_for_object_literal_strict(parser_T *parser);

/* Advanced parsing features */

/**
 * @brief Parse ternary conditional expression
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Ternary expression AST node
 */
AST_T *parser_parse_ternary(parser_T *parser, scope_T *scope);

/**
 * @brief Parse compound assignment (+=, -=, etc.)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Compound assignment AST node
 */
AST_T *parser_parse_compound_assignment(parser_T *parser, scope_T *scope);

/* Database-like file operations */

/**
 * @brief Parse file get operation
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* File get AST node
 */
AST_T *parser_parse_file_get(parser_T *parser, scope_T *scope);

/**
 * @brief Parse file put operation
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* File put AST node
 */
AST_T *parser_parse_file_put(parser_T *parser, scope_T *scope);

/**
 * @brief Parse property access chain for dot notation (helper function)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Property access chain AST node
 */
AST_T *parser_parse_property_access_chain(parser_T *parser, scope_T *scope);

/* Parser error handling and recovery functions */

/**
 * @brief Check if parser has encountered errors
 * @param parser Parser instance to check
 * @return true if parser has errors, false otherwise
 */
bool parser_has_errors(parser_T *parser);

/**
 * @brief Check if parser is currently in panic mode (error recovery)
 * @param parser Parser instance to check
 * @return true if in panic mode, false otherwise
 */
bool parser_in_panic_mode(parser_T *parser);

/**
 * @brief Get count of errors recovered during parsing
 * @param parser Parser instance to check
 * @return Number of errors recovered, 0 if no errors or invalid parser
 */
size_t parser_get_error_count(parser_T *parser);

/**
 * @brief Parse file reference string with @ prefix for cross-file references
 * @param parser Parser instance (can be NULL for utility parsing)
 * @param ref_string Reference string (e.g., "@ ../addresses.json office.alice")
 * @return AST_T* File reference node or NULL if parsing fails
 */
AST_T *parser_parse_file_reference(parser_T *parser, const char *ref_string);

/**
 * @brief Parse import statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Import statement AST node
 */
AST_T *parser_parse_import_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse export statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Export statement AST node
 */
AST_T *parser_parse_export_statement(parser_T *parser, scope_T *scope);

/**
 * @brief Parse a class definition
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Class definition AST node
 */
AST_T *parser_parse_class_definition(parser_T *parser, scope_T *scope);

/**
 * @brief Parse a class method definition
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Function definition AST node for the method
 */
AST_T *parser_parse_class_method(parser_T *parser, scope_T *scope);

/**
 * @brief Parse new expression (class instantiation)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* New expression AST node or NULL on error
 */
AST_T *parser_parse_new_expression(parser_T *parser, scope_T *scope);

#endif