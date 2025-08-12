# Building ZEN

## Prerequisites

### Required
- GCC or Clang C compiler
- Make
- libcurl (for HTTP support)
- libpcre2 (for regex support)

### Optional
- libyaml (for YAML support)
- valgrind (for memory checking)

## Installation

### Ubuntu/Debian
```bash
# Required dependencies
sudo apt-get install build-essential libcurl4-openssl-dev libpcre2-dev

# Optional for full YAML support
sudo apt-get install libyaml-dev
```

### Termux (Android)
```bash
# Required dependencies
pkg install clang make libcurl pcre2

# Optional for full YAML support
pkg install libyaml
```

### macOS
```bash
# Using Homebrew
brew install curl pcre2

# Optional for full YAML support
brew install libyaml
```

## Building

```bash
# Build the interpreter
make

# Install to system (default: /usr/local/bin)
sudo make install

# Custom installation prefix
make install PREFIX=$HOME/.local
```

## Feature Detection

The build system automatically detects available libraries:

- **YAML Support**: If libyaml is not installed, YAML functions will return error messages but the interpreter will still work for all other features
- **HTTP Support**: Requires libcurl
- **Regex Support**: Requires libpcre2

During build, you'll see messages like:
```
YAML support: Enabled (using libyaml)
```
or
```
YAML support: Disabled (using stub, install libyaml-dev for full support)
```

## Troubleshooting

### Missing YAML support
If you see "YAML support not available" errors when running ZEN programs:
1. Install libyaml-dev (or libyaml on some systems)
2. Rebuild with `make clean && make`

### Missing libraries
Check which libraries are missing:
```bash
ldd ./zen | grep "not found"
```

### Build errors
Clean and rebuild:
```bash
make clean
make
```