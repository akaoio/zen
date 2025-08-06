#include <stdio.h>
#include "zen/core/visitor.h"
#include "zen/core/lexer.h" 
#include "zen/core/parser.h"

int main() {
    char* input = "function test\n    print \"hello\"\n\ntest";
    
    printf("Parsing: %s\n", input);
    
    lexer_T* lexer = init_lexer(input);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("Failed to create parser\n");
        return 1;
    }
    
    printf("Starting parse...\n");
    AST_T* ast = parser_parse(parser, parser->scope);
    if (!ast) {
        printf("Failed to parse\n");
        return 1;
    }
    
    printf("Parse completed, creating visitor...\n");
    visitor_T* visitor = init_visitor();
    if (!visitor) {
        printf("Failed to create visitor\n");
        return 1;
    }
    
    printf("Starting visitor...\n");
    AST_T* result = visitor_visit(visitor, ast);
    printf("Visitor completed with result: %p\n", (void*)result);
    
    return 0;
}