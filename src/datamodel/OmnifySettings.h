#pragma once

#include <json.hpp>
#include <memory>
#include <string>

#include "ChordQualitySelectionStyle.h"
#include "DawOrDevice.h"
#include "MidiButton.h"
#include "VoicingModifier.h"
#include "VoicingStyle.h"
#include "voicing_styles/OmnichordChords.h"
#include "voicing_styles/OmnichordStrum.h"

class OmnifySettings {
   public:
    DawOrDevice input = Daw{};
    DawOrDevice output = Daw{};
    int chordChannel = 1;
    int strumChannel = 2;

    int strumCooldownMs = 300;
    int strumGateTimeMs = 500;
    int strumPlateCC = 1;

    std::shared_ptr<VoicingStyle<VoicingFor::Chord>> chordVoicingStyle = std::make_shared<OmnichordChords>();
    std::shared_ptr<VoicingStyle<VoicingFor::Strum>> strumVoicingStyle = std::make_shared<OmnichordStrum>();
    VoicingModifier voicingModifier = VoicingModifier::NONE;

    ChordQualitySelectionStyle chordQualitySelectionStyle = ButtonPerChordQuality();
    MidiButton latchButton;
    MidiButton stopButton;

    OmnifySettings() = default;

    nlohmann::json to_json() const;
    static OmnifySettings from_json(const nlohmann::json& j, VoicingStyleRegistry<VoicingFor::Chord>& chordRegistry,
                                    VoicingStyleRegistry<VoicingFor::Strum>& strumRegistry);
};