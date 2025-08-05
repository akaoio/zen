/**
 * @file test_string_functions.c
 * @brief Tests for ZEN standard library string functions
 */

#include "../../framework/test.h"
#include "zen/stdlib/string.h"
#include "zen/types/value.h"

TEST_SUITE(string_functions_tests)

TEST(test_zen_string_length) {
    Value* str = value_new_string("Hello World");
    Value* result = zen_string_length(str);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_NUMBER);
    ASSERT_DOUBLE_EQ(result->as.number, 11.0, 0.001);
    
    value_unref(str);
    value_unref(result);
    
    // Test empty string
    str = value_new_string("");
    result = zen_string_length(str);
    ASSERT_DOUBLE_EQ(result->as.number, 0.0, 0.001);
    
    value_unref(str);
    value_unref(result);
}

TEST(test_zen_string_upper) {
    Value* str = value_new_string("hello world");
    Value* result = zen_string_upper(str);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "HELLO WORLD");
    
    value_unref(str);
    value_unref(result);
    
    // Test mixed case
    str = value_new_string("HeLLo WoRLd");
    result = zen_string_upper(str);
    ASSERT_STR_EQ(result->as.string->data, "HELLO WORLD");
    
    value_unref(str);
    value_unref(result);
}

TEST(test_zen_string_lower) {
    Value* str = value_new_string("HELLO WORLD");
    Value* result = zen_string_lower(str);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "hello world");
    
    value_unref(str);
    value_unref(result);
    
    // Test mixed case
    str = value_new_string("HeLLo WoRLd");
    result = zen_string_lower(str);
    ASSERT_STR_EQ(result->as.string->data, "hello world");
    
    value_unref(str);
    value_unref(result);
}

TEST(test_zen_string_trim) {
    Value* str = value_new_string("  hello world  ");
    Value* result = zen_string_trim(str);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "hello world");
    
    value_unref(str);
    value_unref(result);
    
    // Test string with only leading whitespace
    str = value_new_string("   hello");
    result = zen_string_trim(str);
    ASSERT_STR_EQ(result->as.string->data, "hello");
    
    value_unref(str);
    value_unref(result);
    
    // Test string with only trailing whitespace
    str = value_new_string("hello   ");
    result = zen_string_trim(str);
    ASSERT_STR_EQ(result->as.string->data, "hello");
    
    value_unref(str);
    value_unref(result);
    
    // Test string with tabs and newlines
    str = value_new_string("\t\n hello \t\n");
    result = zen_string_trim(str);
    ASSERT_STR_EQ(result->as.string->data, "hello");
    
    value_unref(str);
    value_unref(result);
}

TEST(test_zen_string_split) {
    Value* str = value_new_string("apple,banana,cherry");
    Value* delimiter = value_new_string(",");
    Value* result = zen_string_split(str, delimiter);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_ARRAY);
    // Array should contain 3 elements
    // Exact testing depends on array implementation
    
    value_unref(str);
    value_unref(delimiter);
    value_unref(result);
    
    // Test split with space
    str = value_new_string("hello world test");
    delimiter = value_new_string(" ");
    result = zen_string_split(str, delimiter);
    
    ASSERT_EQ(result->type, VALUE_ARRAY);
    // Should contain 3 elements: "hello", "world", "test"
    
    value_unref(str);
    value_unref(delimiter);
    value_unref(result);
}

TEST(test_zen_string_contains) {
    Value* str = value_new_string("hello world");
    Value* substring = value_new_string("world");
    Value* result = zen_string_contains(str, substring);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_BOOLEAN);
    ASSERT_TRUE(result->as.boolean);
    
    value_unref(str);
    value_unref(substring);
    value_unref(result);
    
    // Test string not containing substring
    str = value_new_string("hello world");
    substring = value_new_string("foo");
    result = zen_string_contains(str, substring);
    
    ASSERT_FALSE(result->as.boolean);
    
    value_unref(str);
    value_unref(substring);
    value_unref(result);
    
    // Test case sensitivity
    str = value_new_string("Hello World");
    substring = value_new_string("hello");
    result = zen_string_contains(str, substring);
    
    ASSERT_FALSE(result->as.boolean);  // Should be case sensitive
    
    value_unref(str);
    value_unref(substring);
    value_unref(result);
}

