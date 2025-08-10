#include <stdio.h>
#include "zen/core/ast.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"

int main() {
    const char *source = "print \"Hello\"\\nset x 10";
    
    lexer_T *lexer = lexer_new(source);
    scope_T *scope = scope_new();
    parser_T *parser = parser_new(lexer);
    
    AST_T *ast = parser_parse(parser, scope);
    
    printf("AST type: %d\n", ast ? ast->type : -1);
    if (ast && ast->type == AST_COMPOUND) {
        printf("Compound size: %zu\n", ast->compound_size);
        printf("Compound statements: %p\n", (void*)ast->compound_statements);
    }
    
    parser_free(parser);
    lexer_free(lexer);
    scope_free(scope);
    if (ast) ast_free(ast);
    
    return 0;
}