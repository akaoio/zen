/**
 * @file test_value_basic.c
 * @brief Basic value system tests
 */

#include "../../framework/test.h"
#include "zen/types/value.h"

TEST_SUITE(value_basic_tests)

TEST(test_value_new_string) {
    Value* value = value_new_string("Hello World");
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, VALUE_STRING);
    ASSERT_NOT_NULL(value->as.string);
    ASSERT_STR_EQ(value->as.string->data, "Hello World");
    ASSERT_EQ(value->as.string->length, 11);
    
    value_unref(value);
}

TEST(test_value_new_number) {
    Value* value = value_new_number(42.5);
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(value->as.number, 42.5, 0.001);
    
    value_unref(value);
}

TEST(test_value_new_boolean) {
    Value* value_true = value_new_boolean(true);
    Value* value_false = value_new_boolean(false);
    
    ASSERT_NOT_NULL(value_true);
    ASSERT_EQ(value_true->type, VALUE_BOOLEAN);
    ASSERT_TRUE(value_true->as.boolean);
    
    ASSERT_NOT_NULL(value_false);
    ASSERT_EQ(value_false->type, VALUE_BOOLEAN);
    ASSERT_FALSE(value_false->as.boolean);
    
    value_unref(value_true);
    value_unref(value_false);
}

TEST(test_value_new_null) {
    Value* value = value_new_null();
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, VALUE_NULL);
    
    value_unref(value);
}

TEST(test_value_copy_string) {
    Value* original = value_new_string("Test String");
    Value* copy = value_copy(original);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(copy->type, VALUE_STRING);
    ASSERT_STR_EQ(copy->as.string->data, "Test String");
    
    // Should be independent copies
    ASSERT_NE(copy->as.string->data, original->as.string->data);
    
    value_unref(original);
    value_unref(copy);
}

TEST(test_value_copy_number) {
    Value* original = value_new_number(3.14159);
    Value* copy = value_copy(original);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(copy->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(copy->as.number, 3.14159, 0.00001);
    
    value_unref(original);
    value_unref(copy);
}

TEST(test_value_to_string_string) {
    Value* value = value_new_string("Hello");
    char* str = value_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "Hello");
    
    free(str);
    value_unref(value);
}

TEST(test_value_to_string_number) {
    Value* value = value_new_number(42.0);
    char* str = value_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "42");
    
    free(str);
    value_unref(value);
    
    // Test float
    value = value_new_number(3.14);
    str = value_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "3.14");
    
    free(str);
    value_unref(value);
}

TEST(test_value_to_string_boolean) {
    Value* value_true = value_new_boolean(true);
    Value* value_false = value_new_boolean(false);
    
    char* str_true = value_to_string(value_true);
    char* str_false = value_to_string(value_false);
    
    ASSERT_STR_EQ(str_true, "true");
    ASSERT_STR_EQ(str_false, "false");
    
    free(str_true);
    free(str_false);
    value_unref(value_true);
    value_unref(value_false);
}

TEST(test_value_to_string_null) {
    Value* value = value_new_null();
    char* str = value_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "null");
    
    free(str);
    value_unref(value);
}

TEST(test_value_equals_string) {
    Value* value1 = value_new_string("Hello");
    Value* value2 = value_new_string("Hello");
    Value* value3 = value_new_string("World");
    
    ASSERT_TRUE(value_equals(value1, value2));
    ASSERT_FALSE(value_equals(value1, value3));
    
    value_unref(value1);
    value_unref(value2);
    value_unref(value3);
}

TEST(test_value_equals_number) {
    Value* value1 = value_new_number(42.0);
    Value* value2 = value_new_number(42.0);
    Value* value3 = value_new_number(43.0);
    
    ASSERT_TRUE(value_equals(value1, value2));
    ASSERT_FALSE(value_equals(value1, value3));
    
    value_unref(value1);
    value_unref(value2);
    value_unref(value3);
}

