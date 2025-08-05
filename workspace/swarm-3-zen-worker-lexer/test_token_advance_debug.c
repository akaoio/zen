#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/ast.h"

int main() {
    printf("Testing token advancement with 'set x 42'...\n");
    
    lexer_T* lexer = init_lexer("set x 42");
    if (!lexer) {
        printf("ERROR: Failed to initialize lexer\n");
        return 1;
    }
    
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("ERROR: Failed to initialize parser\n");
        return 1;
    }
    
    printf("Token before parsing: type=%d, value='%s'\n", 
           parser->current_token->type, 
           parser->current_token->value ? parser->current_token->value : "(null)");
    
    // Parse one statement
    AST_T* ast = parser_parse_statement(parser, parser->scope);
    
    printf("Token after parsing: type=%d, value='%s'\n", 
           parser->current_token->type, 
           parser->current_token->value ? parser->current_token->value : "(null)");
    
    if (ast) {
        printf("Statement parsed successfully, type: %d\n", ast->type);
    } else {
        printf("Failed to parse statement\n");
    }
    
    // Check if we're at EOF
    if (parser->current_token->type == 0) { // TOKEN_EOF
        printf("✓ Correctly reached EOF\n");
    } else {
        printf("⚠ Not at EOF - this could cause infinite loop!\n");
    }
    
    return 0;
}