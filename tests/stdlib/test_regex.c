/**
 * @file test_regex.c
 * @brief Comprehensive tests for PCRE2-based regex functionality in ZEN
 * @details Tests pattern matching, replacement, splitting, and compilation
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zen/stdlib/regex.h"
#include "zen/core/runtime_value.h"
#include "zen/core/memory.h"

/**
 * @brief Test basic regex matching functionality
 */
void test_regex_match_basic() {
    printf("Testing basic regex matching...\n");
    
    // Test simple digit matching
    RuntimeValue* text = rv_new_string("Hello 123 World 456");
    RuntimeValue* pattern = rv_new_string("\\d+");
    
    RuntimeValue* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == RV_OBJECT);
    
    // Check if match was found
    RuntimeValue* matched = rv_object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == RV_BOOLEAN);
    assert(matched->data.boolean == true);
    
    // Check match count
    RuntimeValue* count = rv_object_get(result, "count");
    assert(count != NULL);
    assert(count->type == RV_NUMBER);
    assert((int)count->data.number >= 1);
    
    // Check matches array exists
    RuntimeValue* matches = rv_object_get(result, "matches");
    assert(matches != NULL);
    assert(matches->type == RV_ARRAY);
    
    rv_unref(text);
    rv_unref(pattern);
    rv_unref(result);
    printf("✓ Basic regex matching test passed\n");
}

/**
 * @brief Test regex matching with no matches
 */
void test_regex_match_no_match() {
    printf("Testing regex matching with no matches...\n");
    
    RuntimeValue* text = rv_new_string("Hello World");
    RuntimeValue* pattern = rv_new_string("\\d+");
    
    RuntimeValue* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == RV_OBJECT);
    
    // Check if no match was found
    RuntimeValue* matched = rv_object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == RV_BOOLEAN);
    assert(matched->data.boolean == false);
    
    // Check match count is zero
    RuntimeValue* count = rv_object_get(result, "count");
    assert(count != NULL);
    assert(count->type == RV_NUMBER);
    assert((int)count->data.number == 0);
    
    rv_unref(text);
    rv_unref(pattern);
    rv_unref(result);
    printf("✓ No match test passed\n");
}

/**
 * @brief Test regex replacement functionality
 */
void test_regex_replace() {
    printf("Testing regex replacement...\n");
    
    RuntimeValue* text = rv_new_string("Hello 123 World 456");
    RuntimeValue* pattern = rv_new_string("\\d+");
    RuntimeValue* replacement = rv_new_string("NUMBER");
    
    RuntimeValue* result = regex_replace(text, pattern, replacement);
    
    assert(result != NULL);
    assert(result->type == RV_STRING);
    
    // Check that digits were replaced
    const char* result_str = rv_get_string(result);
    assert(strstr(result_str, "NUMBER") != NULL);
    assert(strstr(result_str, "123") == NULL);
    assert(strstr(result_str, "456") == NULL);
    
    rv_unref(text);
    rv_unref(pattern);
    rv_unref(replacement);
    rv_unref(result);
    printf("✓ Regex replacement test passed\n");
}

/**
 * @brief Test regex splitting functionality
 */
void test_regex_split() {
    printf("Testing regex splitting...\n");
    
    RuntimeValue* text = rv_new_string("apple,banana;orange:grape");
    RuntimeValue* pattern = rv_new_string("[,:;]");
    
    RuntimeValue* result = regex_split(text, pattern);
    
    assert(result != NULL);
    assert(result->type == RV_ARRAY);
    
    // Should split into 4 parts
    size_t length = rv_array_size(result);
    assert(length == 4);
    
    // Check individual parts
    RuntimeValue* part1 = rv_array_get(result, 0);
    assert(part1 != NULL);
    assert(part1->type == RV_STRING);
    assert(strcmp(rv_get_string(part1), "apple") == 0);
    
    RuntimeValue* part2 = rv_array_get(result, 1);
    assert(part2 != NULL);
    assert(strcmp(rv_get_string(part2), "banana") == 0);
    
    rv_unref(text);
    rv_unref(pattern);
    rv_unref(result);
    printf("✓ Regex splitting test passed\n");
}

