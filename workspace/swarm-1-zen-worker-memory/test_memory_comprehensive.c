/**
 * @file test_memory_comprehensive.c
 * @brief Comprehensive test suite for memory management system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "zen/core/memory.h"

#define ASSERT_TEST(condition, test_name) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", test_name); \
            return false; \
        } else { \
            printf("PASS: %s\n", test_name); \
        } \
    } while(0)

bool test_basic_allocation(void) {
    printf("\n=== Testing Basic Allocation ===\n");
    
    // Test memory_alloc
    void* ptr = memory_alloc(1024);
    ASSERT_TEST(ptr != NULL, "memory_alloc returns non-NULL");
    
    // Test that memory is zero-initialized
    char* char_ptr = (char*)ptr;
    bool all_zero = true;
    for (int i = 0; i < 1024; i++) {
        if (char_ptr[i] != 0) {
            all_zero = false;
            break;
        }
    }
    ASSERT_TEST(all_zero, "memory is zero-initialized");
    
    // Test memory_free
    memory_free(ptr);
    
    // Test memory_alloc with zero size
    void* zero_ptr = memory_alloc(0);
    ASSERT_TEST(zero_ptr == NULL, "memory_alloc(0) returns NULL");
    
    return true;
}

bool test_string_operations(void) {
    printf("\n=== Testing String Operations ===\n");
    
    const char* test_str = "Hello World";
    char* dup_str = memory_strdup(test_str);
    
    ASSERT_TEST(dup_str != NULL, "memory_strdup returns non-NULL");
    ASSERT_TEST(strcmp(dup_str, test_str) == 0, "memory_strdup duplicates correctly");
    
    memory_free(dup_str);
    
    // Test with NULL input
    char* null_dup = memory_strdup(NULL);
    ASSERT_TEST(null_dup == NULL, "memory_strdup(NULL) returns NULL");
    
    return true;
}

bool test_reallocation(void) {
    printf("\n=== Testing Reallocation ===\n");
    
    // Test basic reallocation
    void* ptr = memory_alloc(100);
    ASSERT_TEST(ptr != NULL, "initial allocation succeeds");
    
    void* new_ptr = memory_realloc(ptr, 200);
    ASSERT_TEST(new_ptr != NULL, "reallocation to larger size succeeds");
    
    // Test reallocation to smaller size
    void* smaller_ptr = memory_realloc(new_ptr, 50);
    ASSERT_TEST(smaller_ptr != NULL, "reallocation to smaller size succeeds");
    
    // Test reallocation to zero (should act like free)
    void* zero_ptr = memory_realloc(smaller_ptr, 0);
    ASSERT_TEST(zero_ptr == NULL, "reallocation to 0 acts like free");
    
    // Test reallocation of NULL (should act like alloc)
    void* null_realloc = memory_realloc(NULL, 100);
    ASSERT_TEST(null_realloc != NULL, "reallocation of NULL acts like alloc");
    memory_free(null_realloc);
    
    return true;
}

bool test_reference_counting(void) {
    printf("\n=== Testing Reference Counting ===\n");
    
    size_t ref_count = 1;
    
    // Test increment
    size_t new_count = memory_ref_inc(&ref_count);
    ASSERT_TEST(new_count == 2, "memory_ref_inc increments correctly");
    ASSERT_TEST(ref_count == 2, "reference count value updated");
    
    // Test get
    size_t get_count = memory_ref_get(&ref_count);
    ASSERT_TEST(get_count == 2, "memory_ref_get returns correct value");
    
    // Test decrement
    size_t dec_count = memory_ref_dec(&ref_count);
    ASSERT_TEST(dec_count == 1, "memory_ref_dec decrements correctly");
    ASSERT_TEST(ref_count == 1, "reference count value updated after decrement");
    
    // Test decrement to zero
    dec_count = memory_ref_dec(&ref_count);
    ASSERT_TEST(dec_count == 0, "memory_ref_dec can reach zero");
    
    // Test operations with NULL pointer
    ASSERT_TEST(memory_ref_inc(NULL) == 0, "memory_ref_inc(NULL) returns 0");
    ASSERT_TEST(memory_ref_dec(NULL) == 0, "memory_ref_dec(NULL) returns 0");
    ASSERT_TEST(memory_ref_get(NULL) == 0, "memory_ref_get(NULL) returns 0");
    
    return true;
}

bool test_debugging_features(void) {
    printf("\n=== Testing Debugging Features ===\n");
    
    // Test debug enable/disable
    ASSERT_TEST(!memory_debug_is_enabled(), "debugging initially disabled");
    
    memory_debug_enable(true);
    ASSERT_TEST(memory_debug_is_enabled(), "debugging can be enabled");
    
    memory_debug_enable(false);
    ASSERT_TEST(!memory_debug_is_enabled(), "debugging can be disabled");
    
    // Test statistics (initially should be zero or close to zero)
    MemoryStats stats;
    memory_reset_stats();
    memory_get_stats(&stats);
    
    ASSERT_TEST(stats.current_allocated == 0, "stats reset correctly");
    
    // Allocate some memory and check stats
    void* test_ptr = memory_alloc(500);
    memory_get_stats(&stats);
    
    ASSERT_TEST(stats.allocation_count >= 1, "allocation count increases");
    ASSERT_TEST(stats.current_allocated >= 500, "current allocated increases");
    
    memory_free(test_ptr);
    memory_get_stats(&stats);
    
    ASSERT_TEST(stats.free_count >= 1, "free count increases");
    
    return true;
}

bool test_debug_functions(void) {
    printf("\n=== Testing Debug Functions ===\n");
    
    // Enable debugging to track allocations
    memory_debug_enable(true);
    
    // Test debug allocation functions
    void* debug_ptr = memory_debug_alloc(256, __FILE__, __LINE__);
    ASSERT_TEST(debug_ptr != NULL, "memory_debug_alloc works");
    
    // Test debug reallocation
    void* debug_realloc_ptr = memory_debug_realloc(debug_ptr, 512, __FILE__, __LINE__);
    ASSERT_TEST(debug_realloc_ptr != NULL, "memory_debug_realloc works");
    
    // Test debug string duplication
    char* debug_str = memory_debug_strdup("test string", __FILE__, __LINE__);
    ASSERT_TEST(debug_str != NULL, "memory_debug_strdup works");
    ASSERT_TEST(strcmp(debug_str, "test string") == 0, "debug string duplication is correct");
    
    // Clean up
    memory_debug_free(debug_realloc_ptr, __FILE__, __LINE__);
    memory_debug_free(debug_str, __FILE__, __LINE__);
    
    // Check for leaks (should be 0 after cleanup)
    size_t leaks = memory_check_leaks();
    ASSERT_TEST(leaks == 0, "no memory leaks detected");
    
    memory_debug_enable(false);
    return true;
}

bool test_validation_functions(void) {
    printf("\n=== Testing Validation Functions ===\n");
    
    // Enable debugging for pointer tracking
    memory_debug_enable(true);
    
    void* valid_ptr = memory_alloc(128);
    
    // Test pointer validation (only works when debugging enabled)
    ASSERT_TEST(memory_is_tracked(valid_ptr), "allocated pointer is tracked");
    ASSERT_TEST(memory_validate_ptr(valid_ptr, 0), "pointer validation works");
    ASSERT_TEST(memory_validate_ptr(valid_ptr, 128), "pointer validation with size works");
    ASSERT_TEST(!memory_validate_ptr(valid_ptr, 256), "pointer validation fails with wrong size");
    
    // Test with invalid pointer
    ASSERT_TEST(!memory_is_tracked(NULL), "NULL pointer is not tracked");
    ASSERT_TEST(!memory_validate_ptr(NULL, 0), "NULL pointer validation fails");
    
    memory_free(valid_ptr);
    
    // After free, pointer should no longer be tracked
    ASSERT_TEST(!memory_is_tracked(valid_ptr), "freed pointer is no longer tracked");
    
    memory_debug_enable(false);
    return true;
}

int main(void) {
    printf("Starting comprehensive memory management tests...\n");
    
    bool all_passed = true;
    
    all_passed &= test_basic_allocation();
    all_passed &= test_string_operations();
    all_passed &= test_reallocation();
    all_passed &= test_reference_counting();
    all_passed &= test_debugging_features();
    all_passed &= test_debug_functions();
    all_passed &= test_validation_functions();
    
    // Final cleanup
    memory_debug_cleanup();
    
    printf("\n=== Test Results ===\n");
    if (all_passed) {
        printf("✓ All tests PASSED!\n");
        printf("Memory management system is working correctly.\n");
        return 0;
    } else {
        printf("✗ Some tests FAILED!\n");
        return 1;
    }
}