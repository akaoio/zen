#include <stdio.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/scope.h"
#include "zen/core/ast.h"

int main() {
    char *code = "import \"test_module.zen\"\ntry\n    null.x 1 2\ncatch e\n    print \"caught\"";
    
    lexer_T *lexer = lexer_new(code);
    parser_T *parser = parser_new(lexer);
    scope_T *scope = scope_new();
    
    AST_T *ast = parser_parse(parser, scope);
    
    printf("Parsed successfully, AST type: %d\n", ast ? ast->type : -1);
    
    // Don't execute, just exit
    if (ast) ast_free(ast);
    scope_free(scope);
    parser_free(parser);
    lexer_free(lexer);
    
    return 0;
}