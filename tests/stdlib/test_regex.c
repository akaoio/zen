/**
 * @file test_regex.c
 * @brief Comprehensive tests for PCRE2-based regex functionality in ZEN
 * @details Tests pattern matching, replacement, splitting, and compilation
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zen/stdlib/regex.h"
#include "zen/types/value.h"
#include "zen/types/array.h"
#include "zen/types/object.h"
#include "zen/core/memory.h"

/**
 * @brief Test basic regex matching functionality
 */
void test_regex_match_basic() {
    printf("Testing basic regex matching...\n");
    
    // Test simple digit matching
    Value* text = value_new_string("Hello 123 World 456");
    Value* pattern = value_new_string("\\d+");
    
    Value* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_OBJECT);
    
    // Check if match was found
    Value* matched = object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == VALUE_BOOLEAN);
    assert(matched->as.boolean == true);
    
    // Check match count
    Value* count = object_get(result, "count");
    assert(count != NULL);
    assert(count->type == VALUE_NUMBER);
    assert((int)count->as.number >= 1);
    
    // Check matches array exists
    Value* matches = object_get(result, "matches");
    assert(matches != NULL);
    assert(matches->type == VALUE_ARRAY);
    
    value_free(text);
    value_free(pattern);
    value_free(result);
    printf("✓ Basic regex matching test passed\n");
}

/**
 * @brief Test regex matching with no matches
 */
void test_regex_match_no_match() {
    printf("Testing regex matching with no matches...\n");
    
    Value* text = value_new_string("Hello World");
    Value* pattern = value_new_string("\\d+");
    
    Value* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_OBJECT);
    
    // Check if no match was found
    Value* matched = object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == VALUE_BOOLEAN);
    assert(matched->as.boolean == false);
    
    // Check match count is zero
    Value* count = object_get(result, "count");
    assert(count != NULL);
    assert(count->type == VALUE_NUMBER);
    assert((int)count->as.number == 0);
    
    value_free(text);
    value_free(pattern);
    value_free(result);
    printf("✓ No match test passed\n");
}

/**
 * @brief Test regex replacement functionality
 */
void test_regex_replace() {
    printf("Testing regex replacement...\n");
    
    Value* text = value_new_string("Hello 123 World 456");
    Value* pattern = value_new_string("\\d+");
    Value* replacement = value_new_string("NUMBER");
    
    Value* result = regex_replace(text, pattern, replacement);
    
    assert(result != NULL);
    assert(result->type == VALUE_STRING);
    
    // Check that digits were replaced
    const char* result_str = result->as.string->data;
    assert(strstr(result_str, "NUMBER") != NULL);
    assert(strstr(result_str, "123") == NULL);
    assert(strstr(result_str, "456") == NULL);
    
    value_free(text);
    value_free(pattern);
    value_free(replacement);
    value_free(result);
    printf("✓ Regex replacement test passed\n");
}

/**
 * @brief Test regex splitting functionality
 */
void test_regex_split() {
    printf("Testing regex splitting...\n");
    
    Value* text = value_new_string("apple,banana;orange:grape");
    Value* pattern = value_new_string("[,:;]");
    
    Value* result = regex_split(text, pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_ARRAY);
    
    // Should split into 4 parts
    size_t length = array_length(result);
    assert(length == 4);
    
    // Check individual parts
    Value* part1 = array_get(result, 0);
    assert(part1 != NULL);
    assert(part1->type == VALUE_STRING);
    assert(strcmp(part1->as.string->data, "apple") == 0);
    
    Value* part2 = array_get(result, 1);
    assert(part2 != NULL);
    assert(strcmp(part2->as.string->data, "banana") == 0);
    
    value_free(text);
    value_free(pattern);
    value_free(result);
    printf("✓ Regex splitting test passed\n");
}

/**
 * @brief Test regex compilation
 */
void test_regex_compile() {
    printf("Testing regex compilation...\n");
    
    Value* pattern = value_new_string("\\b\\w+@\\w+\\.\\w+\\b");
    
    Value* result = regex_compile(pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_OBJECT);
    
    // Check compilation status
    Value* compiled = object_get(result, "compiled");
    assert(compiled != NULL);
    assert(compiled->type == VALUE_BOOLEAN);
    assert(compiled->as.boolean == true);
    
    // Check pattern is stored
    Value* stored_pattern = object_get(result, "pattern");
    assert(stored_pattern != NULL);
    assert(stored_pattern->type == VALUE_STRING);
    assert(strcmp(stored_pattern->as.string->data, pattern->as.string->data) == 0);
    
    value_free(pattern);
    value_free(result);
    printf("✓ Regex compilation test passed\n");
}

/**
 * @brief Test error handling with invalid patterns
 */
void test_regex_error_handling() {
    printf("Testing regex error handling...\n");
    
    Value* text = value_new_string("test");
    Value* invalid_pattern = value_new_string("[invalid");  // Unclosed bracket
    
    Value* result = regex_match(text, invalid_pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_ERROR);
    
    value_free(text);
    value_free(invalid_pattern);
    value_free(result);
    printf("✓ Error handling test passed\n");
}

/**
 * @brief Test with null/invalid inputs
 */
void test_regex_input_validation() {
    printf("Testing input validation...\n");
    
    // Test with null inputs
    Value* result1 = regex_match(NULL, NULL);
    assert(result1 != NULL);
    assert(result1->type == VALUE_ERROR);
    value_free(result1);
    
    // Test with non-string inputs
    Value* number = value_new_number(123);
    Value* pattern = value_new_string("\\d+");
    
    Value* result2 = regex_match(number, pattern);
    assert(result2 != NULL);
    assert(result2->type == VALUE_ERROR);
    
    value_free(number);
    value_free(pattern);
    value_free(result2);
    printf("✓ Input validation test passed\n");
}

/**
 * @brief Test UTF-8 support
 */
void test_regex_utf8() {
    printf("Testing UTF-8 support...\n");
    
    Value* text = value_new_string("Hello 世界 123");
    Value* pattern = value_new_string("\\p{Han}+");  // Match Chinese characters
    
    Value* result = regex_match(text, pattern);
    
    assert(result != NULL);
    assert(result->type == VALUE_OBJECT);
    
    Value* matched = object_get(result, "matched");
    assert(matched != NULL);
    assert(matched->type == VALUE_BOOLEAN);
    assert(matched->as.boolean == true);
    
    value_free(text);
    value_free(pattern);
    value_free(result);
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