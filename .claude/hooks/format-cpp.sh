#!/bin/bash

# Shared script to lint, fix, and format C++ files
# Silent on success, prints red error on failure
# By default, fails on errors. Use --no-fail to always exit 0 (for hooks).

FAIL_FAST=true
FILE_PATH=""

for arg in "$@"; do
    case $arg in
        --no-fail)
            FAIL_FAST=false
            ;;
        *)
            FILE_PATH="$arg"
            ;;
    esac
done

# Skip files in submodules
if [[ "$FILE_PATH" == *"/foleys_gui_magic/"* ]] || [[ "$FILE_PATH" == *"/JUCE/"* ]]; then
    exit 0
fi

if [ "$FAIL_FAST" = true ]; then
    set -euo pipefail
fi

# Check if the file is a C++ file
if [[ "$FILE_PATH" =~ \.(cpp|h|hpp)$ ]]; then
    # For header files, run clang-tidy on the corresponding .cpp file instead
    # This gives clang-tidy the compile commands it needs (headers aren't in compile_commands.json)
    # clang-tidy will also check the header since it's included by the .cpp
    TIDY_TARGET="$FILE_PATH"
    if [[ "$FILE_PATH" =~ \.h$ ]]; then
        CPP_FILE="${FILE_PATH%.h}.cpp"
        if [[ -f "$CPP_FILE" ]]; then
            TIDY_TARGET="$CPP_FILE"
        fi
    fi

    # Step 1: Run clang-tidy with auto-fixes (uses .clang-tidy config file and compile_commands.json)
    # Note: We target arm64 specifically since universal builds create multiple compiler jobs
    SDK_PATH=$(xcrun --show-sdk-path)

    # Find the build directory relative to the file being processed
    # The file is in vst/, so the build dir is vst/build
    FILE_DIR="$(dirname "$TIDY_TARGET")"
    BUILD_DIR="$FILE_DIR/build"

    /opt/homebrew/opt/llvm/bin/clang-tidy "$TIDY_TARGET" \
        -p "$BUILD_DIR" \
        --fix \
        --fix-errors \
        --extra-arg=-target --extra-arg=arm64-apple-macosx \
        --extra-arg=-isysroot --extra-arg="$SDK_PATH"

    # Step 2: Run clang-format on the original file
    clang-format -i "$FILE_PATH"
fi
