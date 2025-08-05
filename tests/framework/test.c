/**
 * @file test.c
 * @brief Implementation of ZEN language test framework
 */

#include "test.h"
#include <time.h>

// Global test statistics
int zen_test_total = 0;
int zen_test_passed = 0;
int zen_test_failed = 0;
const char* zen_current_test_name = NULL;
const char* zen_current_suite_name = NULL;

// Test fixture support
TestFixture zen_test_fixture = {NULL, NULL};

// Internal state
static clock_t suite_start_time;
static clock_t test_start_time;
static bool current_test_failed = false;

void zen_test_init(void) {
    zen_test_total = 0;
    zen_test_passed = 0;
    zen_test_failed = 0;
    zen_current_test_name = NULL;
    zen_current_suite_name = NULL;
    
    printf(TEST_COLOR_CYAN "=== ZEN Language Test Framework ===" TEST_COLOR_RESET "\n");
    printf("Starting test execution...\n\n");
}

int zen_test_finalize(void) {
    double suite_time = ((double)(clock() - suite_start_time)) / CLOCKS_PER_SEC;
    
    printf("\n" TEST_COLOR_CYAN "=== Test Results ===" TEST_COLOR_RESET "\n");
    printf("Suite: %s\n", zen_current_suite_name ? zen_current_suite_name : "Unknown");
    printf("Total tests: %d\n", zen_test_total);
    printf("Passed: " TEST_COLOR_GREEN "%d" TEST_COLOR_RESET "\n", zen_test_passed);
    printf("Failed: " TEST_COLOR_RED "%d" TEST_COLOR_RESET "\n", zen_test_failed);
    printf("Execution time: %.3f seconds\n", suite_time);
    
    if (zen_test_failed == 0) {
        printf("\n" TEST_COLOR_GREEN "🎉 ALL TESTS PASSED!" TEST_COLOR_RESET "\n");
        return 0;
    } else {
        printf("\n" TEST_COLOR_RED "❌ %d TEST(S) FAILED!" TEST_COLOR_RESET "\n", zen_test_failed);
        return 1;
    }
}

void zen_test_fail(const char* message, const char* file, int line) {
    current_test_failed = true;
    printf(TEST_COLOR_RED "    FAIL: %s:%d - %s" TEST_COLOR_RESET "\n", file, line, message);
}

void zen_test_suite_start(const char* suite_name) {
    zen_current_suite_name = suite_name;
    suite_start_time = clock();
    printf(TEST_COLOR_MAGENTA "Running test suite: %s" TEST_COLOR_RESET "\n", suite_name);
    printf("----------------------------------------\n");
}

void zen_test_suite_end(void) {
    printf("----------------------------------------\n");
}

void zen_test_start(const char* test_name) {
    zen_current_test_name = test_name;
    zen_test_total++;
    current_test_failed = false;
    test_start_time = clock();
    
    printf("  %s ... ", test_name);
    fflush(stdout);
    
    // Run setup if available
    if (zen_test_fixture.setup) {
        zen_test_fixture.setup();
    }
}

void zen_test_end(void) {
    // Run teardown if available
    if (zen_test_fixture.teardown) {
        zen_test_fixture.teardown();
    }
    
    double test_time = ((double)(clock() - test_start_time)) / CLOCKS_PER_SEC;
    
    if (current_test_failed) {
        zen_test_failed++;
        printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET " (%.3fs)\n", test_time);
    } else {
        zen_test_passed++;
        printf(TEST_COLOR_GREEN "PASSED" TEST_COLOR_RESET " (%.3fs)\n", test_time);
    }
    
    zen_current_test_name = NULL;
}