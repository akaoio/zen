#!/bin/bash
# ZEN Language Dependencies Setup Script

set -e

echo "🔧 ZEN Language Dependencies Setup"
echo "=================================="

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    INSTALL_CMD="sudo apt-get install -y"
    UPDATE_CMD="sudo apt-get update"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    INSTALL_CMD="brew install"
    UPDATE_CMD="brew update"
else
    echo "❌ Unsupported OS: $OSTYPE"
    exit 1
fi

echo "📦 Detected OS: $OS"

# Update package manager
echo "📦 Updating package manager..."
$UPDATE_CMD

# Install build tools
echo "🔨 Installing build tools..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD build-essential cmake pkg-config
else
    $INSTALL_CMD cmake pkg-config
fi

# Install required libraries
echo "📚 Installing required libraries..."


# libyaml - YAML parsing
echo "  → Installing libyaml..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD libyaml-dev
else
    $INSTALL_CMD libyaml
fi

# PCRE2 - Regular expressions
echo "  → Installing PCRE2..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD libpcre2-dev
else
    $INSTALL_CMD pcre2
fi

# libcurl - HTTP client
echo "  → Installing libcurl..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD libcurl4-openssl-dev
else
    $INSTALL_CMD curl
fi

# SQLite3 - Optional database
echo "  → Installing SQLite3 (optional)..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD libsqlite3-dev
else
    $INSTALL_CMD sqlite3
fi

# Install development tools
echo "🛠️  Installing development tools..."

# CMocka - Unit testing
echo "  → Installing CMocka..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD libcmocka-dev
else
    $INSTALL_CMD cmocka
fi

# Valgrind - Memory debugging (Linux only)
if [[ "$OS" == "linux" ]]; then
    echo "  → Installing Valgrind..."
    $INSTALL_CMD valgrind
fi

# clang-format - Code formatting
echo "  → Installing clang-format..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD clang-format
else
    $INSTALL_CMD clang-format
fi

# cppcheck - Static analysis
echo "  → Installing cppcheck..."
$INSTALL_CMD cppcheck

# gcov/lcov - Code coverage
echo "  → Installing coverage tools..."
if [[ "$OS" == "linux" ]]; then
    $INSTALL_CMD lcov
else
    $INSTALL_CMD lcov
fi

# Node.js check and setup
echo "📦 Checking Node.js installation..."
if ! command -v node &> /dev/null; then
    echo "⚠️  Node.js is not installed."
    echo "   Node.js is optional but recommended for development tools."
    echo "   Visit: https://nodejs.org/ to install it."
else
    echo "✓ Node.js: $(node --version)"
    echo "✓ npm: $(npm --version)"
fi

# Create directory structure
echo "📁 Creating project directories..."
mkdir -p src/{core,types,runtime,stdlib}
mkdir -p src/include/{core,types,runtime,stdlib}
mkdir -p tests/{unit,integration,fixtures,framework}
mkdir -p tools/{zenfmt,zenlint,zendoc}
mkdir -p examples
mkdir -p docs/{api,guides,internals}
mkdir -p scripts

# Verify installations
echo ""
echo "✅ Verifying installations..."
echo "=================================="

check_command() {
    if command -v $1 &> /dev/null; then
        echo "✓ $1: $(command -v $1)"
    else
        echo "✗ $1: NOT FOUND"
    fi
}

check_library() {
    if [[ "$OS" == "linux" ]]; then
        if pkg-config --exists $1 2>/dev/null; then
            echo "✓ $1: $(pkg-config --modversion $1 2>/dev/null || echo "installed")"
        else
            echo "✗ $1: NOT FOUND"
        fi
    else
        if brew list $2 &>/dev/null; then
            echo "✓ $1: installed via homebrew"
        else
            echo "✗ $1: NOT FOUND"
        fi
    fi
}

# Check commands
check_command gcc
check_command cmake
check_command make
check_command clang-format
check_command cppcheck
check_command node
check_command npm

# Check libraries
check_library yaml-0.1 libyaml
check_library libpcre2-8 pcre2
check_library libcurl curl
check_library sqlite3 sqlite3
check_library cmocka cmocka

if [[ "$OS" == "linux" ]]; then
    check_command valgrind
    check_command lcov
fi

echo ""
echo "🎉 Setup complete! You can now build ZEN with:"
echo "   make clean && make"
echo ""
echo "📝 Note: If any dependencies failed to install, please install them manually."