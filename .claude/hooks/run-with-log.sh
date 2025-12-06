#!/bin/bash
# Runs a command, redirecting output to a temp file and opening it in VSCode
# Usage: run-with-log.sh <command> [args...]

tmpfile=$(mktemp /tmp/claude-cmd-XXXXXX)
mv "$tmpfile" "$tmpfile.log"
tmpfile="$tmpfile.log"
echo "$tmpfile" >&2  # stderr is unbuffered
echo "$tmpfile"      # also to stdout for capture
echo "Running: $@" > "$tmpfile"
echo "Started: $(date)" >> "$tmpfile"
echo "---" >> "$tmpfile"

# Run command in background, open log in VSCode, return immediately
(
    "$@" >> "$tmpfile" 2>&1
    exit_code=$?
    echo "---" >> "$tmpfile"
    echo "Finished: $(date)" >> "$tmpfile"
    echo "Exit code: $exit_code" >> "$tmpfile"
) &

open -a "Visual Studio Code" "$tmpfile"
