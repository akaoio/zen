/**
 * @file test_string_functions.c
 * @brief Tests for string manipulation functions
 */

#include "../../framework/test.h"
#include "zen/stdlib/string.h"
#include "zen/core/runtime_value.h"

// Forward declarations
DECLARE_TEST(test_string_length);
DECLARE_TEST(test_string_upper);
DECLARE_TEST(test_string_lower);
DECLARE_TEST(test_string_trim);
DECLARE_TEST(test_string_split);
DECLARE_TEST(test_string_contains);
DECLARE_TEST(test_string_replace);
DECLARE_TEST(test_string_functions_with_empty_strings);
DECLARE_TEST(test_string_functions_with_special_characters);
DECLARE_TEST(test_string_functions_with_unicode);

TEST(test_string_length) {
    RuntimeValue* str = rv_new_string("Hello World");
    RuntimeValue* args[] = { str };
    RuntimeValue* result = string_length(args, 1);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_NUMBER);
    ASSERT_DOUBLE_EQ(result->data.number, 11.0, 0.001);
    
    rv_unref(str);
    rv_unref(result);
    
    // Test empty string
    str = rv_new_string("");
    RuntimeValue* args2[] = { str };
    result = string_length(args2, 1);
    ASSERT_DOUBLE_EQ(result->data.number, 0.0, 0.001);
    
    rv_unref(str);
    rv_unref(result);
}

TEST(test_string_upper) {
    RuntimeValue* str = rv_new_string("hello world");
    RuntimeValue* args[] = { str };
    RuntimeValue* result = string_upper(args, 1);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "HELLO WORLD");
    
    rv_unref(str);
    rv_unref(result);
    
    // Test mixed case
    str = rv_new_string("HeLLo WoRLd");
    RuntimeValue* args2[] = { str };
    result = string_upper(args2, 1);
    ASSERT_STR_EQ(result->data.string.data, "HELLO WORLD");
    
    rv_unref(str);
    rv_unref(result);
}

TEST(test_string_lower) {
    RuntimeValue* str = rv_new_string("HELLO WORLD");
    RuntimeValue* args[] = { str };
    RuntimeValue* result = string_lower(args, 1);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "hello world");
    
    rv_unref(str);
    rv_unref(result);
    
    // Test mixed case
    str = rv_new_string("HeLLo WoRLd");
    RuntimeValue* args2[] = { str };
    result = string_lower(args2, 1);
    ASSERT_STR_EQ(result->data.string.data, "hello world");
    
    rv_unref(str);
    rv_unref(result);
}

TEST(test_string_trim) {
    RuntimeValue* str = rv_new_string("  hello world  ");
    RuntimeValue* args[] = { str };
    RuntimeValue* result = string_trim(args, 1);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "hello world");
    
    rv_unref(str);
    rv_unref(result);
    
    // Test string with only leading whitespace
    str = rv_new_string("   hello");
    RuntimeValue* args2[] = { str };
    result = string_trim(args2, 1);
    ASSERT_STR_EQ(result->data.string.data, "hello");
    
    rv_unref(str);
    rv_unref(result);
    
    // Test string with only trailing whitespace
    str = rv_new_string("hello   ");
    RuntimeValue* args3[] = { str };
    result = string_trim(args3, 1);
    ASSERT_STR_EQ(result->data.string.data, "hello");
    
    rv_unref(str);
    rv_unref(result);
    
    // Test string with tabs and newlines
    str = rv_new_string("\t\n hello \t\n");
    RuntimeValue* args4[] = { str };
    result = string_trim(args4, 1);
    ASSERT_STR_EQ(result->data.string.data, "hello");
    
    rv_unref(str);
    rv_unref(result);
}

TEST(test_string_split) {
    RuntimeValue* str = rv_new_string("apple,banana,cherry");
    RuntimeValue* delimiter = rv_new_string(",");
    RuntimeValue* args[] = {str, delimiter};
    RuntimeValue* result = string_split(args, 2);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_ARRAY);
    // Array should contain 3 elements
    // Exact testing depends on array implementation
    
    rv_unref(str);
    rv_unref(delimiter);
    rv_unref(result);
    
    // Test split with space
    str = rv_new_string("hello world test");
    delimiter = rv_new_string(" ");
    RuntimeValue* args2[] = {str, delimiter};
    result = string_split(args2, 2);
    
    ASSERT_EQ(result->type, RV_ARRAY);
    // Should contain 3 elements: "hello", "world", "test"
    
    rv_unref(str);
    rv_unref(delimiter);
    rv_unref(result);
}

