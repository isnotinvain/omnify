#!/bin/bash
# Runs a command, redirecting output to a temp file and opening it in VSCode
# Usage: run-with-log.sh <command> [args...]

tmpfile=$(mktemp /tmp/claude-cmd-XXXXXX)
mv "$tmpfile" "$tmpfile.log"
tmpfile="$tmpfile.log"
echo "Running: $@" > "$tmpfile"
echo "Started: $(date)" >> "$tmpfile"
echo "---" >> "$tmpfile"

# Run command in background, open log in VSCode, wait for completion
"$@" >> "$tmpfile" 2>&1 &
pid=$!
open -a "Visual Studio Code" "$tmpfile"
wait $pid
exit_code=$?

echo "---" >> "$tmpfile"
echo "Finished: $(date)" >> "$tmpfile"
echo "Exit code: $exit_code" >> "$tmpfile"

exit $exit_code
