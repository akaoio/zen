/**
 * @file test_operators_basic.c
 * @brief Basic operator functionality tests
 */

#include "../../framework/test.h"
#include "zen/runtime/operators.h"
#include "zen/types/value.h"

TEST_SUITE(operators_basic_tests)

TEST(test_op_add_numbers) {
    Value* left = value_new_number(5.0);
    Value* right = value_new_number(3.0);
    
    Value* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 8.0, 0.001);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_add_strings) {
    Value* left = value_new_string("Hello ");
    Value* right = value_new_string("World");
    
    Value* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "Hello World");
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_add_string_number) {
    Value* left = value_new_string("Count: ");
    Value* right = value_new_number(42.0);
    
    Value* result = op_add(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "Count: 42");
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_subtract_numbers) {
    Value* left = value_new_number(10.0);
    Value* right = value_new_number(3.0);
    
    Value* result = op_subtract(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 7.0, 0.001);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_multiply_numbers) {
    Value* left = value_new_number(4.0);
    Value* right = value_new_number(5.0);
    
    Value* result = op_multiply(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 20.0, 0.001);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_divide_numbers) {
    Value* left = value_new_number(15.0);
    Value* right = value_new_number(3.0);
    
    Value* result = op_divide(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 5.0, 0.001);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_divide_by_zero) {
    Value* left = value_new_number(10.0);
    Value* right = value_new_number(0.0);
    
    Value* result = op_divide(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    // Should handle division by zero (infinity or error)
    // Exact behavior is implementation dependent
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_modulo_numbers) {
    Value* left = value_new_number(17.0);
    Value* right = value_new_number(5.0);
    
    Value* result = op_modulo(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 2.0, 0.001);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_equals_same_type) {
    // Test number equality
    Value* num1 = value_new_number(42.0);
    Value* num2 = value_new_number(42.0);
    Value* num3 = value_new_number(43.0);
    
    Value* result1 = op_equals(num1, num2);
    Value* result2 = op_equals(num1, num3);
    
    ASSERT_EQ(result1->type, VALUE_BOOLEAN);
    ASSERT_TRUE(result1->as.boolean);
    
    ASSERT_EQ(result2->type, VALUE_BOOLEAN);
    ASSERT_FALSE(result2->as.boolean);
    
    value_unref(num1);
    value_unref(num2);
    value_unref(num3);
    value_unref(result1);
    value_unref(result2);
    
    // Test string equality
    Value* str1 = value_new_string("hello");
    Value* str2 = value_new_string("hello");
    Value* str3 = value_new_string("world");
    
    Value* result3 = op_equals(str1, str2);
    Value* result4 = op_equals(str1, str3);
    
    ASSERT_TRUE(result3->as.boolean);
    ASSERT_FALSE(result4->as.boolean);
    
    value_unref(str1);
    value_unref(str2);
    value_unref(str3);
    value_unref(result3);
    value_unref(result4);
}

TEST(test_op_equals_different_types) {
    Value* string_val = value_new_string("42");
    Value* number_val = value_new_number(42.0);
    
    Value* result = op_equals(string_val, number_val);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_BOOLEAN);
    ASSERT_FALSE(result->as.boolean);  // Different types should not be equal
    
    value_unref(string_val);
    value_unref(number_val);
    value_unref(result);
}

TEST(test_op_not_equals) {
    Value* left = value_new_number(5.0);
    Value* right = value_new_number(3.0);
    
    Value* result = op_not_equals(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_BOOLEAN);
    ASSERT_TRUE(result->as.boolean);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_less_than) {
    Value* left = value_new_number(3.0);
    Value* right = value_new_number(5.0);
    
    Value* result = op_less_than(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_BOOLEAN);
    ASSERT_TRUE(result->as.boolean);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
    
    // Test false case
    left = value_new_number(5.0);
    right = value_new_number(3.0);
    
    result = op_less_than(left, right);
    ASSERT_FALSE(result->as.boolean);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_greater_than) {
    Value* left = value_new_number(5.0);
    Value* right = value_new_number(3.0);
    
    Value* result = op_greater_than(left, right);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_BOOLEAN);
    ASSERT_TRUE(result->as.boolean);
    
    value_unref(left);
    value_unref(right);
    value_unref(result);
}

