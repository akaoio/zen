#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"

void test_basic_parsing() {
    printf("Testing basic parser functionality...\n");
    
    // Test simple expression
    const char* code = "set x 5 + 3";
    lexer_T* lexer = init_lexer(code);
    assert(lexer != NULL);
    
    parser_T* parser = init_parser(lexer);
    assert(parser != NULL);
    
    // Test parsing
    AST_T* ast = parser_parse(parser, parser->scope);
    assert(ast != NULL);
    
    printf("✓ Basic parsing successful\n");
    
    // Cleanup
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
}

void test_expression_parsing() {
    printf("Testing expression parsing...\n");
    
    const char* code = "x + y * 2";
    lexer_T* lexer = init_lexer(code);
    parser_T* parser = init_parser(lexer);
    
    // Test direct expression parsing
    AST_T* expr = parser_parse_expr(parser, parser->scope);
    assert(expr != NULL);
    assert(expr->type == AST_BINARY_OP);
    
    printf("✓ Expression parsing successful\n");
    
    ast_free(expr);
    parser_free(parser);
    lexer_free(lexer);
}

void test_array_parsing() {
    printf("Testing array parsing...\n");
    
    const char* code = "1, 2, 3, 4";
    lexer_T* lexer = init_lexer(code);
    parser_T* parser = init_parser(lexer);
    
    AST_T* array = parser_parse_array(parser, parser->scope);
    assert(array != NULL);
    assert(array->type == AST_ARRAY);
    
    printf("✓ Array parsing successful\n");
    
    ast_free(array);
    parser_free(parser);
    lexer_free(lexer);
}

void test_object_parsing() {
    printf("Testing object parsing...\n");
    
    const char* code = "name \"John\", age 30";
    lexer_T* lexer = init_lexer(code);
    parser_T* parser = init_parser(lexer);
    
    AST_T* obj = parser_parse_object(parser, parser->scope);
    assert(obj != NULL);
    assert(obj->type == AST_OBJECT);
    
    printf("✓ Object parsing successful\n");
    
    ast_free(obj);
    parser_free(parser);
    lexer_free(lexer);
}

int main() {
    printf("=== Parser Functionality Tests ===\n");
    
    test_basic_parsing();
    test_expression_parsing();
    test_array_parsing();
    test_object_parsing();
    
    printf("\n✓ All parser tests passed!\n");
    return 0;
}