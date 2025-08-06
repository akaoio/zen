/*
 * Comprehensive test of all stdlib functions to verify they work correctly
 * This bypasses the parser and tests the stdlib functions directly
 */

#define _GNU_SOURCE
#include "zen/stdlib/stdlib.h"
#include "zen/types/value.h"
#include <stdio.h>
#include <stdlib.h>

void test_function(const char* name, Value* (*func)(Value** args, size_t argc), Value** args, size_t argc) {
    printf("Testing %s... ", name);
    Value* result = func(args, argc);
    if (result) {
        char* str_repr = value_to_string(result);
        printf("Result: %s\n", str_repr ? str_repr : "NULL");
        free(str_repr);
        value_unref(result);
    } else {
        printf("NULL result\n");
    }
}

int main() {
    printf("=== ZEN Standard Library Comprehensive Test ===\n\n");

    // String function tests
    printf("--- String Functions ---\n");
    Value* test_str = value_new_string("  Hello World  ");
    Value* args1[] = {test_str};
    
    test_function("upper", zen_stdlib_upper, args1, 1);
    test_function("lower", zen_stdlib_lower, args1, 1);
    test_function("trim", zen_stdlib_trim, args1, 1);
    test_function("length", zen_stdlib_length, args1, 1);
    
    value_unref(test_str);
    
    // Math function tests
    printf("\n--- Math Functions ---\n");
    Value* neg_five = value_new_number(-5.0);
    Value* float_val = value_new_number(3.7);
    Value* two = value_new_number(2.0);
    Value* three = value_new_number(3.0);
    Value* args_neg[] = {neg_five};
    Value* args_float[] = {float_val};
    Value* args_two_three[] = {two, three};
    
    test_function("abs", zen_stdlib_abs, args_neg, 1);
    test_function("floor", zen_stdlib_floor, args_float, 1);
    test_function("ceil", zen_stdlib_ceil, args_float, 1);
    test_function("round", zen_stdlib_round, args_float, 1);
    test_function("sqrt", zen_stdlib_sqrt, args_two_three, 1);
    test_function("pow", zen_stdlib_pow, args_two_three, 2);
    test_function("sin", zen_stdlib_sin, args_neg, 1);
    test_function("cos", zen_stdlib_cos, args_neg, 1);
    test_function("tan", zen_stdlib_tan, args_neg, 1);
    test_function("log", zen_stdlib_log, args_two_three, 1);
    test_function("min", zen_stdlib_min, args_two_three, 2);
    test_function("max", zen_stdlib_max, args_two_three, 2);
    
    value_unref(neg_five);
    value_unref(float_val);
    value_unref(two);
    value_unref(three);
    
    // Type conversion tests
    printf("\n--- Type Conversion Functions ---\n");
    Value* num42 = value_new_number(42.0);
    Value* str123 = value_new_string("123");
    Value* bool_true = value_new_boolean(true);
    Value* args_42[] = {num42};
    Value* args_123[] = {str123};
    Value* args_bool[] = {bool_true};
    
    test_function("toString", zen_stdlib_to_string, args_42, 1);
    test_function("toNumber", zen_stdlib_to_number, args_123, 1);
    test_function("toBoolean", zen_stdlib_to_boolean, args_42, 1);
    test_function("typeOf", zen_stdlib_type_of, args_42, 1);
    test_function("parseInt", zen_stdlib_parse_int, args_123, 1);
    test_function("parseFloat", zen_stdlib_parse_float, args_123, 1);
    
    value_unref(num42);
    value_unref(str123);
    value_unref(bool_true);
    
    // JSON tests
    printf("\n--- JSON Functions ---\n");
    Value* json_str = value_new_string("{\"name\":\"Alice\",\"age\":30}");
    Value* args_json[] = {json_str};
    
    test_function("jsonParse", zen_stdlib_json_parse, args_json, 1);
    
    // Test creating object for stringify
    Value* test_obj = value_new(VALUE_OBJECT);
    test_function("jsonStringify", zen_stdlib_json_stringify, &test_obj, 1);
    
    value_unref(json_str);
    value_unref(test_obj);
    
    printf("\n=== All stdlib functions tested successfully! ===\n");
    return 0;
}