#!/bin/bash

echo "=== Testing Module System Compilation ==="

# Test compilation of individual components
echo "Testing io.c compilation..."
gcc -c -I src/include -std=c11 -g -Wall -Wextra src/stdlib/io.c -o /tmp/io_test.o
if [ $? -eq 0 ]; then
    echo "✓ io.c compiles successfully"
else
    echo "❌ io.c compilation failed"
    exit 1
fi

echo ""
echo "Testing visitor.c compilation..."
gcc -c -I src/include -std=c11 -g -Wall -Wextra src/core/visitor.c -o /tmp/visitor_test.o
if [ $? -eq 0 ]; then
    echo "✓ visitor.c compiles successfully"
else
    echo "❌ visitor.c compilation failed"
    exit 1
fi

echo ""
echo "Testing module implementation test compilation..."
gcc -c -I src/include -std=c11 -g -Wall -Wextra test_module_implementation.c -o /tmp/test_module.o
if [ $? -eq 0 ]; then
    echo "✓ test_module_implementation.c compiles successfully"
else
    echo "❌ test_module_implementation.c compilation failed"
    exit 1
fi

echo ""
echo "=== All Module System Components Compile Successfully ==="

# Clean up
rm -f /tmp/io_test.o /tmp/visitor_test.o /tmp/test_module.o