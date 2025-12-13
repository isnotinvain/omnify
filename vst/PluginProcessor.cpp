#include "PluginProcessor.h"

#include <nlohmann/json.hpp>

#include "BinaryData.h"
#include "ui/GuiItems.h"
#include "ui/LcarsLookAndFeel.h"

//==============================================================================
OmnifyAudioProcessor::OmnifyAudioProcessor()
    : foleys::MagicProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", GeneratedParams::createParameterLayout(params)),
      additionalSettings(GeneratedAdditionalSettings::Settings::defaults()) {
    FOLEYS_SET_SOURCE_PATH(__FILE__);

    // Update parameter map after APVTS is created so Foleys can see the parameters
    magicState.updateParameterMap();

    // Load the GUI layout - from file in debug mode for hot reload, from binary data in release
#if JUCE_DEBUG
    auto xmlFile = juce::File(__FILE__).getParentDirectory().getChildFile("Resources/magic.xml");
    if (xmlFile.existsAsFile()) {
        magicState.setGuiValueTree(xmlFile);
    } else {
        magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);
    }
#else
    magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);
#endif

    setupValueListeners();

    // Load default settings from bundled JSON (first run, before any saved state is restored)
    loadDefaultSettings();

    // Start the Python daemon and listen for ready signal
    daemonManager.setListener(this);
    daemonManager.start();
}

OmnifyAudioProcessor::~OmnifyAudioProcessor() {
    chordVoicingStyleValue.removeListener(this);
    strumVoicingStyleValue.removeListener(this);
    chordVoicingFilePathValue.removeListener(this);
}

//==============================================================================
void OmnifyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(buffer);
    MidiLearnComponent::broadcastMidi(midiMessages);
}

//==============================================================================
void OmnifyAudioProcessor::initialiseBuilder(foleys::MagicGUIBuilder& builder) {
    builder.registerJUCEFactories();
    builder.registerJUCELookAndFeels();

    auto lcarsLookAndFeel = std::make_unique<LcarsLookAndFeel>();
    lcarsLookAndFeel->setBuilder(&builder);
    builder.registerLookAndFeel("Lcars", std::move(lcarsLookAndFeel));

    builder.registerFactory("OmnifyMidiLearn", &MidiLearnItem::factory);
    builder.registerFactory("ChordQualitySelector", &ChordQualitySelectorItem::factory);
    builder.registerFactory("VariantSelector", &VariantSelectorItem::factory);
    builder.registerFactory("LcarsSettings", &LcarsSettingsItem::factory);
    builder.registerFactory("FilePicker", &FilePickerItem::factory);
    builder.registerFactory("MidiDeviceSelector", &MidiDeviceSelectorItem::factory);
}

//==============================================================================
void OmnifyAudioProcessor::postSetStateInformation() {
    loadAdditionalSettingsFromValueTree();
    pushVariantIndexesToValueTree();
}

//==============================================================================
void OmnifyAudioProcessor::setupValueListeners() {
    chordVoicingStyleValue.referTo(magicState.getValueTree().getPropertyAsValue("variant_chord_voicing_style", nullptr));
    strumVoicingStyleValue.referTo(magicState.getValueTree().getPropertyAsValue("variant_strum_voicing_style", nullptr));
    chordVoicingFilePathValue.referTo(magicState.getPropertyAsValue("chords_json_file"));

    chordVoicingStyleValue.addListener(this);
    strumVoicingStyleValue.addListener(this);
    chordVoicingFilePathValue.addListener(this);
}

void OmnifyAudioProcessor::valueChanged(juce::Value& value) {
    if (value.refersToSameSourceAs(chordVoicingStyleValue)) {
        int index = static_cast<int>(chordVoicingStyleValue.getValue());
        // When switching to FileStyle, restore path from ValueTree (it persists there even when variant changes)
        switch (index) {
            case 0:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::RootPositionStyle{};
                break;
            case 1: {
                GeneratedAdditionalSettings::FileStyle fs;
                // Restore path from state property (persists across variant switches)
                fs.path = magicState.getPropertyAsValue("chords_json_file").toString().toStdString();
                additionalSettings.chord_voicing_style = fs;
                break;
            }
            case 2:
                additionalSettings.chord_voicing_style = GeneratedAdditionalSettings::Omni84Style{};
                break;
        }
        saveAdditionalSettingsToValueTree();
    } else if (value.refersToSameSourceAs(strumVoicingStyleValue)) {
        int index = static_cast<int>(strumVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                additionalSettings.strum_voicing_style = GeneratedAdditionalSettings::PlainAscendingStrumStyle{};
                break;
            case 1:
                additionalSettings.strum_voicing_style = GeneratedAdditionalSettings::OmnichordStrumStyle{};
                break;
        }
        saveAdditionalSettingsToValueTree();
    } else if (value.refersToSameSourceAs(chordVoicingFilePathValue)) {
        auto path = chordVoicingFilePathValue.getValue().toString().toStdString();
        // Update path in FileStyle if that's the current variant
        if (auto* fileStyle = std::get_if<GeneratedAdditionalSettings::FileStyle>(&additionalSettings.chord_voicing_style)) {
            fileStyle->path = path;
            saveAdditionalSettingsToValueTree();
        }
    }
}

void OmnifyAudioProcessor::pushVariantIndexesToValueTree() {
    // Write variant indexes so UI can restore them on rebuild
    magicState.getValueTree().setProperty("variant_chord_voicing_style",
                                          static_cast<int>(additionalSettings.chord_voicing_style.index()), nullptr);
    magicState.getValueTree().setProperty("variant_strum_voicing_style",
                                          static_cast<int>(additionalSettings.strum_voicing_style.index()), nullptr);

    // Write file path from FileStyle if that's the active variant
    if (auto* fileStyle = std::get_if<GeneratedAdditionalSettings::FileStyle>(&additionalSettings.chord_voicing_style)) {
        magicState.getPropertyAsValue("chords_json_file").setValue(juce::String(fileStyle->path));
    }
}

