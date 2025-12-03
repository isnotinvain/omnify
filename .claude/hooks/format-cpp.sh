#!/bin/bash

# Shared script to lint, fix, and format C++ files
FILE_PATH="$1"

# Check if the file is a C++ file
if [[ "$FILE_PATH" =~ \.(cpp|h|hpp)$ ]]; then
    # Step 1: Run clang-tidy with auto-fixes (uses .clang-tidy config file and compile_commands.json)
    # Note: We target arm64 specifically since universal builds create multiple compiler jobs
    echo "Running clang-tidy on $FILE_PATH..."
    SDK_PATH=$(xcrun --show-sdk-path)
    /opt/homebrew/opt/llvm/bin/clang-tidy "$FILE_PATH" \
        -p vst/build \
        --fix \
        --fix-errors \
        --extra-arg=-target --extra-arg=arm64-apple-macosx \
        --extra-arg=-isysroot --extra-arg="$SDK_PATH" 2>&1 | grep -v -e "^[0-9]* warning" -e "Use -header-filter=" || true
    echo "✓ Linted and fixed $FILE_PATH"

    # Step 2: Run clang-format
    echo "Running clang-format on $FILE_PATH..."
    clang-format -i "$FILE_PATH"
    echo "✓ Formatted $FILE_PATH"
fi
