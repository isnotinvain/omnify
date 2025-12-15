#pragma once

#include <juce_core/juce_core.h>

/**
 * Shared logging and temp directory for Omnify plugin.
 * Creates a unique session directory under /var/folders/.../T/omnify-<uuid>/
 * Both C++ and Python daemon use this same directory.
 */
namespace OmnifyLogger {

/** Get the unique session temp directory. Creates it on first call. */
juce::File getTempDir();

/** Log a message to the shared log file. Thread-safe. */
void log(const juce::String& message);

}  // namespace OmnifyLogger