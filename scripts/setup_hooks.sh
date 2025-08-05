#!/bin/bash
# Setup Git hooks for the ZEN project

echo "🔧 Setting up Git hooks..."

# Configure git to use our hooks directory
git config core.hooksPath .githooks

echo "✅ Git hooks configured!"
echo ""
echo "The following hooks are now active:"
echo "  - pre-commit: Enforces manifest compliance and code standards"
echo ""
echo "To bypass hooks temporarily (NOT RECOMMENDED):"
echo "  git commit --no-verify"