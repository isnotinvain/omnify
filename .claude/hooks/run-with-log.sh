#!/bin/bash
# Runs a command with unbuffered output and logs to a file
# Usage: run-with-log.sh <logfile> <command...>
LOG="$1"; shift
"$@" 2>&1 | sed -l '' | tee "$LOG"
