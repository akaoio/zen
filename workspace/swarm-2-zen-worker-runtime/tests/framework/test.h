/**
 * @file test.h
 * @brief Lightweight test framework for ZEN language implementation
 * 
 * This framework provides:
 * - Test assertions and macros
 * - Test suite organization
 * - Memory leak detection integration
 * - Automated test discovery
 */

#ifndef ZEN_TEST_FRAMEWORK_H
#define ZEN_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// ANSI color codes for test output
#define TEST_COLOR_RESET   "\033[0m"
#define TEST_COLOR_RED     "\033[31m"
#define TEST_COLOR_GREEN   "\033[32m"
#define TEST_COLOR_YELLOW  "\033[33m"
#define TEST_COLOR_BLUE    "\033[34m"
#define TEST_COLOR_MAGENTA "\033[35m"
#define TEST_COLOR_CYAN    "\033[36m"

// Global test statistics
extern int test_total;
extern int test_passed;
extern int test_failed;
extern const char* current_test_name;
extern const char* current_suite_name;

// Test result tracking - improved memory management
typedef struct {
    bool passed;
    char* message;  // Dynamically allocated, needs cleanup
    const char* file;
    int line;
} TestResult;

// Global test cleanup tracking
typedef struct {
    TestResult* results;
    size_t capacity;
    size_t count;
} TestResultList;

extern TestResultList test_results;

// Memory state for test leak detection
extern size_t test_start_current_allocated;
extern size_t test_start_outstanding_allocs;

/**
 * @brief Initialize test framework
 */
void test_init(void);

/**
 * @brief Clean up all test framework resources
 * This function ensures all dynamically allocated test resources are freed
 */
void test_cleanup(void);

/**
 * @brief Finalize test framework and print results
 * @return 0 if all tests passed, 1 if any failed
 */
int test_finalize(void);

/**
 * @brief Record test failure with detailed information
 * @param message Error message
 * @param file Source file name
 * @param line Line number
 */
void test_fail(const char* message, const char* file, int line);

/**
 * @brief Start a test suite
 * @param suite_name Name of the test suite
 */
void test_suite_start(const char* suite_name);

/**
 * @brief End a test suite
 */
void test_suite_end(void);

/**
 * @brief Start an individual test
 * @param test_name Name of the test
 */
void test_start(const char* test_name);

/**
 * @brief End an individual test
 */
void test_end(void);

// Forward declare all test functions
#define DECLARE_TEST(name) static void test_##name(void);

// Call a test function with framework setup
#define RUN_TEST(name) \
    do { \
        test_start(#name); \
        test_##name(); \
        test_end(); \
    } while(0)

// Define a test function
#define TEST(name) static void test_##name(void)

// Test suite macro - creates main function and runner
#define TEST_SUITE_BEGIN(name) \
    int main(void) { \
        test_init(); \
        test_suite_start(#name);

#define TEST_SUITE_END \
        test_suite_end(); \
        int result = test_finalize(); \
        test_cleanup(); \
        return result; \
    }

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected true, got false: %s", #condition); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected false, got true: %s", #condition); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected %ld, got %ld", (long)(expected), (long)(actual)); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NE(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected not equal to %ld, but got %ld", (long)(expected), (long)(actual)); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (!actual || !expected || strcmp(actual, expected) != 0) { \
            char msg[512]; \
            snprintf(msg, sizeof(msg), "Expected \"%s\", got \"%s\"", \
                    expected ? expected : "(null)", \
                    actual ? actual : "(null)"); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_NE(actual, expected) \
    do { \
        if (actual && expected && strcmp(actual, expected) == 0) { \
            char msg[512]; \
            snprintf(msg, sizeof(msg), "Expected not equal to \"%s\", but got \"%s\"", expected, actual); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if (ptr != NULL) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected NULL, got %p", (void*)ptr); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if (ptr == NULL) { \
            test_fail("Expected non-NULL pointer, got NULL", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_DOUBLE_EQ(actual, expected, epsilon) \
    do { \
        double diff = fabs((actual) - (expected)); \
        if (diff > (epsilon)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected %f, got %f (diff: %f > %f)", \
                    (double)(expected), (double)(actual), diff, (double)(epsilon)); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Memory testing helpers
#define ASSERT_NO_MEMORY_LEAKS() \
    do { \
        MemoryStats stats; \
        memory_get_stats(&stats); \
        if (stats.current_allocated > test_start_current_allocated) { \
            char msg[256]; \
            size_t leaked = stats.current_allocated - test_start_current_allocated; \
            snprintf(msg, sizeof(msg), "Memory leak detected: %zu bytes", leaked); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Enhanced memory leak detection with detailed reporting
#define ASSERT_NO_MEMORY_LEAKS_DETAILED() \
    do { \
        MemoryStats stats; \
        memory_get_stats(&stats); \
        size_t leaked_bytes = stats.current_allocated - test_start_current_allocated; \
        size_t leaked_allocs = (stats.allocation_count - stats.free_count) - test_start_outstanding_allocs; \
        if (leaked_bytes > 0 || leaked_allocs > 0) { \
            char msg[512]; \
            snprintf(msg, sizeof(msg), "Memory leak: %zu bytes, %zu outstanding allocations", \
                    leaked_bytes, leaked_allocs); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Memory snapshot for tests that need to check intermediate states
#define MEMORY_SNAPSHOT_SAVE() \
    MemoryStats snapshot; \
    memory_get_stats(&snapshot);

#define MEMORY_SNAPSHOT_CHECK(var_name) \
    do { \
        MemoryStats current; \
        memory_get_stats(&current); \
        if (current.current_allocated != var_name.current_allocated) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Memory changed: %zu -> %zu bytes", \
                    var_name.current_allocated, current.current_allocated); \
            test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Test output macros
#define TEST_INFO(fmt, ...) \
    printf(TEST_COLOR_CYAN "[INFO] " TEST_COLOR_RESET fmt "\n", ##__VA_ARGS__)

#define TEST_WARNING(fmt, ...) \
    printf(TEST_COLOR_YELLOW "[WARN] " TEST_COLOR_RESET fmt "\n", ##__VA_ARGS__)

#define TEST_ERROR(fmt, ...) \
    printf(TEST_COLOR_RED "[ERROR] " TEST_COLOR_RESET fmt "\n", ##__VA_ARGS__)

// Test fixture support
typedef struct {
    void (*setup)(void);
    void (*teardown)(void);
} TestFixture;

extern TestFixture test_fixture;

#define TEST_FIXTURE_SETUP(func) \
    test_fixture.setup = func

#define TEST_FIXTURE_TEARDOWN(func) \
    test_fixture.teardown = func

// Skip test macro (for tests that need features not yet implemented)
#define SKIP_TEST(reason) \
    do { \
        printf(TEST_COLOR_YELLOW "[SKIP] %s: %s" TEST_COLOR_RESET "\n", current_test_name, reason); \
        return; \
    } while(0)

#endif // ZEN_TEST_FRAMEWORK_H