TEST(test_value_equals_boolean) {
    Value* value1 = value_new_boolean(true);
    Value* value2 = value_new_boolean(true);
    Value* value3 = value_new_boolean(false);
    
    ASSERT_TRUE(value_equals(value1, value2));
    ASSERT_FALSE(value_equals(value1, value3));
    
    value_unref(value1);
    value_unref(value2);
    value_unref(value3);
}

TEST(test_value_equals_null) {
    Value* value1 = value_new_null();
    Value* value2 = value_new_null();
    Value* value3 = value_new_string("not null");
    
    ASSERT_TRUE(value_equals(value1, value2));
    ASSERT_FALSE(value_equals(value1, value3));
    
    value_unref(value1);
    value_unref(value2);
    value_unref(value3);
}

TEST(test_value_equals_different_types) {
    Value* string_val = value_new_string("42");
    Value* number_val = value_new_number(42.0);
    Value* bool_val = value_new_boolean(true);
    Value* null_val = value_new_null();
    
    // Different types should not be equal
    ASSERT_FALSE(value_equals(string_val, number_val));
    ASSERT_FALSE(value_equals(string_val, bool_val));
    ASSERT_FALSE(value_equals(string_val, null_val));
    ASSERT_FALSE(value_equals(number_val, bool_val));
    ASSERT_FALSE(value_equals(number_val, null_val));
    ASSERT_FALSE(value_equals(bool_val, null_val));
    
    value_unref(string_val);
    value_unref(number_val);
    value_unref(bool_val);
    value_unref(null_val);
}

TEST(test_value_type_name) {
    Value* string_val = value_new_string("test");
    Value* number_val = value_new_number(42.0);
    Value* bool_val = value_new_boolean(true);
    Value* null_val = value_new_null();
    
    ASSERT_STR_EQ(value_type_name(string_val), "string");
    ASSERT_STR_EQ(value_type_name(number_val), "number");
    ASSERT_STR_EQ(value_type_name(bool_val), "boolean");
    ASSERT_STR_EQ(value_type_name(null_val), "null");
    
    value_unref(string_val);
    value_unref(number_val);
    value_unref(bool_val);
    value_unref(null_val);
}

TEST(test_value_reference_counting) {
    Value* value = value_new_string("Reference Test");
    
    // Initial reference count should be 1
    ASSERT_EQ(value->ref_count, 1);
    
    // Increment reference
    Value* ref = value_ref(value);
    ASSERT_EQ(ref, value);  // Should return same pointer
    ASSERT_EQ(value->ref_count, 2);
    
    // Decrement reference
    value_unref(ref);
    ASSERT_EQ(value->ref_count, 1);
    
    // Final decrement should free the value
    value_unref(value);
    // value is now invalid - don't access it
}

TEST(test_value_empty_string) {
    Value* value = value_new_string("");
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, VALUE_STRING);
    ASSERT_STR_EQ(value->as.string->data, "");
    ASSERT_EQ(value->as.string->length, 0);
    
    value_unref(value);
}

TEST(test_value_null_string) {
    Value* value = value_new_string(NULL);
    
    // Should handle NULL input gracefully
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, VALUE_STRING);
    // Implementation dependent - might create empty string or handle specially
    
    value_unref(value);
}

TEST(test_value_special_numbers) {
    // Test infinity
    Value* inf_val = value_new_number(1.0/0.0);
    ASSERT_NOT_NULL(inf_val);
    ASSERT_EQ(inf_val->type, VALUE_NUMBER);
    
    // Test NaN
    Value* nan_val = value_new_number(0.0/0.0);
    ASSERT_NOT_NULL(nan_val);
    ASSERT_EQ(nan_val->type, VALUE_NUMBER);
    
    // Test very large number
    Value* large_val = value_new_number(1e308);
    ASSERT_NOT_NULL(large_val);
    ASSERT_EQ(large_val->type, VALUE_NUMBER);
    
    value_unref(inf_val);
    value_unref(nan_val);
    value_unref(large_val);
}

END_TEST_SUITE