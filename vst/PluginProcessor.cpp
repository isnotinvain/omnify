#include "PluginProcessor.h"

#include <nlohmann/json.hpp>

#include "BinaryData.h"
#include "PluginEditor.h"

namespace {
// Create the minimal APVTS layout with just 2 realtime params
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(
    juce::AudioParameterFloat*& strumGateTimeParam,
    juce::AudioParameterFloat*& strumCooldownParam) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto gateParam = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("strum_gate_time_secs", 1), "Strum Gate Time", 0.0f, 5.0f, 0.5f);
    strumGateTimeParam = gateParam.get();
    layout.add(std::move(gateParam));

    auto cooldownParam = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("strum_cooldown_secs", 1), "Strum Cooldown", 0.0f, 5.0f, 0.3f);
    strumCooldownParam = cooldownParam.get();
    layout.add(std::move(cooldownParam));

    return layout;
}
}  // namespace

//==============================================================================
OmnifyAudioProcessor::OmnifyAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS",
                 createParameterLayout(strumGateTimeParam, strumCooldownParam)),
      settings(GeneratedSettings::DaemomnifySettings::defaults()) {
    setupValueListeners();

    // Listen for realtime parameter changes
    parameters.addParameterListener("strum_gate_time_secs", this);
    parameters.addParameterListener("strum_cooldown_secs", this);

    // Load default settings from bundled JSON (first run, before any saved state is restored)
    loadDefaultSettings();

    // Start the Python daemon and listen for ready signal
    daemonManager.setListener(this);
    daemonManager.start();
}

OmnifyAudioProcessor::~OmnifyAudioProcessor() {
    parameters.removeParameterListener("strum_gate_time_secs", this);
    parameters.removeParameterListener("strum_cooldown_secs", this);

    midiDeviceNameValue.removeListener(this);
    chordChannelValue.removeListener(this);
    strumChannelValue.removeListener(this);
    chordVoicingStyleValue.removeListener(this);
    strumVoicingStyleValue.removeListener(this);
    chordVoicingFilePathValue.removeListener(this);

    latchToggleButtonTypeValue.removeListener(this);
    latchToggleButtonNumberValue.removeListener(this);
    latchIsToggleValue.removeListener(this);
    stopButtonTypeValue.removeListener(this);
    stopButtonNumberValue.removeListener(this);
    strumPlateCcTypeValue.removeListener(this);
    strumPlateCcNumberValue.removeListener(this);

    for (size_t i = 0; i < NUM_CHORD_QUALITIES; ++i) {
        chordQualityTypeValues[i].removeListener(this);
        chordQualityNumberValues[i].removeListener(this);
    }
}

//==============================================================================
void OmnifyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    // prepareToPlay is called after state restoration (if any), so settings are now finalized
    DBG("OmnifyAudioProcessor: Settings loaded in prepareToPlay");
    {
        std::lock_guard<std::mutex> lock(initMutex);
        settingsAreLoaded = true;
    }
    trySendInitialSettings();
}

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(buffer);
    MidiLearnComponent::broadcastMidi(midiMessages);
}

//==============================================================================
juce::AudioProcessorEditor* OmnifyAudioProcessor::createEditor() {
    return new OmnifyAudioProcessorEditor(*this);
}

//==============================================================================
void OmnifyAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // Save settings to stateTree
    saveSettingsToValueTree();

    // Combine APVTS state and our stateTree
    juce::ValueTree combined{"OmnifyStateV1"};
    combined.appendChild(parameters.copyState(), nullptr);
    combined.appendChild(stateTree.createCopy(), nullptr);

    juce::MemoryOutputStream stream(destData, true);
    combined.writeToStream(stream);
}

void OmnifyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto combined = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (combined.isValid()) {
        // Restore APVTS state
        auto apvtsState = combined.getChildWithName(parameters.state.getType());
        if (apvtsState.isValid()) {
            parameters.replaceState(apvtsState);
        }

        // Restore our stateTree
        auto savedStateTree = combined.getChildWithName(stateTree.getType());
        if (savedStateTree.isValid()) {
            stateTree = savedStateTree.createCopy();
        }

        // Load settings from the restored tree
        loadSettingsFromValueTree();
        pushVariantIndexesToValueTree();

        // Re-setup value listeners since stateTree was replaced
        setupValueListeners();
    }
}

//==============================================================================
void OmnifyAudioProcessor::setupValueListeners() {
    // Bind to ValueTree properties for non-realtime settings
    midiDeviceNameValue.referTo(stateTree.getPropertyAsValue("midi_device_name", nullptr));
    chordChannelValue.referTo(stateTree.getPropertyAsValue("chord_channel", nullptr));
    strumChannelValue.referTo(stateTree.getPropertyAsValue("strum_channel", nullptr));
    chordVoicingStyleValue.referTo(stateTree.getPropertyAsValue("variant_chord_voicing_style", nullptr));
    strumVoicingStyleValue.referTo(stateTree.getPropertyAsValue("variant_strum_voicing_style", nullptr));
    chordVoicingFilePathValue.referTo(stateTree.getPropertyAsValue("chord_voicing_file", nullptr));

    // MidiLearn bindings
    latchToggleButtonTypeValue.referTo(stateTree.getPropertyAsValue("latch_toggle_button_type", nullptr));
    latchToggleButtonNumberValue.referTo(stateTree.getPropertyAsValue("latch_toggle_button_number", nullptr));
    latchIsToggleValue.referTo(stateTree.getPropertyAsValue("latch_is_toggle", nullptr));
    stopButtonTypeValue.referTo(stateTree.getPropertyAsValue("stop_button_type", nullptr));
    stopButtonNumberValue.referTo(stateTree.getPropertyAsValue("stop_button_number", nullptr));
    strumPlateCcTypeValue.referTo(stateTree.getPropertyAsValue("strum_plate_cc_type", nullptr));
    strumPlateCcNumberValue.referTo(stateTree.getPropertyAsValue("strum_plate_cc_number", nullptr));

    midiDeviceNameValue.addListener(this);
    chordChannelValue.addListener(this);
    strumChannelValue.addListener(this);
    chordVoicingStyleValue.addListener(this);
    strumVoicingStyleValue.addListener(this);
    chordVoicingFilePathValue.addListener(this);

    latchToggleButtonTypeValue.addListener(this);
    latchToggleButtonNumberValue.addListener(this);
    latchIsToggleValue.addListener(this);
    stopButtonTypeValue.addListener(this);
    stopButtonNumberValue.addListener(this);
    strumPlateCcTypeValue.addListener(this);
    strumPlateCcNumberValue.addListener(this);

    // Chord quality selector bindings - use enum names for stability
    for (size_t i = 0; i < NUM_CHORD_QUALITIES; ++i) {
        auto prefix = juce::String("chord_quality_") + GeneratedSettings::ChordQualities::ENUM_NAMES[i];
        chordQualityTypeValues[i].referTo(stateTree.getPropertyAsValue(prefix + "_type", nullptr));
        chordQualityNumberValues[i].referTo(stateTree.getPropertyAsValue(prefix + "_number", nullptr));
        chordQualityTypeValues[i].addListener(this);
        chordQualityNumberValues[i].addListener(this);
    }

    // "One CC for All" chord quality selection (CCRangePerChordQuality)
    chordQualityCcTypeValue.referTo(stateTree.getPropertyAsValue("chord_quality_cc_type", nullptr));
    chordQualityCcNumberValue.referTo(stateTree.getPropertyAsValue("chord_quality_cc_number", nullptr));
    chordQualityCcTypeValue.addListener(this);
    chordQualityCcNumberValue.addListener(this);

    // Chord quality selection style variant (0 = ButtonPerChordQuality, 1 = CCRangePerChordQuality)
    chordQualitySelectionStyleValue.referTo(
        stateTree.getPropertyAsValue("variant_chord_quality_selection_style", nullptr));
    chordQualitySelectionStyleValue.addListener(this);
}

