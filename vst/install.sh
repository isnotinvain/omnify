#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source and destination paths
SOURCE_PLUGIN="$SCRIPT_DIR/build/Omnify_artefacts/Release/VST3/Omnify.vst3"
DESTINATION_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
DESTINATION_PLUGIN="$DESTINATION_DIR/Omnify.vst3"

echo "Starting plugin installation..."

# Ensure the destination directory exists
echo "Creating destination directory if it doesn't exist: $DESTINATION_DIR"
mkdir -p "$DESTINATION_DIR"

# Delete old plugin if it exists
if [ -d "$DESTINATION_PLUGIN" ]; then
    echo "Deleting old plugin: $DESTINATION_PLUGIN"
    rm -rf "$DESTINATION_PLUGIN"
else
    echo "Old plugin not found at $DESTINATION_PLUGIN, skipping deletion."
fi

# Copy new plugin
echo "Copying new plugin from $SOURCE_PLUGIN to $DESTINATION_DIR"
cp -R "$SOURCE_PLUGIN" "$DESTINATION_DIR"

if [ $? -eq 0 ]; then
    echo "Plugin copied successfully to $DESTINATION_PLUGIN"

    # Re-sign the plugin for macOS
    echo "Signing plugin..."
    codesign --force --deep --sign - "$DESTINATION_PLUGIN"

    if [ $? -eq 0 ]; then
        echo -e "\033[0;32mPlugin installed and signed successfully!\033[0m"
    else
        echo -e "\033[0;31mWarning: Plugin copied but signing failed. Plugin may not load.\033[0m"
    fi
else
    echo -e "\033[0;31mError: Failed to copy the plugin.\033[0m"
    exit 1
fi

exit 0