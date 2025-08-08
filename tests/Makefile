# ZEN Language Test Suite Makefile
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11 -I../src/include -Iframework
LDFLAGS = -lm -lcurl -lpcre2-8 -lyaml
VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

# Directories
TEST_FRAMEWORK_DIR = framework
UNIT_TEST_DIR = unit
INTEGRATION_TEST_DIR = integration
LANGUAGE_TEST_DIR = language
STDLIB_TEST_DIR = stdlib
MEMORY_TEST_DIR = memory

# Build directory
BUILD_DIR = build
SRC_DIR = ../src

# Source files for the main ZEN implementation
ZEN_SOURCES = $(wildcard $(SRC_DIR)/core/*.c $(SRC_DIR)/types/*.c $(SRC_DIR)/runtime/*.c $(SRC_DIR)/stdlib/*.c)
ZEN_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/zen_%.o,$(ZEN_SOURCES))

# Test framework
FRAMEWORK_SOURCES = $(wildcard $(TEST_FRAMEWORK_DIR)/*.c)
FRAMEWORK_OBJECTS = $(patsubst $(TEST_FRAMEWORK_DIR)/%.c,$(BUILD_DIR)/framework_%.o,$(FRAMEWORK_SOURCES))

# Unit tests
UNIT_LEXER_TESTS = $(wildcard $(UNIT_TEST_DIR)/lexer/*.c)
UNIT_PARSER_TESTS = $(wildcard $(UNIT_TEST_DIR)/parser/*.c)
UNIT_VALUE_TESTS = $(wildcard $(UNIT_TEST_DIR)/value/*.c)
UNIT_OPERATOR_TESTS = $(wildcard $(UNIT_TEST_DIR)/operators/*.c)

# Integration tests
INTEGRATION_BASIC_TESTS = $(wildcard $(INTEGRATION_TEST_DIR)/basic/*.c)

# Language tests
LANGUAGE_VARIABLE_TESTS = $(wildcard $(LANGUAGE_TEST_DIR)/variables/*.c)

# Standard library tests
STDLIB_STRING_TESTS = $(wildcard $(STDLIB_TEST_DIR)/string/*.c)

# Memory tests
MEMORY_TESTS = $(wildcard $(MEMORY_TEST_DIR)/*.c)

# All test source files
ALL_TEST_SOURCES = $(UNIT_LEXER_TESTS) $(UNIT_PARSER_TESTS) $(UNIT_VALUE_TESTS) $(UNIT_OPERATOR_TESTS) \
                   $(INTEGRATION_BASIC_TESTS) $(LANGUAGE_VARIABLE_TESTS) $(STDLIB_STRING_TESTS) $(MEMORY_TESTS)

# Test executables
TEST_EXECUTABLES = $(patsubst %.c,$(BUILD_DIR)/%.test,$(ALL_TEST_SOURCES))

# Default target
all: $(TEST_EXECUTABLES)

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(UNIT_TEST_DIR)/lexer
	@mkdir -p $(BUILD_DIR)/$(UNIT_TEST_DIR)/parser
	@mkdir -p $(BUILD_DIR)/$(UNIT_TEST_DIR)/value
	@mkdir -p $(BUILD_DIR)/$(UNIT_TEST_DIR)/operators
	@mkdir -p $(BUILD_DIR)/$(INTEGRATION_TEST_DIR)/basic
	@mkdir -p $(BUILD_DIR)/$(LANGUAGE_TEST_DIR)/variables
	@mkdir -p $(BUILD_DIR)/$(STDLIB_TEST_DIR)/string
	@mkdir -p $(BUILD_DIR)/$(MEMORY_TEST_DIR)

# Compile ZEN source files
$(BUILD_DIR)/zen_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# Compile test framework
$(BUILD_DIR)/framework_%.o: $(TEST_FRAMEWORK_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Compile and link test executables
$(BUILD_DIR)/%.test: %.c $(FRAMEWORK_OBJECTS) $(ZEN_OBJECTS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< $(FRAMEWORK_OBJECTS) $(ZEN_OBJECTS) $(LDFLAGS) -o $@

# Test execution targets

# Run all tests
test: test-unit test-integration test-language test-stdlib test-memory
	@echo "=== ALL TESTS COMPLETED ==="

# Run unit tests
test-unit: test-unit-lexer test-unit-parser test-unit-value test-unit-operators
	@echo "=== UNIT TESTS COMPLETED ==="

test-unit-lexer: $(patsubst %.c,$(BUILD_DIR)/%.test,$(UNIT_LEXER_TESTS))
	@echo "Running lexer unit tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

test-unit-parser: $(patsubst %.c,$(BUILD_DIR)/%.test,$(UNIT_PARSER_TESTS))
	@echo "Running parser unit tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

test-unit-value: $(patsubst %.c,$(BUILD_DIR)/%.test,$(UNIT_VALUE_TESTS))
	@echo "Running value system unit tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

test-unit-operators: $(patsubst %.c,$(BUILD_DIR)/%.test,$(UNIT_OPERATOR_TESTS))
	@echo "Running operator unit tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

# Run integration tests
test-integration: $(patsubst %.c,$(BUILD_DIR)/%.test,$(INTEGRATION_BASIC_TESTS))
	@echo "Running integration tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

# Run language tests
test-language: $(patsubst %.c,$(BUILD_DIR)/%.test,$(LANGUAGE_VARIABLE_TESTS))
	@echo "Running language tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

# Run standard library tests
test-stdlib: $(patsubst %.c,$(BUILD_DIR)/%.test,$(STDLIB_STRING_TESTS))
	@echo "Running standard library tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

# Run memory tests
test-memory: $(patsubst %.c,$(BUILD_DIR)/%.test,$(MEMORY_TESTS))
	@echo "Running memory management tests..."
	@for test in $^; do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

# Run tests with Valgrind
test-valgrind: $(TEST_EXECUTABLES)
	@echo "Running all tests with Valgrind memory checking..."
	@for test in $^; do \
		echo "Running $$test with Valgrind..."; \
		valgrind $(VALGRIND_FLAGS) ./$$test || exit 1; \
	done

test-valgrind-unit: $(patsubst %.c,$(BUILD_DIR)/%.test,$(UNIT_LEXER_TESTS) $(UNIT_PARSER_TESTS) $(UNIT_VALUE_TESTS) $(UNIT_OPERATOR_TESTS))
	@echo "Running unit tests with Valgrind..."
	@for test in $^; do \
		echo "Running $$test with Valgrind..."; \
		valgrind $(VALGRIND_FLAGS) ./$$test || exit 1; \
	done

test-valgrind-memory: $(patsubst %.c,$(BUILD_DIR)/%.test,$(MEMORY_TESTS))
	@echo "Running memory tests with Valgrind..."
	@for test in $^; do \
		echo "Running $$test with Valgrind..."; \
		valgrind $(VALGRIND_FLAGS) ./$$test || exit 1; \
	done

# Generate coverage report (requires gcov and lcov)
test-coverage: CFLAGS += --coverage
test-coverage: LDFLAGS += --coverage
test-coverage: clean $(TEST_EXECUTABLES)
	@echo "Running tests with coverage..."
	@$(MAKE) test
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@lcov --capture --directory . --output-file coverage/coverage.info
	@lcov --remove coverage/coverage.info '/usr/*' --output-file coverage/coverage.info
	@lcov --remove coverage/coverage.info '*/tests/*' --output-file coverage/coverage.info
	@genhtml coverage/coverage.info --output-directory coverage/html
	@echo "Coverage report generated in coverage/html/"

# Quick smoke test - just compile and run one test from each category
smoke-test: $(BUILD_DIR)/$(UNIT_TEST_DIR)/lexer/test_lexer_basic.test \
            $(BUILD_DIR)/$(UNIT_TEST_DIR)/value/test_value_basic.test \
            $(BUILD_DIR)/$(INTEGRATION_TEST_DIR)/basic/test_basic_programs.test
	@echo "Running smoke tests..."
	@./$(BUILD_DIR)/$(UNIT_TEST_DIR)/lexer/test_lexer_basic.test
	@./$(BUILD_DIR)/$(UNIT_TEST_DIR)/value/test_value_basic.test
	@./$(BUILD_DIR)/$(INTEGRATION_TEST_DIR)/basic/test_basic_programs.test
	@echo "Smoke tests passed!"

# Performance test - run tests and measure execution time
test-performance: $(TEST_EXECUTABLES)
	@echo "Running performance tests..."
	@time $(MAKE) test > /dev/null
	@echo "Performance test completed"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -rf coverage

# Deep clean - remove all generated files
distclean: clean
	find . -name "*.gcno" -delete
	find . -name "*.gcda" -delete
	find . -name "*.gcov" -delete

# List all available tests
list-tests:
	@echo "Available test executables:"
	@echo "Unit Tests:"
	@echo "  Lexer: $(UNIT_LEXER_TESTS)"
	@echo "  Parser: $(UNIT_PARSER_TESTS)"
	@echo "  Value: $(UNIT_VALUE_TESTS)"
	@echo "  Operators: $(UNIT_OPERATOR_TESTS)"
	@echo "Integration Tests:"
	@echo "  Basic: $(INTEGRATION_BASIC_TESTS)"
	@echo "Language Tests:"
	@echo "  Variables: $(LANGUAGE_VARIABLE_TESTS)"
	@echo "Standard Library Tests:"
	@echo "  String: $(STDLIB_STRING_TESTS)"
	@echo "Memory Tests:"
	@echo "  Memory: $(MEMORY_TESTS)"

# Help target
help:
	@echo "ZEN Language Test Suite"
	@echo "======================"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build all test executables"
	@echo "  test             - Run all tests"
	@echo "  test-unit        - Run all unit tests"
	@echo "  test-integration - Run integration tests"
	@echo "  test-language    - Run language feature tests"
	@echo "  test-stdlib      - Run standard library tests"
	@echo "  test-memory      - Run memory management tests"
	@echo "  test-valgrind    - Run all tests with Valgrind"
	@echo "  test-coverage    - Generate code coverage report"
	@echo "  smoke-test       - Run quick smoke tests"
	@echo "  test-performance - Measure test execution time"
	@echo "  clean            - Remove build artifacts"
	@echo "  distclean        - Remove all generated files"
	@echo "  list-tests       - List all available tests"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Individual test categories:"
	@echo "  test-unit-lexer     - Run lexer unit tests"
	@echo "  test-unit-parser    - Run parser unit tests"
	@echo "  test-unit-value     - Run value system tests"
	@echo "  test-unit-operators - Run operator tests"

.PHONY: all test test-unit test-integration test-language test-stdlib test-memory \
        test-valgrind test-coverage smoke-test test-performance clean distclean \
        list-tests help test-unit-lexer test-unit-parser test-unit-value test-unit-operators