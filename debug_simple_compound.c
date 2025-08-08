#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h" 
#include "src/include/zen/core/ast.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include <stdio.h>

int main() {
    printf("Debug: Testing simple object parsing\n");
    
    const char* input = "set data name \"test\", value 42";
    
    printf("Debug: Input: %s\n", input);
    
    lexer_T* lexer = lexer_new(input);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new(NULL);
    
    AST_T* ast = parser_parse_statement(parser, scope);
    if (!ast) {
        printf("Error: Failed to parse statement\n");
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return 1;
    }
    
    printf("Debug: AST type: %d\n", ast->type);
    if (ast->type == AST_VARIABLE_DEFINITION) {
        printf("Debug: Variable name: %s\n", ast->variable_definition_variable_name ? ast->variable_definition_variable_name : "NULL");
        if (ast->variable_definition_value) {
            printf("Debug: Value AST type: %d\n", ast->variable_definition_value->type);
            if (ast->variable_definition_value->type == AST_OBJECT) {
                printf("Debug: Object size: %zu\n", ast->variable_definition_value->object_size);
            }
        }
    }
    
    ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    printf("Debug: Test completed successfully\n");
    return 0;
}