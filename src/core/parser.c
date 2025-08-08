#include "zen/core/parser.h"

#include "zen/core/ast_memory_pool.h"
#include "zen/core/logger.h"
#include "zen/core/memory.h"
#include "zen/core/scope.h"
#include "zen/stdlib/stdlib.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations

/**
 * @brief Create parser instance
 * @param lexer Lexical analyzer instance
 * @return parser_T* New parser instance
 */
parser_T *parser_new(lexer_T *lexer)
{
    parser_T *parser = memory_alloc(sizeof(struct PARSER_STRUCT));
    if (!parser) {
        return NULL;
    }

    parser->lexer = lexer;
    parser->current_token = lexer_get_next_token(lexer);
    parser->prev_token = NULL;  // Initialize to NULL, not same as current_token
    parser->scope = scope_new();
    if (!parser->scope) {
        if (parser->current_token) {
            token_free(parser->current_token);
        }
        memory_free(parser);
        return NULL;
    }

    // AST memory pools are not currently in use - AST uses direct memory allocation
    // ast_pool_global_init();  // Disabled to prevent memory management conflicts

    return parser;
}

/**
 * @brief Free a parser instance and its resources
 * @param parser The parser instance to free
 */
void parser_free(parser_T *parser)
{
    if (!parser) {
        return;
    }

    if (parser->current_token) {
        token_free(parser->current_token);
    }

    if (parser->prev_token && parser->prev_token != parser->current_token) {
        token_free(parser->prev_token);
    }

    if (parser->scope) {
        scope_free(parser->scope);
    }

    // AST memory pools are not currently in use - cleanup not needed
    // ast_pool_global_cleanup();  // Disabled to prevent memory management conflicts

    memory_free(parser);
}

/**
 * @brief Consume expected token type
 * @param parser Parser instance
 * @param token_type Expected token type
 */
void parser_eat(parser_T *parser, int token_type)
{
    if ((int)parser->current_token->type == token_type) {
        if (parser->prev_token && parser->prev_token != parser->current_token) {
            token_free(parser->prev_token);
        }

        parser->prev_token = parser->current_token;
        parser->current_token = lexer_get_next_token(parser->lexer);
    } else {
        // Log parser error using proper logging system
        LOG_ERROR(LOG_CAT_PARSER,
                  "Unexpected token '%s', expected token type %d but got type %d",
                  parser->current_token->value ? parser->current_token->value : "(null)",
                  token_type,
                  (int)parser->current_token->type);

        // Set parser error state and enter panic mode for recovery
        parser->error_recovery.in_panic_mode = true;

        // Skip tokens until we find a synchronization point for recovery
        while (parser->current_token && parser->current_token->type != TOKEN_EOF) {
            // Synchronize on statement boundaries (newline, semicolon, braces)
            if (parser->current_token->type == TOKEN_NEWLINE ||
                parser->current_token->type == TOKEN_SEMICOLON ||
                parser->current_token->type == TOKEN_LBRACE ||
                parser->current_token->type == TOKEN_RBRACE ||
                parser->current_token->type == TOKEN_SET ||
                parser->current_token->type == TOKEN_FUNCTION) {
                break;
            }

            if (parser->prev_token && parser->prev_token != parser->current_token) {
                token_free(parser->prev_token);
            }
            parser->prev_token = parser->current_token;
            parser->current_token = lexer_get_next_token(parser->lexer);
        }

        // Exit panic mode after synchronization
        parser->error_recovery.in_panic_mode = false;
        parser->analytics.errors_recovered++;
    }
}

/**
 * @brief Parse input and return AST
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Root AST node
 */
AST_T *parser_parse(parser_T *parser, scope_T *scope)
{
    if (!parser || !scope || !parser->current_token) {
        return NULL;
    }

    AST_T *result = parser_parse_statements(parser, scope);

    // If parsing encountered errors, return an error AST node instead of crashing
    if (parser_has_errors(parser)) {
        LOG_WARN(LOG_CAT_PARSER,
                 "Parsing completed with %zu errors recovered. Some syntax may be incorrect.",
                 parser_get_error_count(parser));
    }

    return result;
}

/**
 * @brief Peek at token type at given offset
 * @param parser Parser instance
 * @param offset Token offset (0 = current, 1 = next, etc.)
 * @return int Token type or TOKEN_EOF if not available
 */
int parser_peek_token_type(parser_T *parser, int offset)
{
    if (!parser || !parser->lexer || offset < 0) {
        return TOKEN_EOF;
    }

    token_T *token = lexer_peek_token(parser->lexer, offset);
    if (!token) {
        return TOKEN_EOF;
    }

    int type = token->type;
    token_free(token);
    return type;
}

/**
 * @brief Parse a single statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Statement AST node
 */
AST_T *parser_parse_statement(parser_T *parser, scope_T *scope)
{
    // Parse statement based on current token

    switch (parser->current_token->type) {
    case TOKEN_SET:
        return parser_parse_variable_definition(parser, scope);
    case TOKEN_FUNCTION:
        return parser_parse_function_definition(parser, scope);
    case TOKEN_ID:
        // Check if this ID is followed by property access - if so, parse as expression
        if (parser_peek_token_type(parser, 1) == TOKEN_DOT ||
            parser_peek_token_type(parser, 1) == TOKEN_LBRACKET) {
            return parser_parse_expr(parser, scope);
        }
        return parser_parse_id(parser, scope);
    case TOKEN_IF:
        return parser_parse_if_statement(parser, scope);
    case TOKEN_WHILE:
        return parser_parse_while_loop(parser, scope);
    case TOKEN_FOR:
        return parser_parse_for_loop(parser, scope);
    case TOKEN_RETURN:
        return parser_parse_return_statement(parser, scope);
    case TOKEN_BREAK:
        return parser_parse_break_statement(parser, scope);
    case TOKEN_CONTINUE:
        return parser_parse_continue_statement(parser, scope);
    case TOKEN_GET:
        return parser_parse_file_get(parser, scope);
    case TOKEN_PUT:
        return parser_parse_file_put(parser, scope);
    case TOKEN_IMPORT:
        return parser_parse_import_statement(parser, scope);
    case TOKEN_EXPORT:
        return parser_parse_export_statement(parser, scope);
    case TOKEN_CLASS:
        return parser_parse_class_definition(parser, scope);
    default:
        // Handle as expression (variable assignment, function call, etc.)
        return parser_parse_expr(parser, scope);
    }
}

/**
 * @brief Parse multiple statements
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Compound AST node containing statements
 */
AST_T *parser_parse_statements(parser_T *parser, scope_T *scope)
{
    if (!parser) {
        return NULL;
    }
    if (!parser->current_token) {
        return NULL;
    }

    AST_T *compound = ast_new(AST_COMPOUND);
    compound->scope = scope;
    compound->compound_statements = NULL;
    compound->compound_size = 0;

    while (parser->current_token->type != TOKEN_EOF &&
           parser->current_token->type != TOKEN_DEDENT) {
        while (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);

            if (parser->current_token->type == TOKEN_EOF ||
                parser->current_token->type == TOKEN_DEDENT)
                break;
        }

        if (parser->current_token->type == TOKEN_EOF ||
            parser->current_token->type == TOKEN_DEDENT) {
            break;
        }

        AST_T *ast_statement = parser_parse_statement(parser, scope);

        if (ast_statement) {
            ast_statement->scope = scope;

            compound->compound_size++;
            compound->compound_statements =
                memory_realloc(compound->compound_statements,
                               compound->compound_size * sizeof(struct AST_STRUCT *));

            compound->compound_statements[compound->compound_size - 1] = ast_statement;
        }

        if (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
        }
    }

    return compound;
}

