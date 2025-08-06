#include "zen/core/parser.h"
#include "zen/core/scope.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Create parser instance
 * @param lexer Lexical analyzer instance
 * @return parser_T* New parser instance
 */
parser_T* init_parser(lexer_T* lexer)
{
    parser_T* parser = calloc(1, sizeof(struct PARSER_STRUCT));
    parser->lexer = lexer;
    parser->current_token = lexer_get_next_token(lexer);
    parser->prev_token = parser->current_token;
    parser->scope = init_scope();
    return parser;
}

/**
 * @brief Consume expected token type
 * @param parser Parser instance
 * @param token_type Expected token type
 */
void parser_eat(parser_T* parser, int token_type)
{
    if ((int)parser->current_token->type == token_type)
    {
        parser->prev_token = parser->current_token;
        parser->current_token = lexer_get_next_token(parser->lexer);
    }
    else
    {
        printf(
            "Unexpected token `%s`, with type %d",
            parser->current_token->value,
            (int)parser->current_token->type
        );
        exit(1);
    }
}

/**
 * @brief Parse input and return AST
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Root AST node
 */
AST_T* parser_parse(parser_T* parser, scope_T* scope)
{
    return parser_parse_statements(parser, scope);
}

/**
 * @brief Parse a single statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Statement AST node
 */
AST_T* parser_parse_statement(parser_T* parser, scope_T* scope)
{
    switch (parser->current_token->type)
    {
        case TOKEN_SET: return parser_parse_variable_definition(parser, scope);
        case TOKEN_FUNCTION: return parser_parse_function_definition(parser, scope);
        case TOKEN_ID: return parser_parse_id(parser, scope);
        case TOKEN_IF: return parser_parse_if_statement(parser, scope);
        case TOKEN_WHILE: return parser_parse_while_loop(parser, scope);
        case TOKEN_FOR: return parser_parse_for_loop(parser, scope);
        case TOKEN_RETURN: return parser_parse_return_statement(parser, scope);
        case TOKEN_BREAK: return parser_parse_break_statement(parser, scope);
        case TOKEN_CONTINUE: return parser_parse_continue_statement(parser, scope);
        default: 
            // Try parsing as expression statement
            return parser_parse_expr(parser, scope);
    }
}

/**
 * @brief Parse multiple statements
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Compound AST node containing statements
 */
