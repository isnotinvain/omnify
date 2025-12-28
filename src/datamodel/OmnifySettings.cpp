#include "OmnifySettings.h"

nlohmann::json OmnifySettings::to_json() const {
    nlohmann::json j;
    j["input"] = input;
    j["output"] = output;
    j["chordChannel"] = chordChannel;
    j["strumChannel"] = strumChannel;
    j["strumCooldownMs"] = strumCooldownMs;
    j["strumGateTimeMs"] = strumGateTimeMs;
    j["strumPlateCC"] = strumPlateCC;
    chordVoicingStyle->to_json(j["chordVoicingStyle"]);
    strumVoicingStyle->to_json(j["strumVoicingStyle"]);
    j["voicingModifier"] = voicingModifier;
    j["chordQualitySelectionStyle"] = chordQualitySelectionStyle;
    j["latchButton"] = latchButton;
    j["stopButton"] = stopButton;
    return j;
}

OmnifySettings OmnifySettings::from_json(const nlohmann::json& j, VoicingStyleRegistry<VoicingFor::Chord>& chordRegistry,
                                         VoicingStyleRegistry<VoicingFor::Strum>& strumRegistry) {
    OmnifySettings settings;

    settings.input = j.at("input").get<DawOrDevice>();
    settings.output = j.at("output").get<DawOrDevice>();
    settings.chordChannel = j.at("chordChannel").get<int>();
    settings.strumChannel = j.at("strumChannel").get<int>();
    settings.strumCooldownMs = j.at("strumCooldownMs").get<int>();
    settings.strumGateTimeMs = j.at("strumGateTimeMs").get<int>();
    settings.strumPlateCC = j.at("strumPlateCC").get<int>();

    settings.chordVoicingStyle = chordRegistry.from_json(j.at("chordVoicingStyle"));
    settings.strumVoicingStyle = strumRegistry.from_json(j.at("strumVoicingStyle"));
    settings.voicingModifier = j.at("voicingModifier").get<VoicingModifier>();

    settings.chordQualitySelectionStyle = j.at("chordQualitySelectionStyle").get<ChordQualitySelectionStyle>();
    settings.latchButton = j.at("latchButton").get<MidiButton>();
    settings.stopButton = j.at("stopButton").get<MidiButton>();

    return settings;
}