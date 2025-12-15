#pragma once

#include <map>
#include <string>
#include <vector>

#include "../datamodel/VoicingStyle.h"

class Omni84 : public VoicingStyle<VoicingFor::Chord> {
   public:
    static inline const std::map<ChordQuality, int> OCTAVE_BEGIN_MAP = {
        {ChordQuality::MAJOR, 36},
        {ChordQuality::MINOR, 48},
        {ChordQuality::DOM_7, 60},
    };

    Omni84() = default;

    std::string displayName() const override { return "Omni-84"; }

    std::vector<int> constructChord(ChordQuality quality, int root) const override {
        int pitchClass = root % 12;
        auto it = OCTAVE_BEGIN_MAP.find(quality);
        int octaveBegin = (it != OCTAVE_BEGIN_MAP.end()) ? it->second : 36;
        return {octaveBegin + pitchClass};
    }

    void to_json(nlohmann::json& j) const override { j = nlohmann::json{{"type", "Omni84"}}; }

    static std::shared_ptr<VoicingStyle<VoicingFor::Chord>> from_json(const nlohmann::json&) { return std::make_shared<Omni84>(); }
};