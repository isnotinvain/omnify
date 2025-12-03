#pragma once

#include <juce_core/juce_core.h>

#include <utility>
#include <vector>

enum ChordType { MAJOR, MINOR, DOMINANT7 };

struct Chord {
    std::vector<int> offsets;
    juce::String name;
    juce::String parameterId;
    ChordType chordType;

    Chord(std::vector<int> o, juce::String n, juce::String pid, ChordType t)
        : offsets(std::move(std::move(o))),
          name(std::move(std::move(n))),
          parameterId(std::move(std::move(pid))),
          chordType(t) {}
};

// Global chord mode definitions - shared across the plugin
extern const std::vector<Chord> CHORD_MODES;
extern const std::unordered_map<ChordType, std::unordered_map<int, std::vector<int>>> ARPS;
