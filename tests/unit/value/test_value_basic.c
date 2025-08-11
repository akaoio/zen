/**
 * @file test_value_basic.c
 * @brief Basic value system tests
 */

#include "../../framework/test.h"
#include "zen/core/runtime_value.h"
#include "zen/core/memory.h"

// Forward declare all tests
DECLARE_TEST(test_value_new_string);
DECLARE_TEST(test_value_new_number);
DECLARE_TEST(test_value_new_boolean);
DECLARE_TEST(test_value_new_null);
DECLARE_TEST(test_value_copy_string);
DECLARE_TEST(test_value_copy_number);
DECLARE_TEST(test_value_to_string_string);
DECLARE_TEST(test_value_to_string_number);
DECLARE_TEST(test_value_to_string_boolean);
DECLARE_TEST(test_value_to_string_null);
DECLARE_TEST(test_value_equals_string);
DECLARE_TEST(test_value_equals_number);
DECLARE_TEST(test_value_equals_boolean);
DECLARE_TEST(test_value_equals_null);
DECLARE_TEST(test_value_equals_different_types);
DECLARE_TEST(test_value_type_name);
DECLARE_TEST(test_value_reference_counting);
DECLARE_TEST(test_value_empty_string);
DECLARE_TEST(test_value_null_string);
DECLARE_TEST(test_value_special_numbers);

TEST(test_value_new_string) {
    RuntimeValue* value = rv_new_string("Hello World");
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, RV_STRING);
    ASSERT_NOT_NULL(value->data.string.data);
    ASSERT_STR_EQ(value->data.string.data, "Hello World");
    ASSERT_EQ(value->data.string.length, 11);
    
    rv_unref(value);
}

TEST(test_value_new_number) {
    RuntimeValue* value = rv_new_number(42.5);
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(value->data.number, 42.5, 0.001);
    
    rv_unref(value);
}

TEST(test_value_new_boolean) {
    RuntimeValue* value_true = rv_new_boolean(true);
    RuntimeValue* value_false = rv_new_boolean(false);
    
    ASSERT_NOT_NULL(value_true);
    ASSERT_EQ(value_true->type, RV_BOOLEAN);
    ASSERT_TRUE(value_true->data.boolean);
    
    ASSERT_NOT_NULL(value_false);
    ASSERT_EQ(value_false->type, RV_BOOLEAN);
    ASSERT_FALSE(value_false->data.boolean);
    
    rv_unref(value_true);
    rv_unref(value_false);
}

TEST(test_value_new_null) {
    RuntimeValue* value = rv_new_null();
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, RV_NULL);
    
    rv_unref(value);
}

TEST(test_value_copy_string) {
    RuntimeValue* original = rv_new_string("Test String");
    RuntimeValue* copy = rv_copy(original);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(copy->type, RV_STRING);
    ASSERT_STR_EQ(copy->data.string.data, "Test String");
    
    // Should be independent copies
    ASSERT_NE(copy->data.string.data, original->data.string.data);
    
    rv_unref(original);
    rv_unref(copy);
}

TEST(test_value_copy_number) {
    RuntimeValue* original = rv_new_number(3.14159);
    RuntimeValue* copy = rv_copy(original);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(copy->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(copy->data.number, 3.14159, 0.00001);
    
    rv_unref(original);
    rv_unref(copy);
}

TEST(test_value_to_string_string) {
    RuntimeValue* value = rv_new_string("Hello");
    char* str = rv_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "Hello");
    
    memory_free(str);
    rv_unref(value);
}

TEST(test_value_to_string_number) {
    RuntimeValue* value = rv_new_number(42.0);
    char* str = rv_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "42");
    
    memory_free(str);
    rv_unref(value);
    
    // Test float
    value = rv_new_number(3.14);
    str = rv_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "3.14");
    
    memory_free(str);
    rv_unref(value);
}

TEST(test_value_to_string_boolean) {
    RuntimeValue* value_true = rv_new_boolean(true);
    RuntimeValue* value_false = rv_new_boolean(false);
    
    char* str_true = rv_to_string(value_true);
    char* str_false = rv_to_string(value_false);
    
    ASSERT_STR_EQ(str_true, "true");
    ASSERT_STR_EQ(str_false, "false");
    
    memory_free(str_true);
    memory_free(str_false);
    rv_unref(value_true);
    rv_unref(value_false);
}

TEST(test_value_to_string_null) {
    RuntimeValue* value = rv_new_null();
    char* str = rv_to_string(value);
    
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "null");
    
    memory_free(str);
    rv_unref(value);
}

TEST(test_value_equals_string) {
    RuntimeValue* value1 = rv_new_string("Hello");
    RuntimeValue* value2 = rv_new_string("Hello");
    RuntimeValue* value3 = rv_new_string("World");
    
    ASSERT_TRUE(rv_equals(value1, value2));
    ASSERT_FALSE(rv_equals(value1, value3));
    
    rv_unref(value1);
    rv_unref(value2);
    rv_unref(value3);
}

TEST(test_value_equals_number) {
    RuntimeValue* value1 = rv_new_number(42.0);
    RuntimeValue* value2 = rv_new_number(42.0);
    RuntimeValue* value3 = rv_new_number(43.0);
    
    ASSERT_TRUE(rv_equals(value1, value2));
    ASSERT_FALSE(rv_equals(value1, value3));
    
    rv_unref(value1);
    rv_unref(value2);
    rv_unref(value3);
}

