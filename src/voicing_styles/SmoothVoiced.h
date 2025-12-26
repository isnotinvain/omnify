#pragma once

#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class SmoothVoiced : public VoicingStyle<VoicingFor::Chord> {
   public:
    SmoothVoiced() = default;

    std::string displayName() const override { return "Smooth Voiced"; }
    std::string description() const override { return "Octaves use different inversions."; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& offsets = getChordQualityData(quality).offsets;
        return smooth(offsets, root);
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "SmoothVoiced"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json&) { return std::make_shared<SmoothVoiced>(); }

    static std::vector<int> smooth(std::vector<int> offsets, int root) {
        auto octave = root / 12;
        auto pitchClass = root % 12;
        std::vector<int> notes;
        notes.reserve(offsets.size());

        std::vector<int> inversionOffsets(offsets.size(), 0);

        switch (octave) {
            case 3:
                inversionOffsets[inversionOffsets.size() - 2] = -12;
                inversionOffsets[inversionOffsets.size() - 1] = -12;
                break;
            case 4:
                inversionOffsets[inversionOffsets.size() - 1] = -12;
                break;
            case 5:
                break;
            case 6:
                inversionOffsets[0] = 12;
                break;
            case 7:
                inversionOffsets[0] = 12;
                inversionOffsets[1] = 12;
                break;
            default:
                return {};
        }

        for (size_t i = 0; i < offsets.size(); i++) {
            notes.push_back(60 + pitchClass + offsets[i] + inversionOffsets[i]);
        }

        return notes;
    }
};
