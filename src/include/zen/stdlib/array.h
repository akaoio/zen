#ifndef ZEN_STDLIB_ARRAY_H
#define ZEN_STDLIB_ARRAY_H

#include "zen/core/runtime_value.h"

#include <stddef.h>

// Array manipulation functions
RuntimeValue *array_length(RuntimeValue **args, size_t argc);
RuntimeValue *array_push(RuntimeValue **args, size_t argc);
RuntimeValue *array_pop(RuntimeValue **args, size_t argc);
RuntimeValue *array_shift(RuntimeValue **args, size_t argc);
RuntimeValue *array_unshift(RuntimeValue **args, size_t argc);
RuntimeValue *array_slice(RuntimeValue **args, size_t argc);
RuntimeValue *array_concat(RuntimeValue **args, size_t argc);
RuntimeValue *array_reverse(RuntimeValue **args, size_t argc);
RuntimeValue *array_sort(RuntimeValue **args, size_t argc);
RuntimeValue *array_includes(RuntimeValue **args, size_t argc);
RuntimeValue *array_index_of(RuntimeValue **args, size_t argc);
RuntimeValue *array_join(RuntimeValue **args, size_t argc);

#endif  // ZEN_STDLIB_ARRAY_H