AST_T* parser_parse_statements(parser_T* parser, scope_T* scope)
{
    AST_T* compound = init_ast(AST_COMPOUND);
    compound->scope = scope;
    compound->compound_value = calloc(1, sizeof(struct AST_STRUCT*));
    compound->compound_size = 0;

    // Parse statements until we reach EOF, DEDENT, or encounter an error
    while (parser->current_token->type != TOKEN_EOF && 
           parser->current_token->type != TOKEN_DEDENT)
    {
        // Skip any leading newlines
        while (parser->current_token->type == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
            
            // Check again if we've reached end conditions after eating newlines
            if (parser->current_token->type == TOKEN_EOF || 
                parser->current_token->type == TOKEN_DEDENT)
                break;
        }
        
        // If we've reached the end, break out
        if (parser->current_token->type == TOKEN_EOF || 
            parser->current_token->type == TOKEN_DEDENT)
            break;
            
        // Parse the next statement
        AST_T* ast_statement = parser_parse_statement(parser, scope);
        
        if (ast_statement) {
            ast_statement->scope = scope;
            
            // Expand the compound array
            compound->compound_size++;
            compound->compound_value = realloc(
                compound->compound_value,
                compound->compound_size * sizeof(struct AST_STRUCT*)
            );
            compound->compound_value[compound->compound_size-1] = ast_statement;
        }
        
        // If there's a newline after the statement, consume it
        // But don't require it (for REPL compatibility)
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
AST_T* parser_parse_expr(parser_T* parser, scope_T* scope)
{
    return parser_parse_binary_expr(parser, scope, 0);
}

/**
 * @brief Parse binary expression with precedence climbing
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @param min_precedence Minimum precedence level
 * @return AST_T* Binary expression AST node
 */
AST_T* parser_parse_binary_expr(parser_T* parser, scope_T* scope, int min_precedence)
{
    AST_T* left = parser_parse_unary_expr(parser, scope);
    
    while (parser_is_binary_operator(parser->current_token->type) &&
           parser_get_precedence(parser->current_token->type) >= min_precedence)
    {
        int op_type = parser->current_token->type;
        int precedence = parser_get_precedence(op_type);
        parser_eat(parser, op_type);
        
        AST_T* right = parser_parse_binary_expr(parser, scope, precedence + 1);
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
AST_T* parser_parse_unary_expr(parser_T* parser, scope_T* scope)
{
    // Handle unary operators
    if (parser->current_token->type == TOKEN_NOT ||
        parser->current_token->type == TOKEN_MINUS)
    {
        int op_type = parser->current_token->type;
        parser_eat(parser, op_type);
        AST_T* operand = parser_parse_unary_expr(parser, scope);
        AST_T* unary = ast_new_unary_op(op_type, operand);
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
AST_T* parser_parse_primary_expr(parser_T* parser, scope_T* scope)
{
    switch (parser->current_token->type)
    {
        case TOKEN_STRING: return parser_parse_string(parser, scope);
        case TOKEN_NUMBER: return parser_parse_number(parser, scope);
        case TOKEN_TRUE:
        case TOKEN_FALSE: return parser_parse_boolean(parser, scope);
        case TOKEN_NULL: return parser_parse_null(parser, scope);
        case TOKEN_LBRACKET: return parser_parse_array(parser, scope);
        case TOKEN_LPAREN: {
            parser_eat(parser, TOKEN_LPAREN);
            AST_T* expr = parser_parse_expr(parser, scope);
            parser_eat(parser, TOKEN_RPAREN);
            return expr;
        }
        case TOKEN_ID: return parser_parse_id_or_object(parser, scope);
        default: break;
    }
    return init_ast(AST_NOOP);
}

/**
 * @brief Parse function call
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function call AST node
 */
AST_T* parser_parse_function_call(parser_T* parser, scope_T* scope)
{
    AST_T* function_call = init_ast(AST_FUNCTION_CALL);
    function_call->function_call_name = parser->prev_token->value;
    function_call->function_call_arguments = calloc(1, sizeof(struct AST_STRUCT*));
    function_call->function_call_arguments_size = 0;

    // Parse first argument if present
    if (parser->current_token->type != TOKEN_NEWLINE && 
        parser->current_token->type != TOKEN_EOF &&
        parser->current_token->type != TOKEN_DEDENT)
    {
        AST_T* ast_expr = parser_parse_expr(parser, scope);
        if (ast_expr) {
            function_call->function_call_arguments[0] = ast_expr;
            function_call->function_call_arguments_size = 1;

            // Parse additional arguments separated by spaces or commas
            while (parser->current_token->type == TOKEN_COMMA ||
                   (parser->current_token->type == TOKEN_STRING || 
                    parser->current_token->type == TOKEN_ID ||
                    parser->current_token->type == TOKEN_NUMBER ||
                    parser->current_token->type == TOKEN_TRUE ||
                    parser->current_token->type == TOKEN_FALSE ||
                    parser->current_token->type == TOKEN_NULL ||
                    parser->current_token->type == TOKEN_LBRACKET))
            {
                if (parser->current_token->type == TOKEN_COMMA)
                    parser_eat(parser, TOKEN_COMMA);

                AST_T* ast_expr = parser_parse_expr(parser, scope);
                if (ast_expr) {
                    function_call->function_call_arguments_size += 1;
                    function_call->function_call_arguments = realloc(
                        function_call->function_call_arguments,
                        function_call->function_call_arguments_size * sizeof(struct AST_STRUCT*)
                    );
                    function_call->function_call_arguments[function_call->function_call_arguments_size-1] = ast_expr;
                }
            }
        }
    }

    function_call->scope = scope;
    return function_call;
}

/**
 * @brief Parse variable definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable definition AST node
 */
AST_T* parser_parse_variable_definition(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_SET); // set
    
    if (parser->current_token->type != TOKEN_ID) {
        printf("Expected variable name after 'set'\n");
        exit(1);
    }
    
    char* variable_definition_variable_name = parser->current_token->value;
    parser_eat(parser, TOKEN_ID); // variable name
    AST_T* variable_definition_value = parser_parse_expr(parser, scope);

    AST_T* variable_definition = init_ast(AST_VARIABLE_DEFINITION);
    variable_definition->variable_definition_variable_name = variable_definition_variable_name;
    variable_definition->variable_definition_value = variable_definition_value;
    variable_definition->scope = scope;

    return variable_definition;
}

/**
 * @brief Parse function definition
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Function definition AST node
 */
AST_T* parser_parse_function_definition(parser_T* parser, scope_T* scope)
{
    AST_T* ast = init_ast(AST_FUNCTION_DEFINITION);
    parser_eat(parser, TOKEN_ID); // function

    if (parser->current_token->type != TOKEN_ID) {
        printf("Expected function name after 'function'\n");
        exit(1);
    }

    char* function_name = parser->current_token->value;
    ast->function_definition_name = calloc(strlen(function_name) + 1, sizeof(char));
    strcpy(ast->function_definition_name, function_name);
    parser_eat(parser, TOKEN_ID); // function name

    // Parse space-separated arguments
    ast->function_definition_args = calloc(1, sizeof(struct AST_STRUCT*));
    ast->function_definition_args_size = 0;

    // Parse function arguments (space-separated IDs)
    while (parser->current_token->type == TOKEN_ID)
    {
        AST_T* arg = parser_parse_variable(parser, scope);
        if (arg) {
            ast->function_definition_args_size += 1;
            ast->function_definition_args = realloc(
                ast->function_definition_args,
                ast->function_definition_args_size * sizeof(struct AST_STRUCT*)
            );
            ast->function_definition_args[ast->function_definition_args_size-1] = arg;
        }
    }
    
    // Expect newline and indent for function body
    if (parser->current_token->type == TOKEN_NEWLINE)
        parser_eat(parser, TOKEN_NEWLINE);
    
    if (parser->current_token->type == TOKEN_INDENT)
        parser_eat(parser, TOKEN_INDENT);
    
    ast->function_definition_body = parser_parse_statements(parser, scope);
    
    // Expect dedent after function body
    if (parser->current_token->type == TOKEN_DEDENT)
        parser_eat(parser, TOKEN_DEDENT);

    ast->scope = scope;
    return ast;
}

/**
 * @brief Parse variable reference
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Variable AST node
 */
AST_T* parser_parse_variable(parser_T* parser, scope_T* scope)
{
    char* token_value = parser->current_token->value;
    parser_eat(parser, TOKEN_ID); // var name or function call name

    // FIXED LOGIC: If the next token is a binary operator, this is definitely a variable
    // used in an expression (like "x + y"), so return a variable node.
    if (parser_is_binary_operator(parser->current_token->type)) {
        AST_T* ast_variable = init_ast(AST_VARIABLE);
        ast_variable->variable_name = token_value;
        ast_variable->scope = scope;
        return ast_variable;
    }
    
    // If the next token could be a function argument AND it's not end-of-statement,
    // treat this as a function call
    if (parser->current_token->type == TOKEN_STRING || 
        parser->current_token->type == TOKEN_ID ||
        parser->current_token->type == TOKEN_NUMBER ||
        parser->current_token->type == TOKEN_TRUE ||
        parser->current_token->type == TOKEN_FALSE ||
        parser->current_token->type == TOKEN_NULL ||
        parser->current_token->type == TOKEN_LBRACKET) {
        return parser_parse_function_call(parser, scope);
    }

    // Otherwise, it's a variable (end of statement, etc.)
    AST_T* ast_variable = init_ast(AST_VARIABLE);
    ast_variable->variable_name = token_value;
    ast_variable->scope = scope;
    return ast_variable;
}

/**
 * @brief Parse string literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* String AST node
 */
AST_T* parser_parse_string(parser_T* parser, scope_T* scope)
{
    AST_T* ast_string = init_ast(AST_STRING);
    ast_string->string_value = parser->current_token->value;
    parser_eat(parser, TOKEN_STRING);
    ast_string->scope = scope;
    return ast_string;
}

/**
 * @brief Parse identifier or object literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Appropriate AST node based on identifier type
 */
AST_T* parser_parse_id_or_object(parser_T* parser, scope_T* scope)
{
    // Check for object literal (key-value pairs)
    if (parser_peek_for_object_literal(parser))
    {
        return parser_parse_object(parser, scope);
    }
    
    return parser_parse_id(parser, scope);
}

/**
 * @brief Parse identifier
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Appropriate AST node based on identifier type
 */
AST_T* parser_parse_id(parser_T* parser, scope_T* scope)
{
    if (strcmp(parser->current_token->value, "set") == 0)
    {
        return parser_parse_variable_definition(parser, scope);
    }
    else if (strcmp(parser->current_token->value, "function") == 0)
    {
        return parser_parse_function_definition(parser, scope);
    }
    else
    {
        return parser_parse_variable(parser, scope);
    }
}

/* ============================================================================
 * LITERAL PARSING FUNCTIONS
 * ============================================================================ */

/**
 * @brief Parse number literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Number AST node
 */
AST_T* parser_parse_number(parser_T* parser, scope_T* scope)
{
    double value = atof(parser->current_token->value);
    parser_eat(parser, TOKEN_NUMBER);
    
    AST_T* ast_number = ast_new_number(value);
    ast_number->scope = scope;
    return ast_number;
}

/**
 * @brief Parse boolean literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Boolean AST node
 */
AST_T* parser_parse_boolean(parser_T* parser, scope_T* scope)
{
    int value = (parser->current_token->type == TOKEN_TRUE) ? 1 : 0;
    parser_eat(parser, parser->current_token->type);
    
    AST_T* ast_boolean = ast_new_boolean(value);
    ast_boolean->scope = scope;
    return ast_boolean;
}

/**
 * @brief Parse null literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Null AST node
 */
AST_T* parser_parse_null(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_NULL);
    
    AST_T* ast_null = ast_new_null();
    ast_null->scope = scope;
    return ast_null;
}

/**
 * @brief Parse array literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Array AST node
 */
AST_T* parser_parse_array(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_LBRACKET);
    
    AST_T** elements = NULL;
    size_t element_count = 0;
    
    // Parse array elements
    if (parser->current_token->type != TOKEN_RBRACKET)
    {
        // Parse first element
        AST_T* element = parser_parse_expr(parser, scope);
        if (element)
        {
            elements = calloc(1, sizeof(AST_T*));
            elements[0] = element;
            element_count = 1;
        }
        
        // Parse additional elements separated by commas
        while (parser->current_token->type == TOKEN_COMMA)
        {
            parser_eat(parser, TOKEN_COMMA);
            
            // Allow trailing comma
            if (parser->current_token->type == TOKEN_RBRACKET)
                break;
                
            element = parser_parse_expr(parser, scope);
            if (element)
            {
                element_count++;
                elements = realloc(elements, element_count * sizeof(AST_T*));
                elements[element_count - 1] = element;
            }
        }
    }
    
    parser_eat(parser, TOKEN_RBRACKET);
    
    AST_T* ast_array = ast_new_array(elements, element_count);
    ast_array->scope = scope;
    return ast_array;
}

/**
 * @brief Parse object literal
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Object AST node
 */
AST_T* parser_parse_object(parser_T* parser, scope_T* scope)
{
    char** keys = NULL;
    AST_T** values = NULL;
    size_t pair_count = 0;
    
    // Parse first key-value pair
    if (parser->current_token->type == TOKEN_ID)
    {
        char* key = parser->current_token->value;
        parser_eat(parser, TOKEN_ID);
        
        AST_T* value = parser_parse_expr(parser, scope);
        if (value)
        {
            keys = calloc(1, sizeof(char*));
            values = calloc(1, sizeof(AST_T*));
            keys[0] = calloc(strlen(key) + 1, sizeof(char));
            strcpy(keys[0], key);
            values[0] = value;
            pair_count = 1;
        }
        
        // Parse additional pairs separated by commas
        while (parser->current_token->type == TOKEN_COMMA)
        {
            parser_eat(parser, TOKEN_COMMA);
            
            // Allow trailing comma or end of statement
            if (parser->current_token->type != TOKEN_ID)
                break;
                
            key = parser->current_token->value;
            parser_eat(parser, TOKEN_ID);
            
            value = parser_parse_expr(parser, scope);
            if (value)
            {
                pair_count++;
                keys = realloc(keys, pair_count * sizeof(char*));
                values = realloc(values, pair_count * sizeof(AST_T*));
                keys[pair_count - 1] = calloc(strlen(key) + 1, sizeof(char));
                strcpy(keys[pair_count - 1], key);
                values[pair_count - 1] = value;
            }
        }
    }
    
    AST_T* ast_object = ast_new_object(keys, values, pair_count);
    ast_object->scope = scope;
    return ast_object;
}

/* ============================================================================
 * CONTROL FLOW PARSING FUNCTIONS
 * ============================================================================ */

/**
 * @brief Parse if statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* If statement AST node
 */
AST_T* parser_parse_if_statement(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_IF);
    
    AST_T* condition = parser_parse_expr(parser, scope);
    
    // Handle optional 'then' keyword
    if (parser->current_token->type == TOKEN_THEN)
        parser_eat(parser, TOKEN_THEN);
    
    // Expect newline and indent for body
    if (parser->current_token->type == TOKEN_NEWLINE)
        parser_eat(parser, TOKEN_NEWLINE);
    if (parser->current_token->type == TOKEN_INDENT)
        parser_eat(parser, TOKEN_INDENT);
    
    AST_T* then_branch = parser_parse_statements(parser, scope);
    
    if (parser->current_token->type == TOKEN_DEDENT)
        parser_eat(parser, TOKEN_DEDENT);
    
    AST_T* else_branch = NULL;
    
    // Handle elif/else clauses
    if (parser->current_token->type == TOKEN_ELIF)
    {
        else_branch = parser_parse_if_statement(parser, scope); // Recursive for elif
    }
    else if (parser->current_token->type == TOKEN_ELSE)
    {
        parser_eat(parser, TOKEN_ELSE);
        
        if (parser->current_token->type == TOKEN_NEWLINE)
            parser_eat(parser, TOKEN_NEWLINE);
        if (parser->current_token->type == TOKEN_INDENT)
            parser_eat(parser, TOKEN_INDENT);
            
        else_branch = parser_parse_statements(parser, scope);
        
        if (parser->current_token->type == TOKEN_DEDENT)
            parser_eat(parser, TOKEN_DEDENT);
    }
    
    AST_T* if_stmt = init_ast(AST_IF_STATEMENT);
    if_stmt->condition = condition;
    if_stmt->then_branch = then_branch;
    if_stmt->else_branch = else_branch;
    if_stmt->scope = scope;
    
    return if_stmt;
}

/**
 * @brief Parse while loop
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* While loop AST node
 */
AST_T* parser_parse_while_loop(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_WHILE);
    
    AST_T* condition = parser_parse_expr(parser, scope);
    
    // Expect newline and indent for body
    if (parser->current_token->type == TOKEN_NEWLINE)
        parser_eat(parser, TOKEN_NEWLINE);
    if (parser->current_token->type == TOKEN_INDENT)
        parser_eat(parser, TOKEN_INDENT);
    
    AST_T* body = parser_parse_statements(parser, scope);
    
    if (parser->current_token->type == TOKEN_DEDENT)
        parser_eat(parser, TOKEN_DEDENT);
    
    AST_T* while_loop = init_ast(AST_WHILE_LOOP);
    while_loop->loop_condition = condition;
    while_loop->loop_body = body;
    while_loop->scope = scope;
    
    return while_loop;
}

/**
 * @brief Parse for loop
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* For loop AST node
 */
AST_T* parser_parse_for_loop(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_FOR);
    
    if (parser->current_token->type != TOKEN_ID)
    {
        printf("Expected variable name after 'for'\n");
        exit(1);
    }
    
    char* iterator_var = parser->current_token->value;
    parser_eat(parser, TOKEN_ID);
    
    parser_eat(parser, TOKEN_IN);
    
    AST_T* iterable = parser_parse_expr(parser, scope);
    
    // Expect newline and indent for body
    if (parser->current_token->type == TOKEN_NEWLINE)
        parser_eat(parser, TOKEN_NEWLINE);
    if (parser->current_token->type == TOKEN_INDENT)
        parser_eat(parser, TOKEN_INDENT);
    
    AST_T* body = parser_parse_statements(parser, scope);
    
    if (parser->current_token->type == TOKEN_DEDENT)
        parser_eat(parser, TOKEN_DEDENT);
    
    AST_T* for_loop = init_ast(AST_FOR_LOOP);
    for_loop->iterator_variable = calloc(strlen(iterator_var) + 1, sizeof(char));
    strcpy(for_loop->iterator_variable, iterator_var);
    for_loop->iterable = iterable;
    for_loop->for_body = body;
    for_loop->scope = scope;
    
    return for_loop;
}