void OmnifyAudioProcessor::valueChanged(juce::Value& value) {
    bool settingsChanged = false;

    if (value.refersToSameSourceAs(midiDeviceNameValue)) {
        settings.midi_device_name = midiDeviceNameValue.getValue().toString().toStdString();
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(chordChannelValue)) {
        settings.chord_channel = static_cast<int>(chordChannelValue.getValue());
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(strumChannelValue)) {
        settings.strum_channel = static_cast<int>(strumChannelValue.getValue());
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(chordVoicingStyleValue)) {
        int index = static_cast<int>(chordVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                settings.chord_voicing_style = GeneratedSettings::RootPositionStyle{};
                break;
            case 1: {
                GeneratedSettings::FileStyle fs;
                fs.path = chordVoicingFilePathValue.getValue().toString().toStdString();
                settings.chord_voicing_style = fs;
                break;
            }
            case 2:
                settings.chord_voicing_style = GeneratedSettings::Omni84Style{};
                break;
        }
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(strumVoicingStyleValue)) {
        int index = static_cast<int>(strumVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                settings.strum_voicing_style = GeneratedSettings::PlainAscendingStrumStyle{};
                break;
            case 1:
                settings.strum_voicing_style = GeneratedSettings::OmnichordStrumStyle{};
                break;
        }
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(chordVoicingFilePathValue)) {
        auto path = chordVoicingFilePathValue.getValue().toString().toStdString();
        if (auto* fileStyle = std::get_if<GeneratedSettings::FileStyle>(&settings.chord_voicing_style)) {
            fileStyle->path = path;
            settingsChanged = true;
        }
    } else if (value.refersToSameSourceAs(latchToggleButtonTypeValue) ||
               value.refersToSameSourceAs(latchToggleButtonNumberValue) ||
               value.refersToSameSourceAs(latchIsToggleValue)) {
        // Update latch_toggle_button from ValueTree properties
        auto typeStr = latchToggleButtonTypeValue.getValue().toString();
        int number = static_cast<int>(latchToggleButtonNumberValue.getValue());
        bool isToggle = static_cast<bool>(latchIsToggleValue.getValue());

        if (typeStr == "note") {
            settings.latch_toggle_button = GeneratedSettings::MidiNoteButton{number};
        } else if (typeStr == "cc") {
            settings.latch_toggle_button = GeneratedSettings::MidiCCButton{number, isToggle};
        }
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(stopButtonTypeValue) ||
               value.refersToSameSourceAs(stopButtonNumberValue)) {
        // Update stop_button from ValueTree properties
        auto typeStr = stopButtonTypeValue.getValue().toString();
        int number = static_cast<int>(stopButtonNumberValue.getValue());

        if (typeStr == "note") {
            settings.stop_button = GeneratedSettings::MidiNoteButton{number};
        } else if (typeStr == "cc") {
            // stop_button is never toggle mode
            settings.stop_button = GeneratedSettings::MidiCCButton{number, false};
        }
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(strumPlateCcTypeValue) ||
               value.refersToSameSourceAs(strumPlateCcNumberValue)) {
        // strum_plate_cc is just an int (CC number)
        int number = static_cast<int>(strumPlateCcNumberValue.getValue());
        if (number > 0) {
            settings.strum_plate_cc = number;
            settingsChanged = true;
        }
    } else if (value.refersToSameSourceAs(chordQualitySelectionStyleValue)) {
        // Variant selector changed - rebuild the appropriate variant
        int variantIndex = static_cast<int>(chordQualitySelectionStyleValue.getValue());
        if (variantIndex == 0) {
            // ButtonPerChordQuality - rebuild from per-quality values
            GeneratedSettings::ButtonPerChordQuality bpq;
            for (size_t j = 0; j < NUM_CHORD_QUALITIES; ++j) {
                auto typeStr = chordQualityTypeValues[j].getValue().toString();
                int number = static_cast<int>(chordQualityNumberValues[j].getValue());
                if (number > 0) {
                    auto quality = static_cast<GeneratedSettings::ChordQuality>(j);
                    if (typeStr == "note") {
                        bpq.notes[number] = quality;
                    } else if (typeStr == "cc") {
                        bpq.ccs[number] = quality;
                    }
                }
            }
            settings.chord_quality_selection_style = bpq;
        } else if (variantIndex == 1) {
            // CCRangePerChordQuality - use the single CC value
            int ccNumber = static_cast<int>(chordQualityCcNumberValue.getValue());
            GeneratedSettings::CCRangePerChordQuality ccrpq;
            ccrpq.cc = ccNumber > 0 ? ccNumber : 1;  // Default to CC 1 if not set
            settings.chord_quality_selection_style = ccrpq;
        }
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(chordQualityCcTypeValue) ||
               value.refersToSameSourceAs(chordQualityCcNumberValue)) {
        // "One CC for All" value changed - only apply if that variant is active
        int variantIndex = static_cast<int>(chordQualitySelectionStyleValue.getValue());
        if (variantIndex == 1) {
            int ccNumber = static_cast<int>(chordQualityCcNumberValue.getValue());
            if (ccNumber > 0) {
                GeneratedSettings::CCRangePerChordQuality ccrpq;
                ccrpq.cc = ccNumber;
                settings.chord_quality_selection_style = ccrpq;
                settingsChanged = true;
            }
        }
    } else {
        // Check if it's a chord quality per-button value
        int variantIndex = static_cast<int>(chordQualitySelectionStyleValue.getValue());
        if (variantIndex == 0) {  // Only apply if ButtonPerChordQuality is active
            for (size_t i = 0; i < NUM_CHORD_QUALITIES; ++i) {
                if (value.refersToSameSourceAs(chordQualityTypeValues[i]) ||
                    value.refersToSameSourceAs(chordQualityNumberValues[i])) {
                    // Rebuild the entire ButtonPerChordQuality from current values
                    GeneratedSettings::ButtonPerChordQuality bpq;

                    for (size_t j = 0; j < NUM_CHORD_QUALITIES; ++j) {
                        auto typeStr = chordQualityTypeValues[j].getValue().toString();
                        int number = static_cast<int>(chordQualityNumberValues[j].getValue());

                        if (number > 0) {
                            auto quality = static_cast<GeneratedSettings::ChordQuality>(j);
                            if (typeStr == "note") {
                                bpq.notes[number] = quality;
                            } else if (typeStr == "cc") {
                                bpq.ccs[number] = quality;
                            }
                        }
                    }

                    settings.chord_quality_selection_style = bpq;
                    settingsChanged = true;
                    break;
                }
            }
        }
    }

    if (settingsChanged) {
        saveSettingsToValueTree();
        // Only send to daemon after initial settings have been sent
        // (avoids spamming during initialization when properties are being set up)
        if (initialSettingsSent) {
            sendSettingsToDaemon();
        }
    }
}

void OmnifyAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    // Realtime parameters - send dedicated OSC messages
    if (parameterID == "strum_gate_time_secs") {
        settings.strum_gate_time_secs = newValue;
        sendRealtimeParam("/strum_gate", newValue);
    } else if (parameterID == "strum_cooldown_secs") {
        settings.strum_cooldown_secs = newValue;
        sendRealtimeParam("/strum_cooldown", newValue);
    }
}

void OmnifyAudioProcessor::pushVariantIndexesToValueTree() {
    // Write variant indexes so UI can restore them on rebuild
    stateTree.setProperty("variant_chord_voicing_style",
                          static_cast<int>(settings.chord_voicing_style.index()), nullptr);
    stateTree.setProperty("variant_strum_voicing_style",
                          static_cast<int>(settings.strum_voicing_style.index()), nullptr);
    stateTree.setProperty("variant_chord_quality_selection_style",
                          static_cast<int>(settings.chord_quality_selection_style.index()), nullptr);

    // Write file path from FileStyle if that's the active variant
    if (auto* fileStyle = std::get_if<GeneratedSettings::FileStyle>(&settings.chord_voicing_style)) {
        stateTree.setProperty("chord_voicing_file", juce::String(fileStyle->path), nullptr);
    }

    // Write scalar settings to ValueTree properties
    stateTree.setProperty("chord_channel", settings.chord_channel, nullptr);
    stateTree.setProperty("strum_channel", settings.strum_channel, nullptr);
    stateTree.setProperty("midi_device_name", juce::String(settings.midi_device_name), nullptr);

    // Write MidiLearn values to ValueTree
    // latch_toggle_button
    if (auto* noteBtn =
            std::get_if<GeneratedSettings::MidiNoteButton>(&settings.latch_toggle_button)) {
        stateTree.setProperty("latch_toggle_button_type", "note", nullptr);
        stateTree.setProperty("latch_toggle_button_number", noteBtn->note, nullptr);
        stateTree.setProperty("latch_is_toggle", false, nullptr);
    } else if (auto* ccBtn =
                   std::get_if<GeneratedSettings::MidiCCButton>(&settings.latch_toggle_button)) {
        stateTree.setProperty("latch_toggle_button_type", "cc", nullptr);
        stateTree.setProperty("latch_toggle_button_number", ccBtn->cc, nullptr);
        stateTree.setProperty("latch_is_toggle", ccBtn->is_toggle, nullptr);
    }

    // stop_button
    if (auto* noteBtn = std::get_if<GeneratedSettings::MidiNoteButton>(&settings.stop_button)) {
        stateTree.setProperty("stop_button_type", "note", nullptr);
        stateTree.setProperty("stop_button_number", noteBtn->note, nullptr);
    } else if (auto* ccBtn = std::get_if<GeneratedSettings::MidiCCButton>(&settings.stop_button)) {
        stateTree.setProperty("stop_button_type", "cc", nullptr);
        stateTree.setProperty("stop_button_number", ccBtn->cc, nullptr);
    }

    // strum_plate_cc - just a CC number
    stateTree.setProperty("strum_plate_cc_type", "cc", nullptr);
    stateTree.setProperty("strum_plate_cc_number", settings.strum_plate_cc, nullptr);

    // Chord quality selector - handle both variants
    // First clear all chord quality properties (use enum names for stability)
    for (size_t i = 0; i < NUM_CHORD_QUALITIES; ++i) {
        auto prefix = juce::String("chord_quality_") + GeneratedSettings::ChordQualities::ENUM_NAMES[i];
        stateTree.setProperty(prefix + "_type", "", nullptr);
        stateTree.setProperty(prefix + "_number", -1, nullptr);
    }
    // Clear the "One CC for All" property too
    stateTree.setProperty("chord_quality_cc_type", "", nullptr);
    stateTree.setProperty("chord_quality_cc_number", -1, nullptr);

    // Now write the mappings from the settings based on which variant is active
    if (auto* bpq = std::get_if<GeneratedSettings::ButtonPerChordQuality>(
            &settings.chord_quality_selection_style)) {
        // Write note mappings
        for (const auto& [noteNum, quality] : bpq->notes) {
            int qualityIdx = static_cast<int>(quality);
            auto prefix = juce::String("chord_quality_") + GeneratedSettings::ChordQualities::ENUM_NAMES[qualityIdx];
            stateTree.setProperty(prefix + "_type", "note", nullptr);
            stateTree.setProperty(prefix + "_number", noteNum, nullptr);
        }
        // Write CC mappings
        for (const auto& [ccNum, quality] : bpq->ccs) {
            int qualityIdx = static_cast<int>(quality);
            auto prefix = juce::String("chord_quality_") + GeneratedSettings::ChordQualities::ENUM_NAMES[qualityIdx];
            stateTree.setProperty(prefix + "_type", "cc", nullptr);
            stateTree.setProperty(prefix + "_number", ccNum, nullptr);
        }
    } else if (auto* ccrpq = std::get_if<GeneratedSettings::CCRangePerChordQuality>(
                   &settings.chord_quality_selection_style)) {
        // "One CC for All" mode - just store the single CC number
        stateTree.setProperty("chord_quality_cc_type", "cc", nullptr);
        stateTree.setProperty("chord_quality_cc_number", ccrpq->cc, nullptr);
    }
}

