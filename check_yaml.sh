#!/bin/sh
# Check if yaml.h is available
echo '#include <yaml.h>' | gcc -E -xc - > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "yes"
else
    echo "no"
fi