/**
 * @brief Parse expression with operator precedence
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Expression AST node
 */
AST_T *parser_parse_expr(parser_T *parser, scope_T *scope)
{
    // Parse comma expressions (lowest precedence)
    AST_T *left = parser_parse_ternary_expr(parser, scope);

    // Check for comma-separated expressions
    if (parser->current_token->type == TOKEN_COMMA) {
        // Determine if this is an object literal (identifiers) or array literal (values)
        bool is_object_literal = false;

        // Check if first element is a variable (identifier)
        if (left->type == AST_VARIABLE) {
            // Look ahead to see if all comma-separated elements are identifiers
            // This determines object literal vs array literal context
            is_object_literal = true;

            // Temporarily save parser state to look ahead
            token_T *saved_token = parser->current_token;
            unsigned int saved_i = parser->lexer->i;

            // Check next elements after comma
            while (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
                if (parser->current_token->type != TOKEN_ID) {
                    is_object_literal = false;
                    break;
                }
                parser_eat(parser, TOKEN_ID);
            }

            // Restore parser state
            parser->current_token = saved_token;
            parser->lexer->i = saved_i;
        }

        if (is_object_literal) {
            // Create object literal for identifier patterns like "name, age"
            AST_T *object = ast_new(AST_OBJECT);
            object->object_keys = memory_alloc(sizeof(char *));
            object->object_values = memory_alloc(sizeof(AST_T *));
            object->object_size = 1;

            // First key from left expression
            object->object_keys[0] = memory_strdup(left->variable_name);
            object->object_values[0] = ast_new_variable(left->variable_name);

            size_t obj_size = 1;

            while (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);

                if (parser->current_token->type == TOKEN_ID) {
                    obj_size++;
                    object->object_keys =
                        memory_realloc(object->object_keys, obj_size * sizeof(char *));
                    object->object_values =
                        memory_realloc(object->object_values, obj_size * sizeof(AST_T *));

                    char *key = memory_strdup(parser->current_token->value);
                    object->object_keys[obj_size - 1] = key;
                    object->object_values[obj_size - 1] = ast_new_variable(key);

                    parser_eat(parser, TOKEN_ID);
                    object->object_size = obj_size;
                } else {
                    break;  // Invalid comma expression
                }
            }

            return object;
        } else {
            // Create array literal for value patterns like "1, 2, 3"
            AST_T *array = ast_new(AST_ARRAY);
            array->array_elements = memory_alloc(sizeof(AST_T *));
            array->array_size = 1;

            // First element from left expression
            array->array_elements[0] = left;

            size_t arr_size = 1;

            while (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);

                // Parse next expression in array
                AST_T *element = parser_parse_ternary_expr(parser, scope);
                if (element) {
                    arr_size++;
                    array->array_elements =
                        memory_realloc(array->array_elements, arr_size * sizeof(AST_T *));
                    array->array_elements[arr_size - 1] = element;
                    array->array_size = arr_size;
                } else {
                    break;  // Invalid expression
                }
            }

            return array;
        }
    }

    return left;
}

/**
 * @brief Parse function call
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function call AST node
 */
AST_T *parser_parse_function_call(parser_T *parser, scope_T *scope)
{
    char *name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    AST_T *function_call = ast_new(AST_FUNCTION_CALL);
    function_call->function_call_name = name;
    function_call->function_call_arguments = NULL;
    function_call->function_call_arguments_size = 0;
    function_call->scope = scope;

    while (parser->current_token->type != TOKEN_NEWLINE &&
           parser->current_token->type != TOKEN_EOF &&
           parser->current_token->type != TOKEN_DEDENT &&
           parser->current_token->type != TOKEN_RPAREN &&
           parser->current_token->type != TOKEN_RBRACKET &&
           parser->current_token->type != TOKEN_COMMA) {
        AST_T *arg = parser_parse_expr(parser, scope);
        if (!arg)
            break;

        function_call->function_call_arguments_size++;
        function_call->function_call_arguments = memory_realloc(
            function_call->function_call_arguments,
            function_call->function_call_arguments_size * sizeof(struct AST_STRUCT *));
        function_call->function_call_arguments[function_call->function_call_arguments_size - 1] =
            arg;
    }

    return function_call;
}

/**
 * @brief Parse variable definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable definition AST node
 */
AST_T *parser_parse_variable_definition(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_SET);

    // CRITICAL FIX: Must duplicate token value BEFORE consuming it
    char *var_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    AST_T *value = parser_parse_expr(parser, scope);

    AST_T *var_def = ast_new_variable_definition(var_name, value);
    var_def->scope = scope;

    // Free the duplicated string since ast_new_variable_definition makes its own copy
    memory_free(var_name);

    scope_add_variable_definition(scope, var_def);

    return var_def;
}

/**
 * @brief Parse function definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function definition AST node
 */
AST_T *parser_parse_function_definition(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_FUNCTION);

    char *func_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    AST_T **args = NULL;
    size_t arg_count = 0;
    bool has_rest_param = false;

    while (parser->current_token->type == TOKEN_ID || parser->current_token->type == TOKEN_SPREAD) {
        arg_count++;
        args = memory_realloc(args, arg_count * sizeof(AST_T *));

        if (parser->current_token->type == TOKEN_SPREAD) {
            // Handle rest parameter: ...args
            if (has_rest_param) {
                // Error: multiple rest parameters not allowed
                LOG_ERROR(LOG_CAT_PARSER,
                          "Multiple rest parameters not allowed in function definition");
                break;
            }

            parser_eat(parser, TOKEN_SPREAD);

            if (parser->current_token->type != TOKEN_ID) {
                // Error: spread must be followed by identifier
                LOG_ERROR(LOG_CAT_PARSER, "Rest parameter must be followed by identifier");
                break;
            }

            char *rest_param_name = memory_strdup(parser->current_token->value);
            AST_T *rest_param_var = ast_new_variable(rest_param_name);
            args[arg_count - 1] = ast_new_spread(rest_param_var);
            memory_free(rest_param_name);
            parser_eat(parser, TOKEN_ID);
            has_rest_param = true;

            // Rest parameter must be the last parameter
            break;
        } else {
            // Handle regular parameter
            if (has_rest_param) {
                // Error: rest parameter must be last
                LOG_ERROR(LOG_CAT_PARSER, "Rest parameter must be the last parameter");
                break;
            }

            char *arg_name = memory_strdup(parser->current_token->value);
            args[arg_count - 1] = ast_new_variable(arg_name);
            memory_free(arg_name);
            parser_eat(parser, TOKEN_ID);
        }
    }

    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    AST_T *body = parser_parse_statements(parser, scope);

    if (parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    AST_T *func_def = ast_new_function_definition(func_name, args, arg_count, body);
    memory_free(func_name);
    if (args) {
        memory_free(args);
    }
    func_def->scope = scope;

    // Function will be added to scope during visitor execution
    // Don't add it here to avoid double-addition

    return func_def;
}

/**
 * @brief Parse variable reference
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable AST node
 */
