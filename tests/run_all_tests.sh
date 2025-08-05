#!/bin/bash

# ZEN Language Comprehensive Test Runner
# This script runs all test categories and provides detailed reporting

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test configuration
VALGRIND_ENABLED=${VALGRIND_ENABLED:-true}
COVERAGE_ENABLED=${COVERAGE_ENABLED:-false}
VERBOSE=${VERBOSE:-false}

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_header() {
    echo
    print_status $CYAN "=============================================="
    print_status $CYAN "$1"
    print_status $CYAN "=============================================="
    echo
}

print_section() {
    echo
    print_status $BLUE "--- $1 ---"
    echo
}

# Function to run a test category
run_test_category() {
    local category=$1
    local description=$2
    
    print_section "Running $description"
    
    if make test-$category; then
        print_status $GREEN "✓ $description PASSED"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        print_status $RED "✗ $description FAILED"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# Function to run Valgrind tests
run_valgrind_tests() {
    if [ "$VALGRIND_ENABLED" = "true" ]; then
        print_section "Running Memory Leak Detection (Valgrind)"
        
        if command -v valgrind >/dev/null 2>&1; then
            if make test-valgrind; then
                print_status $GREEN "✓ Valgrind Memory Tests PASSED"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                print_status $RED "✗ Valgrind Memory Tests FAILED"
                FAILED_TESTS=$((FAILED_TESTS + 1))
            fi
        else
            print_status $YELLOW "⚠ Valgrind not installed, skipping memory tests"
        fi
        
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
}

# Function to generate coverage report
generate_coverage() {
    if [ "$COVERAGE_ENABLED" = "true" ]; then
        print_section "Generating Code Coverage Report"
        
        if command -v lcov >/dev/null 2>&1; then
            if make test-coverage; then
                print_status $GREEN "✓ Coverage report generated in tests/coverage/html/"
            else
                print_status $RED "✗ Coverage report generation failed"
            fi
        else
            print_status $YELLOW "⚠ lcov not installed, skipping coverage report"
        fi
    fi
}

# Function to test example ZEN programs
test_zen_examples() {
    print_section "Testing ZEN Example Programs"
    
    local examples_dir="tests/examples"
    local zen_executable="../zen"
    
    if [ ! -f "$zen_executable" ]; then
        print_status $YELLOW "⚠ ZEN executable not found, building first..."
        if ! make -C .. clean && make -C ..; then
            print_status $RED "✗ Failed to build ZEN executable"
            return 1
        fi
    fi
    
    local example_passed=0
    local example_failed=0
    
    for zen_file in "$examples_dir"/*.zen; do
        if [ -f "$zen_file" ]; then
            local filename=$(basename "$zen_file")
            echo "Testing $filename..."
            
            if timeout 10s "$zen_executable" "$zen_file" >/dev/null 2>&1; then
                print_status $GREEN "✓ $filename executed successfully"
                example_passed=$((example_passed + 1))
            else
                print_status $RED "✗ $filename failed to execute"
                example_failed=$((example_failed + 1))
            fi
        fi
    done
    
    if [ $example_failed -eq 0 ]; then
        print_status $GREEN "✓ All ZEN examples passed ($example_passed/$((example_passed + example_failed)))"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        print_status $RED "✗ Some ZEN examples failed ($example_passed/$((example_passed + example_failed)) passed)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# Function to run performance benchmarks
run_performance_tests() {
    print_section "Running Performance Tests"
    
    echo "Measuring test execution time..."
    start_time=$(date +%s)
    
    # Run a quick smoke test to measure performance
    if make smoke-test >/dev/null 2>&1; then
        end_time=$(date +%s)
        execution_time=$((end_time - start_time))
        
        if [ $execution_time -lt 10 ]; then
            print_status $GREEN "✓ Performance test passed (${execution_time}s - Fast)"
        elif [ $execution_time -lt 30 ]; then
            print_status $YELLOW "⚠ Performance test acceptable (${execution_time}s - Moderate)"
        else
            print_status $RED "✗ Performance test slow (${execution_time}s - Needs optimization)"
        fi
    else
        print_status $RED "✗ Performance test failed"
    fi
}

# Function to display final summary
show_summary() {
    print_header "TEST SUMMARY"
    
    echo "Total test categories: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    echo
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_status $GREEN "🎉 ALL TESTS PASSED! ZEN language implementation is working correctly."
        echo
        print_status $CYAN "Success Criteria Met:"
        print_status $GREEN "✓ All unit tests passing"
        print_status $GREEN "✓ All integration tests passing"
        print_status $GREEN "✓ All language features working"
        print_status $GREEN "✓ All standard library functions operational"
        if [ "$VALGRIND_ENABLED" = "true" ] && command -v valgrind >/dev/null 2>&1; then
            print_status $GREEN "✓ No memory leaks detected"
        fi
        echo
        print_status $CYAN "ZEN Language Implementation: READY FOR USE"
        exit 0
    else
        print_status $RED "❌ $FAILED_TESTS TEST CATEGORIES FAILED"
        echo
        print_status $YELLOW "Issues to address:"
        print_status $RED "• Review failed test output above"
        print_status $RED "• Fix implementation bugs"
        print_status $RED "• Ensure all MANIFEST.json functions are implemented"
        print_status $RED "• Check for memory leaks if Valgrind tests failed"
        echo
        print_status $RED "ZEN Language Implementation: NEEDS WORK"
        exit 1
    fi
}

# Main execution
main() {
    print_header "ZEN LANGUAGE COMPREHENSIVE TEST SUITE"
    
    print_status $CYAN "Starting comprehensive testing of ZEN language implementation..."
    print_status $CYAN "This will verify 100% functionality across all components."
    echo
    
    # Build the test suite first
    print_section "Building Test Suite"
    if make clean && make all; then
        print_status $GREEN "✓ Test suite built successfully"
    else
        print_status $RED "✗ Failed to build test suite"
        exit 1
    fi
    
    # Run all test categories
    run_test_category "unit" "Unit Tests (Lexer, Parser, Value System, Operators)"
    run_test_category "integration" "Integration Tests (Complete ZEN Programs)"
    run_test_category "language" "Language Tests (ZEN Syntax and Semantics)"
    run_test_category "stdlib" "Standard Library Tests"
    run_test_category "memory" "Memory Management Tests"
    
    # Run Valgrind tests
    run_valgrind_tests
    
    # Test example ZEN programs
    test_zen_examples
    
    # Run performance tests
    run_performance_tests
    
    # Generate coverage report if enabled
    generate_coverage
    
    # Show final summary
    show_summary
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-valgrind)
            VALGRIND_ENABLED=false
            shift
            ;;
        --coverage)
            COVERAGE_ENABLED=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "ZEN Language Test Runner"
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --no-valgrind    Skip Valgrind memory tests"
            echo "  --coverage       Generate code coverage report"
            echo "  --verbose        Enable verbose output"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Change to tests directory
cd "$(dirname "$0")"

# Run main function
main