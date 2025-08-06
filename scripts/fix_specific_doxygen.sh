#!/bin/bash
# Fix specific doxygen documentation issues
# Task: 20250806-1225.yaml

set -e

echo "🔧 Fixing specific doxygen documentation issues..."

# Fix init_scope in scope.c
if [ -f "src/core/scope.c" ]; then
    echo "  📝 Fixing init_scope doxygen"
    # Find and replace existing doxygen
    sed -i '/\/\*\*/,/\*\// {
        /init_scope/i\
/**\
 * @brief Create new scope instance\
 * @return New scope instance\
 */
        /\/\*\*/d
        /^ \* @brief/d
        /\*\//d
    }' src/core/scope.c
fi

# Fix init_visitor in visitor.c  
if [ -f "src/core/visitor.c" ]; then
    echo "  📝 Fixing init_visitor doxygen"
    sed -i '/\/\*\*/,/\*\// {
        /init_visitor/i\
/**\
 * @brief Create new visitor instance\
 * @return New visitor instance\
 */
        /\/\*\*/d
        /^ \* @brief/d
        /\*\//d
    }' src/core/visitor.c
fi

# Fix print_help in main.c
if [ -f "src/main.c" ]; then
    echo "  📝 Fixing print_help doxygen"
    sed -i '/\/\*\*/,/\*\// {
        /print_help/i\
/**\
 * @brief Print help information\
 */
        /\/\*\*/d
        /^ \* @brief/d
        /\*\//d
    }' src/main.c
fi

# Fix zen_math_random in math.c
if [ -f "src/stdlib/math.c" ]; then
    echo "  📝 Fixing zen_math_random doxygen"
    sed -i '/\/\*\*/,/\*\// {
        /zen_math_random/i\
/**\
 * @brief Random number generator\
 * @return Random number value\
 */
        /\/\*\*/d
        /^ \* @brief/d
        /\*\//d
    }' src/stdlib/math.c
fi

echo "✅ Specific doxygen fixes applied!"