AST_T *parser_parse_variable(parser_T *parser, scope_T *scope)
{
    char *var_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    AST_T *var = ast_new_variable(var_name);
    var->scope = scope;

    memory_free(var_name);  // Free the duplicated string since ast_new_variable makes its own copy
    return var;
}

/**
 * @brief Parse string literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* String literal AST node
 */
AST_T *parser_parse_string(parser_T *parser, scope_T *scope)
{
    const char *str_value = parser->current_token->value;
    parser_eat(parser, TOKEN_STRING);

    AST_T *str = ast_new_string(str_value);
    str->scope = scope;

    return str;
}

/**
 * @brief Parse identifier
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Identifier AST node
 */
AST_T *parser_parse_id(parser_T *parser, scope_T *scope)
{
    return parser_parse_id_or_object(parser, scope);
}

/**
 * @brief Parse binary expression with operator precedence
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @param min_precedence Minimum precedence level
 * @return AST_T* Binary expression AST node
 */
AST_T *parser_parse_binary_expr(parser_T *parser, scope_T *scope, int min_precedence)
{
    AST_T *left = parser_parse_unary_expr(parser, scope);

    while (parser_is_binary_operator(parser->current_token->type) &&
           parser_get_precedence(parser->current_token->type) >= min_precedence) {
        int op_type = parser->current_token->type;
        int precedence = parser_get_precedence(op_type);
        parser_eat(parser, op_type);

        AST_T *right = parser_parse_binary_expr(parser, scope, precedence + 1);
        left = ast_new_binary_op(op_type, left, right);
        left->scope = scope;
    }

    return left;
}

/**
 * @brief Parse unary expression
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Unary expression AST node
 */
AST_T *parser_parse_unary_expr(parser_T *parser, scope_T *scope)
{
    if (parser->current_token->type == TOKEN_NOT || parser->current_token->type == TOKEN_MINUS) {
        int op_type = parser->current_token->type;
        parser_eat(parser, op_type);
        AST_T *operand = parser_parse_unary_expr(parser, scope);
        AST_T *unary = ast_new_unary_op(op_type, operand);
        unary->scope = scope;
        return unary;
    }

    return parser_parse_primary_expr(parser, scope);
}

/**
 * @brief Parse primary expression (literals, variables, parentheses)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Primary expression AST node
 */
AST_T *parser_parse_primary_expr(parser_T *parser, scope_T *scope)
{
    AST_T *expr = NULL;

    switch (parser->current_token->type) {
    case TOKEN_STRING:
        expr = parser_parse_string(parser, scope);
        break;
    case TOKEN_NUMBER:
        expr = parser_parse_number(parser, scope);
        break;
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        expr = parser_parse_boolean(parser, scope);
        break;
    case TOKEN_NULL:
        expr = parser_parse_null(parser, scope);
        break;
    case TOKEN_UNDECIDABLE:
        expr = parser_parse_undecidable(parser, scope);
        break;
    case TOKEN_LBRACKET:
        expr = parser_parse_array(parser, scope);
        break;
    case TOKEN_LPAREN: {
        parser_eat(parser, TOKEN_LPAREN);
        expr = parser_parse_expr(parser, scope);
        parser_eat(parser, TOKEN_RPAREN);
        break;
    }
    case TOKEN_ID:
        expr = parser_parse_id_or_object(parser, scope);
        break;
    case TOKEN_NEW:
        expr = parser_parse_new_expression(parser, scope);
        break;
    default:
        expr = ast_new(AST_NOOP);
        break;
    }

    while (parser->current_token->type == TOKEN_LBRACKET ||
           parser->current_token->type == TOKEN_DOT) {
        if (parser->current_token->type == TOKEN_LBRACKET) {
            parser_eat(parser, TOKEN_LBRACKET);
            AST_T *index = parser_parse_expr(parser, scope);
            parser_eat(parser, TOKEN_RBRACKET);

            AST_T *array_access = ast_new(AST_PROPERTY_ACCESS);
            array_access->object = expr;
            array_access->property_name = memory_alloc(32);
            snprintf(array_access->property_name,
                     32,
                     "[%ld]",
                     (long)(index->type == AST_NUMBER ? index->number_value : 0));
            array_access->scope = scope;
            array_access->left = index;

            expr = array_access;

        } else if (parser->current_token->type == TOKEN_DOT) {
            parser_eat(parser, TOKEN_DOT);
            char *property = memory_strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);

            AST_T *prop_access = ast_new_property_access(expr, property);
            prop_access->scope = scope;
            expr = prop_access;
        }
    }

    return expr;
}

/**
 * @brief Parse identifier or object literal based on context
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Identifier or object literal AST node
 */
AST_T *parser_parse_id_or_object(parser_T *parser, scope_T *scope)
{
    // Check if this looks like an object literal (key-value pairs)
    if (parser_peek_for_object_literal(parser)) {
        return parser_parse_object(parser, scope);
    }

    char *original_name =
        memory_strdup(parser->current_token->value);  // Must duplicate before parser_eat

    if (parser->current_token->type == TOKEN_ID) {
        parser_eat(parser, TOKEN_ID);

        // Check if this is property access (should not be handled as function call)
        bool is_property_access = (parser->current_token->type == TOKEN_DOT ||
                                   parser->current_token->type == TOKEN_LBRACKET);

        // Check if this identifier has arguments (function call with args)
        bool has_args = (parser->current_token->type != TOKEN_NEWLINE &&
                         parser->current_token->type != TOKEN_EOF &&
                         parser->current_token->type != TOKEN_DEDENT &&
                         parser->current_token->type != TOKEN_RPAREN &&
                         parser->current_token->type != TOKEN_RBRACKET &&
                         parser->current_token->type != TOKEN_COMMA &&
                         parser->current_token->type != TOKEN_DOT &&
                         parser->current_token->type != TOKEN_LBRACKET &&
                         !parser_is_binary_operator(parser->current_token->type));

        // Check if this identifier is a stdlib function (for zero-arg calls)
        bool is_stdlib_function = (stdlib_get(original_name) != NULL);

        // For ZEN language: treat standalone identifiers as function calls by default
        // This allows zero-argument user-defined functions to work: "hello" -> hello()
        // The visitor will determine if it's actually a function or variable
        bool is_standalone = (parser->current_token->type == TOKEN_NEWLINE ||
                              parser->current_token->type == TOKEN_EOF ||
                              parser->current_token->type == TOKEN_DEDENT);

        // Don't treat as function call if it's property access - let primary expr handle it
        if (!is_property_access && (has_args || is_stdlib_function || is_standalone)) {
            AST_T *function_call = ast_new(AST_FUNCTION_CALL);
            function_call->function_call_name = original_name;  // Transfer ownership
            function_call->function_call_arguments = NULL;
            function_call->function_call_arguments_size = 0;
            function_call->scope = scope;

            // Only parse arguments if there are any
            if (has_args) {
                while (parser->current_token->type != TOKEN_NEWLINE &&
                       parser->current_token->type != TOKEN_EOF &&
                       parser->current_token->type != TOKEN_DEDENT &&
                       parser->current_token->type != TOKEN_RPAREN &&
                       parser->current_token->type != TOKEN_RBRACKET &&
                       parser->current_token->type != TOKEN_COMMA &&
                       !parser_is_binary_operator(parser->current_token->type)) {
                    AST_T *arg = parser_parse_expr(parser, scope);
                    if (!arg)
                        break;

                    function_call->function_call_arguments_size++;
                    function_call->function_call_arguments = memory_realloc(
                        function_call->function_call_arguments,
                        function_call->function_call_arguments_size * sizeof(struct AST_STRUCT *));
                    function_call
                        ->function_call_arguments[function_call->function_call_arguments_size - 1] =
                        arg;
                }
            }

            return function_call;
        }

        // Not a function call, treat as variable
        AST_T *var =
            ast_new_variable(original_name);  // Pass string, ast_new_variable will duplicate
        var->scope = scope;
        memory_free(original_name);  // Free our copy since ast_new_variable made its own
        return var;
    }

    memory_free(original_name);  // Clean up in error case
    return ast_new(AST_NOOP);
}

