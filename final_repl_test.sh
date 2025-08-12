#\!/bin/bash
echo "Testing all types in REPL..."
(
echo 'null'
echo 'true'
echo 'false'
echo '42'
echo '3.14'
echo '"Hello World\!"'
echo '[1, 2, 3, 4, 5]'
echo 'set obj name "Alice", age 30'
echo 'obj'
echo 'set add function x y'
echo '  return x + y'
echo 'add'
echo 'exit'
) | timeout 3 ./zen
