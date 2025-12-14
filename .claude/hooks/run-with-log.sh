#!/bin/bash
# Runs a command, logging output to a temp file while also showing stdout
# Usage: run-with-log.sh <command> [args...]

tmpfile=$(mktemp /tmp/claude-cmd-XXXXXX)
mv "$tmpfile" "$tmpfile.log"
tmpfile="$tmpfile.log"
echo "Log: $tmpfile"
"$@" 2>&1 | tee "$tmpfile"