void OmnifyAudioProcessor::loadAdditionalSettingsFromValueTree() {
    auto jsonString = magicState.getValueTree().getProperty(ADDITIONAL_SETTINGS_KEY, "").toString();
    if (jsonString.isNotEmpty()) {
        try {
            additionalSettings = GeneratedAdditionalSettings::fromJson(jsonString.toStdString());
        } catch (...) {
            additionalSettings = GeneratedAdditionalSettings::Settings::defaults();
        }
    }
}

void OmnifyAudioProcessor::saveAdditionalSettingsToValueTree() {
    auto jsonString = GeneratedAdditionalSettings::toJson(additionalSettings);
    magicState.getValueTree().setProperty(ADDITIONAL_SETTINGS_KEY, juce::String(jsonString), nullptr);
}

void OmnifyAudioProcessor::loadDefaultSettings() {
    try {
        // Parse the bundled default_settings.json
        juce::String jsonStr(BinaryData::default_settings_json, BinaryData::default_settings_jsonSize);
        auto json = nlohmann::json::parse(jsonStr.toStdString());

        // Set scalar parameters via APVTS
        // midi_device_name - find index in choices list
        if (json.contains("midi_device_name") && params.midi_device_name) {
            juce::String deviceName = juce::String(json["midi_device_name"].get<std::string>());
            int idx = params.midi_device_name->choices.indexOf(deviceName);
            if (idx >= 0) {
                params.midi_device_name->setValueNotifyingHost(
                    params.midi_device_name->convertTo0to1(static_cast<float>(idx)));
            }
        }

        // chord_channel (1-indexed in JSON, 0-indexed in param)
        if (json.contains("chord_channel") && params.chord_channel) {
            int channel = json["chord_channel"].get<int>() - 1;
            params.chord_channel->setValueNotifyingHost(
                params.chord_channel->convertTo0to1(static_cast<float>(channel)));
        }

        // strum_channel (1-indexed in JSON, 0-indexed in param)
        if (json.contains("strum_channel") && params.strum_channel) {
            int channel = json["strum_channel"].get<int>() - 1;
            params.strum_channel->setValueNotifyingHost(
                params.strum_channel->convertTo0to1(static_cast<float>(channel)));
        }

        // strum_cooldown_secs
        if (json.contains("strum_cooldown_secs") && params.strum_cooldown_secs) {
            float value = json["strum_cooldown_secs"].get<float>();
            params.strum_cooldown_secs->setValueNotifyingHost(
                params.strum_cooldown_secs->convertTo0to1(value));
        }

        // strum_gate_time_secs
        if (json.contains("strum_gate_time_secs") && params.strum_gate_time_secs) {
            float value = json["strum_gate_time_secs"].get<float>();
            params.strum_gate_time_secs->setValueNotifyingHost(
                params.strum_gate_time_secs->convertTo0to1(value));
        }

        // strum_plate_cc
        if (json.contains("strum_plate_cc") && params.strum_plate_cc) {
            int value = json["strum_plate_cc"].get<int>();
            params.strum_plate_cc->setValueNotifyingHost(
                params.strum_plate_cc->convertTo0to1(static_cast<float>(value)));
        }

        // additional_settings - parse directly with generated code
        if (json.contains("additional_settings")) {
            std::string additionalJson = json["additional_settings"].dump();
            additionalSettings = GeneratedAdditionalSettings::fromJson(additionalJson);
            saveAdditionalSettingsToValueTree();
            pushVariantIndexesToValueTree();
        }

        DBG("OmnifyAudioProcessor: Loaded default settings from bundled JSON");
    } catch (const std::exception& e) {
        DBG("OmnifyAudioProcessor: Failed to load default settings: " << e.what());
    }
}

//==============================================================================
void OmnifyAudioProcessor::daemonReady() {
    DBG("OmnifyAudioProcessor: Daemon ready, sending settings");
    sendSettingsToDaemon();
}

void OmnifyAudioProcessor::sendSettingsToDaemon() {
    // Build JSON object matching DaemomnifySettings structure
    nlohmann::json settings;

    // midi_device_name comes from state property (set by MidiDeviceSelector UI)
    auto deviceName = magicState.getPropertyAsValue("midi_device_name").toString();
    settings["midi_device_name"] = deviceName.toStdString();

    // Channel params are 1-indexed choices (strings "1" through "16")
    if (params.chord_channel) {
        settings["chord_channel"] = params.chord_channel->getIndex() + 1;
    }
    if (params.strum_channel) {
        settings["strum_channel"] = params.strum_channel->getIndex() + 1;
    }

    // Float params
    if (params.strum_cooldown_secs) {
        settings["strum_cooldown_secs"] = params.strum_cooldown_secs->get();
    }
    if (params.strum_gate_time_secs) {
        settings["strum_gate_time_secs"] = params.strum_gate_time_secs->get();
    }

    // Int param
    if (params.strum_plate_cc) {
        settings["strum_plate_cc"] = params.strum_plate_cc->get();
    }

    // Additional settings as nested object
    auto additionalJson = nlohmann::json::parse(GeneratedAdditionalSettings::toJson(additionalSettings));
    settings["additional_settings"] = additionalJson;

    // Send as OSC message with JSON string
    std::string jsonStr = settings.dump();
    daemonManager.getOscSender().send("/settings", juce::String(jsonStr));

    DBG("OmnifyAudioProcessor: Sent settings: " << jsonStr);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
