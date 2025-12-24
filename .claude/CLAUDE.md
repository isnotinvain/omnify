# Omnify

Transforms any MIDI instrument into an omnichord / autoharp style instrument

## Architecture
- VST plugin in `src/` - C++20 with JUCE

## C++ Dependencies
- JUCE is a git submodule in src/
- We are free to add bugfixes and features to JUCE as needed
- You can explore this codebase for debugging as well

## Key Files
- `src/PluginProcessor.cpp` - Main VST plugin logic
- `src/PluginEditor.cpp` - VST UI

## Patterns
- Complex nested settings stored as JSON blob in VST (simpler than exposing every field)
- LCARS (Star Trek) visual theme in the VST UI

## Commands
- You have available to you a helper script for running commands that output a lot.
  - it is: .claude/hooks/run-with-log.sh <log_file> <command>
  - It will `tee` the <command>'s stdout to <log_file> and back to you
  - You can use | head or | tail on it
  - If you don't get enough from that, don't re-run <command> -- inspect the <log_file>
  - see "Compile VST" below for an example
- Compile VST: `.claude/hooks/run-with-log.sh /tmp/vst-compile.log ./src/compile.sh -d` (drop `-d` for release)

## Interaction style
- Don't make Edits immediately after propsing an alternate solution -- ask for confirmation first.
- If you explain multiple ways something could be done, ask me which one I want before proceding with Edits
- When writing brand new files, pause after each one so we can discuss before moving on (unless of course you are in allow edits mode, then do your thing)

## Coding Style
- Don't add comments that are pretty obvious from the code they are commenting, or from the function signature and name. If the code is clear, if the variable names are clear, and if the function signatures are clear, skip the comments entirely. Do add comments for gotchas / bug fixes / workarounds, methods with important rules / constraints that aren't immediately obvious, or complicated algorithms.
