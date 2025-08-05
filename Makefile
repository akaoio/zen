# ZEN Language Makefile
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11 -Isrc/include
LDFLAGS = 
PREFIX ?= /usr/local

# Main executable
exec = zen
sources = $(wildcard src/main.c src/core/*.c src/types/*.c src/runtime/*.c src/stdlib/*.c)
objects = $(sources:.c=.o)

# Directories
SRCDIR = src
TESTDIR = tests
TOOLDIR = tools

# Default target
all: $(exec)

$(exec): $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o $(exec)

%.o: %.c
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
	-rm -f $(exec)

# Code formatting
format:
	find src -name '*.c' -o -name '*.h' | xargs clang-format -i

format-check:
	find src -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror


# Linting (when cppcheck is available)
lint:
	@which cppcheck > /dev/null 2>&1 && cppcheck --enable=all --error-exitcode=1 src/ || echo "cppcheck not installed, skipping lint"

# Testing targets (for future implementation)
test:
	@echo "Tests not yet implemented"

test-unit:
	@echo "Unit tests not yet implemented"

test-integration:
	@echo "Integration tests not yet implemented"

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
	@echo "  make lint         - Run static analysis"
	@echo "  make test         - Run all tests"
	@echo "  make valgrind     - Check for memory leaks"
	@echo "  make setup-dev    - Setup development environment"
	@echo "  make help         - Show this help message"

.PHONY: all clean install uninstall format format-check lint test test-unit test-integration valgrind coverage debug-lexer debug-ast enforce enforce-generate vision setup-dev setup-hooks help
