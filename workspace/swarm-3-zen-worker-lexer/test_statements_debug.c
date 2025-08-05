#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/ast.h"

int main() {
    printf("Testing parser_parse_statements with 'set x 42'...\n");
    
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
    
    printf("Current token before parse_statements: type=%d, value='%s'\n", 
           parser->current_token->type, 
           parser->current_token->value ? parser->current_token->value : "(null)");
    
    printf("About to call parser_parse_statements...\n");
    AST_T* ast = parser_parse_statements(parser, parser->scope);
    
    if (ast) {
        printf("Successfully parsed statements! AST type: %d, compound_size: %zu\n", 
               ast->type, ast->compound_size);
        
        if (ast->compound_size > 0) {
            printf("First compound element type: %d\n", ast->compound_value[0]->type);
        }
    } else {
        printf("Failed to parse statements\n");
    }
    
    return 0;
}