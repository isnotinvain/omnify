#pragma once

#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class RootPosition : public VoicingStyle<VoicingFor::Chord> {
   public:
    RootPosition() = default;

    std::string displayName() const override { return "Root Position"; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& offsets = getChordQualityData(quality).offsets;
        std::vector<int> notes;
        notes.reserve(offsets.size());
        for (int offset : offsets) {
            notes.push_back(root + offset);
        }
        return notes;
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "RootPosition"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json&) { return std::make_shared<RootPosition>(); }
};
