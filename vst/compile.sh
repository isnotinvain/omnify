#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Parse command line arguments
INSTALL=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -i|--install)
            INSTALL=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--install|-i]"
            exit 1
            ;;
    esac
done

echo "Starting compilation..."

# Build the plugin
echo "Building plugin in Release mode..."
cd "$SCRIPT_DIR/build"
cmake ..
make

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
