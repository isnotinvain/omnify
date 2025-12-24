#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Parse command line arguments
INSTALL=false
DEBUG=false
FORMAT=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -i|--install)
            INSTALL=true
            shift
            ;;
        -d|--debug)
            DEBUG=true
            shift
            ;;
        -f|--format)
            FORMAT=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--install|-i] [--debug|-d] [--format|-f]"
            exit 1
            ;;
    esac
done

echo "Starting compilation..."

# Format all C++ files (if requested)
# Only iterate over .h files - format-cpp.sh will run clang-tidy on the corresponding .cpp if it exists
if [ "$FORMAT" = true ]; then
    echo "Formatting C++ files..."
    find "$SCRIPT_DIR" -type f -name "*.h" ! -path "*/JUCE/*" ! -path "*/build/*" ! -path "*/nlohmann/*" \
        | xargs -P $(sysctl -n hw.ncpu) -I {} "$(dirname "$SCRIPT_DIR")/.claude/hooks/format-cpp.sh" --no-fail {}
fi

# Determine build type
if [ "$DEBUG" = true ]; then
    BUILD_TYPE="Debug"
    echo "Building plugin in Debug mode..."
else
    BUILD_TYPE="Release"
    echo "Building plugin in Release mode..."
fi

cd "$SCRIPT_DIR/build"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo -e "\033[0;32mBuild completed successfully!\033[0m"

    # Run install script if flag is set
    if [ "$INSTALL" = true ]; then
        echo ""
        echo "Running install script..."
        "$SCRIPT_DIR/install.sh"
    else
        echo ""
        echo "Skipping installation. Use --install or -i flag to install after building."
    fi
else
    echo -e "\033[0;31mError: Build failed.\033[0m"
    exit 1
fi
