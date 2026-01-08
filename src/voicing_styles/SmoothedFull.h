#pragma once

#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class SmoothedFull : public VoicingStyle<VoicingFor::Chord> {
   public:
    std::string displayName() const override { return "Smoothed Full"; }
    std::string description() const override { return "The full chord (3 or 4 notes), constrained to the root's octave."; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& offsets = getChordQualityData(quality).offsets;
        std::vector<int> res;
        res.reserve(offsets.size());

        // Find the C at the bottom of this root's octave
        int octaveStart = (root / 12) * 12;

        for (int offset : offsets) {
            int note = root + offset;
            int pitchClass = note % 12;
            res.push_back(octaveStart + pitchClass);
        }

        return res;
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "SmoothedFull"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json& j) { return std::make_shared<SmoothedFull>(); }
};
