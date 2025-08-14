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
AST_T *parser_parse_try_catch(parser_T *parser, scope_T *scope);
AST_T *parser_parse_throw(parser_T *parser, scope_T *scope);
AST_T *parser_parse_import_statement(parser_T *parser, scope_T *scope);
AST_T *parser_parse_export_statement(parser_T *parser, scope_T *scope);
AST_T *parser_parse_indented_object(parser_T *parser, scope_T *scope);
AST_T *parser_parse_class_definition(parser_T *parser, scope_T *scope);
AST_T *parser_parse_new_expression(parser_T *parser, scope_T *scope);
AST_T *parser_parse_nested_object_value(parser_T *parser, scope_T *scope);

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

    // Initialize context flags
    parser->context.in_variable_assignment = false;
    parser->context.in_method_body = false;
    parser->context.in_function_call = false;
    parser->context.in_print_statement = false;

    // Initialize recursion depth for proper DEDENT handling
    parser->recursion_depth = 0;

    parser->scope = scope_new();
    if (!parser->scope) {
        if (parser->current_token) {
            token_free(parser->current_token);
        }
        memory_free(parser);
        return NULL;
    }

    // Initialize AST memory pools for better performance
    ast_pool_global_init();

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

    // Cleanup AST memory pools
    ast_pool_global_cleanup();

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

    // Reset recursion tracking for new parse context
    // This is critical for import statements which create new parse contexts
    parser->recursion_depth = 0;

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
        // Always parse identifiers as expressions
        // This allows proper handling of property access, function calls, etc.
        return parser_parse_expr(parser, scope);
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
    case TOKEN_TRY:
        return parser_parse_try_catch(parser, scope);
    case TOKEN_THROW:
        return parser_parse_throw(parser, scope);
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

    parser->recursion_depth++;

    while (parser->current_token->type != TOKEN_EOF) {
        // Handle DEDENT tokens
        if (parser->current_token->type == TOKEN_DEDENT) {
            if (parser->recursion_depth > 1) {
                // Inside a nested block - DEDENT ends this block
                break;
            } else {
                // At top level - skip DEDENT tokens
                // They can appear due to how the lexer generates tokens when exiting indented
                // blocks
                parser_eat(parser, TOKEN_DEDENT);
                continue;
            }
        }
        while (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);

            if (parser->current_token->type == TOKEN_EOF ||
                parser->current_token->type == TOKEN_DEDENT)
                break;
        }

        if (parser->current_token->type == TOKEN_EOF) {
            break;
        }
        if (parser->current_token->type == TOKEN_DEDENT) {
            if (parser->recursion_depth > 1) {
                break;
            } else {
                // At top level - skip DEDENT
                parser_eat(parser, TOKEN_DEDENT);
                continue;
            }
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

    parser->recursion_depth--;
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

    // Check for comma-separated expressions (array literals)
    // But not if we're parsing function arguments in parentheses
    if (parser->current_token->type == TOKEN_COMMA && !parser->context.in_parentheses) {
        // Create array literal for comma-separated expressions
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

    // Set context flag for parsing function arguments
    parser->context.in_function_call = true;

    // Parse function arguments - continue while we have valid argument tokens
    bool parsing_args = true;
    while (parsing_args && parser->current_token->type != TOKEN_NEWLINE &&
           parser->current_token->type != TOKEN_EOF &&
           parser->current_token->type != TOKEN_DEDENT &&
           !parser_is_binary_operator(parser->current_token->type)) {
        LOG_PARSER_DEBUG(
            "Parsing function call argument %zu for '%s', current token type: %d, value: '%s'",
            function_call->function_call_arguments_size + 1,
            function_call->function_call_name,
            parser->current_token->type,
            parser->current_token->value ? parser->current_token->value : "null");

        // Check if current token can start an expression
        bool can_parse_expr = false;
        switch (parser->current_token->type) {
        case TOKEN_ID:
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NULL:
        case TOKEN_LBRACKET:
        case TOKEN_LPAREN:
        case TOKEN_NOT:
        case TOKEN_MINUS:
        case TOKEN_NEW:
            can_parse_expr = true;
            break;
        default:
            can_parse_expr = false;
            break;
        }

        if (!can_parse_expr) {
            LOG_PARSER_DEBUG("Token cannot start expression, stopping argument parsing");
            break;
        }

        // For stdlib functions, parse full expressions
        // For user functions, stop at binary operators for correct precedence
        AST_T *arg = NULL;
        if (stdlib_get(function_call->function_call_name)) {
            // Stdlib function: parse full expression (e.g., "print 5 + 3")
            LOG_PARSER_DEBUG("Parsing stdlib function argument as full expression");
            arg = parser_parse_ternary_expr(parser, scope);
            if (arg) {
                LOG_PARSER_DEBUG("Parsed stdlib arg with type %d", arg->type);
            }
        } else {
            // User function: stop at binary operators
            // This ensures "f 2 + f 3" is parsed as "(f 2) + (f 3)"
            arg = parser_parse_unary_expr(parser, scope);
        }
        if (!arg) {
            LOG_PARSER_DEBUG("No argument parsed, breaking");
            break;
        }

        function_call->function_call_arguments_size++;
        function_call->function_call_arguments = memory_realloc(
            function_call->function_call_arguments,
            function_call->function_call_arguments_size * sizeof(struct AST_STRUCT *));
        function_call->function_call_arguments[function_call->function_call_arguments_size - 1] =
            arg;

        LOG_PARSER_DEBUG("Added argument %zu, next token type: %d, value: %s",
                         function_call->function_call_arguments_size,
                         parser->current_token->type,
                         parser->current_token->value ? parser->current_token->value : "null");

        // Check for comma separator for next argument
        if (parser->current_token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            // After comma, we must continue parsing next argument
            continue;
        }

        // Check if there's another argument (space-separated)
        // If the next token could start another expression, continue
        // Otherwise stop parsing arguments
        switch (parser->current_token->type) {
        case TOKEN_ID:
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NULL:
        case TOKEN_LBRACKET:
        case TOKEN_LPAREN:
        case TOKEN_NOT:
        case TOKEN_MINUS:
        case TOKEN_NEW:
            // Continue parsing next space-separated argument
            continue;
        default:
            // No more arguments
            parsing_args = false;
            break;
        }
    }

    // Clear context flag
    parser->context.in_function_call = false;

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
    // Ensure parser->scope is set for object literal detection
    scope_T *saved_scope = parser->scope;
    parser->scope = scope;

    parser_eat(parser, TOKEN_SET);

    // Check if this is a property assignment (e.g., set obj.prop value)
    // We need to peek ahead to see if there's a dot after the identifier
    // Note: peek offset 0 gets the next token from lexer (after current_token)
    if (parser->current_token->type == TOKEN_ID && parser_peek_token_type(parser, 0) == TOKEN_DOT) {
        // This is a property assignment, parse it manually
        // Current token is the object name (e.g., "obj")
        char *obj_name = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        // Now we should have the dot
        parser_eat(parser, TOKEN_DOT);

        // Now we should have the property name
        if (parser->current_token->type != TOKEN_ID) {
            LOG_ERROR(LOG_CAT_PARSER, "Expected property name after dot");
            memory_free(obj_name);
            parser->scope = saved_scope;
            return ast_new_noop();
        }

        char *prop_name = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        // Create the property access node for the left side
        AST_T *obj_var = ast_new_variable(obj_name);
        obj_var->scope = scope;
        AST_T *prop_access = ast_new_property_access(obj_var, prop_name);
        prop_access->scope = scope;

        memory_free(obj_name);
        memory_free(prop_name);

        // Parse the value to assign
        parser->context.in_variable_assignment = true;
        AST_T *value = parser_parse_expr(parser, scope);
        parser->context.in_variable_assignment = false;

        // Create an assignment node
        AST_T *assignment = ast_new_assignment(prop_access, value);
        assignment->scope = scope;

        // Restore original parser scope
        parser->scope = saved_scope;
        return assignment;
    }

    // Regular variable definition
    // CRITICAL FIX: Must duplicate token value BEFORE consuming it
    char *var_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    // Set variable assignment context
    parser->context.in_variable_assignment = true;

    // Check if this is an object literal before parsing as a general expression
    AST_T *value = NULL;

    // Handle multiline expressions: if we encounter a newline followed by indented content,
    // we need to parse the indented content as the value
    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
        if (parser->current_token->type == TOKEN_INDENT) {
            parser_eat(parser, TOKEN_INDENT);
            // Parse the indented content as an object literal
            value = parser_parse_indented_object(parser, scope);
            // Consume the DEDENT if present
            if (parser->current_token->type == TOKEN_DEDENT) {
                parser_eat(parser, TOKEN_DEDENT);
            }
        } else {
            // No indented content after newline, this is an error or empty assignment
            value = NULL;
        }
    } else if (parser_peek_for_object_literal(parser)) {
        // Use new enhanced parser for object literals - creates AST_OBJECT instead of
        // AST_FUNCTION_CALL
        value = parser_parse_object_literal(parser, scope);
    } else {
        // Parse as general expression (numbers, strings, arrays, etc.)
        value = parser_parse_expr(parser, scope);
    }

    // Clear variable assignment context
    parser->context.in_variable_assignment = false;

    AST_T *var_def = ast_new_variable_definition(var_name, value);
    var_def->scope = scope;

    // Free the duplicated string since ast_new_variable_definition makes its own copy
    memory_free(var_name);

    // NOTE: We do NOT add the variable to scope here anymore.
    // The visitor will do this after evaluating the expression.
    // This prevents issues with unevaluated expressions and scope corruption.

    // Restore original parser scope
    parser->scope = saved_scope;

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

    // CRITICAL FIX: Add function to scope during parsing for object literal disambiguation
    // This allows parser_detect_object_literal to check user-defined functions
    // The visitor will also add it, but scope_add_function_definition handles duplicates safely
    scope_add_function_definition(scope, func_def);

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
        // If we're parsing inside parentheses, stop at closing paren or comma
        if (parser->context.in_parentheses && (parser->current_token->type == TOKEN_RPAREN ||
                                               parser->current_token->type == TOKEN_COMMA)) {
            break;
        }
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
    case TOKEN_GET:
        expr = parser_parse_file_get(parser, scope);
        break;
    case TOKEN_PUT:
        expr = parser_parse_file_put(parser, scope);
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
                     "%ld",
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

    // Check if this property access should become a method call
    // In ZEN syntax, obj.method arg1 arg2 is a method call
    if (expr && expr->type == AST_PROPERTY_ACCESS) {
        // CRITICAL FIX: Never treat array access (expr->left != NULL) as a method call
        // Array access should always return the element value
        bool is_array_access = (expr->left != NULL);

        // Check if there are arguments following the property access
        bool has_args = (parser->current_token->type != TOKEN_NEWLINE &&
                         parser->current_token->type != TOKEN_EOF &&
                         parser->current_token->type != TOKEN_DEDENT &&
                         parser->current_token->type != TOKEN_RPAREN &&
                         parser->current_token->type != TOKEN_RBRACKET &&
                         parser->current_token->type != TOKEN_COMMA &&
                         parser->current_token->type != TOKEN_DOT &&
                         parser->current_token->type != TOKEN_LBRACKET &&
                         !parser_is_binary_operator(parser->current_token->type));

        // REMOVED: Standalone property access as method call feature
        // This was causing confusion in REPL where obj.property should return the value
        // Methods should be called with explicit arguments or parentheses
        // Properties should always return their values

        if (has_args && !is_array_access) {
            // Convert property access to method call
            AST_T *method_call = ast_new(AST_FUNCTION_CALL);
            method_call->function_call_expression =
                expr;  // The property access becomes the function expression
            method_call->function_call_name =
                memory_strdup(expr->property_name);  // Copy property name for debugging
            method_call->function_call_arguments = NULL;
            method_call->function_call_arguments_size = 0;
            method_call->scope = scope;

            // Parse arguments if any
            if (has_args) {
                size_t arg_capacity = 4;
                method_call->function_call_arguments = memory_alloc(sizeof(AST_T *) * arg_capacity);

                // Set context flag for parsing method arguments
                parser->context.in_function_call = true;

                while (parser->current_token->type != TOKEN_NEWLINE &&
                       parser->current_token->type != TOKEN_EOF &&
                       parser->current_token->type != TOKEN_DEDENT &&
                       parser->current_token->type != TOKEN_RPAREN &&
                       parser->current_token->type != TOKEN_RBRACKET &&
                       !parser_is_binary_operator(parser->current_token->type)) {
                    // Parse as unary expression to stop at binary operators
                    AST_T *arg = parser_parse_unary_expr(parser, scope);
                    if (!arg)
                        break;

                    if (method_call->function_call_arguments_size >= arg_capacity) {
                        arg_capacity *= 2;
                        method_call->function_call_arguments = memory_realloc(
                            method_call->function_call_arguments, sizeof(AST_T *) * arg_capacity);
                    }

                    method_call
                        ->function_call_arguments[method_call->function_call_arguments_size++] =
                        arg;

                    // Handle comma separator for next argument
                    if (parser->current_token->type == TOKEN_COMMA) {
                        parser_eat(parser, TOKEN_COMMA);
                        continue;  // Continue parsing next argument after comma
                    }

                    // Check if there's another argument (space-separated)
                    bool has_next_arg = false;
                    switch (parser->current_token->type) {
                    case TOKEN_ID:
                    case TOKEN_NUMBER:
                    case TOKEN_STRING:
                    case TOKEN_TRUE:
                    case TOKEN_FALSE:
                    case TOKEN_NULL:
                    case TOKEN_LBRACKET:
                    case TOKEN_LPAREN:
                    case TOKEN_NOT:
                    case TOKEN_MINUS:
                    case TOKEN_NEW:
                        has_next_arg = true;
                        break;
                    default:
                        has_next_arg = false;
                        break;
                    }

                    if (!has_next_arg) {
                        break;  // No more arguments
                    }
                }

                // Clear context flag
                parser->context.in_function_call = false;
            }

            expr = method_call;
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
    // Only check for object literals in variable assignment contexts
    // Skip object literal detection in method bodies and function calls
    if (parser->context.in_variable_assignment && !parser->context.in_method_body &&
        !parser->context.in_function_call && parser_peek_for_object_literal(parser)) {
        return parser_parse_object(parser, scope);
    }

    if (parser->current_token->type != TOKEN_ID) {
        return ast_new(AST_NOOP);
    }

    char *original_name = memory_strdup(parser->current_token->value);

    // Peek at the next token to decide what to do
    // Use offset 0 to peek at the token after current_token
    int next_token_type = parser_peek_token_type(parser, 0);

    // Check if this is property access (should be handled by primary expr)
    bool is_property_access = (next_token_type == TOKEN_DOT || next_token_type == TOKEN_LBRACKET);

    if (is_property_access) {
        // Don't consume the ID token yet - let parser_parse_primary_expr handle property access
        // Just return a variable node
        AST_T *var = ast_new_variable(original_name);
        var->scope = scope;
        memory_free(original_name);
        parser_eat(parser, TOKEN_ID);  // Consume ID after creating the node
        return var;
    }

    // Now consume the ID token for other cases
    parser_eat(parser, TOKEN_ID);

    // Check for compound assignment operators
    if (parser->current_token->type == TOKEN_PLUS_EQUALS ||
        parser->current_token->type == TOKEN_MINUS_EQUALS ||
        parser->current_token->type == TOKEN_MULTIPLY_EQUALS ||
        parser->current_token->type == TOKEN_DIVIDE_EQUALS ||
        parser->current_token->type == TOKEN_MODULO_EQUALS) {
        // Create the target variable AST
        AST_T *target = ast_new(AST_VARIABLE);
        target->variable_name = original_name;  // Transfer ownership
        target->scope = scope;

        // Get the operator type
        int compound_op = parser->current_token->type;
        parser_eat(parser, compound_op);

        // Parse the value expression
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

    // Check for parenthesized function call syntax: func()
    bool has_parens = (parser->current_token->type == TOKEN_LPAREN);

    // Check if this identifier has arguments (function call with args)
    // We check if the next token could be an argument to this function
    bool has_args =
        has_parens ||  // Parentheses always indicate function call
        (parser->current_token->type != TOKEN_NEWLINE && parser->current_token->type != TOKEN_EOF &&
         parser->current_token->type != TOKEN_DEDENT &&
         parser->current_token->type != TOKEN_RPAREN &&
         parser->current_token->type != TOKEN_RBRACKET &&
         parser->current_token->type != TOKEN_COMMA && parser->current_token->type != TOKEN_DOT &&
         parser->current_token->type != TOKEN_LBRACKET &&
         !parser_is_binary_operator(parser->current_token->type));

    // Check if this identifier is a stdlib function (for zero-arg calls)
    bool is_stdlib_function = (stdlib_get(original_name) != NULL);

    // For ZEN language: treat standalone identifiers as function calls by default
    // This allows zero-argument user-defined functions to work: "hello" -> hello()
    // The visitor will determine if it's actually a function or variable
    // BUT: Don't do this inside function call arguments!
    bool is_standalone =
        (parser->current_token->type == TOKEN_NEWLINE || parser->current_token->type == TOKEN_EOF ||
         parser->current_token->type == TOKEN_DEDENT);

    // Treat as function call if appropriate
    bool should_be_function_call = false;
    if (parser->context.in_function_call) {
        // Inside function arguments: only treat as function call if it's a known function
        // This prevents variables like "x" from being treated as function calls
        AST_T *fdef = scope_get_function_definition(scope, original_name);
        bool is_user_function = (fdef != NULL);
        // Only treat as function call if it's a known function
        // Don't treat unknown IDs (like variable "x") as function calls
        should_be_function_call = is_stdlib_function || is_user_function;
    } else {
        // Outside function arguments: normal behavior
        should_be_function_call = has_args || is_stdlib_function || is_standalone;
    }

    if (should_be_function_call) {
        AST_T *function_call = ast_new(AST_FUNCTION_CALL);
        function_call->function_call_name = original_name;  // Transfer ownership
        function_call->function_call_arguments = NULL;
        function_call->function_call_arguments_size = 0;
        function_call->scope = scope;

        // Only parse arguments if there are any
        if (has_args) {
            // Set context flag for parsing function arguments
            parser->context.in_function_call = true;

            // Handle parenthesized arguments
            if (has_parens) {
                parser_eat(parser, TOKEN_LPAREN);

                // Set flag to indicate we're parsing inside parentheses
                parser->context.in_parentheses = true;

                // Parse comma-separated arguments until we hit RPAREN
                while (parser->current_token->type != TOKEN_RPAREN &&
                       parser->current_token->type != TOKEN_EOF) {
                    // Parse argument as expression, but stop at RPAREN or COMMA
                    // We need to parse carefully to not consume the closing paren
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

                    // Check for comma separator
                    if (parser->current_token->type == TOKEN_COMMA) {
                        parser_eat(parser, TOKEN_COMMA);
                    } else if (parser->current_token->type != TOKEN_RPAREN) {
                        // Error: expected comma or closing paren
                        break;
                    }
                }

                parser_eat(parser, TOKEN_RPAREN);
                parser->context.in_parentheses = false;  // Clear the flag
                parser->context.in_function_call = false;
            } else {
                // Original space-separated argument parsing
                while (parser->current_token->type != TOKEN_NEWLINE &&
                       parser->current_token->type != TOKEN_EOF &&
                       parser->current_token->type != TOKEN_DEDENT &&
                       parser->current_token->type != TOKEN_RPAREN &&
                       parser->current_token->type != TOKEN_RBRACKET &&
                       !parser_is_binary_operator(parser->current_token->type)) {
                    // Check if current token can start an expression
                    bool can_parse_expr = false;
                    switch (parser->current_token->type) {
                    case TOKEN_ID:
                    case TOKEN_NUMBER:
                    case TOKEN_STRING:
                    case TOKEN_TRUE:
                    case TOKEN_FALSE:
                    case TOKEN_NULL:
                    case TOKEN_LBRACKET:
                    case TOKEN_LPAREN:
                    case TOKEN_NOT:
                    case TOKEN_MINUS:
                    case TOKEN_NEW:
                        can_parse_expr = true;
                        break;
                    default:
                        can_parse_expr = false;
                        break;
                    }

                    if (!can_parse_expr) {
                        break;
                    }

                    // Parse argument based on function type and position
                    AST_T *arg = NULL;
                    if (stdlib_get(function_call->function_call_name)) {
                        // Stdlib function: parse full expression
                        arg = parser_parse_ternary_expr(parser, scope);
                    } else {
                        // User function: parse full expression like stdlib functions
                        // This is needed for recursive calls like "countdown n - 1"
                        // The old behavior of stopping at binary operators broke recursion
                        arg = parser_parse_ternary_expr(parser, scope);
                    }
                    if (!arg)
                        break;

                    function_call->function_call_arguments_size++;
                    function_call->function_call_arguments = memory_realloc(
                        function_call->function_call_arguments,
                        function_call->function_call_arguments_size * sizeof(struct AST_STRUCT *));
                    function_call
                        ->function_call_arguments[function_call->function_call_arguments_size - 1] =
                        arg;

                    // Handle comma separator for next argument
                    if (parser->current_token->type == TOKEN_COMMA) {
                        parser_eat(parser, TOKEN_COMMA);
                        continue;  // Continue parsing next argument after comma
                    }

                    // Check if there's another argument (space-separated)
                    bool has_next_arg = false;
                    switch (parser->current_token->type) {
                    case TOKEN_ID:
                    case TOKEN_NUMBER:
                    case TOKEN_STRING:
                    case TOKEN_TRUE:
                    case TOKEN_FALSE:
                    case TOKEN_NULL:
                    case TOKEN_LBRACKET:
                    case TOKEN_LPAREN:
                    case TOKEN_NOT:
                    case TOKEN_MINUS:
                    case TOKEN_NEW:
                        has_next_arg = true;
                        break;
                    default:
                        has_next_arg = false;
                        break;
                    }

                    if (!has_next_arg) {
                        LOG_PARSER_DEBUG(
                            "No more arguments for function '%s', stopping at token type %d",
                            function_call->function_call_name,
                            parser->current_token->type);
                        break;  // No more arguments
                    }
                    LOG_PARSER_DEBUG("Continuing to parse next argument for function '%s'",
                                     function_call->function_call_name);
                }
            }  // End of else block for space-separated arguments

            // Clear context flag
            parser->context.in_function_call = false;
        }

        return function_call;
    }

    // Not a function call, treat as variable
    AST_T *var = ast_new_variable(original_name);  // Pass string, ast_new_variable will duplicate
    var->scope = scope;
    memory_free(original_name);  // Free our copy since ast_new_variable made its own
    return var;
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
        AST_T *element = parser_parse_ternary_expr(parser, scope);

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
    size_t capacity = 16;  // Initial capacity, will grow as needed

    while (parser->current_token->type == TOKEN_ID) {
        // Grow arrays if needed
        if (pair_count >= capacity) {
            capacity *= 2;
            keys = memory_realloc(keys, capacity * sizeof(char *));
            values = memory_realloc(values, capacity * sizeof(AST_T *));
            if (!keys || !values) {
                // Memory allocation failed - stop parsing
                LOG_ERROR(LOG_CAT_PARSER, "Memory allocation failed for object pairs");
                break;
            }
        }
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
        case TOKEN_ID: {
            // Check if this is a nested object literal
            // We need to look ahead more carefully to distinguish between:
            // 1. Variable reference: ID followed by comma/end
            // 2. Nested object: ID followed by value (forming key-value pair)
            token_T *next_token = lexer_peek_token(parser->lexer, 0);
            
            // Check if this looks like a nested object pattern
            // Nested object would be: ID value [, ID value]*
            bool is_nested_object = false;
            if (next_token) {
                // If next token can be a value (not comma, not end), it's likely nested object
                if (next_token->type == TOKEN_STRING ||
                    next_token->type == TOKEN_NUMBER ||
                    next_token->type == TOKEN_TRUE ||
                    next_token->type == TOKEN_FALSE ||
                    next_token->type == TOKEN_NULL ||
                    next_token->type == TOKEN_LBRACKET ||
                    next_token->type == TOKEN_ID) {
                    // This looks like nested object: current_id next_token form a key-value pair
                    is_nested_object = true;
                }
            }
            
            if (is_nested_object) {
                // Parse as nested object using the specialized nested parser
                if (next_token) token_free(next_token);
                value = parser_parse_nested_object_value(parser, scope);
            } else {
                // Regular variable reference
                if (next_token) token_free(next_token);
                value = parser_parse_variable(parser, scope);
            }
            break;
        }
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
    if (object) {
        object->scope = scope;
    }

    // Clean up keys array and the individual key strings
    // (ast_new_object copies the keys, so we need to free our copies)
    if (keys) {
        for (size_t i = 0; i < pair_count; i++) {
            if (keys[i]) {
                memory_free(keys[i]);
            }
        }
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

    // Free the duplicated iterator string since ast_new_for_loop makes its own copy
    memory_free(iterator);

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
    case TOKEN_RANGE:
        return 5;  // Same precedence as + and -
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
    case TOKEN_RANGE:
        return 1;
    default:
        return 0;
    }
}

/**
 * @brief Enhanced detection algorithm for object literals in ZEN syntax
 * @param parser Parser instance
 * @return int 1 if object literal, 0 otherwise
 * @note This function implements robust lookahead to identify ID VALUE [, ID VALUE]* patterns
 * @note Ensures proper lexer state restoration and handles ZEN's brace-free object syntax
 */
int parser_detect_object_literal(parser_T *parser)
{
    if (!parser || !parser->lexer) {
        return 0;
    }

    LOG_PARSER_DEBUG("parser_detect_object_literal: parser->scope = %p", parser->scope);

    // Save current lexer state for restoration
    size_t saved_current = parser->lexer->i;
    size_t saved_line = parser->lexer->line_number;
    size_t saved_column = parser->lexer->column_number;

    // Use current_token as the first token, then peek ahead for the rest
    token_T *tokens[5] = {NULL};
    int token_count = 0;

    // First token is the current token
    if (parser->current_token && parser->current_token->type != TOKEN_EOF) {
        // Create a manual copy of current token instead of using token_copy
        tokens[0] = memory_alloc(sizeof(token_T));
        tokens[0]->type = parser->current_token->type;
        tokens[0]->value =
            parser->current_token->value ? memory_strdup(parser->current_token->value) : NULL;
        token_count++;

        // Peek ahead for additional tokens (starting from offset 0, which should be the next token)
        for (int i = 1; i < 5; i++) {
            token_T *peeked = lexer_peek_token(
                parser->lexer, i - 1);  // offset i-1 because we already have current
            if (!peeked || peeked->type == TOKEN_EOF) {
                if (peeked)
                    token_free(peeked);
                break;
            }
            tokens[i] = peeked;
            token_count++;
        }
    }

    // Restore lexer state immediately after collection
    parser->lexer->i = saved_current;
    parser->lexer->line_number = saved_line;
    parser->lexer->column_number = saved_column;

    // Collected tokens for analysis

    // Analyze collected tokens for object literal patterns
    int is_object_literal = 0;

    if (token_count >= 1 && tokens[0]->type == TOKEN_ID) {
        // CRITICAL: Don't treat known function names as object literal keys
        if (tokens[0]->value) {
            // Check against known functions and builtins
            if (strcmp(tokens[0]->value, "print") == 0 || strcmp(tokens[0]->value, "input") == 0 ||
                strcmp(tokens[0]->value, "readFile") == 0 ||
                strcmp(tokens[0]->value, "writeFile") == 0 ||
                strcmp(tokens[0]->value, "appendFile") == 0 ||
                strcmp(tokens[0]->value, "fileExists") == 0 ||
                strcmp(tokens[0]->value, "length") == 0 || strcmp(tokens[0]->value, "upper") == 0 ||
                strcmp(tokens[0]->value, "lower") == 0 || strcmp(tokens[0]->value, "trim") == 0 ||
                strcmp(tokens[0]->value, "split") == 0 ||
                strcmp(tokens[0]->value, "contains") == 0 ||
                strcmp(tokens[0]->value, "replace") == 0) {
                // This is a known builtin function - not an object literal
                goto cleanup_and_return;
            }

            // CRITICAL FIX: Also check user-defined functions in scope
            // Use the scope parameter if available, otherwise use parser->scope
            scope_T *check_scope = parser->scope ? parser->scope : NULL;
            if (check_scope && scope_get_function_definition(check_scope, tokens[0]->value)) {
                // This is a user-defined function - not an object literal
                LOG_PARSER_DEBUG("Detected user function '%s', not object literal",
                                 tokens[0]->value);
                is_object_literal = 0;
                goto cleanup_and_return;
            }

            // Check stdlib functions
            if (stdlib_get(tokens[0]->value) != NULL) {
                // This is a stdlib function - not an object literal
                is_object_literal = 0;
                goto cleanup_and_return;
            }
        }

        if (token_count >= 4) {
            // Pattern: ID VALUE, ID VALUE (strong object literal indicator)
            if ((tokens[1]->type == TOKEN_STRING || tokens[1]->type == TOKEN_NUMBER ||
                 tokens[1]->type == TOKEN_TRUE || tokens[1]->type == TOKEN_FALSE ||
                 tokens[1]->type == TOKEN_NULL || tokens[1]->type == TOKEN_UNDECIDABLE ||
                 tokens[1]->type == TOKEN_LBRACKET || tokens[1]->type == TOKEN_ID) &&
                tokens[2]->type == TOKEN_COMMA && tokens[3]->type == TOKEN_ID) {
                is_object_literal = 1;
            }
        }

        // Try single-pair pattern if multi-pair failed and we have at least 2 tokens
        if (!is_object_literal && token_count >= 2) {
            // Pattern: ID VALUE (potential object literal)
            // CRITICAL FIX: Exclude TOKEN_LBRACKET to prevent arr[0] being detected as object
            // literal
            if (tokens[1]->type == TOKEN_STRING || tokens[1]->type == TOKEN_NUMBER ||
                tokens[1]->type == TOKEN_TRUE || tokens[1]->type == TOKEN_FALSE ||
                tokens[1]->type == TOKEN_NULL || tokens[1]->type == TOKEN_UNDECIDABLE) {
                // CRITICAL FIX: For ID NUMBER patterns, be much more conservative
                // This pattern commonly represents function calls like "test_func 42"
                // Only treat as object literal if we have clear evidence (like comma, colon, etc.)
                if (tokens[1]->type == TOKEN_NUMBER) {
                    // In variable assignment context, be more lenient
                    if (parser->context.in_variable_assignment) {
                        // In variable assignment, treat ID NUMBER as object literal
                        is_object_literal = 1;
                    } else if (token_count >= 3 &&
                        (tokens[2]->type == TOKEN_COMMA || tokens[2]->type == TOKEN_COLON)) {
                        // For number values, require explicit object literal indicators
                        is_object_literal = 1;
                    }
                    // Otherwise, assume it's a function call with numeric argument
                } else {
                    // Check third token to ensure it's not a function call or binary operation
                    if (token_count < 3 || (tokens[2]->type != TOKEN_LPAREN &&
                                            !parser_is_binary_operator(tokens[2]->type))) {
                        is_object_literal = 1;
                    }
                }
            } else if (tokens[1]->type == TOKEN_ID) {
                // ID ID pattern - ambiguous, could be function call OR object literal
                // In variable assignment context, be more lenient
                if (parser->context.in_variable_assignment) {
                    // In variable assignment, treat ID ID as object literal
                    is_object_literal = 1;
                } else if (token_count >= 3 && tokens[2]->type == TOKEN_COMMA) {
                    // ID ID, pattern - likely object literal
                    is_object_literal = 1;
                } else if (token_count >= 4 && tokens[2]->type == TOKEN_ID) {
                    // ID ID ID pattern - could be multi-arg function call
                    // Don't treat as object literal
                    is_object_literal = 0;
                } else {
                    // ID ID alone is ambiguous - assume it's a function call
                    // This is the safer default for ZEN's syntax
                    is_object_literal = 0;
                }
            } else if (tokens[1]->type == TOKEN_COMMA) {
                // ID, pattern - comma-separated keys without values
                is_object_literal = 1;
            }
        }
    }

cleanup_and_return:

    // Clean up all allocated tokens
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]) {
            token_free(tokens[i]);
        }
    }

    return is_object_literal;
}

/**
 * @brief Look ahead to determine if parsing an object literal (LEGACY)
 * @param parser Parser instance
 * @return int 1 if object literal, 0 otherwise
 * @deprecated This function is replaced by parser_detect_object_literal()
 */
int parser_peek_for_object_literal(parser_T *parser)
{
    // Legacy function - delegate to new enhanced detection
    int result = parser_detect_object_literal(parser);
    return result;
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
 * @brief Parse indented object block
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object AST node
 */
AST_T *parser_parse_indented_object(parser_T *parser, scope_T *scope)
{
    char **keys = NULL;
    AST_T **values = NULL;
    size_t pair_count = 0;

    // Parse indented key-value pairs
    while (parser->current_token->type == TOKEN_ID) {
        char *key = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        AST_T *value = NULL;

        // Check what comes after the key
        if (parser->current_token->type == TOKEN_NEWLINE) {
            // Nested object case: key followed by newline and indent
            parser_eat(parser, TOKEN_NEWLINE);
            if (parser->current_token->type == TOKEN_INDENT) {
                parser_eat(parser, TOKEN_INDENT);
                // Recursively parse nested object
                value = parser_parse_indented_object(parser, scope);
                // Consume DEDENT
                if (parser->current_token->type == TOKEN_DEDENT) {
                    parser_eat(parser, TOKEN_DEDENT);
                }
            } else {
                // Key with no value - use key as variable reference
                value = ast_new_variable(key);
            }
        } else if (parser->current_token->type == TOKEN_COMMA ||
                   parser->current_token->type == TOKEN_DEDENT ||
                   parser->current_token->type == TOKEN_EOF) {
            // Key with no value - use key as variable reference
            value = ast_new_variable(key);
        } else {
            // Key with value on same line
            // Parse the value (string, number, etc.)
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
            case TOKEN_LBRACKET:
                value = parser_parse_array(parser, scope);
                break;
            case TOKEN_ID:
                // Another identifier - could be variable reference or start of nested object
                // For now, treat as variable reference
                value = parser_parse_variable(parser, scope);
                break;
            default:
                value = ast_new(AST_NOOP);
                break;
            }
        }

        // Add key-value pair
        pair_count++;
        keys = memory_realloc(keys, pair_count * sizeof(char *));
        values = memory_realloc(values, pair_count * sizeof(AST_T *));
        keys[pair_count - 1] = key;
        values[pair_count - 1] = value;

        // Check for continuation
        if (parser->current_token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            // Skip optional newline after comma
            if (parser->current_token->type == TOKEN_NEWLINE) {
                parser_eat(parser, TOKEN_NEWLINE);
            }
        } else if (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
            // If no more content at this indentation level, we're done
            if (parser->current_token->type == TOKEN_DEDENT ||
                parser->current_token->type == TOKEN_EOF) {
                break;
            }
        } else {
            // No comma or newline, we're done
            break;
        }
    }

    // Create object AST node
    AST_T *object = ast_new_object(keys, values, pair_count);
    if (object) {
        object->scope = scope;
    }

    // Clean up keys (ast_new_object copies them)
    if (keys) {
        for (size_t i = 0; i < pair_count; i++) {
            if (keys[i]) {
                memory_free(keys[i]);
            }
        }
        memory_free(keys);
    }
    if (values) {
        memory_free(values);
    }

    return object;
}

/**
 * @brief Parse nested object value (continues until pattern breaks)
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Nested object AST node
 */
AST_T *parser_parse_nested_object_value(parser_T *parser, scope_T *scope)
{
    char **keys = NULL;
    AST_T **values = NULL;
    size_t pair_count = 0;

    // Parse key-value pairs for the nested object
    // Pattern: ID value [, ID value]*
    while (parser->current_token->type == TOKEN_ID) {
        char *key = memory_strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        // Parse the value for this key
        AST_T *value = NULL;
        
        // Check if we have a value or if this key stands alone
        if (parser->current_token->type == TOKEN_COMMA ||
            parser->current_token->type == TOKEN_EOF ||
            parser->current_token->type == TOKEN_NEWLINE ||
            parser->current_token->type == TOKEN_DEDENT) {
            // No value, use key as variable reference
            value = ast_new_variable(key);
        } else {
            // Parse the value
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
            case TOKEN_LBRACKET:
                value = parser_parse_array(parser, scope);
                break;
            case TOKEN_ID:
                // Check if this is a deeper nested object
                token_T *next = lexer_peek_token(parser->lexer, 0);
                if (next) {
                    if (next->type == TOKEN_STRING ||
                        next->type == TOKEN_NUMBER ||
                        next->type == TOKEN_TRUE ||
                        next->type == TOKEN_FALSE ||
                        next->type == TOKEN_NULL ||
                        next->type == TOKEN_LBRACKET ||
                        next->type == TOKEN_ID) {
                        // This is a nested object within nested object
                        token_free(next);
                        value = parser_parse_nested_object_value(parser, scope);
                    } else {
                        // Just a variable reference
                        token_free(next);
                        value = parser_parse_variable(parser, scope);
                    }
                } else {
                    value = parser_parse_variable(parser, scope);
                }
                break;
            default:
                value = ast_new(AST_NOOP);
                break;
            }
        }

        // Add the key-value pair
        pair_count++;
        keys = memory_realloc(keys, pair_count * sizeof(char *));
        values = memory_realloc(values, pair_count * sizeof(AST_T *));
        keys[pair_count - 1] = key;
        values[pair_count - 1] = value;

        // Check for continuation within the nested object
        // After we've parsed a key-value pair, check if there's more
        // We continue if we see ", ID value" pattern BUT NOT ", ID ID value" (which starts a new top-level key)
        if (parser->current_token->type == TOKEN_COMMA) {
            // Look ahead without consuming to check the pattern after comma
            token_T *after_comma = lexer_peek_token(parser->lexer, 0);
            
            if (after_comma && after_comma->type == TOKEN_ID) {
                // Check what comes after this ID
                token_T *after_id = lexer_peek_token(parser->lexer, 1);
                
                bool continue_nested = false;
                if (after_id) {
                    // If pattern is ", ID literal-value", continue nested object
                    if (after_id->type == TOKEN_STRING ||
                        after_id->type == TOKEN_NUMBER ||
                        after_id->type == TOKEN_TRUE ||
                        after_id->type == TOKEN_FALSE ||
                        after_id->type == TOKEN_NULL ||
                        after_id->type == TOKEN_LBRACKET) {
                        continue_nested = true;
                    } else if (after_id->type == TOKEN_ID) {
                        // Pattern is ", ID ID ..." - this could be:
                        // 1. End of nested object, start of new top-level key with nested value
                        // 2. Continuation of nested object with variable reference
                        // We end the nested object here to avoid ambiguity
                        continue_nested = false;
                    }
                    token_free(after_id);
                }
                
                if (after_comma) token_free(after_comma);
                
                if (continue_nested) {
                    // Consume the comma and continue parsing
                    parser_eat(parser, TOKEN_COMMA);
                    continue;
                } else {
                    // Don't consume the comma - let parent handle it
                    break;
                }
            } else {
                if (after_comma) token_free(after_comma);
                // No ID after comma, end nested object
                break;
            }
        } else {
            // No comma means end of nested object
            break;
        }
    }

    // Create the object AST node
    AST_T *object = ast_new_object(keys, values, pair_count);
    if (object) {
        object->scope = scope;
    }

    // Clean up keys array (ast_new_object copies them)
    if (keys) {
        for (size_t i = 0; i < pair_count; i++) {
            if (keys[i]) {
                memory_free(keys[i]);
            }
        }
        memory_free(keys);
    }
    if (values) {
        memory_free(values);
    }

    return object;
}

