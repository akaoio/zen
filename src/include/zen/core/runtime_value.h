#ifndef RUNTIME_VALUE_H
#define RUNTIME_VALUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct RUNTIME_VALUE_STRUCT RuntimeValue;

typedef enum {
    RV_NULL,
    RV_NUMBER,
    RV_STRING,
    RV_BOOLEAN,
    RV_ARRAY,
    RV_OBJECT,
    RV_FUNCTION,
    RV_ERROR
} RuntimeValueType;

struct RUNTIME_VALUE_STRUCT {
    RuntimeValueType type;
    int ref_count;

    union {
        double number;
        struct {
            char *data;
            size_t length;
        } string;
        bool boolean;
        struct {
            RuntimeValue **elements;
            size_t count;
            size_t capacity;
        } array;
        struct {
            char **keys;
            RuntimeValue **values;
            size_t count;
            size_t capacity;
        } object;
        struct {
            void *ast_node;  // AST_T* but avoiding circular dependency
            void *scope;     // scope_T*
        } function;
        struct {
            char *message;
            int code;
        } error;
    } data;
};

// Creation functions
RuntimeValue *rv_new_null(void);
RuntimeValue *rv_new_number(double value);
RuntimeValue *rv_new_string(const char *value);
RuntimeValue *rv_new_boolean(bool value);
RuntimeValue *rv_new_array(void);
RuntimeValue *rv_new_object(void);
RuntimeValue *rv_new_function(void *ast_node, void *scope);
RuntimeValue *rv_new_error(const char *message, int code);

// Reference counting
RuntimeValue *rv_ref(RuntimeValue *value);
void rv_unref(RuntimeValue *value);

// Array operations
void rv_array_push(RuntimeValue *array, RuntimeValue *element);
RuntimeValue *rv_array_get(RuntimeValue *array, size_t index);
void rv_array_set(RuntimeValue *array, size_t index, RuntimeValue *element);
size_t rv_array_length(RuntimeValue *array);

// Object operations
void rv_object_set(RuntimeValue *object, const char *key, RuntimeValue *value);
RuntimeValue *rv_object_get(RuntimeValue *object, const char *key);
bool rv_object_has(RuntimeValue *object, const char *key);
void rv_object_delete(RuntimeValue *object, const char *key);

// Type checking
bool rv_is_null(RuntimeValue *value);
bool rv_is_number(RuntimeValue *value);
bool rv_is_string(RuntimeValue *value);
bool rv_is_boolean(RuntimeValue *value);
bool rv_is_array(RuntimeValue *value);
bool rv_is_object(RuntimeValue *value);
bool rv_is_function(RuntimeValue *value);
bool rv_is_error(RuntimeValue *value);
bool rv_is_truthy(RuntimeValue *value);

// Utility functions
RuntimeValue *rv_copy(RuntimeValue *value);
char *rv_to_string(RuntimeValue *value);
const char *rv_get_string(RuntimeValue *value);
bool rv_equals(RuntimeValue *a, RuntimeValue *b);
const char *rv_type_name(RuntimeValue *value);

#endif