/**
 * @brief Parse number literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Number literal AST node
 */
AST_T *parser_parse_number(parser_T *parser, scope_T *scope)
{
    double num_value = atof(parser->current_token->value);
    parser_eat(parser, TOKEN_NUMBER);

    AST_T *num = ast_new_number(num_value);
    num->scope = scope;

    return num;
}

/**
 * @brief Parse boolean literal (true/false)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Boolean literal AST node
 */
AST_T *parser_parse_boolean(parser_T *parser, scope_T *scope)
{
    int bool_value = (parser->current_token->type == TOKEN_TRUE) ? 1 : 0;
    parser_eat(parser, parser->current_token->type);

    AST_T *boolean = ast_new_boolean(bool_value);
    boolean->scope = scope;

    return boolean;
}

/**
 * @brief Parse null literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Null literal AST node
 */
AST_T *parser_parse_null(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_NULL);

    AST_T *null_node = ast_new_null();
    null_node->scope = scope;

    return null_node;
}

/**
 * @brief Parse undecidable literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Undecidable literal AST node
 */
AST_T *parser_parse_undecidable(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_UNDECIDABLE);

    AST_T *undecidable_node = ast_new_undecidable();
    undecidable_node->scope = scope;

    return undecidable_node;
}

/**
 * @brief Parse array literal with comma-separated elements
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Array literal AST node
 */
AST_T *parser_parse_array(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_LBRACKET);

    AST_T **elements = NULL;
    size_t element_count = 0;

    while (parser->current_token->type != TOKEN_RBRACKET &&
           parser->current_token->type != TOKEN_EOF) {
        AST_T *element = parser_parse_expr(parser, scope);

        if (element) {
            element_count++;
            elements = memory_realloc(elements, element_count * sizeof(AST_T *));
            elements[element_count - 1] = element;
        }

        if (parser->current_token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
        } else {
            break;
        }
    }

    parser_eat(parser, TOKEN_RBRACKET);

    AST_T *array = ast_new_array(elements, element_count);
    array->scope = scope;

    return array;
}

/**
 * @brief Parse object literal with key-value pairs
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object literal AST node
 */
AST_T *parser_parse_object(parser_T *parser, scope_T *scope)
{
    char **keys = NULL;
    AST_T **values = NULL;
    size_t pair_count = 0;
    size_t max_pairs = 100;  // Safety limit to prevent infinite loops

    while (parser->current_token->type == TOKEN_ID && pair_count < max_pairs) {
        char *key = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        // Check if we have a value for this key
        if (parser->current_token->type == TOKEN_EOF ||
            parser->current_token->type == TOKEN_NEWLINE ||
            parser->current_token->type == TOKEN_DEDENT ||
            parser->current_token->type == TOKEN_COMMA) {
            // No value for this key OR comma followed by another key
            // For "name, age" case, treat both as keys without values
            AST_T *key_value = ast_new_variable(key);  // Use key as variable reference

            pair_count++;
            keys = memory_realloc(keys, pair_count * sizeof(char *));
            values = memory_realloc(values, pair_count * sizeof(AST_T *));

            keys[pair_count - 1] = key;  // Don't free key here, it's used in the object
            values[pair_count - 1] = key_value;

            // Handle comma continuation
            if (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
                // After comma, we must have another key or end
                if (parser->current_token->type != TOKEN_ID) {
                    break;  // No more keys after comma
                }
                continue;  // Continue to next key
            } else {
                break;  // End of object
            }
        }

        // Parse value without triggering object literal detection
        AST_T *value = NULL;
        switch (parser->current_token->type) {
        case TOKEN_STRING:
            value = parser_parse_string(parser, scope);
            break;
        case TOKEN_NUMBER:
            value = parser_parse_number(parser, scope);
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            value = parser_parse_boolean(parser, scope);
            break;
        case TOKEN_NULL:
            value = parser_parse_null(parser, scope);
            break;
        case TOKEN_UNDECIDABLE:
            value = parser_parse_undecidable(parser, scope);
            break;
        case TOKEN_LBRACKET:
            value = parser_parse_array(parser, scope);
            break;
        case TOKEN_ID:
            // For object values, treat ID as variable reference only
            value = parser_parse_variable(parser, scope);
            break;
        default:
            value = ast_new(AST_NOOP);
            break;
        }

        pair_count++;
        keys = memory_realloc(keys, pair_count * sizeof(char *));
        values = memory_realloc(values, pair_count * sizeof(AST_T *));

        keys[pair_count - 1] = key;
        values[pair_count - 1] = value;

        // Check for continuation
        if (parser->current_token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            // After comma, we must have another key-value pair or end
            if (parser->current_token->type != TOKEN_ID) {
                break;  // No more pairs after comma
            }
        } else {
            break;  // No comma means end of object
        }
    }

    AST_T *object = ast_new_object(keys, values, pair_count);
    object->scope = scope;

    // Clean up keys array (ast_new_object copies the data)
    if (keys) {
        memory_free(keys);
    }
    if (values) {
        memory_free(values);
    }

    return object;
}

/**
 * @brief Parse if/elif/else statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* If statement AST node
 */
AST_T *parser_parse_if_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_IF);

    AST_T *condition = parser_parse_expr(parser, scope);

    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    AST_T *then_branch = parser_parse_statements(parser, scope);

    if (parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    AST_T *else_branch = NULL;
    if (parser->current_token->type == TOKEN_ELSE) {
        parser_eat(parser, TOKEN_ELSE);

        if (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
        }

        if (parser->current_token->type == TOKEN_INDENT) {
            parser_eat(parser, TOKEN_INDENT);
        }

        else_branch = parser_parse_statements(parser, scope);

        if (parser->current_token->type == TOKEN_DEDENT) {
            parser_eat(parser, TOKEN_DEDENT);
        }
    }

    AST_T *if_stmt = ast_new_if_statement(condition, then_branch, else_branch);
    if_stmt->scope = scope;

    return if_stmt;
}

/**
 * @brief Parse while loop statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* While loop AST node
 */
AST_T *parser_parse_while_loop(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_WHILE);

    AST_T *condition = parser_parse_expr(parser, scope);

    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    AST_T *body = parser_parse_statements(parser, scope);

    if (parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    AST_T *while_loop = ast_new_while_loop(condition, body);
    while_loop->scope = scope;

    return while_loop;
}

/**
 * @brief Parse for loop statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* For loop AST node
 */
AST_T *parser_parse_for_loop(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_FOR);

    char *iterator = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    parser_eat(parser, TOKEN_IN);

    AST_T *iterable = parser_parse_expr(parser, scope);

    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    AST_T *body = parser_parse_statements(parser, scope);

    if (parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    AST_T *for_loop = ast_new_for_loop(iterator, iterable, body);
    for_loop->scope = scope;

    return for_loop;
}

