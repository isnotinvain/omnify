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

    # For release builds, bundle the Python daemon with PyInstaller
    echo "Bundling Python daemon with PyInstaller..."
    PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
    PYINSTALLER_DIST=$(mktemp -d)
    PYINSTALLER_WORK=$(mktemp -d)

    # Run PyInstaller
    uv run pyinstaller "$PROJECT_ROOT/daemomnify.spec" \
        --distpath "$PYINSTALLER_DIST" \
        --workpath "$PYINSTALLER_WORK" \
        --noconfirm

    echo "Python daemon bundled to: $PYINSTALLER_DIST/daemomnify"
fi

cd "$SCRIPT_DIR/build"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo -e "\033[0;32mBuild completed successfully!\033[0m"

    # For release builds, copy bundled Python daemon into VST bundle
    if [ "$DEBUG" = false ]; then
        echo ""
        echo "Copying bundled daemon into VST..."
        VST_RESOURCES="$SCRIPT_DIR/build/Omnify_artefacts/Release/VST3/Omnify.vst3/Contents/Resources"
        mkdir -p "$VST_RESOURCES"
        cp "$PYINSTALLER_DIST/daemomnify" "$VST_RESOURCES/"
        cp "$SCRIPT_DIR/Resources/launch_daemon.sh" "$VST_RESOURCES/"
        chmod +x "$VST_RESOURCES/daemomnify"
        chmod +x "$VST_RESOURCES/launch_daemon.sh"
        echo "VST bundle is ready at: $SCRIPT_DIR/build/Omnify_artefacts/Release/VST3/Omnify.vst3"

        # Cleanup temp directories
        rm -rf "$PYINSTALLER_DIST" "$PYINSTALLER_WORK"
    fi

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