TEST(test_zen_string_replace) {
    Value* str = value_new_string("hello world hello");
    Value* old_substr = value_new_string("hello");
    Value* new_substr = value_new_string("hi");
    Value* result = zen_string_replace(str, old_substr, new_substr);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_STRING);
    ASSERT_STR_EQ(result->as.string->data, "hi world hi");
    
    value_unref(str);
    value_unref(old_substr);
    value_unref(new_substr);
    value_unref(result);
    
    // Test replace with empty string (removal)
    str = value_new_string("hello world");
    old_substr = value_new_string(" ");
    new_substr = value_new_string("");
    result = zen_string_replace(str, old_substr, new_substr);
    
    ASSERT_STR_EQ(result->as.string->data, "helloworld");
    
    value_unref(str);
    value_unref(old_substr);
    value_unref(new_substr);
    value_unref(result);
}

TEST(test_string_functions_with_empty_strings) {
    Value* empty_str = value_new_string("");
    
    // Test length of empty string
    Value* length_result = zen_string_length(empty_str);
    ASSERT_DOUBLE_EQ(length_result->as.number, 0.0, 0.001);
    value_unref(length_result);
    
    // Test upper of empty string
    Value* upper_result = zen_string_upper(empty_str);
    ASSERT_STR_EQ(upper_result->as.string->data, "");
    value_unref(upper_result);
    
    // Test lower of empty string
    Value* lower_result = zen_string_lower(empty_str);
    ASSERT_STR_EQ(lower_result->as.string->data, "");
    value_unref(lower_result);
    
    // Test trim of empty string
    Value* trim_result = zen_string_trim(empty_str);
    ASSERT_STR_EQ(trim_result->as.string->data, "");
    value_unref(trim_result);
    
    value_unref(empty_str);
}

TEST(test_string_functions_with_special_characters) {
    Value* str = value_new_string("Hello\nWorld\tTest");
    
    Value* length_result = zen_string_length(str);
    ASSERT_DOUBLE_EQ(length_result->as.number, 16.0, 0.001);  // Including \n and \t
    value_unref(length_result);
    
    Value* upper_result = zen_string_upper(str);
    ASSERT_STR_EQ(upper_result->as.string->data, "HELLO\nWORLD\tTEST");
    value_unref(upper_result);
    
    value_unref(str);
}

TEST(test_string_functions_with_unicode) {
    // Test with unicode characters (if supported)
    Value* str = value_new_string("Héllo Wørld 🌍");
    
    Value* length_result = zen_string_length(str);
    // Length might be byte count or character count depending on implementation
    ASSERT_TRUE(length_result->as.number > 0);
    value_unref(length_result);
    
    Value* upper_result = zen_string_upper(str);
    ASSERT_NOT_NULL(upper_result);
    ASSERT_EQ(upper_result->type, VALUE_STRING);
    value_unref(upper_result);
    
    value_unref(str);
}

TEST(test_string_functions_error_handling) {
    // Test with null values
    Value* null_str = value_new_null();
    
    Value* length_result = zen_string_length(null_str);
    // Should handle gracefully - might return 0 or null
    ASSERT_NOT_NULL(length_result);
    
    value_unref(null_str);
    value_unref(length_result);
}

TEST(test_string_split_edge_cases) {
    // Test split with delimiter not found
    Value* str = value_new_string("hello world");
    Value* delimiter = value_new_string(",");
    Value* result = zen_string_split(str, delimiter);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, VALUE_ARRAY);
    // Should return array with single element (original string)
    
    value_unref(str);
    value_unref(delimiter);
    value_unref(result);
    
    // Test split with empty delimiter
    str = value_new_string("hello");
    delimiter = value_new_string("");
    result = zen_string_split(str, delimiter);
    
    ASSERT_NOT_NULL(result);
    // Behavior is implementation dependent
    
    value_unref(str);
    value_unref(delimiter);
    value_unref(result);
}

TEST(test_string_replace_edge_cases) {
    // Test replace with substring not found
    Value* str = value_new_string("hello world");
    Value* old_substr = value_new_string("foo");
    Value* new_substr = value_new_string("bar");
    Value* result = zen_string_replace(str, old_substr, new_substr);
    
    ASSERT_STR_EQ(result->as.string->data, "hello world");  // Unchanged
    
    value_unref(str);
    value_unref(old_substr);
    value_unref(new_substr);
    value_unref(result);
    
    // Test replace entire string
    str = value_new_string("hello");
    old_substr = value_new_string("hello");
    new_substr = value_new_string("world");
    result = zen_string_replace(str, old_substr, new_substr);
    
    ASSERT_STR_EQ(result->as.string->data, "world");
    
    value_unref(str);
    value_unref(old_substr);
    value_unref(new_substr);
    value_unref(result);
}

END_TEST_SUITE