/**
 * @brief Parse return statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Return statement AST node
 */
AST_T *parser_parse_return_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_RETURN);

    AST_T *value = NULL;
    if (parser->current_token->type != TOKEN_NEWLINE && parser->current_token->type != TOKEN_EOF) {
        value = parser_parse_expr(parser, scope);
    }

    AST_T *return_stmt = ast_new_return_statement(value);
    return_stmt->scope = scope;

    return return_stmt;
}

/**
 * @brief Parse break statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Break statement AST node
 */
AST_T *parser_parse_break_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_BREAK);

    AST_T *break_stmt = ast_new(AST_BREAK);
    break_stmt->scope = scope;

    return break_stmt;
}

/**
 * @brief Parse continue statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Continue statement AST node
 */
AST_T *parser_parse_continue_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_CONTINUE);

    AST_T *continue_stmt = ast_new(AST_CONTINUE);
    continue_stmt->scope = scope;

    return continue_stmt;
}

/**
 * @brief Get operator precedence for token type
 * @param token_type Token type to check
 * @return int Precedence level
 */
int parser_get_precedence(int token_type)
{
    switch (token_type) {
    case TOKEN_OR:
        return 1;
    case TOKEN_AND:
        return 2;
    case TOKEN_EQUALS:
    case TOKEN_NOT_EQUALS:
        return 3;
    case TOKEN_LESS_THAN:
    case TOKEN_GREATER_THAN:
    case TOKEN_LESS_EQUALS:
    case TOKEN_GREATER_EQUALS:
        return 4;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 5;
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
    case TOKEN_MODULO:
        return 6;
    default:
        return 0;
    }
}

/**
 * @brief Check if token type is a binary operator
 * @param token_type Token type to check (TOKEN_* constant)
 * @return int Returns 1 if token_type represents a binary operator, 0 otherwise
 * @note Recognizes arithmetic operators (+, -, *, /, %), comparison operators
 *       (=, !=, <, >, <=, >=), and logical operators (and, or)
 * @note Used by parser to determine operator precedence and expression parsing
 * @see parser_get_precedence() for operator precedence values
 * @see parser_parse_binary_expr() for usage in binary expression parsing
 */
int parser_is_binary_operator(int token_type)
{
    switch (token_type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
    case TOKEN_MODULO:
    case TOKEN_EQUALS:
    case TOKEN_NOT_EQUALS:
    case TOKEN_LESS_THAN:
    case TOKEN_GREATER_THAN:
    case TOKEN_LESS_EQUALS:
    case TOKEN_GREATER_EQUALS:
    case TOKEN_AND:
    case TOKEN_OR:
        return 1;
    default:
        return 0;
    }
}

/**
 * @brief Look ahead to determine if parsing an object literal
 * @param parser Parser instance
 * @return int 1 if object literal, 0 otherwise
 */
int parser_peek_for_object_literal(parser_T *parser)
{
    if (!parser || !parser->lexer) {
        return 0;
    }

    // Save current lexer state
    size_t saved_current = parser->lexer->i;
    size_t saved_line = parser->lexer->line_number;
    size_t saved_column = parser->lexer->column_number;

    // Look for pattern: ID followed by something that could be a value
    // In ZEN object literals look like: key value, key2 value2
    token_T *first_token = lexer_peek_token(parser->lexer, 0);

    if (!first_token || first_token->type != TOKEN_ID) {
        // Restore lexer state
        parser->lexer->i = saved_current;
        parser->lexer->line_number = saved_line;
        parser->lexer->column_number = saved_column;
        token_free(first_token);
        return 0;
    }

    // Look at second token (offset 1 from current position)
    token_T *second_token = lexer_peek_token(parser->lexer, 1);

    // Restore lexer state before analysis
    parser->lexer->i = saved_current;
    parser->lexer->line_number = saved_line;
    parser->lexer->column_number = saved_column;

    if (!second_token) {
        token_free(first_token);
        return 0;
    }

    // Check if second token could be a value in an object literal
    // Valid patterns: ID value, ID "string", ID number, ID true/false/null, ID array, ID ID
    // Also handle comma-separated keys: ID, ID (treated as object with key references)
    int is_object_literal = 0;
    switch (second_token->type) {
    case TOKEN_STRING:
    case TOKEN_NUMBER:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NULL:
    case TOKEN_UNDECIDABLE:
    case TOKEN_LBRACKET:  // Array as value
    case TOKEN_ID:        // Another identifier as value
        is_object_literal = 1;
        break;
    case TOKEN_COMMA:  // Comma-separated keys without values
        is_object_literal = 1;
        break;
    default:
        is_object_literal = 0;
        break;
    }

    token_free(first_token);
    token_free(second_token);
    return is_object_literal;
}

/**
 * @brief Strict look ahead for object literal detection
 * @param parser Parser instance
 * @return int 1 if strict object literal, 0 otherwise
 */
int parser_peek_for_object_literal_strict(parser_T *parser)
{
    if (!parser || !parser->lexer) {
        return 0;
    }

    // Save current lexer state
    size_t saved_current = parser->lexer->i;
    size_t saved_line = parser->lexer->line_number;
    size_t saved_column = parser->lexer->column_number;

    // Strict mode: Look for clear object literal patterns
    // Must see: ID value, ID value (comma-separated key-value pairs)
    token_T *tokens[5] = {NULL};  // Look ahead up to 5 tokens
    int token_count = 0;

    // Collect tokens for analysis
    for (int i = 0; i < 5; i++) {
        tokens[i] = lexer_peek_token(parser->lexer, i);
        if (!tokens[i] || tokens[i]->type == TOKEN_EOF) {
            break;
        }
        token_count++;
    }

    // Restore lexer state
    parser->lexer->i = saved_current;
    parser->lexer->line_number = saved_line;
    parser->lexer->column_number = saved_column;

    int is_strict_object_literal = 0;

    if (token_count >= 4) {
        // Pattern: ID value, ID value  (strict object literal)
        if (tokens[0]->type == TOKEN_ID &&
            (tokens[1]->type == TOKEN_STRING || tokens[1]->type == TOKEN_NUMBER ||
             tokens[1]->type == TOKEN_TRUE || tokens[1]->type == TOKEN_FALSE ||
             tokens[1]->type == TOKEN_NULL || tokens[1]->type == TOKEN_UNDECIDABLE ||
             tokens[1]->type == TOKEN_ID) &&
            tokens[2]->type == TOKEN_COMMA && tokens[3]->type == TOKEN_ID) {
            is_strict_object_literal = 1;
        }
    } else if (token_count >= 2) {
        // Simple case: ID followed by a clear value (not followed by parentheses or operators)
        if (tokens[0]->type == TOKEN_ID &&
            (tokens[1]->type == TOKEN_STRING || tokens[1]->type == TOKEN_NUMBER ||
             tokens[1]->type == TOKEN_TRUE || tokens[1]->type == TOKEN_FALSE ||
             tokens[1]->type == TOKEN_NULL || tokens[1]->type == TOKEN_UNDECIDABLE ||
             tokens[1]->type == TOKEN_LBRACKET)) {
            // Check if there's a third token that would indicate it's not a function call
            if (token_count < 3 ||
                (tokens[2]->type != TOKEN_LPAREN && !parser_is_binary_operator(tokens[2]->type))) {
                is_strict_object_literal = 1;
            }
        }
    }

    // Clean up tokens
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]) {
            token_free(tokens[i]);
        }
    }

    return is_strict_object_literal;
}

