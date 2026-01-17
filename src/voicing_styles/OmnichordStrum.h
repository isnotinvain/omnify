#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class OmnichordStrum : public VoicingStyle<VoicingFor::Strum> {
   public:
    OmnichordStrum() = default;

    std::string displayName() const override { return "Omnichord"; }
    std::string description() const override {
        return "Behaves like a real Omnichord. Three 'most important' notes of the chord, using the same inversions as the Omnichord, repeated "
               "across octaves.";
    }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& triad = getChordQualityData(quality).triadOffsets;
        std::vector<int> res;
        res.reserve(15);
        int rootOctaveStart = findLowestFSharp(root);
        for (int o : {-12, 0, 12, 24, 36}) {
            int thisOctaveStart = rootOctaveStart + o;
            for (int offset : triad) {
                int note = root + offset;
                res.push_back(clamp(thisOctaveStart, note));
            }
        }

        // We only need 13 notes not 15
        res.resize(13);
        return res;
    }

   private:
    static int clamp(int lowestFSharp, int n) {
        int pitchClass = n % 12;
        int distanceFromFSharp = pitchClass - 6;
        if (distanceFromFSharp < 0) {
            distanceFromFSharp += 12;
        }
        return lowestFSharp + distanceFromFSharp;
    }

    static int findLowestFSharp(int root) {
        int rootShifted = std::max(0, root - 6);
        int rootOctave = rootShifted / 12;
        int lowestFSharp = (rootOctave * 12) + 6;
        return lowestFSharp;
    }
};
