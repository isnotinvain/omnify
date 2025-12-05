#!/bin/bash

# Shared script to lint, fix, and format Python files
# Silent on success, prints red error on failure

FILE_PATH="$1"

# Check if the file is a Python file
if [[ "$FILE_PATH" == *.py ]]; then
    # Step 1: Run ruff check with auto-fix (fixes linting issues like import sorting)
    if ! uv run ruff check --fix "$FILE_PATH" 2>&1; then
        echo -e "\033[31mruff check failed on $FILE_PATH\033[0m"
        exit 0
    fi

    # Step 2: Run ruff format (formats the fixed code)
    if ! uv run ruff format "$FILE_PATH" 2>&1; then
        echo -e "\033[31mruff format failed on $FILE_PATH\033[0m"
        exit 0
    fi
fi