/**
 * @brief Parse return statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Return statement AST node
 */
AST_T* parser_parse_return_statement(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_RETURN);
    
    AST_T* return_value = NULL;
    
    // Check if there's a return value
    if (parser->current_token->type != TOKEN_NEWLINE &&
        parser->current_token->type != TOKEN_EOF &&
        parser->current_token->type != TOKEN_DEDENT)
    {
        return_value = parser_parse_expr(parser, scope);
    }
    
    AST_T* return_stmt = init_ast(AST_RETURN);
    return_stmt->return_value = return_value;
    return_stmt->scope = scope;
    
    return return_stmt;
}

/**
 * @brief Parse break statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Break statement AST node
 */
AST_T* parser_parse_break_statement(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_BREAK);
    
    AST_T* break_stmt = init_ast(AST_BREAK);
    break_stmt->scope = scope;
    
    return break_stmt;
}

/**
 * @brief Parse continue statement
 * @param parser Parser instance
 * @param scope Scope context for parsing
 * @return AST_T* Continue statement AST node
 */
AST_T* parser_parse_continue_statement(parser_T* parser, scope_T* scope)
{
    parser_eat(parser, TOKEN_CONTINUE);
    
    AST_T* continue_stmt = init_ast(AST_CONTINUE);
    continue_stmt->scope = scope;
    
    return continue_stmt;
}

