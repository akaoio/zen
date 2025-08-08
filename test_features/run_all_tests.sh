#!/bin/bash

# ZEN Feature Test Runner
# This script runs all feature tests and documents what works and what doesn't

echo "=== ZEN Language Feature Testing ==="
echo "Testing each claimed feature from idea.md"
echo "======================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Results file
RESULTS_FILE="test_results.md"
echo "# ZEN Language Feature Test Results" > $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "Generated on: $(date)" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# Function to run a test
run_test() {
    local test_file=$1
    local test_name=$2
    
    echo -e "${YELLOW}Running: $test_name${NC}"
    echo "## $test_name" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    echo "Test file: \`$test_file\`" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    
    # Run the test and capture output and exit code
    output=$(../zen "$test_file" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        echo "**Status:** ✅ PASSED" >> $RESULTS_FILE
    else
        echo -e "${RED}✗ FAILED (exit code: $exit_code)${NC}"
        echo "**Status:** ❌ FAILED (exit code: $exit_code)" >> $RESULTS_FILE
    fi
    
    echo "" >> $RESULTS_FILE
    echo "### Output:" >> $RESULTS_FILE
    echo '```' >> $RESULTS_FILE
    echo "$output" >> $RESULTS_FILE
    echo '```' >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    echo "---" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    
    # Small delay to avoid overwhelming the system
    sleep 0.1
}

# Run all tests
run_test "01_basic_variables.zen" "Basic Variables and Types"
run_test "02_operators.zen" "Operators"
run_test "03_arrays.zen" "Arrays"
run_test "04_objects.zen" "Objects"
run_test "05_functions.zen" "Functions"
run_test "06_control_flow.zen" "Control Flow"
run_test "07_error_handling.zen" "Error Handling"
run_test "08_builtin_functions.zen" "Built-in Functions"
run_test "09_file_io.zen" "File I/O"
run_test "10_advanced_features.zen" "Advanced Features"

echo ""
echo "=== Test Summary ==="
echo "Results saved to: $RESULTS_FILE"

# Count passes and failures
passes=$(grep -c "✅ PASSED" $RESULTS_FILE)
failures=$(grep -c "❌ FAILED" $RESULTS_FILE)

echo -e "Passed: ${GREEN}$passes${NC}"
echo -e "Failed: ${RED}$failures${NC}"

# Create a summary section in the results file
echo "" >> $RESULTS_FILE
echo "## Summary" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "- **Total Tests:** $(($passes + $failures))" >> $RESULTS_FILE
echo "- **Passed:** $passes" >> $RESULTS_FILE
echo "- **Failed:** $failures" >> $RESULTS_FILE
echo "- **Success Rate:** $(awk "BEGIN {printf \"%.1f\", ($passes/($passes+$failures))*100}")%" >> $RESULTS_FILE