/**
 * @brief Parse ternary conditional expressions
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Ternary expression AST node
 */
AST_T *parser_parse_ternary_expr(parser_T *parser, scope_T *scope)
{
    return parser_parse_null_coalescing_expr(parser, scope);
}

/**
 * @brief Parse null coalescing expressions
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Null coalescing expression AST node
 */
AST_T *parser_parse_null_coalescing_expr(parser_T *parser, scope_T *scope)
{
    return parser_parse_binary_expr(parser, scope, 0);
}

/**
 * @brief Parse ternary conditional expressions (alternative implementation)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Ternary expression AST node
 */
AST_T *parser_parse_ternary(parser_T *parser, scope_T *scope)
{
    // Parse the base expression (condition)
    AST_T *condition = parser_parse_null_coalescing_expr(parser, scope);
    if (!condition) {
        return NULL;
    }

    // Check for ternary operator (?)
    if (parser->current_token->type == TOKEN_QUESTION) {
        parser_eat(parser, TOKEN_QUESTION);

        // Parse the true expression
        AST_T *true_expr = parser_parse_expr(parser, scope);
        if (!true_expr) {
            ast_free(condition);
            return NULL;
        }

        // Expect colon (:)
        if (parser->current_token->type != TOKEN_COLON) {
            ast_free(condition);
            ast_free(true_expr);
            return NULL;
        }
        parser_eat(parser, TOKEN_COLON);

        // Parse the false expression
        AST_T *false_expr = parser_parse_expr(parser, scope);
        if (!false_expr) {
            ast_free(condition);
            ast_free(true_expr);
            return NULL;
        }

        // Create ternary AST node
        AST_T *ternary = ast_new(AST_TERNARY);
        ternary->ternary_condition = condition;
        ternary->ternary_true_expr = true_expr;
        ternary->ternary_false_expr = false_expr;
        ternary->scope = scope;

        return ternary;
    }

    // No ternary operator found, return the condition as-is
    return condition;
}

/**
 * @brief Parse compound assignment operators (+=, -=, etc.)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Compound assignment AST node
 */
AST_T *parser_parse_compound_assignment(parser_T *parser, scope_T *scope)
{
    // Parse the target (left-hand side)
    AST_T *target = parser_parse_primary_expr(parser, scope);
    if (!target) {
        return NULL;
    }

    // Check for compound assignment operators
    int compound_op = 0;
    switch (parser->current_token->type) {
    case TOKEN_PLUS_EQUALS:
        compound_op = TOKEN_PLUS;
        break;
    case TOKEN_MINUS_EQUALS:
        compound_op = TOKEN_MINUS;
        break;
    case TOKEN_MULTIPLY_EQUALS:
        compound_op = TOKEN_MULTIPLY;
        break;
    case TOKEN_DIVIDE_EQUALS:
        compound_op = TOKEN_DIVIDE;
        break;
    case TOKEN_MODULO_EQUALS:
        compound_op = TOKEN_MODULO;
        break;
    default:
        // Not a compound assignment, return target as regular expression
        return target;
    }

    // Consume the compound assignment operator
    parser_eat(parser, parser->current_token->type);

    // Parse the value (right-hand side)
    AST_T *value = parser_parse_expr(parser, scope);
    if (!value) {
        ast_free(target);
        return NULL;
    }

    // Create compound assignment AST node
    AST_T *compound_assignment = ast_new(AST_COMPOUND_ASSIGNMENT);
    compound_assignment->compound_op_type = compound_op;
    compound_assignment->compound_target = target;
    compound_assignment->compound_value = value;
    compound_assignment->scope = scope;

    return compound_assignment;
}

/**
 * @brief Parse file get operation
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* File get AST node
 */
AST_T *parser_parse_file_get(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_GET);

    // Parse file path expression (can be string literal or variable)
    AST_T *file_path = parser_parse_expr(parser, scope);

    // Parse property path (dot notation)
    AST_T *property_path = NULL;
    if (parser->current_token->type == TOKEN_DOT) {
        parser_eat(parser, TOKEN_DOT);
        property_path = parser_parse_property_access_chain(parser, scope);
    }

    AST_T *file_get = ast_new_file_get(file_path, property_path);
    file_get->scope = scope;

    return file_get;
}

/**
 * @brief Parse file put operation
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* File put AST node
 */
/**
 * @brief Parse file reference with @ prefix
 * @param parser Parser instance
 * @param ref_string Reference string (e.g., "@ ../addresses.json office.alice")
 * @return AST_T* File reference node or NULL if parsing fails
 */
AST_T *parser_parse_file_reference(parser_T *parser, const char *ref_string)
{
    (void)parser;  // Mark parameter as intentionally unused

    if (!ref_string || strlen(ref_string) < 3) {
        return NULL;  // Minimum: "@ a"
    }

    // Skip "@ " prefix (2 characters)
    const char *content = ref_string + 2;

    // Find the space that separates file path from property path
    char *space_pos = strchr(content, ' ');
    if (!space_pos) {
        // No property path - just file reference
        return ast_new_file_reference(content, NULL);
    }

    // Split into file path and property path
    size_t file_path_len = space_pos - content;
    char *target_file = memory_alloc(file_path_len + 1);
    strncpy(target_file, content, file_path_len);
    target_file[file_path_len] = '\0';

    const char *property_path = space_pos + 1;  // Skip the space

    AST_T *ref_node = ast_new_file_reference(target_file, property_path);
    memory_free(target_file);

    return ref_node;
}

/**
 * @brief Parse a 'put' statement for file manipulation
 * @param parser Parser instance
 * @param scope Current parsing scope
 * @return AST node representing the file put operation
 */
AST_T *parser_parse_file_put(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_PUT);

    // Parse file path expression
    AST_T *file_path = parser_parse_expr(parser, scope);

    // Parse property path (dot notation)
    AST_T *property_path = NULL;
    if (parser->current_token->type == TOKEN_DOT) {
        parser_eat(parser, TOKEN_DOT);
        property_path = parser_parse_property_access_chain(parser, scope);
    }

    // Parse value to put - check for @ prefix for file references or object syntax
    AST_T *value = NULL;

    if (parser->current_token->type == TOKEN_STRING && parser->current_token->value &&
        strncmp(parser->current_token->value, "@ ", 2) == 0) {
        // This is a file reference with @ prefix
        value = parser_parse_file_reference(parser, parser->current_token->value);
        parser_eat(parser, TOKEN_STRING);  // Consume the reference string

        if (!value) {
            LOG_ERROR(LOG_CAT_PARSER, "Invalid file reference format");
            value = ast_new(AST_NULL);
        }
    } else if (parser->current_token->type == TOKEN_ID) {
        // Check if this is object syntax (key-value pairs)
        // Look ahead to see if we have "key value" or "key value, key value" pattern
        value = parser_parse_id_or_object(parser, scope);
    } else {
        // Regular value (string, number, etc.)
        value = parser_parse_expr(parser, scope);
    }

    AST_T *file_put = ast_new_file_put(file_path, property_path, value);
    file_put->scope = scope;

    return file_put;
}

/**
 * @brief Parse property access chain for dot notation (helper function)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Property access chain AST node
 */