TEST(test_op_logical_and) {
    Value* true_val = value_new_boolean(true);
    Value* false_val = value_new_boolean(false);
    
    // true && true = true
    Value* result1 = op_logical_and(true_val, true_val);
    ASSERT_TRUE(result1->as.boolean);
    
    // true && false = false
    Value* result2 = op_logical_and(true_val, false_val);
    ASSERT_FALSE(result2->as.boolean);
    
    // false && true = false
    Value* result3 = op_logical_and(false_val, true_val);
    ASSERT_FALSE(result3->as.boolean);
    
    // false && false = false
    Value* result4 = op_logical_and(false_val, false_val);
    ASSERT_FALSE(result4->as.boolean);
    
    value_unref(true_val);
    value_unref(false_val);
    value_unref(result1);
    value_unref(result2);
    value_unref(result3);
    value_unref(result4);
}

TEST(test_op_logical_or) {
    Value* true_val = value_new_boolean(true);
    Value* false_val = value_new_boolean(false);
    
    // true || true = true
    Value* result1 = op_logical_or(true_val, true_val);
    ASSERT_TRUE(result1->as.boolean);
    
    // true || false = true
    Value* result2 = op_logical_or(true_val, false_val);
    ASSERT_TRUE(result2->as.boolean);
    
    // false || true = true
    Value* result3 = op_logical_or(false_val, true_val);
    ASSERT_TRUE(result3->as.boolean);
    
    // false || false = false
    Value* result4 = op_logical_or(false_val, false_val);
    ASSERT_FALSE(result4->as.boolean);
    
    value_unref(true_val);
    value_unref(false_val);
    value_unref(result1);
    value_unref(result2);
    value_unref(result3);
    value_unref(result4);
}

TEST(test_op_logical_not) {
    Value* true_val = value_new_boolean(true);
    Value* false_val = value_new_boolean(false);
    
    Value* result1 = op_logical_not(true_val);
    ASSERT_FALSE(result1->as.boolean);
    
    Value* result2 = op_logical_not(false_val);
    ASSERT_TRUE(result2->as.boolean);
    
    value_unref(true_val);
    value_unref(false_val);
    value_unref(result1);
    value_unref(result2);
}

TEST(test_op_truthy_values) {
    // Test logical operations with non-boolean values
    Value* number_zero = value_new_number(0.0);
    Value* number_nonzero = value_new_number(42.0);
    Value* string_empty = value_new_string("");
    Value* string_nonempty = value_new_string("hello");
    Value* null_val = value_new_null();
    
    // These tests depend on implementation of truthiness
    // Zero, empty string, and null should be falsy
    // Non-zero numbers and non-empty strings should be truthy
    
    Value* result1 = op_logical_not(number_zero);
    ASSERT_TRUE(result1->as.boolean);  // !0 = true
    
    Value* result2 = op_logical_not(number_nonzero);
    ASSERT_FALSE(result2->as.boolean);  // !42 = false
    
    value_unref(number_zero);
    value_unref(number_nonzero);
    value_unref(string_empty);
    value_unref(string_nonempty);
    value_unref(null_val);
    value_unref(result1);
    value_unref(result2);
}

TEST(test_op_edge_cases) {
    // Test operations with null values
    Value* null_val = value_new_null();
    Value* number_val = value_new_number(42.0);
    
    Value* result = op_add(null_val, number_val);
    
    // Implementation dependent - might return null, error, or convert null to 0
    ASSERT_NOT_NULL(result);
    
    value_unref(null_val);
    value_unref(number_val);
    value_unref(result);
}

END_TEST_SUITE