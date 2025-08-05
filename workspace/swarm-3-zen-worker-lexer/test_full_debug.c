#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/ast.h"

int main() {
    printf("Testing full pipeline with 'set x 42'...\n");
    
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
    
    printf("About to parse...\n");
    AST_T* ast = parser_parse(parser, parser->scope);
    
    if (!ast) {
        printf("ERROR: Failed to parse\n");
        return 1;
    }
    
    printf("Successfully parsed! AST type: %d\n", ast->type);
    
    visitor_T* visitor = init_visitor();
    if (!visitor) {
        printf("ERROR: Failed to initialize visitor\n");
        return 1;
    }
    
    printf("About to visit AST...\n");
    AST_T* result = visitor_visit(visitor, ast);
    
    if (result) {
        printf("Successfully visited AST! Result type: %d\n", result->type);
    } else {
        printf("Failed to visit AST\n");
    }
    
    return 0;
}