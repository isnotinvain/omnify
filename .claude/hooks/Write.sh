#!/bin/bash

# Call shared C++ formatting script
SCRIPT_DIR="$(dirname "$0")"
"$SCRIPT_DIR/format-cpp.sh" "$1"
