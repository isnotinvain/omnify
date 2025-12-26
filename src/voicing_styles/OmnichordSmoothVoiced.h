#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"
#include "OmnichordChords.h"
#include "SmoothVoiced.h"

class OmnichordSmoothVoiced : public VoicingStyle<VoicingFor::Chord> {
   public:
    OmnichordSmoothVoiced() = default;

    std::string displayName() const override { return "Omnichord Smooth Voiced"; }
    std::string description() const override { return "Octaves use different inversions."; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        auto normalizedRoot = 60 + (root % 12);
        auto middleOctaveNotes = oc.constructChord(quality, normalizedRoot);
        std::vector<int> offsets;
        offsets.reserve(middleOctaveNotes.size());
        for (int x : middleOctaveNotes) {
            offsets.push_back(x - 60);
        }
        std::sort(offsets.begin(), offsets.end());
        return SmoothVoiced::smooth(offsets, root);
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "OmnichordSmoothVoiced"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json&) { return std::make_shared<OmnichordSmoothVoiced>(); }

   private:
    OmnichordChords oc;
};