/**
 * @brief Test regex compilation
 */
void test_regex_compile() {
    printf("Testing regex compilation...\n");
    
    RuntimeValue* pattern = rv_new_string("\\b\\w+@\\w+\\.\\w+\\b");
    
    RuntimeValue* result = regex_compile(pattern);
    
    assert(result != NULL);
    assert(result->type == RV_OBJECT);
    
    // Check compilation status
    RuntimeValue* compiled = rv_object_get(result, "compiled");
    assert(compiled != NULL);
    assert(compiled->type == RV_BOOLEAN);
    assert(compiled->data.boolean == true);
    
    // Check pattern is stored
    RuntimeValue* stored_pattern = rv_object_get(result, "pattern");
    assert(stored_pattern != NULL);
    assert(stored_pattern->type == RV_STRING);
    assert(strcmp(rv_get_string(stored_pattern), rv_get_string(pattern)) == 0);
    
    rv_unref(pattern);
    rv_unref(result);
    printf("✓ Regex compilation test passed\n");
}

/**
 * @brief Test error handling with invalid patterns
 */
void test_regex_error_handling() {
    printf("Testing regex error handling...\n");
    
    RuntimeValue* text = rv_new_string("test");
    RuntimeValue* invalid_pattern = rv_new_string("[invalid");  // Unclosed bracket
    
    RuntimeValue* result = regex_match(text, invalid_pattern);
    
    assert(result != NULL);
    assert(result->type == RV_ERROR);
    
    rv_unref(text);
    rv_unref(invalid_pattern);
    rv_unref(result);
    printf("✓ Error handling test passed\n");
}

/**
 * @brief Test with null/invalid inputs
 */
void test_regex_input_validation() {
    printf("Testing input validation...\n");
    
    // Test with null inputs
    RuntimeValue* result1 = regex_match(NULL, NULL);
    assert(result1 != NULL);
    assert(result1->type == RV_ERROR);
    rv_unref(result1);
    
    // Test with non-string inputs
    RuntimeValue* number = rv_new_number(123);
    RuntimeValue* pattern = rv_new_string("\\d+");
    
    RuntimeValue* result2 = regex_match(number, pattern);
    assert(result2 != NULL);
    assert(result2->type == RV_ERROR);
    
    rv_unref(number);
    rv_unref(pattern);
    rv_unref(result2);
    printf("✓ Input validation test passed\n");
}

/**
 * @brief Test UTF-8 support
 */
void test_regex_utf8() {
    printf("Testing UTF-8 support...\n");
    
    RuntimeValue* text = rv_new_string("Hello 世界 123");
    RuntimeValue* pattern = rv_new_string("\\p{Han}+");  // Match Chinese characters
    
    RuntimeValue* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == RV_OBJECT);
    
    RuntimeValue* matched = rv_object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == RV_BOOLEAN);
    assert(matched->data.boolean == true);
    
    rv_unref(text);
    rv_unref(pattern);
    rv_unref(result);
    printf("✓ UTF-8 support test passed\n");
}

/**
 * @brief Main test runner
 */
int main() {
    printf("Running ZEN Regex Tests\n");
    printf("======================\n\n");
    
    // Initialize memory debugging
    memory_debug_enable(true);
    
    // Run all tests
    test_regex_match_basic();
    test_regex_match_no_match();
    test_regex_replace();
    test_regex_split();
    test_regex_compile();
    test_regex_error_handling();
    test_regex_input_validation();
    test_regex_utf8();
    
    printf("\n======================\n");
    printf("All regex tests passed!\n");
    
    // Cleanup regex cache
    regex_cleanup();
    
    // Check for memory leaks
    if (memory_check_leaks()) {
        printf("✗ Memory leaks detected!\n");
        return 1;
    } else {
        printf("✓ No memory leaks detected\n");
    }
    
    return 0;
}