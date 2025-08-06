#include "zen/core/parser.h"
#include "zen/core/lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_feature(const char* name, const char* code) {
    printf("--- Testing %s ---\n", name);
    printf("Code: %s\n", code);
    
    size_t len = strlen(code) + 1;
    char* mutable_code = malloc(len);
    strcpy(mutable_code, code);
    lexer_T* lexer = init_lexer(mutable_code);
    parser_T* parser = init_parser(lexer);
    
    AST_T* ast = parser_parse_statements(parser, parser->scope);
    
    printf("Result: %s\n", ast ? "PARSED SUCCESSFULLY" : "PARSE FAILED");
    if (ast) {
        printf("AST Type: %d\n", ast->type);
    }
    
    parser_free(parser);
    lexer_free(lexer);
    free(mutable_code);
    printf("\n");
}

int main() {
    printf("=== Testing Advanced Syntax Features ===\n\n");
    
    // Test 1: Ternary operator
    test_feature("Ternary Operator", 
                 "set condition true\nset value condition ? 100 : 200\nprint value");
    
    // Test 2: Compound assignment
    test_feature("Compound Assignment", 
                 "set a 10\na += 5\na *= 2\nprint a");
    
    // Test 3: Null coalescing
    test_feature("Null Coalescing", 
                 "set x null\nset y x ?? 42\nprint y");
    
    // Test 4: Optional chaining
    test_feature("Optional Chaining", 
                 "set obj null\nset result obj?.value\nprint result");
    
    // Test 5: Range expressions
    test_feature("Range Expression", 
                 "set range 1..10\nprint range");
    
    // Test 6: Spread operator
    test_feature("Spread Operator", 
                 "set arr [1, 2, 3]\nset arr2 [...arr, 4, 5]\nprint arr2");
    
    // Test 7: Destructuring
    test_feature("Destructuring Assignment", 
                 "set [a, b] [10, 20]\nprint a\nprint b");
    
    // Test 8: Lambda expressions
    test_feature("Lambda Expression", 
                 "set add x, y => x + y\nset result add 5, 3\nprint result");
    
    printf("=== Advanced Syntax Testing Complete ===\n");
    
    return 0;
}