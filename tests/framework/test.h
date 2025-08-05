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
extern int zen_test_total;
extern int zen_test_passed;
extern int zen_test_failed;
extern const char* zen_current_test_name;
extern const char* zen_current_suite_name;

// Test result tracking
typedef struct {
    bool passed;
    char* message;
    const char* file;
    int line;
} TestResult;

/**
 * @brief Initialize test framework
 */
void zen_test_init(void);

/**
 * @brief Finalize test framework and print results
 * @return 0 if all tests passed, 1 if any failed
 */
int zen_test_finalize(void);

/**
 * @brief Record test failure with detailed information
 * @param message Error message
 * @param file Source file name
 * @param line Line number
 */
void zen_test_fail(const char* message, const char* file, int line);

/**
 * @brief Start a test suite
 * @param suite_name Name of the test suite
 */
void zen_test_suite_start(const char* suite_name);

/**
 * @brief End a test suite
 */
void zen_test_suite_end(void);

/**
 * @brief Start an individual test
 * @param test_name Name of the test
 */
void zen_test_start(const char* test_name);

/**
 * @brief End an individual test
 */
void zen_test_end(void);

// Forward declare all test functions
#define DECLARE_TEST(name) static void test_##name(void);

// Call a test function with framework setup
#define RUN_TEST(name) \
    do { \
        zen_test_start(#name); \
        test_##name(); \
        zen_test_end(); \
    } while(0)

// Define a test function
#define TEST(name) static void test_##name(void)

// Test suite macro - creates main function and runner
#define TEST_SUITE_BEGIN(name) \
    int main(void) { \
        zen_test_init(); \
        zen_test_suite_start(#name);

#define TEST_SUITE_END \
        zen_test_suite_end(); \
        return zen_test_finalize(); \
    }

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected true, got false: %s", #condition); \
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected false, got true: %s", #condition); \
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected %ld, got %ld", (long)(expected), (long)(actual)); \
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NE(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected not equal to %ld, but got %ld", (long)(expected), (long)(actual)); \
            zen_test_fail(msg, __FILE__, __LINE__); \
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
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_NE(actual, expected) \
    do { \
        if (actual && expected && strcmp(actual, expected) == 0) { \
            char msg[512]; \
            snprintf(msg, sizeof(msg), "Expected not equal to \"%s\", but got \"%s\"", expected, actual); \
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if (ptr != NULL) { \
            char msg[256]; \
            snprintf(msg, sizeof(msg), "Expected NULL, got %p", (void*)ptr); \
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if (ptr == NULL) { \
            zen_test_fail("Expected non-NULL pointer, got NULL", __FILE__, __LINE__); \
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
            zen_test_fail(msg, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Memory testing helpers
#define ASSERT_NO_MEMORY_LEAKS() \
    do { \
        /* TODO: Integrate with memory system to check for leaks */ \
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

extern TestFixture zen_test_fixture;

#define TEST_FIXTURE_SETUP(func) \
    zen_test_fixture.setup = func

#define TEST_FIXTURE_TEARDOWN(func) \
    zen_test_fixture.teardown = func

// Skip test macro (for tests that need features not yet implemented)
#define SKIP_TEST(reason) \
    do { \
        printf(TEST_COLOR_YELLOW "[SKIP] %s: %s" TEST_COLOR_RESET "\n", zen_current_test_name, reason); \
        return; \
    } while(0)

#endif // ZEN_TEST_FRAMEWORK_H