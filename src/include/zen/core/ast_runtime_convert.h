#ifndef AST_RUNTIME_CONVERT_H
#define AST_RUNTIME_CONVERT_H

#include "zen/core/ast.h"
#include "zen/core/runtime_value.h"

/**
 * Convert an AST node to a RuntimeValue
 */
RuntimeValue *ast_to_runtime_value(AST_T *ast);

/**
 * Convert a RuntimeValue to an AST node
 * NOTE: This creates a new AST node that must be freed
 */
AST_T *runtime_value_to_ast(RuntimeValue *rv);

#endif