#include "zen/core/ast.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

RuntimeValue *ast_to_runtime_value(AST_T *ast)
{
    if (!ast)
        return rv_new_null();

    switch (ast->type) {
    case AST_NUMBER:
        return rv_new_number(ast->number_value);

    case AST_STRING:
        return rv_new_string(ast->string_value);

    case AST_BOOLEAN:
        return rv_new_boolean(ast->boolean_value);

    case AST_NULL:
    case AST_UNDECIDABLE:
        return rv_new_null();

    case AST_ARRAY: {
        // Create a new array RuntimeValue
        RuntimeValue *array = rv_new_array();
        // Add each element
        for (size_t i = 0; i < ast->array_size; i++) {
            RuntimeValue *element = ast_to_runtime_value(ast->array_elements[i]);
            rv_array_push(array, element);
            rv_unref(element);
        }
        return array;
    }

    case AST_OBJECT: {
        // Create a new object RuntimeValue
        RuntimeValue *object = rv_new_object();
        // Add each key-value pair
        for (size_t i = 0; i < ast->object_size; i++) {
            RuntimeValue *value = ast_to_runtime_value(ast->object_values[i]);
            rv_object_set(object, ast->object_keys[i], value);
            rv_unref(value);
        }
        return object;
    }

    case AST_FUNCTION_DEFINITION:
        return rv_new_function(ast, ast->scope);

    default:
        // For other node types, return null
        return rv_new_null();
    }
}

AST_T *runtime_value_to_ast(RuntimeValue *rv)
{
    if (!rv)
        return ast_new(AST_NULL);

    switch (rv->type) {
    case RV_NULL:
        return ast_new(AST_NULL);

    case RV_NUMBER: {
        AST_T *node = ast_new(AST_NUMBER);
        node->number_value = rv->data.number;
        return node;
    }

    case RV_STRING: {
        AST_T *node = ast_new(AST_STRING);
        node->string_value = memory_strdup(rv->data.string.data);
        return node;
    }

    case RV_BOOLEAN: {
        AST_T *node = ast_new(AST_BOOLEAN);
        node->boolean_value = rv->data.boolean;
        return node;
    }

    case RV_ARRAY: {
        AST_T *node = ast_new(AST_ARRAY);
        node->array_size = rv->data.array.count;
        if (node->array_size > 0) {
            node->array_elements = memory_alloc(sizeof(AST_T *) * node->array_size);
            for (size_t i = 0; i < node->array_size; i++) {
                node->array_elements[i] = runtime_value_to_ast(rv->data.array.elements[i]);
            }
        } else {
            node->array_elements = NULL;
        }
        return node;
    }

    case RV_OBJECT: {
        AST_T *node = ast_new(AST_OBJECT);
        node->object_size = rv->data.object.count;
        if (node->object_size > 0) {
            node->object_keys = memory_alloc(sizeof(char *) * node->object_size);
            node->object_values = memory_alloc(sizeof(AST_T *) * node->object_size);
            for (size_t i = 0; i < node->object_size; i++) {
                node->object_keys[i] = memory_strdup(rv->data.object.keys[i]);
                node->object_values[i] = runtime_value_to_ast(rv->data.object.values[i]);
            }
        } else {
            node->object_keys = NULL;
            node->object_values = NULL;
        }
        return node;
    }

    case RV_FUNCTION:
        // Return the stored AST function definition
        return (AST_T *)rv->data.function.ast_node;

    default:
        return ast_new(AST_NULL);
    }
}