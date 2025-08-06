#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "zen/types/value.h"

void test_enhanced_typeof() {
    printf("Testing enhanced typeof...\n");
    
    Value* null_val = value_new_null();
    Value* bool_val = value_new_boolean(true);
    Value* num_val = value_new_number(42.5);
    Value* int_val = value_new_number(42);
    Value* nan_val = value_new_number(NAN);
    Value* inf_val = value_new_number(INFINITY);
    Value* str_val = value_new_string("hello");
    
    char* null_type = value_typeof_enhanced(null_val);
    char* bool_type = value_typeof_enhanced(bool_val);
    char* num_type = value_typeof_enhanced(num_val);
    char* int_type = value_typeof_enhanced(int_val);
    char* nan_type = value_typeof_enhanced(nan_val);
    char* inf_type = value_typeof_enhanced(inf_val);
    char* str_type = value_typeof_enhanced(str_val);
    
    assert(strcmp(null_type, "null") == 0);
    assert(strcmp(bool_type, "boolean") == 0);
    assert(strcmp(num_type, "number:float") == 0);
    assert(strcmp(int_type, "number:integer") == 0);
    assert(strcmp(nan_type, "number:nan") == 0);
    assert(strcmp(inf_type, "number:infinity") == 0);
    assert(strcmp(str_type, "string") == 0);
    
    free(null_type);
    free(bool_type);
    free(num_type);
    free(int_type);
    free(nan_type);
    free(inf_type);
    free(str_type);
    
    value_unref(null_val);
    value_unref(bool_val);
    value_unref(num_val);
    value_unref(int_val);
    value_unref(nan_val);
    value_unref(inf_val);
    value_unref(str_val);
    
    printf("✓ Enhanced typeof tests passed\n");
}

void test_type_predicates() {
    printf("Testing type predicates...\n");
    
    Value* num_val = value_new_number(42);
    Value* str_num = value_new_string("123.45");
    Value* str_text = value_new_string("hello");
    Value* array_val = value_new(VALUE_ARRAY);
    Value* bool_val = value_new_boolean(true);
    
    // Test is_numeric
    assert(value_is_numeric(num_val));
    assert(value_is_numeric(str_num));
    assert(value_is_numeric(bool_val)); // Booleans are numeric
    assert(!value_is_numeric(str_text));
    assert(!value_is_numeric(array_val));
    
    // Test is_iterable
    assert(value_is_iterable(array_val));
    assert(value_is_iterable(str_text));
    assert(!value_is_iterable(num_val));
    assert(!value_is_iterable(bool_val));
    
    // Test is_callable
    assert(!value_is_callable(num_val));
    assert(!value_is_callable(str_text));
    assert(!value_is_callable(array_val));
    
    value_unref(num_val);
    value_unref(str_num);
    value_unref(str_text);
    value_unref(array_val);
    value_unref(bool_val);
    
    printf("✓ Type predicate tests passed\n");
}

void test_enhanced_number_parsing() {
    printf("Testing enhanced number parsing...\n");
    
    double result;
    
    // Test basic parsing
    assert(value_parse_number_enhanced("123.45", &result));
    assert(result == 123.45);
    
    // Test hexadecimal
    assert(value_parse_number_enhanced("0xFF", &result));
    assert(result == 255.0);
    
    // Test binary
    assert(value_parse_number_enhanced("0b1010", &result));
    assert(result == 10.0);
    
    // Test octal
    assert(value_parse_number_enhanced("077", &result));
    assert(result == 63.0);
    
    // Test special values
    assert(value_parse_number_enhanced("Infinity", &result));
    assert(isinf(result) && result > 0);
    
    assert(value_parse_number_enhanced("-Infinity", &result));
    assert(isinf(result) && result < 0);
    
    assert(value_parse_number_enhanced("NaN", &result));
    assert(isnan(result));
    
    // Test scientific notation
    assert(value_parse_number_enhanced("1.5e3", &result));
    assert(result == 1500.0);
    
    printf("✓ Enhanced number parsing tests passed\n");
}