AST_T *parser_parse_property_access_chain(parser_T *parser, scope_T *scope)
{
    char *property_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    AST_T *property_node = ast_new_string(property_name);
    memory_free(property_name);

    // Handle nested property access (e.g., a.b.c)
    if (parser->current_token->type == TOKEN_DOT) {
        parser_eat(parser, TOKEN_DOT);
        AST_T *nested = parser_parse_property_access_chain(parser, scope);

        // Create a compound property path
        AST_T *compound = ast_new(AST_COMPOUND);
        compound->compound_size = 2;
        compound->compound_statements = memory_alloc(2 * sizeof(AST_T *));
        compound->compound_statements[0] = property_node;
        compound->compound_statements[1] = nested;
        compound->scope = scope;

        return compound;
    }

    property_node->scope = scope;
    return property_node;
}

/**
 * @brief Check if parser has encountered errors
 * @param parser Parser instance to check
 * @return true if parser has errors, false otherwise
 */
bool parser_has_errors(parser_T *parser)
{
    return parser && parser->analytics.errors_recovered > 0;
}

/**
 * @brief Check if parser is currently in panic mode (error recovery)
 * @param parser Parser instance to check
 * @return true if in panic mode, false otherwise
 */
bool parser_in_panic_mode(parser_T *parser)
{
    return parser && parser->error_recovery.in_panic_mode;
}

/**
 * @brief Get count of errors recovered during parsing
 * @param parser Parser instance to check
 * @return Number of errors recovered, 0 if no errors or invalid parser
 */
size_t parser_get_error_count(parser_T *parser)
{
    return parser ? parser->analytics.errors_recovered : 0;
}

/**
 * @brief Parse import statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Import statement AST node
 */
AST_T *parser_parse_import_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_IMPORT);

    AST_T *import_node = ast_new(AST_IMPORT);
    import_node->scope = scope;

    // Parse import path (string literal)
    if (parser->current_token->type == TOKEN_STRING) {
        import_node->import_path = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_STRING);

        // Simple import: import "module.zen"
        import_node->import_names = NULL;
        import_node->import_names_size = 0;

        return import_node;
    }

    // Check for immediate "from" (malformed import from "file")
    if (parser->current_token->type == TOKEN_FROM) {
        // Skip malformed case, return empty import
        import_node->import_path = memory_strdup("");  // Empty path indicates error
        import_node->import_names = NULL;
        import_node->import_names_size = 0;
        return import_node;
    }

    // Parse named imports: import propA newA, propB newB from "config.json"
    if (parser->current_token->type == TOKEN_ID) {
        size_t names_capacity = 4;
        import_node->import_names = memory_alloc(sizeof(char *) * names_capacity);
        import_node->import_names_size = 0;

        do {
            // Expand array if needed
            if (import_node->import_names_size >= names_capacity) {
                names_capacity *= 2;
                import_node->import_names =
                    memory_realloc(import_node->import_names, sizeof(char *) * names_capacity);
            }

            // Parse name (and optional alias)
            char *import_name = memory_strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);

            // Check for alias: propA newA
            if (parser->current_token->type == TOKEN_ID) {
                char *alias = memory_strdup(parser->current_token->value);
                parser_eat(parser, TOKEN_ID);

                // Store as "original:alias"
                size_t full_len = strlen(import_name) + strlen(alias) + 2;
                char *full_name = memory_alloc(full_len);
                snprintf(full_name, full_len, "%s:%s", import_name, alias);

                memory_free(import_name);
                memory_free(alias);
                import_node->import_names[import_node->import_names_size++] = full_name;
            } else {
                import_node->import_names[import_node->import_names_size++] = import_name;
            }

            // Continue if comma
            if (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
            } else {
                break;
            }
        } while (parser->current_token->type == TOKEN_ID);

        // Expect "from"
        if (parser->current_token->type == TOKEN_FROM) {
            parser_eat(parser, TOKEN_FROM);

            // Parse module path
            if (parser->current_token->type == TOKEN_STRING) {
                import_node->import_path = memory_strdup(parser->current_token->value);
                parser_eat(parser, TOKEN_STRING);
            } else {
                // Error: expected string after "from"
                import_node->import_path = memory_strdup("");  // Empty path indicates error
            }
        } else {
            // Error: expected "from"
            import_node->import_path = memory_strdup("");  // Empty path indicates error
        }

        return import_node;
    }

    // Error: unexpected token
    import_node->import_path = memory_strdup("");  // Empty path indicates error
    import_node->import_names = NULL;
    import_node->import_names_size = 0;

    return import_node;
}

/**
 * @brief Parse export statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Export statement AST node
 */
AST_T *parser_parse_export_statement(parser_T *parser, scope_T *scope)
{
    parser_eat(parser, TOKEN_EXPORT);

    AST_T *export_node = ast_new(AST_EXPORT);
    export_node->scope = scope;

    // Handle different export forms
    if (parser->current_token->type == TOKEN_FUNCTION) {
        // export function name args...
        AST_T *func_def = parser_parse_function_definition(parser, scope);
        if (func_def && func_def->function_definition_name) {
            export_node->export_name = memory_strdup(func_def->function_definition_name);
            export_node->export_value = func_def;
        } else {
            export_node->export_name = memory_strdup("");  // Empty name indicates error
            export_node->export_value = NULL;
        }

        return export_node;
    } else if (parser->current_token->type == TOKEN_SET) {
        // export set name value
        AST_T *var_def = parser_parse_variable_definition(parser, scope);
        if (var_def && var_def->variable_definition_variable_name) {
            export_node->export_name = memory_strdup(var_def->variable_definition_variable_name);
            export_node->export_value = var_def;
        } else {
            export_node->export_name = memory_strdup("");  // Empty name indicates error
            export_node->export_value = NULL;
        }

        return export_node;
    } else if (parser->current_token->type == TOKEN_ID) {
        // export variable_name [alias]
        char *var_name = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        // Check for alias
        if (parser->current_token->type == TOKEN_ID) {
            // Export with alias: export a A
            export_node->export_name = memory_strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);
        } else {
            // Simple export: export a
            export_node->export_name = memory_strdup(var_name);
        }

        // Create a variable reference AST node
        AST_T *var_ref = ast_new_variable(var_name);
        var_ref->scope = scope;
        export_node->export_value = var_ref;

        memory_free(var_name);
        return export_node;
    }

    // Error: unexpected token after export
    export_node->export_name = memory_strdup("");  // Empty name indicates error
    export_node->export_value = NULL;

    return export_node;
}

/**
 * @brief Parse a class definition
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Class definition AST node
 */
