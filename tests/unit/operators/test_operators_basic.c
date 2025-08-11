/**
 * @file test_operators_basic.c
 * @brief Basic operator functionality tests
 */

#include "../../framework/test.h"
#include "zen/runtime/operators.h"
#include "zen/core/runtime_value.h"

TEST(test_op_add_numbers) {
    RuntimeValue* left = rv_new_number(5.0);
    RuntimeValue* right = rv_new_number(3.0);
    
    RuntimeValue* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 8.0, 0.001);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_add_strings) {
    RuntimeValue* left = rv_new_string("Hello ");
    RuntimeValue* right = rv_new_string("World");
    
    RuntimeValue* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "Hello World");
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_add_string_number) {
    RuntimeValue* left = rv_new_string("Count: ");
    RuntimeValue* right = rv_new_number(42.0);
    
    RuntimeValue* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "Count: 42");
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_subtract_numbers) {
    RuntimeValue* left = rv_new_number(10.0);
    RuntimeValue* right = rv_new_number(3.0);
    
    RuntimeValue* result = op_subtract(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 7.0, 0.001);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_multiply_numbers) {
    RuntimeValue* left = rv_new_number(4.0);
    RuntimeValue* right = rv_new_number(5.0);
    
    RuntimeValue* result = op_multiply(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 20.0, 0.001);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_divide_numbers) {
    RuntimeValue* left = rv_new_number(15.0);
    RuntimeValue* right = rv_new_number(3.0);
    
    RuntimeValue* result = op_divide(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 5.0, 0.001);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_divide_by_zero) {
    RuntimeValue* left = rv_new_number(10.0);
    RuntimeValue* right = rv_new_number(0.0);
    
    RuntimeValue* result = op_divide(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    // Should handle division by zero (infinity or error)
    // Exact behavior is implementation dependent
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_modulo_numbers) {
    RuntimeValue* left = rv_new_number(17.0);
    RuntimeValue* right = rv_new_number(5.0);
    
    RuntimeValue* result = op_modulo(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 2.0, 0.001);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_equals_same_type) {
    // Test number equality
    RuntimeValue* num1 = rv_new_number(42.0);
    RuntimeValue* num2 = rv_new_number(42.0);
    RuntimeValue* num3 = rv_new_number(43.0);
    
    RuntimeValue* result1 = op_equals(num1, num2);
    RuntimeValue* result2 = op_equals(num1, num3);
    
    ASSERT_EQ(result1->type, RV_BOOLEAN);
    ASSERT_TRUE(result1->data.boolean);
    
    ASSERT_EQ(result2->type, RV_BOOLEAN);
    ASSERT_FALSE(result2->data.boolean);
    
    rv_unref(num1);
    rv_unref(num2);
    rv_unref(num3);
    rv_unref(result1);
    rv_unref(result2);
    
    // Test string equality
    RuntimeValue* str1 = rv_new_string("hello");
    RuntimeValue* str2 = rv_new_string("hello");
    RuntimeValue* str3 = rv_new_string("world");
    
    RuntimeValue* result3 = op_equals(str1, str2);
    RuntimeValue* result4 = op_equals(str1, str3);
    
    ASSERT_TRUE(result3->data.boolean);
    ASSERT_FALSE(result4->data.boolean);
    
    rv_unref(str1);
    rv_unref(str2);
    rv_unref(str3);
    rv_unref(result3);
    rv_unref(result4);
}

TEST(test_op_equals_different_types) {
    RuntimeValue* string_val = rv_new_string("42");
    RuntimeValue* number_val = rv_new_number(42.0);
    
    RuntimeValue* result = op_equals(string_val, number_val);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_BOOLEAN);
    ASSERT_FALSE(result->data.boolean);  // Different types should not be equal
    
    rv_unref(string_val);
    rv_unref(number_val);
    rv_unref(result);
}

TEST(test_op_not_equals) {
    RuntimeValue* left = rv_new_number(5.0);
    RuntimeValue* right = rv_new_number(3.0);
    
    RuntimeValue* result = op_not_equals(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_BOOLEAN);
    ASSERT_TRUE(result->data.boolean);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_less_than) {
    RuntimeValue* left = rv_new_number(3.0);
    RuntimeValue* right = rv_new_number(5.0);
    
    RuntimeValue* result = op_less_than(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_BOOLEAN);
    ASSERT_TRUE(result->data.boolean);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
    
    // Test false case
    left = rv_new_number(5.0);
    right = rv_new_number(3.0);
    
    result = op_less_than(left, right);
    ASSERT_FALSE(result->data.boolean);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_greater_than) {
    RuntimeValue* left = rv_new_number(5.0);
    RuntimeValue* right = rv_new_number(3.0);
    
    RuntimeValue* result = op_greater_than(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_BOOLEAN);
    ASSERT_TRUE(result->data.boolean);
    
    rv_unref(left);
    rv_unref(right);
    rv_unref(result);
}

