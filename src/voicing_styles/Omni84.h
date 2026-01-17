#pragma once

#include <map>
#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class Omni84 : public VoicingStyle<VoicingFor::Chord> {
   public:
    // clang-format off
    static inline const std::map<ChordQuality, int> OCTAVE_BEGIN_MAP = {
        {ChordQuality::MAJOR,     36},
        {ChordQuality::MINOR,     48},
        {ChordQuality::DOM_7,     60},
        {ChordQuality::MINOR_7,   72},
        {ChordQuality::MAJOR_7,   84},
        {ChordQuality::DIM_7,     96},
        {ChordQuality::AUGMENTED, 108},
    };
    // clang-format on

    Omni84() = default;

    std::string displayName() const override { return "Omni-84"; }
    std::string description() const override {
        return "Outputs only a single root note, octave shifted to match what Omni-84 expects for the current chord quality.\n\nNote: Doesn't "
               "support Sus4 or Add9.";
    }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        int pitchClass = root % 12;
        auto it = OCTAVE_BEGIN_MAP.find(quality);
        int octaveBegin = (it != OCTAVE_BEGIN_MAP.end()) ? it->second : 36;
        return {octaveBegin + pitchClass};
    }
};