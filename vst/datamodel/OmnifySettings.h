#pragma once

#include <json.hpp>
#include <memory>
#include <string>

#include "ChordQualitySelectionStyle.h"
#include "MidiButton.h"
#include "VoicingStyle.h"

class OmnifySettings {
   public:
    std::string midiDeviceName;
    int chordChannel = 1;
    int strumChannel = 2;

    int strumCooldownMs = 300;
    int strumGateTimeMs = 500;
    int strumPlateCC = 1;

    std::shared_ptr<VoicingStyle<VoicingFor::Chord>> chordVoicingStyle;
    std::shared_ptr<VoicingStyle<VoicingFor::Strum>> strumVoicingStyle;

    ChordQualitySelectionStyle chordQualitySelectionStyle;
    MidiButton latchButton;
    MidiButton stopButton;

    OmnifySettings() = default;

    nlohmann::json to_json() const;
    static OmnifySettings from_json(const nlohmann::json& j, VoicingStyleRegistry<VoicingFor::Chord>& chordRegistry,
                                    VoicingStyleRegistry<VoicingFor::Strum>& strumRegistry);
};