TEST(test_op_logical_and) {
    RuntimeValue* true_val = rv_new_boolean(true);
    RuntimeValue* false_val = rv_new_boolean(false);
    
    // true && true = true
    RuntimeValue* result1 = op_logical_and(true_val, true_val);
    ASSERT_TRUE(result1->data.boolean);
    
    // true && false = false
    RuntimeValue* result2 = op_logical_and(true_val, false_val);
    ASSERT_FALSE(result2->data.boolean);
    
    // false && true = false
    RuntimeValue* result3 = op_logical_and(false_val, true_val);
    ASSERT_FALSE(result3->data.boolean);
    
    // false && false = false
    RuntimeValue* result4 = op_logical_and(false_val, false_val);
    ASSERT_FALSE(result4->data.boolean);
    
    rv_unref(true_val);
    rv_unref(false_val);
    rv_unref(result1);
    rv_unref(result2);
    rv_unref(result3);
    rv_unref(result4);
}

TEST(test_op_logical_or) {
    RuntimeValue* true_val = rv_new_boolean(true);
    RuntimeValue* false_val = rv_new_boolean(false);
    
    // true || true = true
    RuntimeValue* result1 = op_logical_or(true_val, true_val);
    ASSERT_TRUE(result1->data.boolean);
    
    // true || false = true
    RuntimeValue* result2 = op_logical_or(true_val, false_val);
    ASSERT_TRUE(result2->data.boolean);
    
    // false || true = true
    RuntimeValue* result3 = op_logical_or(false_val, true_val);
    ASSERT_TRUE(result3->data.boolean);
    
    // false || false = false
    RuntimeValue* result4 = op_logical_or(false_val, false_val);
    ASSERT_FALSE(result4->data.boolean);
    
    rv_unref(true_val);
    rv_unref(false_val);
    rv_unref(result1);
    rv_unref(result2);
    rv_unref(result3);
    rv_unref(result4);
}

TEST(test_op_logical_not) {
    RuntimeValue* true_val = rv_new_boolean(true);
    RuntimeValue* false_val = rv_new_boolean(false);
    
    RuntimeValue* result1 = op_logical_not(true_val);
    ASSERT_FALSE(result1->data.boolean);
    
    RuntimeValue* result2 = op_logical_not(false_val);
    ASSERT_TRUE(result2->data.boolean);
    
    rv_unref(true_val);
    rv_unref(false_val);
    rv_unref(result1);
    rv_unref(result2);
}

TEST(test_op_truthy_values) {
    // Test logical operations with non-boolean values
    RuntimeValue* number_zero = rv_new_number(0.0);
    RuntimeValue* number_nonzero = rv_new_number(42.0);
    RuntimeValue* string_empty = rv_new_string("");
    RuntimeValue* string_nonempty = rv_new_string("hello");
    RuntimeValue* null_val = rv_new_null();
    
    // These tests depend on implementation of truthiness
    // Zero, empty string, and null should be falsy
    // Non-zero numbers and non-empty strings should be truthy
    
    RuntimeValue* result1 = op_logical_not(number_zero);
    ASSERT_TRUE(result1->data.boolean);  // !0 = true
    
    RuntimeValue* result2 = op_logical_not(number_nonzero);
    ASSERT_FALSE(result2->data.boolean);  // !42 = false
    
    rv_unref(number_zero);
    rv_unref(number_nonzero);
    rv_unref(string_empty);
    rv_unref(string_nonempty);
    rv_unref(null_val);
    rv_unref(result1);
    rv_unref(result2);
}

TEST(test_op_edge_cases) {
    // Test operations with null values
    RuntimeValue* null_val = rv_new_null();
    RuntimeValue* number_val = rv_new_number(42.0);
    
    RuntimeValue* result = op_add(null_val, number_val);
    
    // Implementation dependent - might return null, error, or convert null to 0
    ASSERT_NOT_NULL(result);
    
    rv_unref(null_val);
    rv_unref(number_val);
    rv_unref(result);
}

TEST_SUITE_BEGIN(operators_basic_tests)
    RUN_TEST(test_op_add_numbers);
    RUN_TEST(test_op_add_strings);
    RUN_TEST(test_op_add_string_number);
    RUN_TEST(test_op_subtract_numbers);
    RUN_TEST(test_op_multiply_numbers);
    RUN_TEST(test_op_divide_numbers);
    RUN_TEST(test_op_divide_by_zero);
    RUN_TEST(test_op_modulo_numbers);
    RUN_TEST(test_op_equals_same_type);
    RUN_TEST(test_op_equals_different_types);
    RUN_TEST(test_op_not_equals);
    RUN_TEST(test_op_less_than);
    RUN_TEST(test_op_greater_than);
    RUN_TEST(test_op_logical_and);
    RUN_TEST(test_op_logical_or);
    RUN_TEST(test_op_logical_not);
    RUN_TEST(test_op_truthy_values);
    RUN_TEST(test_op_edge_cases);
TEST_SUITE_END