/* ============================================================================
 * UTILITY FUNCTIONS FOR EXPRESSION PARSING
 * ============================================================================ */

/**
 * @brief Get operator precedence
 * @param token_type Token type of operator
 * @return int Precedence level (higher = tighter binding)
 */
int parser_get_precedence(int token_type)
{
    switch (token_type)
    {
        case TOKEN_OR:          return 10;
        case TOKEN_AND:         return 20;
        case TOKEN_EQUALS:
        case TOKEN_NOT_EQUALS:  return 30;
        case TOKEN_LESS_THAN:
        case TOKEN_GREATER_THAN:
        case TOKEN_LESS_EQUALS:
        case TOKEN_GREATER_EQUALS: return 40;
        case TOKEN_PLUS:
        case TOKEN_MINUS:       return 50;
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO:      return 60;
        default:                return 0;
    }
}

/**
 * @brief Check if token is a binary operator
 * @param token_type Token type to check
 * @return int 1 if binary operator, 0 otherwise
 */
int parser_is_binary_operator(int token_type)
{
    return parser_get_precedence(token_type) > 0;
}

/**
 * @brief Peek ahead to check if current position starts an object literal
 * @param parser Parser instance
 * @return int 1 if object literal detected, 0 otherwise
 */
int parser_peek_for_object_literal(parser_T* parser)
{
    // Object literal pattern: key value, key value, ...
    // Look for: ID followed by (ID, STRING, NUMBER, TRUE, FALSE, NULL)
    // then followed by COMMA or end of statement
    if (parser->current_token->type == TOKEN_ID)
    {
        // Save current state
        lexer_T* saved_lexer = parser->lexer;
        token_T* saved_current = parser->current_token;
        token_T* saved_prev = parser->prev_token;
        
        // Look ahead to see if this is a key-value pattern
        token_T* next_token = lexer_get_next_token(parser->lexer);
        
        
        // Restore state
        parser->lexer = saved_lexer;
        parser->current_token = saved_current;
        parser->prev_token = saved_prev;
        
        // Check if next token is a valid value for object literal
        if (next_token->type == TOKEN_STRING || 
            next_token->type == TOKEN_NUMBER ||
            next_token->type == TOKEN_TRUE || 
            next_token->type == TOKEN_FALSE ||
            next_token->type == TOKEN_NULL ||
            next_token->type == TOKEN_ID) // ID could be a variable reference as value
        {
            // This looks like a key-value pair, so it's likely an object literal
            return 1;
        }
    }
    return 0;
}