TEST(test_value_equals_boolean) {
    RuntimeValue* value1 = rv_new_boolean(true);
    RuntimeValue* value2 = rv_new_boolean(true);
    RuntimeValue* value3 = rv_new_boolean(false);
    
    ASSERT_TRUE(rv_equals(value1, value2));
    ASSERT_FALSE(rv_equals(value1, value3));
    
    rv_unref(value1);
    rv_unref(value2);
    rv_unref(value3);
}

TEST(test_value_equals_null) {
    RuntimeValue* value1 = rv_new_null();
    RuntimeValue* value2 = rv_new_null();
    RuntimeValue* value3 = rv_new_string("not null");
    
    ASSERT_TRUE(rv_equals(value1, value2));
    ASSERT_FALSE(rv_equals(value1, value3));
    
    rv_unref(value1);
    rv_unref(value2);
    rv_unref(value3);
}

TEST(test_value_equals_different_types) {
    RuntimeValue* string_val = rv_new_string("42");
    RuntimeValue* number_val = rv_new_number(42.0);
    RuntimeValue* bool_val = rv_new_boolean(true);
    RuntimeValue* null_val = rv_new_null();
    
    // Different types should not be equal
    ASSERT_FALSE(rv_equals(string_val, number_val));
    ASSERT_FALSE(rv_equals(string_val, bool_val));
    ASSERT_FALSE(rv_equals(string_val, null_val));
    ASSERT_FALSE(rv_equals(number_val, bool_val));
    ASSERT_FALSE(rv_equals(number_val, null_val));
    ASSERT_FALSE(rv_equals(bool_val, null_val));
    
    rv_unref(string_val);
    rv_unref(number_val);
    rv_unref(bool_val);
    rv_unref(null_val);
}

TEST(test_value_type_name) {
    RuntimeValue* string_val = rv_new_string("test");
    RuntimeValue* number_val = rv_new_number(42.0);
    RuntimeValue* bool_val = rv_new_boolean(true);
    RuntimeValue* null_val = rv_new_null();
    
    ASSERT_STR_EQ(rv_type_name(string_val), "string");
    ASSERT_STR_EQ(rv_type_name(number_val), "number");
    ASSERT_STR_EQ(rv_type_name(bool_val), "boolean");
    ASSERT_STR_EQ(rv_type_name(null_val), "null");
    
    rv_unref(string_val);
    rv_unref(number_val);
    rv_unref(bool_val);
    rv_unref(null_val);
}

TEST(test_value_reference_counting) {
    RuntimeValue* value = rv_new_string("Reference Test");
    
    // Initial reference count should be 1
    ASSERT_EQ(value->ref_count, 1);
    
    // Increment reference
    RuntimeValue* ref = rv_ref(value);
    ASSERT_EQ(ref, value);  // Should return same pointer
    ASSERT_EQ(value->ref_count, 2);
    
    // Decrement reference
    rv_unref(ref);
    ASSERT_EQ(value->ref_count, 1);
    
    // Final decrement should free the value
    rv_unref(value);
    // value is now invalid - don't access it
}

TEST(test_value_empty_string) {
    RuntimeValue* value = rv_new_string("");
    
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, RV_STRING);
    ASSERT_STR_EQ(value->data.string.data, "");
    ASSERT_EQ(value->data.string.length, 0);
    
    rv_unref(value);
}

TEST(test_value_null_string) {
    RuntimeValue* value = rv_new_string(NULL);
    
    // Should return NULL for NULL input
    ASSERT_NULL(value);
    
    // Test with empty string instead
    value = rv_new_string("");
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(value->type, RV_STRING);
    ASSERT_NOT_NULL(value->data.string.data);
    ASSERT_STR_EQ(value->data.string.data, "");
    ASSERT_EQ(value->data.string.length, 0);
    
    rv_unref(value);
}

TEST(test_value_special_numbers) {
    // Test infinity
    RuntimeValue* inf_val = rv_new_number(1.0/0.0);
    ASSERT_NOT_NULL(inf_val);
    ASSERT_EQ(inf_val->type, RV_NUMBER);
    
    // Test NaN
    RuntimeValue* nan_val = rv_new_number(0.0/0.0);
    ASSERT_NOT_NULL(nan_val);
    ASSERT_EQ(nan_val->type, RV_NUMBER);
    
    // Test very large number
    RuntimeValue* large_val = rv_new_number(1e308);
    ASSERT_NOT_NULL(large_val);
    ASSERT_EQ(large_val->type, RV_NUMBER);
    
    rv_unref(inf_val);
    rv_unref(nan_val);
    rv_unref(large_val);
}

TEST_SUITE_BEGIN(value_basic_tests)
    RUN_TEST(test_value_new_string);
    RUN_TEST(test_value_new_number);
    RUN_TEST(test_value_new_boolean);
    RUN_TEST(test_value_new_null);
    RUN_TEST(test_value_copy_string);
    RUN_TEST(test_value_copy_number);
    RUN_TEST(test_value_to_string_string);
    RUN_TEST(test_value_to_string_number);
    RUN_TEST(test_value_to_string_boolean);
    RUN_TEST(test_value_to_string_null);
    RUN_TEST(test_value_equals_string);
    RUN_TEST(test_value_equals_number);
    RUN_TEST(test_value_equals_boolean);
    RUN_TEST(test_value_equals_null);
    RUN_TEST(test_value_equals_different_types);
    RUN_TEST(test_value_type_name);
    RUN_TEST(test_value_reference_counting);
    RUN_TEST(test_value_empty_string);
    RUN_TEST(test_value_null_string);
    RUN_TEST(test_value_special_numbers);
TEST_SUITE_END