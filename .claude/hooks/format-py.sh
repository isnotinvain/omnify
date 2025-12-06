#!/bin/bash

# Shared script to lint, fix, and format Python files
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

if [ "$FAIL_FAST" = true ]; then
    set -euo pipefail
fi

# Check if the file is a Python file
if [[ "$FILE_PATH" == *.py ]]; then
    # Step 1: Run ruff check with auto-fix (fixes linting issues like import sorting)
    uv run ruff check --fix "$FILE_PATH"

    # Step 2: Run ruff format (formats the fixed code)
    uv run ruff format "$FILE_PATH"
fi
