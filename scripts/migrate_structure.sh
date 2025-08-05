#!/bin/bash
# Migrate ZEN codebase to new modular structure

set -e

echo "🔄 Migrating ZEN codebase to new structure..."
echo "============================================"

# Create backup
echo "📦 Creating backup..."
cp -r src src.backup.$(date +%Y%m%d_%H%M%S)

# Remove generated stubs first (we'll use actual implementations)
echo "🧹 Removing generated stubs..."
rm -f src/core/lexer.c src/core/parser.c src/core/ast.c
rm -f src/stdlib/io.c

# Move implementation files to new locations
echo "📁 Moving files to new structure..."

# Core files
echo "  → Moving lexer.c to core/lexer.c"
mv src/lexer.c src/core/lexer.c

echo "  → Moving parser.c to core/parser.c"
mv src/parser.c src/core/parser.c

echo "  → Moving AST.c to core/ast.c"
mv src/AST.c src/core/ast.c

echo "  → Moving visitor.c to core/visitor.c"
mv src/visitor.c src/core/visitor.c

echo "  → Moving scope.c to core/scope.c"
mv src/scope.c src/core/scope.c

echo "  → Moving token.c to core/token.c"
mv src/token.c src/core/token.c

# Create missing core files if needed
if [ ! -f src/core/error.c ]; then
    echo "  → Creating core/error.c (stub)"
    cat > src/core/error.c << 'EOF'
/*
 * error.c
 * Error handling and reporting
 */

#include "zen/core/error.h"

// TODO: Implement error handling
EOF
fi

if [ ! -f src/core/memory.c ]; then
    echo "  → Creating core/memory.c (stub)"
    cat > src/core/memory.c << 'EOF'
/*
 * memory.c
 * Memory management
 */

#include "zen/core/memory.h"

// TODO: Implement memory management
EOF
fi

# Stdlib files
echo "  → Moving io.c to stdlib/io.c"
mv src/io.c src/stdlib/io.c

# Keep main.c in src root
echo "  → Keeping main.c in src/"

# Move headers to new structure
echo "📁 Moving header files..."

# Create new header directories
mkdir -p src/include/zen/core
mkdir -p src/include/zen/types
mkdir -p src/include/zen/runtime
mkdir -p src/include/zen/stdlib

# Move headers
echo "  → Moving headers to zen/core/"
mv src/include/lexer.h src/include/zen/core/lexer.h
mv src/include/parser.h src/include/zen/core/parser.h
mv src/include/AST.h src/include/zen/core/ast.h
mv src/include/visitor.h src/include/zen/core/visitor.h
mv src/include/scope.h src/include/zen/core/scope.h
mv src/include/token.h src/include/zen/core/token.h

echo "  → Moving io.h to zen/stdlib/"
mv src/include/io.h src/include/zen/stdlib/io.h

# Update include paths in all C files
echo "🔧 Updating include paths..."

# Function to update includes in a file
update_includes() {
    local file=$1
    echo "  → Updating includes in $file"
    
    # Update old includes to new paths
    sed -i 's|#include "lexer.h"|#include "zen/core/lexer.h"|g' "$file"
    sed -i 's|#include "parser.h"|#include "zen/core/parser.h"|g' "$file"
    sed -i 's|#include "AST.h"|#include "zen/core/ast.h"|g' "$file"
    sed -i 's|#include "visitor.h"|#include "zen/core/visitor.h"|g' "$file"
    sed -i 's|#include "scope.h"|#include "zen/core/scope.h"|g' "$file"
    sed -i 's|#include "token.h"|#include "zen/core/token.h"|g' "$file"
    sed -i 's|#include "io.h"|#include "zen/stdlib/io.h"|g' "$file"
    
    # Also handle include/ prefix versions
    sed -i 's|#include "include/lexer.h"|#include "zen/core/lexer.h"|g' "$file"
    sed -i 's|#include "include/parser.h"|#include "zen/core/parser.h"|g' "$file"
    sed -i 's|#include "include/AST.h"|#include "zen/core/ast.h"|g' "$file"
    sed -i 's|#include "include/visitor.h"|#include "zen/core/visitor.h"|g' "$file"
    sed -i 's|#include "include/scope.h"|#include "zen/core/scope.h"|g' "$file"
    sed -i 's|#include "include/token.h"|#include "zen/core/token.h"|g' "$file"
    sed -i 's|#include "include/io.h"|#include "zen/stdlib/io.h"|g' "$file"
}

# Update includes in all source files
for file in src/core/*.c src/stdlib/*.c src/main.c; do
    if [ -f "$file" ]; then
        update_includes "$file"
    fi
done

# Update includes in all header files
for file in src/include/zen/core/*.h src/include/zen/stdlib/*.h; do
    if [ -f "$file" ]; then
        update_includes "$file"
    fi
done

# Update Makefile to use new paths
echo "🔧 Updating Makefile..."
sed -i 's|src/\*.c|src/main.c src/core/*.c src/types/*.c src/runtime/*.c src/stdlib/*.c|g' Makefile

# Clean up empty directories
echo "🧹 Cleaning up..."
rmdir src/include 2>/dev/null || true

echo ""
echo "✅ Migration complete!"
echo ""
echo "📋 Summary:"
echo "  - Moved core files to src/core/"
echo "  - Moved stdlib files to src/stdlib/"
echo "  - Updated all include paths"
echo "  - Created backup in src.backup.*"
echo ""
echo "🔍 Run 'make enforce' to verify the migration"