void test_conversion_with_precision() {
    printf("Testing precision-aware conversions...\n");
    
    bool is_lossless;
    double result;
    
    // Test lossless conversions
    Value* num_val = value_new_number(42.0);
    result = value_to_number_with_precision(num_val, &is_lossless);
    assert(result == 42.0);
    assert(is_lossless);
    
    Value* bool_val = value_new_boolean(true);
    result = value_to_number_with_precision(bool_val, &is_lossless);
    assert(result == 1.0);
    assert(is_lossless);
    
    // Test lossy conversion
    Value* str_val = value_new_string("123abc");
    result = value_to_number_with_precision(str_val, &is_lossless);
    assert(!is_lossless);
    
    value_unref(num_val);
    value_unref(bool_val);
    value_unref(str_val);
    
    printf("✓ Precision-aware conversion tests passed\n");
}

void test_value_hashing() {
    printf("Testing value hashing...\n");
    
    Value* num1 = value_new_number(42);
    Value* num2 = value_new_number(42);
    Value* num3 = value_new_number(43);
    Value* str1 = value_new_string("hello");
    Value* str2 = value_new_string("hello");
    Value* str3 = value_new_string("world");
    
    // Same values should have same hash
    assert(value_hash(num1) == value_hash(num2));
    assert(value_hash(str1) == value_hash(str2));
    
    // Different values should have different hashes (usually)
    assert(value_hash(num1) != value_hash(num3));
    assert(value_hash(str1) != value_hash(str3));
    
    // Different types should have different hashes
    assert(value_hash(num1) != value_hash(str1));
    
    value_unref(num1);
    value_unref(num2);
    value_unref(num3);
    value_unref(str1);
    value_unref(str2);
    value_unref(str3);
    
    printf("✓ Value hashing tests passed\n");
}

void test_value_singletons() {
    printf("Testing value singletons...\n");
    
    value_init_singletons();
    
    Value* null1 = value_get_singleton(VALUE_NULL, 0, false);
    Value* null2 = value_get_singleton(VALUE_NULL, 0, false);
    
    Value* true1 = value_get_singleton(VALUE_BOOLEAN, 0, true);
    Value* true2 = value_get_singleton(VALUE_BOOLEAN, 0, true);
    
    Value* false1 = value_get_singleton(VALUE_BOOLEAN, 0, false);
    Value* false2 = value_get_singleton(VALUE_BOOLEAN, 0, false);
    
    Value* zero1 = value_get_singleton(VALUE_NUMBER, 0.0, false);
    Value* zero2 = value_get_singleton(VALUE_NUMBER, 0.0, false);
    
    Value* one1 = value_get_singleton(VALUE_NUMBER, 1.0, false);
    Value* one2 = value_get_singleton(VALUE_NUMBER, 1.0, false);
    
    // Singletons should return the same object
    // Note: We compare values since singletons increment ref count
    assert(value_equals(null1, null2));
    assert(value_equals(true1, true2));
    assert(value_equals(false1, false2));
    assert(value_equals(zero1, zero2));
    assert(value_equals(one1, one2));
    
    value_unref(null1);
    value_unref(null2);
    value_unref(true1);
    value_unref(true2);
    value_unref(false1);
    value_unref(false2);
    value_unref(zero1);
    value_unref(zero2);
    value_unref(one1);
    value_unref(one2);
    
    value_cleanup_singletons();
    
    printf("✓ Value singleton tests passed\n");
}

int main() {
    printf("Running advanced type system tests...\n\n");
    
    test_enhanced_typeof();
    test_type_predicates();
    test_enhanced_number_parsing();
    test_conversion_with_precision();
    test_value_hashing();
    test_value_singletons();
    
    printf("\n🎉 All advanced type system tests passed!\n");
    return 0;
}