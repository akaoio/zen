#include <stdio.h>
#include <stdlib.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/scope.h"

int main() {
    char* input = "set a true";
    printf("Input: %s\n", input);
    
    lexer_T* lexer = init_lexer(input);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    
    printf("First token type: %d\n", parser->current_token->type);
    printf("First token value: %s\n", parser->current_token->value);
    
    // Try to parse the statement
    AST_T* ast = parser_parse_statement(parser, scope);
    
    if (ast) {
        printf("Statement parsed successfully, type: %d\n", ast->type);
    } else {
        printf("Failed to parse statement\n");
    }
    
    return 0;
}