void OmnifyAudioProcessor::loadSettingsFromValueTree() {
    auto jsonString = stateTree.getProperty(SETTINGS_JSON_KEY, "").toString();
    if (jsonString.isNotEmpty()) {
        try {
            settings = GeneratedSettings::fromJson(jsonString.toStdString());

            // Sync APVTS params from loaded settings
            if (strumGateTimeParam) {
                strumGateTimeParam->setValueNotifyingHost(
                    strumGateTimeParam->convertTo0to1(static_cast<float>(settings.strum_gate_time_secs)));
            }
            if (strumCooldownParam) {
                strumCooldownParam->setValueNotifyingHost(
                    strumCooldownParam->convertTo0to1(static_cast<float>(settings.strum_cooldown_secs)));
            }
        } catch (...) {
            settings = GeneratedSettings::DaemomnifySettings::defaults();
        }
    }
}

void OmnifyAudioProcessor::saveSettingsToValueTree() {
    auto jsonString = GeneratedSettings::toJson(settings);
    stateTree.setProperty(SETTINGS_JSON_KEY, juce::String(jsonString), nullptr);
}

void OmnifyAudioProcessor::loadDefaultSettings() {
    try {
        // Parse the bundled default_settings.json (now flat structure)
        juce::String jsonStr(BinaryData::default_settings_json, BinaryData::default_settings_jsonSize);
        settings = GeneratedSettings::fromJson(jsonStr.toStdString());

        // Sync APVTS params from loaded settings
        if (strumGateTimeParam) {
            strumGateTimeParam->setValueNotifyingHost(
                strumGateTimeParam->convertTo0to1(static_cast<float>(settings.strum_gate_time_secs)));
        }
        if (strumCooldownParam) {
            strumCooldownParam->setValueNotifyingHost(
                strumCooldownParam->convertTo0to1(static_cast<float>(settings.strum_cooldown_secs)));
        }

        // Push all settings to ValueTree properties
        saveSettingsToValueTree();
        pushVariantIndexesToValueTree();

        DBG("OmnifyAudioProcessor: Loaded default settings from bundled JSON");
    } catch (const std::exception& e) {
        DBG("OmnifyAudioProcessor: Failed to load default settings: " << e.what());
    }
}

