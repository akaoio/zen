#!/bin/bash
# ZEN Performance Benchmark Script
# Measures parsing and execution performance for swarm-2 baseline

echo "=== ZEN Performance Benchmark ==="
echo "Target: <2ms parsing for 1000-line files"
echo "================================="
echo

# Check if zen executable exists
if [ ! -f "./zen" ]; then
    echo "Error: zen executable not found. Please run 'make' first."
    exit 1
fi

# Create results directory
mkdir -p benchmark/results

# Function to measure time in milliseconds
get_time_ms() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        python3 -c 'import time; print(int(time.time() * 1000))'
    else
        # Linux
        date +%s%3N
    fi
}

# Test 1: Parse-only benchmark (using --parse-only flag if available)
echo "Test 1: Parsing Performance (1000-line file)"
echo "--------------------------------------------"

# Run 10 iterations and calculate average
total_time=0
iterations=10

for i in $(seq 1 $iterations); do
    start=$(get_time_ms)
    ./zen benchmark/parse_benchmark.zen > /dev/null 2>&1
    end=$(get_time_ms)
    
    elapsed=$((end - start))
    total_time=$((total_time + elapsed))
    echo "  Iteration $i: ${elapsed}ms"
done

avg_time=$((total_time / iterations))
echo "  Average parsing time: ${avg_time}ms"

if [ $avg_time -lt 2 ]; then
    echo "  ✅ PASS: Parsing time < 2ms target"
else
    echo "  ❌ FAIL: Parsing time exceeds 2ms target"
fi

echo

# Test 2: Memory usage
echo "Test 2: Memory Usage Analysis"
echo "-----------------------------"

if command -v /usr/bin/time &> /dev/null; then
    /usr/bin/time -v ./zen benchmark/parse_benchmark.zen 2>&1 | grep -E "Maximum resident set size|User time|System time" | sed 's/^/  /'
else
    echo "  (time command not available for detailed memory analysis)"
fi

echo

# Test 3: Small file benchmarks
echo "Test 3: Small File Performance"
echo "------------------------------"

# Create small test file
cat > benchmark/small_test.zen << 'EOF'
set x 10
set y 20
set result x + y
print result
EOF

small_total=0
for i in $(seq 1 20); do
    start=$(get_time_ms)
    ./zen benchmark/small_test.zen > /dev/null 2>&1
    end=$(get_time_ms)
    elapsed=$((end - start))
    small_total=$((small_total + elapsed))
done

small_avg=$((small_total / 20))
echo "  Average time for 4-line file: ${small_avg}ms"

echo

# Test 4: Lexer throughput
echo "Test 4: Lexer Token Throughput"
echo "-------------------------------"

# Count approximate tokens in benchmark file
token_count=$(wc -w < benchmark/parse_benchmark.zen)
tokens_per_ms=$((token_count / avg_time))
echo "  Approximate tokens: $token_count"
echo "  Tokens per millisecond: $tokens_per_ms"
echo "  Tokens per second: $((tokens_per_ms * 1000))"

echo

# Test 5: Function call overhead
echo "Test 5: Function Call Performance"
echo "---------------------------------"

cat > benchmark/function_test.zen << 'EOF'
function test n
    if n <= 0
        return 0
    return test n - 1

set result test 100
print result
EOF

func_total=0
for i in $(seq 1 10); do
    start=$(get_time_ms)
    ./zen benchmark/function_test.zen > /dev/null 2>&1
    end=$(get_time_ms)
    elapsed=$((end - start))
    func_total=$((func_total + elapsed))
done

func_avg=$((func_total / 10))
echo "  Average time for 100 recursive calls: ${func_avg}ms"

echo
echo "=== Performance Summary ==="
echo "Parse time (1000 lines): ${avg_time}ms"
echo "Small file (4 lines): ${small_avg}ms"
echo "Function recursion (100 calls): ${func_avg}ms"
echo "Token throughput: $((tokens_per_ms * 1000)) tokens/second"

# Save results
timestamp=$(date +%Y%m%d_%H%M%S)
cat > benchmark/results/baseline_${timestamp}.txt << EOF
ZEN Performance Baseline Results
================================
Date: $(date)
Version: Phase 1 Baseline

Parsing Performance:
- 1000-line file: ${avg_time}ms (target: <2ms)
- Small file (4 lines): ${small_avg}ms
- Token throughput: $((tokens_per_ms * 1000)) tokens/second

Execution Performance:
- Function recursion (100 calls): ${func_avg}ms

Memory Usage:
- See detailed output above

Next Steps:
- Implement AST memory pools (40% allocation reduction)
- Bytecode compilation (10x execution improvement)
- JIT optimization for hot paths
EOF

echo
echo "Results saved to: benchmark/results/baseline_${timestamp}.txt"
echo
echo "Phase 2 Targets:"
echo "- Parsing: <2ms (currently ${avg_time}ms)"
echo "- Memory: 40% reduction via AST pools"
echo "- Execution: 10x speedup via bytecode"