/**
 * @brief Parse ZEN object literal creating AST_OBJECT nodes instead of AST_FUNCTION_CALL
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object literal AST node (AST_OBJECT type)
 */
AST_T *parser_parse_object_literal(parser_T *parser, scope_T *scope)
{
    char **keys = NULL;
    AST_T **values = NULL;
    size_t pair_count = 0;
    size_t capacity = 16;  // Initial capacity, will grow as needed

    while (parser->current_token->type == TOKEN_ID) {
        // Grow arrays if needed
        if (pair_count >= capacity) {
            capacity *= 2;
            keys = memory_realloc(keys, capacity * sizeof(char *));
            values = memory_realloc(values, capacity * sizeof(AST_T *));
            if (!keys || !values) {
                // Memory allocation failed - stop parsing
                LOG_ERROR(LOG_CAT_PARSER, "Memory allocation failed for object pairs");
                break;
            }
        }
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
        case TOKEN_ID: {
            // Check if this is a nested object literal
            // We need to look ahead more carefully to distinguish between:
            // 1. Variable reference: ID followed by comma/end
            // 2. Nested object: ID followed by value (forming key-value pair)
            token_T *next_token = lexer_peek_token(parser->lexer, 0);
            
            // Check if this looks like a nested object pattern
            // Nested object would be: ID value [, ID value]*
            bool is_nested_object = false;
            if (next_token) {
                // If next token can be a value (not comma, not end), it's likely nested object
                if (next_token->type == TOKEN_STRING ||
                    next_token->type == TOKEN_NUMBER ||
                    next_token->type == TOKEN_TRUE ||
                    next_token->type == TOKEN_FALSE ||
                    next_token->type == TOKEN_NULL ||
                    next_token->type == TOKEN_LBRACKET) {
                    // Definitely a nested object with literal value
                    is_nested_object = true;
                } else if (next_token->type == TOKEN_ID) {
                    // Two IDs in a row - could be nested object OR variable
                    // Look ahead one more token to disambiguate
                    token_T *next_next_token = lexer_peek_token(parser->lexer, 1);
                    if (next_next_token) {
                        // If we have: ID ID value-token, it's a nested object
                        // If we have: ID ID COMMA/EOF/NEWLINE, first ID is a variable
                        if (next_next_token->type == TOKEN_STRING ||
                            next_next_token->type == TOKEN_NUMBER ||
                            next_next_token->type == TOKEN_TRUE ||
                            next_next_token->type == TOKEN_FALSE ||
                            next_next_token->type == TOKEN_NULL ||
                            next_next_token->type == TOKEN_LBRACKET ||
                            next_next_token->type == TOKEN_ID) {
                            // This is a nested object pattern
                            is_nested_object = true;
                        }
                        token_free(next_next_token);
                    }
                }
            }
            
            if (is_nested_object) {
                // Parse as nested object literal
                if (next_token) token_free(next_token);
                value = parser_parse_nested_object_value(parser, scope);
            } else {
                // Regular variable reference
                if (next_token) token_free(next_token);
                value = parser_parse_variable(parser, scope);
            }
            break;
        }
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
    if (object) {
        object->scope = scope;
    }

    // Clean up keys array and the individual key strings
    // (ast_new_object copies the keys, so we need to free our copies)
    if (keys) {
        for (size_t i = 0; i < pair_count; i++) {
            if (keys[i]) {
                memory_free(keys[i]);
            }
        }
        memory_free(keys);
    }
    if (values) {
        memory_free(values);
    }

    return object;
}

/**
 * @brief Parse ternary conditional expressions
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Ternary expression AST node
 */
AST_T *parser_parse_ternary_expr(parser_T *parser, scope_T *scope)
{
    // Actually parse ternary expressions (condition ? true_expr : false_expr)
    return parser_parse_ternary(parser, scope);
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
    // The target should be a simple variable (already peeked at ID token)
    if (parser->current_token->type != TOKEN_ID) {
        return NULL;
    }

    // Create variable AST for the target
    AST_T *target = ast_new(AST_VARIABLE);
    target->variable_name = memory_strdup(parser->current_token->value);
    target->scope = scope;

    // Consume the ID token
    parser_eat(parser, TOKEN_ID);

    if (!target) {
        return NULL;
    }

    // Check for compound assignment operators
    int compound_op = parser->current_token->type;
    switch (parser->current_token->type) {
    case TOKEN_PLUS_EQUALS:
    case TOKEN_MINUS_EQUALS:
    case TOKEN_MULTIPLY_EQUALS:
    case TOKEN_DIVIDE_EQUALS:
    case TOKEN_MODULO_EQUALS:
        // Valid compound operator
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

    // Parse file path - could be string literal or variable
    // Use primary_expr to avoid consuming property access
    AST_T *file_path = NULL;
    
    if (parser->current_token->type == TOKEN_STRING) {
        // String literal file path
        file_path = parser_parse_string(parser, scope);
    } else if (parser->current_token->type == TOKEN_ID) {
        // Variable containing file path
        file_path = parser_parse_variable(parser, scope);
    } else {
        // Invalid file path
        file_path = ast_new_noop();
    }

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

    // Check for optional inheritance with extends keyword
    char *parent_class = NULL;
    if (parser->current_token && parser->current_token->type == TOKEN_EXTENDS) {
        parser_eat(parser, TOKEN_EXTENDS);
        if (parser->current_token && parser->current_token->type == TOKEN_ID) {
            parent_class = memory_strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);
        } else {
            // Error: expected class name after extends
            memory_free(class_name);
            return NULL;
        }
    }

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
           parser->current_token->type != TOKEN_DEDENT) {
        // Check if current token is a method/function declaration
        if (!(parser->current_token->type == TOKEN_METHOD ||
              parser->current_token->type == TOKEN_FUNCTION)) {
            // Not a method declaration - we've reached the end of methods
            break;
        }
        AST_T *method = parser_parse_class_method(parser, scope);
        if (method) {
            // Method parsed successfully

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
        while (parser->current_token && parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
        }

        // Break if we reach EOF or end of class
        if (!parser->current_token || parser->current_token->type == TOKEN_EOF ||
            parser->current_token->type == TOKEN_DEDENT) {
            break;
        }
    }

    // After parsing all methods, we need to find and consume the DEDENT that ends the class.
    // However, due to how the lexer works, there might be other tokens before the DEDENT
    // if the next line after the class is dedented. We need to handle this carefully.

    // If we already see a DEDENT, consume it
    if (parser->current_token && parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    } else {
        // We exited the method loop because we saw a non-method token.
        // This could be because:
        // 1. There's content after the class at the same or lower indentation
        // 2. The lexer returned that content's tokens before the DEDENT
        //
        // We need to track that we're waiting for a DEDENT and let the main
        // parser handle the tokens until it finds our DEDENT.
        //
        // For now, we'll just note that we didn't find a DEDENT.
        // The main parser will see the DEDENT and handle it.
    }

    // Methods validated

    // Create class definition AST node
    AST_T *class_def = ast_new_class_definition(class_name, parent_class, methods, methods_count);

    // Class created successfully

    // Cleanup
    memory_free(class_name);
    if (parent_class) {
        memory_free(parent_class);
    }
    memory_free(methods);  // ast_new_class_definition makes its own copy

    return class_def;
}

/**
 * @brief Parse class method definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Method definition (as function AST node)
 */
AST_T *parser_parse_class_method(parser_T *parser, scope_T *scope)
{
    // Handle both 'method' keyword (as TOKEN_METHOD) and 'function' keyword
    if (parser->current_token->type == TOKEN_METHOD) {
        parser_eat(parser, TOKEN_METHOD);
    } else if (parser->current_token->type == TOKEN_ID &&
               strcmp(parser->current_token->value, "method") == 0) {
        parser_eat(parser, TOKEN_ID);
    } else if (parser->current_token->type == TOKEN_FUNCTION) {
        parser_eat(parser, TOKEN_FUNCTION);
    } else {
        return NULL;
    }

    // Get method name
    char *method_name = memory_strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    // Parse parameters
    AST_T **args = NULL;
    size_t arg_count = 0;

    while (parser->current_token->type == TOKEN_ID) {
        arg_count++;
        args = memory_realloc(args, arg_count * sizeof(AST_T *));
        char *arg_name = memory_strdup(parser->current_token->value);
        args[arg_count - 1] = ast_new_variable(arg_name);
        memory_free(arg_name);
        parser_eat(parser, TOKEN_ID);
    }

    // Skip optional newline
    if (parser->current_token->type == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
    }

    // Expect indent for method body
    if (parser->current_token->type == TOKEN_INDENT) {
        parser_eat(parser, TOKEN_INDENT);
    }

    // Parse method body
    AST_T *body = parser_parse_statements(parser, scope);

    // Consume dedent after method body
    if (parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    // Create method as function definition
    AST_T *method = ast_new_function_definition(method_name, args, arg_count, body);
    method->scope = scope;

    memory_free(method_name);
    if (args) {
        memory_free(args);
    }

    return method;
}

/**
 * @brief Parse try/catch block
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Try/catch AST node
 */
AST_T *parser_parse_try_catch(parser_T *parser, scope_T *scope)
{
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Consume 'try' token
    parser_eat(parser, TOKEN_TRY);

    // Skip optional colon
    if (parser->current_token && parser->current_token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
    }

    // Expect newline and indent for try block
    parser_eat(parser, TOKEN_NEWLINE);
    parser_eat(parser, TOKEN_INDENT);

    // Parse try block statements
    AST_T *try_block = parser_parse_statements(parser, scope);

    // Consume dedent after try block
    if (parser->current_token && parser->current_token->type == TOKEN_DEDENT) {
        parser_eat(parser, TOKEN_DEDENT);
    }

    // Check for catch block
    AST_T *catch_block = NULL;
    char *exception_variable = NULL;

    if (parser->current_token && parser->current_token->type == TOKEN_CATCH) {
        parser_eat(parser, TOKEN_CATCH);

        // Optional exception variable
        if (parser->current_token && parser->current_token->type == TOKEN_ID) {
            exception_variable = memory_strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);
        }

        // Skip optional colon
        if (parser->current_token && parser->current_token->type == TOKEN_COLON) {
            parser_eat(parser, TOKEN_COLON);
        }

        // Expect newline and indent for catch block
        parser_eat(parser, TOKEN_NEWLINE);
        parser_eat(parser, TOKEN_INDENT);

        // Parse catch block statements
        catch_block = parser_parse_statements(parser, scope);

        // Consume dedent after catch block
        if (parser->current_token && parser->current_token->type == TOKEN_DEDENT) {
            parser_eat(parser, TOKEN_DEDENT);
        }
    }

    // Create try/catch AST node
    AST_T *try_catch = ast_new(AST_TRY_CATCH);
    try_catch->try_block = try_block;
    try_catch->catch_block = catch_block;
    try_catch->exception_variable = exception_variable;
    try_catch->scope = scope;

    return try_catch;
}

/**
 * @brief Parse throw statement
 * @param parser Parser instance
 * @param scope Current scope
 * @return AST_T* Throw AST node
 */
AST_T *parser_parse_throw(parser_T *parser, scope_T *scope)
{
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Consume 'throw' token
    parser_eat(parser, TOKEN_THROW);

    // Parse the exception expression
    AST_T *exception_value = parser_parse_expr(parser, scope);

    // Create throw AST node
    AST_T *throw_node = ast_new(AST_THROW);
    throw_node->exception_value = exception_value;
    throw_node->scope = scope;

    return throw_node;
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
