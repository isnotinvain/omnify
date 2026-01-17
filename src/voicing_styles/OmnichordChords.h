#pragma once

#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class OmnichordChords : public VoicingStyle<VoicingFor::Chord> {
   public:
    std::string displayName() const override { return "Omnichord"; }
    std::string description() const override {
        return "Behaves like a real Omnichord. Three 'most important' notes of the chord, using the same inversions as the Omnichord.";
    }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        // Omnichord voices chords within F#-to-F ranges (12 semitones starting at F#).
        //
        // We want all roots in a standard C-to-B octave (like C4-B4)
        // to share the SAME F# base. Without this, there'd be an octave jump at F#
        // which is surpising to anyone thinking in normal octaves (aka everyone).
        //
        // Solution: use the F# that's below the C of the root's octave.
        // C4-B4 all use F#3, C5-B5 all use F#4, etc.

        const auto& triad = getChordQualityData(quality).triadOffsets;
        std::vector<int> res;
        res.reserve(3);

        // Find the C at the bottom of this root's octave, then go 6 semitones down to F#
        int normalOctaveStart = (root / 12) * 12;
        int fSharpOctaveStart = normalOctaveStart - 6;

        // Map each chord note into the 12-semitone range starting at fSharpOctaveStart.
        // (note + 6) % 12 converts pitch class to position in F#-based octave:
        // F#=0, G=1, G#=2, A=3, A#=4, B=5, C=6, C#=7, D=8, D#=9, E=10, F=11
        for (int offset : triad) {
            int note = root + offset;
            int positionInFSharpOctave = (note + 6) % 12;
            res.push_back(fSharpOctaveStart + positionInFSharpOctave);
        }

        return res;
    }
};