TEST(test_string_contains) {
    RuntimeValue* str = rv_new_string("hello world");
    RuntimeValue* substring = rv_new_string("world");
    RuntimeValue* args[] = {str, substring};
    RuntimeValue* result = string_contains(args, 2);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_BOOLEAN);
    ASSERT_TRUE(result->data.boolean);
    
    rv_unref(str);
    rv_unref(substring);
    rv_unref(result);
    
    // Test string not containing substring
    str = rv_new_string("hello world");
    substring = rv_new_string("foo");
    RuntimeValue* args2[] = {str, substring};
    result = string_contains(args2, 2);
    
    ASSERT_FALSE(result->data.boolean);
    
    rv_unref(str);
    rv_unref(substring);
    rv_unref(result);
    
    // Test case sensitivity
    str = rv_new_string("Hello World");
    substring = rv_new_string("hello");
    RuntimeValue* args3[] = {str, substring};
    result = string_contains(args3, 2);
    
    ASSERT_FALSE(result->data.boolean);  // Should be case sensitive
    
    rv_unref(str);
    rv_unref(substring);
    rv_unref(result);
}

TEST(test_string_replace) {
    RuntimeValue* str = rv_new_string("hello world hello");
    RuntimeValue* old_substr = rv_new_string("hello");
    RuntimeValue* new_substr = rv_new_string("hi");
    RuntimeValue* args[] = {str, old_substr, new_substr};
    RuntimeValue* result = string_replace(args, 3);
    
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->type, RV_STRING);
    ASSERT_STR_EQ(result->data.string.data, "hi world hi");
    
    rv_unref(str);
    rv_unref(old_substr);
    rv_unref(new_substr);
    rv_unref(result);
    
    // Test replace with empty string (removal)
    str = rv_new_string("hello world");
    old_substr = rv_new_string(" ");
    new_substr = rv_new_string("");
    RuntimeValue* args2[] = {str, old_substr, new_substr};
    result = string_replace(args2, 3);
    
    ASSERT_STR_EQ(result->data.string.data, "helloworld");
    
    rv_unref(str);
    rv_unref(old_substr);
    rv_unref(new_substr);
    rv_unref(result);
}

TEST(test_string_functions_with_empty_strings) {
    RuntimeValue* empty_str = rv_new_string("");
    RuntimeValue* args[] = { empty_str };
    
    // Test length of empty string
    RuntimeValue* length_result = string_length(args, 1);
    ASSERT_DOUBLE_EQ(length_result->data.number, 0.0, 0.001);
    rv_unref(length_result);
    
    // Test upper of empty string
    RuntimeValue* upper_result = string_upper(args, 1);
    ASSERT_STR_EQ(upper_result->data.string.data, "");
    rv_unref(upper_result);
    
    // Test lower of empty string
    RuntimeValue* lower_result = string_lower(args, 1);
    ASSERT_STR_EQ(lower_result->data.string.data, "");
    rv_unref(lower_result);
    
    // Test trim of empty string
    RuntimeValue* trim_result = string_trim(args, 1);
    ASSERT_STR_EQ(trim_result->data.string.data, "");
    rv_unref(trim_result);
    
    rv_unref(empty_str);
}

TEST(test_string_functions_with_special_characters) {
    RuntimeValue* str = rv_new_string("Hello\nWorld\tTest");
    RuntimeValue* args[] = { str };
    
    RuntimeValue* length_result = string_length(args, 1);
    ASSERT_DOUBLE_EQ(length_result->data.number, 16.0, 0.001);  // Including \n and \t
    rv_unref(length_result);
    
    RuntimeValue* upper_result = string_upper(args, 1);
    ASSERT_STR_EQ(upper_result->data.string.data, "HELLO\nWORLD\tTEST");
    rv_unref(upper_result);
    
    rv_unref(str);
}

TEST(test_string_functions_with_unicode) {
    // Test with unicode characters (if supported)
    RuntimeValue* str = rv_new_string("Héllo Wørld 🌍");
    RuntimeValue* args[] = { str };
    
    RuntimeValue* length_result = string_length(args, 1);
    // Length might be byte count or character count depending on implementation
    ASSERT_TRUE(length_result->data.number > 0);
    rv_unref(length_result);
    
    RuntimeValue* upper_result = string_upper(args, 1);
    ASSERT_NOT_NULL(upper_result);
    ASSERT_EQ(upper_result->type, RV_STRING);
    rv_unref(upper_result);
    
    rv_unref(str);
}

TEST_SUITE_BEGIN(string_functions_tests)
    RUN_TEST(test_string_length);
    RUN_TEST(test_string_upper);
    RUN_TEST(test_string_lower);
    RUN_TEST(test_string_trim);
    RUN_TEST(test_string_split);
    RUN_TEST(test_string_contains);
    RUN_TEST(test_string_replace);
    RUN_TEST(test_string_functions_with_empty_strings);
    RUN_TEST(test_string_functions_with_special_characters);
    RUN_TEST(test_string_functions_with_unicode);
TEST_SUITE_END