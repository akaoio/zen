#include "zen/types/value.h"
#include "zen/core/ast.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declare the functions from visitor.c that we need
AST_T* value_to_ast(Value* value);

int main() {
    // Simulate what happens in the visitor for zen_string_upper
    Value* test_str = value_new_string("Hello World");
    printf("1. Original string: %s\n", test_str->as.string->data);
    
    // Call zen_string_upper directly (simulate stdlib wrapper)
    Value* upper_result = zen_string_upper(test_str);
    printf("2. zen_string_upper result type: %d\n", upper_result->type);
    if (upper_result->type == VALUE_STRING && upper_result->as.string) {
        printf("   Upper string: %s\n", upper_result->as.string->data);
    }
    
    // Convert back to AST (this is what the visitor does)
    AST_T* ast_result = value_to_ast(upper_result);
    printf("3. value_to_ast result type: %d\n", ast_result->type);
    if (ast_result->type == AST_STRING && ast_result->string_value) {
        printf("   AST string: %s\n", ast_result->string_value);
    } else if (ast_result->type == AST_OBJECT) {
        printf("   ERROR: Got object instead of string!\n");
    }
    
    // Clean up
    value_unref(test_str);
    value_unref(upper_result);
    ast_free(ast_result);
    return 0;
}