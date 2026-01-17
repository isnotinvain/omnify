#pragma once

#include <json.hpp>

#include "ChordQualitySelectionStyle.h"
#include "DawOrDevice.h"
#include "MidiButton.h"
#include "VoicingModifier.h"
#include "VoicingType.h"

class OmnifySettings {
   public:
    DawOrDevice input = Daw{};
    DawOrDevice output = Daw{};
    int chordChannel = 1;
    int strumChannel = 2;

    int strumCooldownMs = 300;
    int strumGateTimeMs = 500;
    int strumPlateCC = 1;

    const VoicingStyle<VoicingFor::Chord>* chordVoicingStyle = chordVoicings().at(ChordVoicingType::Omnichord);
    const VoicingStyle<VoicingFor::Strum>* strumVoicingStyle = strumVoicings().at(StrumVoicingType::Omnichord);
    VoicingModifier voicingModifier = VoicingModifier::NONE;

    ChordQualitySelectionStyle chordQualitySelectionStyle = ButtonPerChordQuality();
    MidiButton latchButton;
    MidiButton stopButton;

    OmnifySettings() = default;

    nlohmann::json to_json() const;
    static OmnifySettings from_json(const nlohmann::json& j);
};