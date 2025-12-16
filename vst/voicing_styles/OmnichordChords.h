#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class OmnichordChords : public VoicingStyle<VoicingFor::Chord> {
   public:
    explicit OmnichordChords(bool relative) : relative(relative) {}

    std::string displayName() const override { return "Omnichord"; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& triad = getChordQualityData(quality).triadOffsets;
        std::vector<int> res;
        res.reserve(3);
        int beginFSharpOctave = 54;
        if (relative) {
            beginFSharpOctave = findLowestFSharp(root);
        }
        for (auto o : triad) {
            res.push_back(clamp(beginFSharpOctave, (root + o)));
        }
        return res;
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "Omnichord"}, {"relative", relative}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json& j) {
        return std::make_shared<OmnichordChords>(j.at("relative").get<bool>());
    }

   private:
    bool relative = true;

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
