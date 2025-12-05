#!/bin/bash

# Shared script to lint, fix, and format C++ files
# Silent on success, prints red error on failure

FILE_PATH="$1"

# Check if the file is a C++ file
if [[ "$FILE_PATH" =~ \.(cpp|h|hpp)$ ]]; then
    # Step 1: Run clang-tidy with auto-fixes (uses .clang-tidy config file and compile_commands.json)
    # Note: We target arm64 specifically since universal builds create multiple compiler jobs
    SDK_PATH=$(xcrun --show-sdk-path)
    if ! /opt/homebrew/opt/llvm/bin/clang-tidy "$FILE_PATH" \
        -p vst/build \
        --fix \
        --fix-errors \
        --extra-arg=-target --extra-arg=arm64-apple-macosx \
        --extra-arg=-isysroot --extra-arg="$SDK_PATH" 2>&1 | grep -v -e "^[0-9]* warning" -e "Use -header-filter="; then
        echo -e "\033[31mclang-tidy failed on $FILE_PATH\033[0m"
        exit 0
    fi

    # Step 2: Run clang-format
    if ! clang-format -i "$FILE_PATH" 2>&1; then
        echo -e "\033[31mclang-format failed on $FILE_PATH\033[0m"
        exit 0
    fi
fi