AST_T *parser_parse_class_definition(parser_T *parser, scope_T *scope)
{
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Consume 'class' token
    parser_eat(parser, TOKEN_CLASS);
    if (!parser->current_token || parser->current_token->type != TOKEN_ID) {
        // Error: expected class name
        return NULL;
    }

    char *class_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    // Check for optional inheritance (extends keyword in some languages, but ZEN might be
    // different)
    char *parent_class = NULL;
    // For now, we'll skip inheritance parsing and just handle basic classes

    // Parse methods - expect newline and indented method definitions
    AST_T **methods = NULL;
    size_t methods_count = 0;
    size_t methods_capacity = 4;  // Start with capacity for 4 methods

    methods = memory_alloc(methods_capacity * sizeof(AST_T *));
    if (!methods) {
        memory_free(class_name);
        return NULL;
    }

    // Skip newlines and handle indentation
    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }
    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    // Parse method definitions - handle both 'method' keyword (as ID) and 'function' keyword
    while (parser->current_token && parser->current_token->type != TOKEN_EOF &&
           parser->current_token->type != TOKEN_DEDENT &&
           ((parser->current_token->type == TOKEN_ID &&
             strcmp(parser->current_token->value, "method") == 0) ||
            parser->current_token->type == TOKEN_FUNCTION)) {
        AST_T *method = parser_parse_class_method(parser, scope);
        if (method) {
            // Expand array if needed
            if (methods_count >= methods_capacity) {
                methods_capacity *= 2;
                AST_T **new_methods = memory_realloc(methods, methods_capacity * sizeof(AST_T *));
                if (!new_methods) {
                    // Cleanup on error
                    for (size_t i = 0; i < methods_count; i++) {
                        ast_free(methods[i]);
                    }
                    memory_free(methods);
                    memory_free(class_name);
                    return NULL;
                }
                methods = new_methods;
            }

            methods[methods_count++] = method;
        }

        // Skip newlines between methods
        while (parser->current_token && (parser->current_token->type == TOKEN_NEWLINE ||
                                         parser->current_token->type == TOKEN_DEDENT)) {
            if (parser->current_token->type == TOKEN_NEWLINE) {
                parser_eat(parser, TOKEN_NEWLINE);
            } else {
                parser_eat(parser, TOKEN_DEDENT);
                break;  // Exit after dedent - end of class body
            }
        }

        // Break if we reach EOF or end of class
        if (!parser->current_token || parser->current_token->type == TOKEN_EOF ||
            parser->current_token->type == TOKEN_DEDENT) {
            break;
        }
    }

    // Consume the final DEDENT token that marks the end of the class body
    if (parser->current_token && parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    // Create class definition AST node
    AST_T *class_def = ast_new_class_definition(class_name, parent_class, methods, methods_count);

    // Cleanup
    memory_free(class_name);
    if (parent_class) {
        memory_free(parent_class);
    }
    memory_free(methods);  // ast_new_class_definition makes its own copy

    return class_def;
}

/**
 * @brief Parse a class method definition
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Function definition AST node for the method
 */
AST_T *parser_parse_class_method(parser_T *parser, scope_T *scope)
{
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Handle both 'method methodName' and 'function methodName' syntax
    if (parser->current_token->type == TOKEN_ID &&
        strcmp(parser->current_token->value, "method") == 0) {
        // Consume 'method' token
        parser_eat(parser, TOKEN_ID);
    } else if (parser->current_token->type == TOKEN_FUNCTION) {
        // Consume 'function' token
        parser_eat(parser, TOKEN_FUNCTION);
    } else {
        // Error: expected 'method' or 'function'
        return NULL;
    }

    // Get method name
    if (!parser->current_token || parser->current_token->type != TOKEN_ID) {
        // Error: expected method name
        return NULL;
    }

    char *method_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    // Parse parameters - methods can have parameters like functions
    AST_T **args = NULL;
    size_t arg_count = 0;
    size_t arg_capacity = 2;

    args = memory_alloc(arg_capacity * sizeof(AST_T *));
    if (!args) {
        memory_free(method_name);
        return NULL;
    }

    // Parse parameters until newline
    while (parser->current_token && parser->current_token->type != TOKEN_NEWLINE &&
           parser->current_token->type != TOKEN_EOF) {
        if (parser->current_token->type == TOKEN_ID) {
            // Parameter name
            AST_T *param = ast_new_variable(parser->current_token->value);

            // Expand args array if needed
            if (arg_count >= arg_capacity) {
                arg_capacity *= 2;
                AST_T **new_args = memory_realloc(args, arg_capacity * sizeof(AST_T *));
                if (!new_args) {
                    // Cleanup on error
                    for (size_t i = 0; i < arg_count; i++) {
                        ast_free(args[i]);
                    }
                    memory_free(args);
                    memory_free(method_name);
                    return NULL;
                }
                args = new_args;
            }

            args[arg_count++] = param;
            parser_eat(parser, TOKEN_ID);
        } else {
            // Skip other tokens (commas, etc.) - advance safely
            parser_eat(parser, parser->current_token->type);
        }
    }

    // Skip newline
    if (parser->current_token && parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    // Handle method body indentation
    if (parser->current_token && parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    // Parse method body (indented block)
    AST_T *body = parser_parse_statements(parser, scope);

    // Handle end of method body
    if (parser->current_token && parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    // Create function definition AST node for the method
    AST_T *method_def = ast_new_function_definition(method_name, args, arg_count, body);

    // Cleanup
    memory_free(method_name);
    memory_free(args);  // ast_new_function_definition makes its own copy

    return method_def;
}

/**
 * @brief Parse new expression (class instantiation)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* New expression AST node or NULL on error
 */
AST_T *parser_parse_new_expression(parser_T *parser, scope_T *scope)
{
    if (!parser || !scope || parser->current_token->type != TOKEN_NEW) {
        return NULL;
    }

    // Consume "new" token
    parser_eat(parser, TOKEN_NEW);

    // Expect class name
    if (parser->current_token->type != TOKEN_ID) {
        return ast_new(AST_NULL);  // Invalid syntax
    }

    char *class_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    // Create new expression AST node
    AST_T *new_expr = ast_new(AST_NEW_EXPRESSION);
    if (!new_expr) {
        memory_free(class_name);
        return NULL;
    }

    new_expr->new_class_name = class_name;
    new_expr->scope = scope;
    new_expr->new_arguments = NULL;
    new_expr->new_arguments_size = 0;

    // Check if there are constructor arguments
    // In ZEN syntax, arguments follow without parentheses: "new Animal arg1 arg2"
    if (parser->current_token->type != TOKEN_NEWLINE && parser->current_token->type != TOKEN_EOF &&
        parser->current_token->type != TOKEN_DEDENT && parser->current_token->type != TOKEN_COMMA &&
        !parser_is_binary_operator(parser->current_token->type)) {
        // Parse arguments like function calls
        size_t arg_capacity = 4;
        AST_T **args = memory_alloc(arg_capacity * sizeof(AST_T *));
        if (!args) {
            ast_free(new_expr);
            return NULL;
        }

        size_t arg_count = 0;
        while (parser->current_token->type != TOKEN_NEWLINE &&
               parser->current_token->type != TOKEN_EOF &&
               parser->current_token->type != TOKEN_DEDENT &&
               parser->current_token->type != TOKEN_COMMA &&
               !parser_is_binary_operator(parser->current_token->type)) {
            AST_T *arg = parser_parse_expr(parser, scope);
            if (!arg)
                break;

            if (arg_count >= arg_capacity) {
                arg_capacity *= 2;
                AST_T **new_args = memory_realloc(args, arg_capacity * sizeof(AST_T *));
                if (!new_args) {
                    // Cleanup on error
                    for (size_t i = 0; i < arg_count; i++) {
                        ast_free(args[i]);
                    }
                    memory_free(args);
                    ast_free(new_expr);
                    return NULL;
                }
                args = new_args;
            }

            args[arg_count++] = arg;

            // Optional comma between arguments
            if (parser->current_token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
            }
        }

        new_expr->new_arguments = args;
        new_expr->new_arguments_size = arg_count;
    }

    return new_expr;
}
