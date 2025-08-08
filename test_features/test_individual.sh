#!/bin/bash

# Individual feature tests with the working binary
ZEN_BIN="/home/x/Projects/zen/workspace/swarm-1-zen-worker-parser/build/zen"
cd /home/x/Projects/zen

echo "=== Testing ZEN Features Individually ==="
echo ""

# Test 1: Simple print
echo "Test 1: Simple print"
echo 'print "Hello World"' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 2: Variables
echo "Test 2: Variables"
echo 'set x 42
print x' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 3: String concatenation
echo "Test 3: String concatenation"
echo 'set name "Alice"
print "Hello " + name' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 4: Arithmetic
echo "Test 4: Arithmetic"
echo 'print 2 + 3
print 10 - 5
print 4 * 3
print 15 / 3' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 5: Comparison with = (not ==)
echo "Test 5: Comparison (= instead of ==)"
echo 'print 5 = 5
print 5 = 3' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 6: Simple if statement
echo "Test 6: If statement"
echo 'set x 10
if x > 5
    print "x is greater than 5"' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 7: Function definition
echo "Test 7: Function definition"
echo 'function greet name
    print "Hello " + name
    
greet "World"' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 8: Arrays
echo "Test 8: Arrays"
echo 'set arr 1, 2, 3
print arr' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 9: Objects
echo "Test 9: Objects"
echo 'set person name "Bob", age 25
print person' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 10: For loop
echo "Test 10: For loop"
echo 'for i in 1..3
    print i' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 11: While loop
echo "Test 11: While loop"
echo 'set count 0
while count < 3
    print count
    set count count + 1' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Test 12: Multiple variable declaration
echo "Test 12: Multiple variable declaration (from idea.md)"
echo 'set made "in Vietnam", by "Nguyen Ky Son", year 2025
print made
print by
print year' > test_temp.zen
timeout 2 $ZEN_BIN test_temp.zen
echo "Exit code: $?"
echo ""

# Clean up
rm -f test_temp.zen