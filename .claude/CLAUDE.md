# Daemomnify

Transforms any MIDI instrument into an omnichord (autoharp-style instrument). Two major components:
1) A python script that handles midi in + transformations + midi out
2) A C++ VST for UI only

## Architecture
- Python core in `src/daemomnify/` - Pydantic settings, MIDI processing, chord voicing logic
- VST plugin in `vst/` - C++17 with JUCE
- Code generation bridges Python↔C++: `vst/generate_vst_params.py` introspects Pydantic models to generate `GeneratedParams.h/cpp` and `GeneratedAdditionalSettings.h/cpp` (don't edit these directly)

## C++ Dependencies
- JUCE is a git submodule in vst/
- We are free to add bugfixes and features to JUCE as needed
- You can explore this codebase for debugging as well

## Key Files
- `src/daemomnify/settings.py` - All configuration (Pydantic models with VST param annotations)
- `src/daemomnify/omnify.py` - Core state machine (chord generation, strum logic)
- `vst/PluginProcessor.cpp` - Main VST plugin logic
- `vst/PluginEditor.cpp` - VST UI

## Patterns
- Complex nested settings stored as JSON blob in VST (simpler than exposing every field)
- LCARS (Star Trek) visual theme in the VST UI

## Commands
- Run daemon: `uv run python -m daemomnify`
- Run tests: `uv run pytest`
- Regenerate VST params after changing settings.py: `uv run python vst/generate_vst_params.py`
- Python: use `uv run`
- Install python packages: `/install`
- You have available to you a helper script for running commands that output a lot.
  - it is: .claude/hooks/run-with-log.sh <log_file> <command>
  - It will `tee` the <command>'s stdout to <log_file> and back to you
  - You can use | head or | tail on it 
  - If you don't get enough from that, don't re-run <command> -- inspect the <log_file>
  - see "Compile VST" below for an example  
- Compile VST: `.claude/hooks/run-with-log.sh /tmp/vst-compile.log ./vst/compile.sh -d` (drop `-d` for release)
- Don't make Edits immediately after propsing an alternate solution -- ask for confirmation first.
- If you explain multiple ways something could be done, ask me which one I want before proceding with Edits