#include <stdio.h>
#include "zen/core/ast.h"

// Debug function to print AST type
void debug_ast_type(int type) {
    switch(type) {
        case 1: printf("AST_COMPOUND\n"); break;
        case 2: printf("AST_ASSIGNMENT\n"); break;
        case 3: printf("AST_FUNCTION_CALL\n"); break;
        case 4: printf("AST_PROPERTY_ACCESS\n"); break;
        case 5: printf("AST_STRING\n"); break;
        case 6: printf("AST_NUMBER\n"); break;
        default: printf("AST_TYPE_%d\n", type); break;
    }
}