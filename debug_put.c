#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/ast.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>

int main() {
    printf("Debug: Testing PUT parsing\n");
    
    const char* input = "put \"test.json\" data";
    
    printf("Debug: Creating lexer\n");
    lexer_T* lexer = lexer_new(input);
    if (!lexer) {
        printf("Error: Failed to create lexer\n");
        return 1;
    }
    
    printf("Debug: Creating parser\n");
    parser_T* parser = parser_new(lexer);
    if (!parser) {
        printf("Error: Failed to create parser\n");
        lexer_free(lexer);
        return 1;
    }
    
    printf("Debug: Creating scope\n");
    scope_T* scope = scope_new(NULL);
    if (!scope) {
        printf("Error: Failed to create scope\n");
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    printf("Debug: Parsing statement\n");
    AST_T* ast = parser_parse_statement(parser, scope);
    if (!ast) {
        printf("Error: Failed to parse statement\n");
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    printf("Debug: Successfully parsed AST type: %d\n", ast->type);
    printf("Debug: Freeing resources\n");
    
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    printf("Debug: Test completed successfully\n");
    return 0;
}