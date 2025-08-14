# ZEN Language Makefile
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11 -Isrc/include
PREFIX ?= /usr/local
BUILD_DIR ?= .

# YAML support enabled with libyaml
YAML_SRC = src/stdlib/yaml.c
YAML_LIB = -lyaml
$(info YAML support: Enabled (using libyaml))

LDFLAGS = -lm -lcurl -lpcre2-8 $(YAML_LIB)

# Main executable
exec = $(BUILD_DIR)/zen
sources := $(wildcard src/main.c src/core/*.c src/types/*.c src/runtime/*.c) \
           src/stdlib/io.c src/stdlib/stdlib.c src/stdlib/string.c \
           src/stdlib/json.c src/stdlib/array.c src/stdlib/math.c \
           src/stdlib/convert.c src/stdlib/datastructures.c \
           src/stdlib/logic.c src/stdlib/module.c src/stdlib/system.c \
           src/stdlib/datetime.c src/stdlib/logging.c \
           src/stdlib/http.c src/stdlib/regex.c src/stdlib/database.c $(YAML_SRC)
objects := $(patsubst %.c,$(BUILD_DIR)/%.o,$(sources))

# Directories
SRCDIR = src
TESTDIR = tests
TOOLDIR = tools

# Default target
all: $(exec)

$(exec): $(objects)
	@mkdir -p $(dir $@)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o $(exec)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# Installation
install: $(exec)
	mkdir -p $(PREFIX)/bin
	cp ./zen $(PREFIX)/bin/zen

uninstall:
	rm -f $(PREFIX)/bin/zen

# Cleaning
clean:
	-rm -f *.out
	-rm -f *.o
	-rm -f src/*.o
	-rm -f src/*/*.o
	-rm -f $(exec)
	-rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/src/*.o $(BUILD_DIR)/src/*/*.o
	-rm -rf $(BUILD_DIR)/zen_*

# Code formatting
format:
	find src -name '*.c' -o -name '*.h' | xargs clang-format -i

format-check:
	find src -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror


# Linting (when cppcheck is available)
lint:
	@which cppcheck > /dev/null 2>&1 && cppcheck --enable=all --error-exitcode=1 src/ || echo "cppcheck not installed, skipping lint"

# Testing targets
test:
	@cd $(TESTDIR) && $(MAKE) test

test-unit:
	@cd $(TESTDIR) && $(MAKE) test-unit

test-integration:
	@cd $(TESTDIR) && $(MAKE) test-integration

test-language:
	@cd $(TESTDIR) && $(MAKE) test-language

test-stdlib:
	@cd $(TESTDIR) && $(MAKE) test-stdlib

test-memory:
	@cd $(TESTDIR) && $(MAKE) test-memory

test-valgrind:
	@cd $(TESTDIR) && $(MAKE) test-valgrind

test-coverage:
	@cd $(TESTDIR) && $(MAKE) test-coverage

test-smoke:
	@cd $(TESTDIR) && $(MAKE) smoke-test

# Memory checking (when valgrind is available)
valgrind: $(exec)
	@which valgrind > /dev/null 2>&1 && valgrind --leak-check=full --show-leak-kinds=all ./$(exec) || echo "valgrind not installed"

# Coverage (for future implementation)
coverage:
	@echo "Coverage not yet implemented"

# Debug helpers
debug-lexer: $(exec)
	./$(exec) --debug-lexer

debug-ast: $(exec)
	./$(exec) --debug-ast

# Manifest enforcement
enforce:
	node scripts/enforce_manifest.js

enforce-generate:
	node scripts/enforce_manifest.js --generate

# Project visualization
vision:
	@node scripts/vision.js

reality:
	@node scripts/reality.js

# Setup development environment
setup-dev: setup-hooks
	@echo "Development environment ready!"

setup-hooks:
	./scripts/setup_hooks.sh

# Help
help:
	@echo "ZEN Language Makefile targets:"
	@echo "  make              - Build the zen interpreter"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install zen to PREFIX/bin (default: /usr/local)"
	@echo "  make uninstall    - Remove installed zen"
	@echo "  make format       - Format code with clang-format"
	@echo "  make format-check - Check code formatting"
	@echo "  make enforce      - Enforce manifest compliance"
	@echo "  make enforce-generate - Generate stubs from manifest"
	@echo "  make vision       - Visualize project structure and status"
	@echo "  make reality      - Show actual project structure with real files/functions"
	@echo "  make lint         - Run static analysis"
	@echo "  make test         - Run comprehensive test suite"
	@echo "  make test-unit    - Run unit tests"
	@echo "  make test-integration - Run integration tests"
	@echo "  make test-language - Run language feature tests"
	@echo "  make test-stdlib  - Run standard library tests"
	@echo "  make test-memory  - Run memory management tests"
	@echo "  make test-valgrind - Run tests with Valgrind"
	@echo "  make test-coverage - Generate code coverage report"
	@echo "  make test-smoke   - Run quick smoke tests"
	@echo "  make valgrind     - Check for memory leaks"
	@echo "  make setup-dev    - Setup development environment"
	@echo "  make help         - Show this help message"

.PHONY: all clean install uninstall format format-check lint test test-unit test-integration test-language test-stdlib test-memory test-valgrind test-coverage test-smoke valgrind coverage debug-lexer debug-ast enforce enforce-generate vision reality setup-dev setup-hooks help
