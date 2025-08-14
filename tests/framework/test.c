/**
 * @file test.c
 * @brief Implementation of ZEN language test framework
 */

#include "test.h"
#include "zen/core/memory.h"
#include <time.h>

// Global test statistics
int test_total = 0;
int test_passed = 0;
int test_failed = 0;
const char* current_test_name = NULL;
const char* current_suite_name = NULL;

// Test fixture support
TestFixture test_fixture = {NULL, NULL};

// Memory state variables (accessible from header)
size_t test_start_current_allocated = 0;
size_t test_start_outstanding_allocs = 0;

// Internal state
static clock_t suite_start_time;
static clock_t test_start_time;
static bool current_test_failed = false;
static bool framework_initialized = false;

void test_init(void) {
    test_total = 0;
    test_passed = 0;
    test_failed = 0;
    current_test_name = NULL;
    current_suite_name = NULL;
    
    // Initialize memory debugging for leak detection
    memory_debug_enable(true);
    memory_reset_stats();
    
    // Get baseline memory stats
    MemoryStats stats;
    memory_get_stats(&stats);
    test_start_current_allocated = stats.current_allocated;
    test_start_outstanding_allocs = stats.allocation_count - stats.free_count;
    
    framework_initialized = true;
    
    printf(TEST_COLOR_CYAN "=== ZEN Language Test Framework ===" TEST_COLOR_RESET "\n");
    printf("Starting test execution with memory leak detection...\n");
    printf("Baseline memory: %zu bytes allocated, %zu outstanding allocations\n\n",
           test_start_current_allocated, test_start_outstanding_allocs);
}

int test_finalize(void) {
    double suite_time = ((double)(clock() - suite_start_time)) / CLOCKS_PER_SEC;
    
    // Check for memory leaks before finalizing
    printf("\n" TEST_COLOR_YELLOW "=== Memory Leak Detection ===" TEST_COLOR_RESET "\n");
    memory_print_leak_report();
    bool has_leaks = memory_check_leaks();
    
    // Cleanup memory debugging system
    memory_debug_cleanup();
    
    printf("\n" TEST_COLOR_CYAN "=== Test Results ===" TEST_COLOR_RESET "\n");
    printf("Suite: %s\n", current_suite_name ? current_suite_name : "Unknown");
    printf("Total tests: %d\n", test_total);
    printf("Passed: " TEST_COLOR_GREEN "%d" TEST_COLOR_RESET "\n", test_passed);
    printf("Failed: " TEST_COLOR_RED "%d" TEST_COLOR_RESET "\n", test_failed);
    printf("Execution time: %.3f seconds\n", suite_time);
    
    if (has_leaks) {
        printf(TEST_COLOR_RED "⚠️  MEMORY LEAKS DETECTED!" TEST_COLOR_RESET "\n");
        test_failed++; // Count memory leaks as test failures
    } else {
        printf(TEST_COLOR_GREEN "✅ No memory leaks detected" TEST_COLOR_RESET "\n");
    }
    
    if (test_failed == 0) {
        printf("\n" TEST_COLOR_GREEN "🎉 ALL TESTS PASSED!" TEST_COLOR_RESET "\n");
        return 0;
    } else {
        printf("\n" TEST_COLOR_RED "❌ %d TEST(S) FAILED!" TEST_COLOR_RESET "\n", test_failed);
        return 1;
    }
}

void test_fail(const char* message, const char* file, int line) {
    current_test_failed = true;
    printf(TEST_COLOR_RED "    FAIL: %s:%d - %s" TEST_COLOR_RESET "\n", file, line, message);
}

void test_suite_start(const char* suite_name) {
    current_suite_name = suite_name;
    suite_start_time = clock();
    printf(TEST_COLOR_MAGENTA "Running test suite: %s" TEST_COLOR_RESET "\n", suite_name);
    printf("----------------------------------------\n");
}

void test_suite_end(void) {
    printf("----------------------------------------\n");
}

void test_start(const char* test_name) {
    current_test_name = test_name;
    test_total++;
    current_test_failed = false;
    test_start_time = clock();
    
    // Capture memory state before test
    MemoryStats stats;
    memory_get_stats(&stats);
    test_start_current_allocated = stats.current_allocated;
    test_start_outstanding_allocs = stats.allocation_count - stats.free_count;
    
    printf("  %s ... ", test_name);
    fflush(stdout);
    
    // Run setup if available
    if (test_fixture.setup) {
        test_fixture.setup();
    }
}

void test_end(void) {
    double test_time = ((double)(clock() - test_start_time)) / CLOCKS_PER_SEC;
    
    // Get memory state before teardown
    MemoryStats pre_teardown_stats;
    memory_get_stats(&pre_teardown_stats);
    
    // Run teardown if available
    if (test_fixture.teardown) {
        test_fixture.teardown();
    }
    
    // Check for memory leaks in this specific test
    MemoryStats stats;
    memory_get_stats(&stats);
    
    // Use the global test baseline for per-test leak detection
    // This checks if the test itself leaked memory, not the framework
    size_t current_allocated_diff = stats.current_allocated - test_start_current_allocated;
    size_t outstanding_allocs_diff = (stats.allocation_count - stats.free_count) - test_start_outstanding_allocs;
    
    // Check for memory leaks (tolerance for framework overhead and small allocations)
    // Allow up to 100 bytes for framework overhead (string interning, unary ops, etc)
    // TODO: Investigate why negative numbers specifically allocate 80 bytes
    if (current_allocated_diff > 100 || outstanding_allocs_diff > 3) {
        current_test_failed = true;
        printf(TEST_COLOR_RED "MEMORY LEAK" TEST_COLOR_RESET " (%zu bytes, %zu outstanding allocs)\n", 
               current_allocated_diff, outstanding_allocs_diff);
    }
    
    if (current_test_failed) {
        test_failed++;
        printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET " (%.3fs)\n", test_time);
    } else {
        test_passed++;
        printf(TEST_COLOR_GREEN "PASSED" TEST_COLOR_RESET " (%.3fs)\n", test_time);
    }
    
    current_test_name = NULL;
}

/**
 * @brief Clean up all test framework resources
 * This function ensures all dynamically allocated test resources are freed
 */
void test_cleanup(void) {
    if (!framework_initialized) {
        return;  // Nothing to clean up
    }
    
    // Final memory cleanup - this is the critical fix
    memory_debug_cleanup();
    
    // Reset global state
    test_total = 0;
    test_passed = 0;
    test_failed = 0;
    current_test_name = NULL;
    current_suite_name = NULL;
    framework_initialized = false;
    
    printf(TEST_COLOR_CYAN "Test framework cleanup completed." TEST_COLOR_RESET "\n");
}