//==============================================================================
void OmnifyAudioProcessor::daemonReady() {
    DBG("OmnifyAudioProcessor: Daemon ready");
    {
        std::lock_guard<std::mutex> lock(initMutex);
        daemonIsReady = true;
    }
    trySendInitialSettings();
}

void OmnifyAudioProcessor::trySendInitialSettings() {
    // Send initial settings when both conditions are met (exactly once)
    std::lock_guard<std::mutex> lock(initMutex);
    if (daemonIsReady && settingsAreLoaded && !initialSettingsSent) {
        initialSettingsSent = true;
        DBG("OmnifyAudioProcessor: Sending initial settings");
        sendSettingsToDaemon();
    }
}

void OmnifyAudioProcessor::sendSettingsToDaemon() {
    // Sync realtime params from APVTS to settings struct before sending
    if (strumGateTimeParam) {
        settings.strum_gate_time_secs = strumGateTimeParam->get();
    }
    if (strumCooldownParam) {
        settings.strum_cooldown_secs = strumCooldownParam->get();
    }

    // Send flat JSON structure
    std::string jsonStr = GeneratedSettings::toJson(settings);
    daemonManager.getOscSender().send("/settings", juce::String(jsonStr));

    DBG("OmnifyAudioProcessor: Sent settings: " << jsonStr);
}

void OmnifyAudioProcessor::sendRealtimeParam(const juce::String& address, float value) {
    daemonManager.getOscSender().send(address, value);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
