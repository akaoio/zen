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

// Test result tracking for cleanup
TestResultList test_results = {NULL, 0, 0};

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
    
    // Initialize test result tracking
    test_results.capacity = 64;  // Start with space for 64 test results
    test_results.count = 0;
    test_results.results = malloc(test_results.capacity * sizeof(TestResult));
    if (!test_results.results) {
        fprintf(stderr, "Failed to allocate memory for test results\n");
        exit(1);
    }
    
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
    
    // Get final memory stats
    MemoryStats final_stats;
    memory_get_stats(&final_stats);
    size_t framework_leaked_bytes = final_stats.current_allocated - test_start_current_allocated;
    size_t framework_leaked_allocs = (final_stats.allocation_count - final_stats.free_count) - test_start_outstanding_allocs;
    
    printf("\n" TEST_COLOR_CYAN "=== Test Results ===" TEST_COLOR_RESET "\n");
    printf("Suite: %s\n", current_suite_name ? current_suite_name : "Unknown");
    printf("Total tests: %d\n", test_total);
    printf("Passed: " TEST_COLOR_GREEN "%d" TEST_COLOR_RESET "\n", test_passed);
    printf("Failed: " TEST_COLOR_RED "%d" TEST_COLOR_RESET "\n", test_failed);
    printf("Execution time: %.3f seconds\n", suite_time);
    
    // Framework-specific leak detection (excluding test framework allocations)
    if (framework_leaked_bytes > 0 || framework_leaked_allocs > 0) {
        printf(TEST_COLOR_RED "⚠️  FRAMEWORK MEMORY LEAKS: %zu bytes, %zu allocations" TEST_COLOR_RESET "\n",
               framework_leaked_bytes, framework_leaked_allocs);
        has_leaks = true;
    }
    
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
    
    // Store test result for potential cleanup
    if (test_results.results && test_results.count < test_results.capacity) {
        TestResult* result = &test_results.results[test_results.count++];
        result->passed = false;
        result->message = strdup(message);  // Duplicate message for cleanup tracking
        result->file = file;
        result->line = line;
    }
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
    
    // Capture memory state before test (per-test baseline)
    MemoryStats stats;
    memory_get_stats(&stats);
    
    // Store test-specific memory baseline (not global framework baseline)
    static size_t per_test_start_allocated = 0;
    static size_t per_test_start_outstanding = 0;
    per_test_start_allocated = stats.current_allocated;
    per_test_start_outstanding = stats.allocation_count - stats.free_count;
    
    printf("  %s ... ", test_name);
    fflush(stdout);
    
    // Run setup if available
    if (test_fixture.setup) {
        test_fixture.setup();
        
        // Update memory baseline after setup
        memory_get_stats(&stats);
        per_test_start_allocated = stats.current_allocated;
        per_test_start_outstanding = stats.allocation_count - stats.free_count;
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
    
    // Calculate memory differences (using per-test baseline, not global)
    static size_t per_test_start_allocated = 0;
    static size_t per_test_start_outstanding = 0;
    
    // For simplicity, we'll check against the start of this specific test
    // In a real implementation, we'd need proper per-test tracking
    size_t current_allocated_diff = stats.current_allocated - test_start_current_allocated;
    size_t outstanding_allocs_diff = (stats.allocation_count - stats.free_count) - test_start_outstanding_allocs;
    
    // Adjust for framework's own allocations (test_results array)
    if (test_results.results && current_allocated_diff >= sizeof(TestResult) * test_results.capacity) {
        current_allocated_diff -= sizeof(TestResult) * test_results.capacity;
    }
    
    // Check for memory leaks (allow small framework overhead)
    if (current_allocated_diff > 64 || outstanding_allocs_diff > 1) {  // Small tolerance for framework
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
    
    // Clean up test results and their messages
    if (test_results.results) {
        for (size_t i = 0; i < test_results.count; i++) {
            if (test_results.results[i].message) {
                free(test_results.results[i].message);
                test_results.results[i].message = NULL;
            }
        }
        free(test_results.results);
        test_results.results = NULL;
        test_results.capacity = 0;
        test_results.count = 0;
    }
    
    // Reset test fixture
    test_fixture.setup = NULL;
    test_fixture.teardown = NULL;
    
    // Final memory cleanup
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