#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include "zen/stdlib/io.h"

int main() {
    memory_debug_enable(true);
    
    printf("1. Reading file...\n");
    char* file_contents = get_file_contents("test_simple_new.zen");
    if (!file_contents) {
        printf("Failed to read file\n");
        return 1;
    }
    printf("   File contents: '%s'\n", file_contents);
    
    printf("2. Creating lexer...\n");
    lexer_T* lexer = init_lexer(file_contents);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }
    printf("   Lexer created\n");
    
    printf("3. Creating parser...\n");
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("Failed to create parser\n");
        return 1;
    }
    printf("   Parser created\n");
    
    printf("4. Creating global scope...\n");
    scope_T* global_scope = init_scope();
    if (!global_scope) {
        printf("Failed to create scope\n");
        return 1;
    }
    printf("   Scope created\n");
    
    printf("5. Parsing statements...\n");
    AST_T* root = parser_parse_statements(parser, global_scope);
    if (!root) {
        printf("Failed to parse statements\n");
        return 1;
    }
    printf("   AST created, type: %d\n", root->type);
    
    printf("6. Creating visitor...\n");
    visitor_T* visitor = init_visitor();
    if (!visitor) {
        printf("Failed to create visitor\n");
        return 1;
    }
    printf("   Visitor created\n");
    
    printf("7. Executing AST...\n");
    AST_T* result = visitor_visit(visitor, root);
    printf("   Execution complete, result type: %d\n", result ? result->type : -1);
    
    return 0;
}