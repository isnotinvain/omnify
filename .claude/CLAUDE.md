# Omnify

Transforms any MIDI instrument into an omnichord / autoharp style instrument. Users play MIDI instruments like automatic harps - strumming on a mod wheel/plate while using pads to select chord qualities and keyboard notes to set chord roots. Outputs chords and strums on separate MIDI channels.

## Directory Structure
```
src/
├── PluginProcessor.cpp/h  # VST lifecycle, settings, APVTS params, MIDI I/O
├── PluginEditor.cpp/h     # VST UI layout
├── Omnify.cpp/h           # Core chord/strum processing engine
├── MidiMessageScheduler.cpp/h  # Sample-accurate delayed MIDI delivery (strum timing)
├── datamodel/             # Settings, ChordQuality, MidiButton, VoicingStyle
├── ui/
│   ├── components/        # MidiLearnComponent, VariantSelector, etc.
│   ├── panels/            # ChordSettingsPanel, StrumSettingsPanel, ChordQualityPanel
│   └── LcarsLookAndFeel.h # Custom JUCE theme (LCARS colors, Orbitron font)
├── voicing_styles/        # Voicing algorithms (OmnichordChords, RootPosition, FromFile, etc.)
├── Resources/             # Fonts, default_settings.json
└── JUCE/                  # Git submodule (customized fork)
```

## Core Architecture
- **PluginProcessor** (`PluginProcessor.cpp`): VST entry point. Handles MIDI I/O routing (DAW or direct hardware), settings management, and device reconciliation via AsyncUpdater.
- **Omnify** (`Omnify.cpp`): State machine processing MIDI input. Tracks current chord, latch mode, strum timing (sample-accurate). Delegates to voicing styles for note generation.
- **MidiMessageScheduler** (`MidiMessageScheduler.cpp`): Sample-accurate priority queue for delayed MIDI messages (e.g., note-offs for strum gate timing).
- **OmnifySettings** (`datamodel/OmnifySettings.h`): Main config struct with MIDI device, channels, voicing styles, button mappings. Serializes to/from JSON.
- **VoicingStyle** (`datamodel/VoicingStyle.h`): Abstract template for chord generation. Subclasses implement `constructChord(quality, root) -> vector<int>`. Two registries: ChordVoicingRegistry and StrumVoicingRegistry.

## Data Model Key Classes
- `ChordQuality` - enum of 9 qualities: MAJOR, MINOR, DOM_7, MAJOR_7, MINOR_7, DIM_7, AUGMENTED, SUS_4, ADD_9
- `MidiButton` - Configurable trigger (note or CC, with button action: FLIP/ON/OFF)
- `ChordQualitySelectionStyle` - Variant: ButtonPerChordQuality or CCRangePerChordQuality
- `RealtimeParams` - Thread-safe atomics for strumGateTimeMs, strumCooldownMs

## Voicing Styles
Located in `src/voicing_styles/`:
- **OmnichordChords/OmnichordStrum** - Authentic Omnichord voicings with relative F# octave positioning
- **RootPosition** - Simple root position triads
- **PlainAscending** - Ascending arpeggio
- **FromFile** - Load voicing from JSON file (supports offset-based or absolute notes)

## C++ Dependencies
- JUCE is a git submodule in src/ - we are free to add bugfixes and features to JUCE as needed
- nlohmann/json (header-only) in src/nlohmann/

## Threading
- APVTS for audio thread-safe parameters (strumGateTimeMs, strumCooldownMs)
- `std::atomic_load/store` with `shared_ptr<OmnifySettings>` for lock-free settings access from audio thread
- `std::atomic_load/store` with `shared_ptr<MidiOutput>` for lock-free output device access from audio thread
- UI thread writes settings via `modifySettings()`, triggers `AsyncUpdater` for device reconciliation
- Audio thread reads settings and output device atomically, processes MIDI in `processBlock()`
- Hardware MIDI input uses `MidiMessageCollector` to bridge input callback thread to audio thread
- MidiLearnComponent uses atomics for thread-safe learning state

## Patterns
- Complex nested settings stored as JSON blob in VST (simpler than exposing every field)
- LCARS (Star Trek) visual theme in the VST UI
- Registry pattern for voicing styles (registered at startup, referenced by type name)
- JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR on all classes

## Website (GitHub Pages)
A Jekyll site in `docs/` deployed to `omnify.symphonicpositronic.com` via GitHub Pages (CNAME in `docs/CNAME`).

### Structure
```
docs/
├── _config.yml          # Jekyll config (kramdown, no plugins)
├── _layouts/default.html # LCARS-framed page layout (top bar, sidebar, bottom bar)
├── _includes/
│   ├── nav.html         # Top nav: Home, Manual, GitHub link (with active state)
│   └── footer.html      # License + "LCARS Interface Active" tagline
├── assets/css/lcars.css # Full LCARS theme (elbows, pills, sidebar strip, colors)
├── assets/images/       # (empty, for screenshots)
├── CNAME                # Custom domain: omnify.symphonicpositronic.com
├── index.md             # Landing page: features, download link, how-it-works
└── manual.md            # User manual: MIDI setup, chord selection, strum, voicing styles
```

### Design
- LCARS (Star Trek) theme matching the VST UI, using Orbitron font from Google Fonts
- Layout has characteristic LCARS elbows, sidebar strip, pill-shaped nav buttons, and color palette
- Two pages: home (product overview + download) and manual

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
- Don't recommend removing features as a solution. If a feature is in the project, it's there on purpose. I will suggest feature removal when I want it, but you should not.
- Never suggest accepting bugs as a solution. Never suggest ignoring thread safety as a solution.

## Coding Style
- Don't add comments that are pretty obvious from the code they are commenting, or from the function signature and name. If the code is clear, if the variable names are clear, and if the function signatures are clear, skip the comments entirely. Do add comments for gotchas / bug fixes / workarounds, methods with important rules / constraints that aren't immediately obvious, or complicated algorithms.
