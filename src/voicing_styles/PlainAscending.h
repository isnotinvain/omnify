#pragma once

#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class PlainAscending : public VoicingStyle<VoicingFor::Strum> {
   public:
    PlainAscending() = default;

    std::string displayName() const override { return "Plain Ascending"; }
    std::string description() const override {
        return "The three 'most important' notes of the chord, in ascending root-position order, repeating up each octave.";
    }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        const auto& triad = getChordQualityData(quality).triadOffsets;
        std::vector<int> res;
        res.reserve(13);

        for (int shift : {-12, 0, 12, 24}) {
            for (int o : triad) {
                res.push_back(root + shift + o);
            }
        }
        // 13th element is one more octave up
        res.push_back(root + 36);
        return res;
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "PlainAscending"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Strum>> from_json(const nlohmann::json&) { return std::make_shared<PlainAscending>(); }
};
