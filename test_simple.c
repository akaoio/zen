#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/ast.h"
#include "src/include/zen/core/scope.h"

int main() {
    printf("AST_VARIABLE_DEFINITION = %d\n", AST_VARIABLE_DEFINITION);
    printf("AST_FUNCTION_DEFINITION = %d\n", AST_FUNCTION_DEFINITION);
    printf("AST_VARIABLE = %d\n", AST_VARIABLE);
    printf("AST_FUNCTION_CALL = %d\n", AST_FUNCTION_CALL);
    
    char* input = "variable_name";
    lexer_T* lexer = lexer_new(input);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    
    AST_T* ast = parser_parse_id(parser, scope);
    
    if (ast) {
        printf("AST type: %d\n", ast->type);
        printf("Expected AST_VARIABLE (%d)\n", AST_VARIABLE);
        ast_free(ast);
    } else {
        printf("AST is NULL\n");
    }
    
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    return 0;
}