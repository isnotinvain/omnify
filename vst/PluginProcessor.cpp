#include "PluginProcessor.h"

#include <nlohmann/json.hpp>

#include "BinaryData.h"
#include "ui/GuiItems.h"
#include "ui/LcarsLookAndFeel.h"

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
    : foleys::MagicProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS",
                 createParameterLayout(strumGateTimeParam, strumCooldownParam)),
      settings(GeneratedSettings::DaemomnifySettings::defaults()) {
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
    strumPlateCcValue.removeListener(this);
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
    loadSettingsFromValueTree();
    pushVariantIndexesToValueTree();
}

//==============================================================================
void OmnifyAudioProcessor::setupValueListeners() {
    // Bind to ValueTree properties for non-realtime settings
    midiDeviceNameValue.referTo(magicState.getPropertyAsValue("midi_device_name"));
    chordChannelValue.referTo(magicState.getValueTree().getPropertyAsValue("chord_channel", nullptr));
    strumChannelValue.referTo(magicState.getValueTree().getPropertyAsValue("strum_channel", nullptr));
    strumPlateCcValue.referTo(magicState.getValueTree().getPropertyAsValue("strum_plate_cc", nullptr));
    chordVoicingStyleValue.referTo(
        magicState.getValueTree().getPropertyAsValue("variant_chord_voicing_style", nullptr));
    strumVoicingStyleValue.referTo(
        magicState.getValueTree().getPropertyAsValue("variant_strum_voicing_style", nullptr));
    chordVoicingFilePathValue.referTo(magicState.getPropertyAsValue("chords_json_file"));

    midiDeviceNameValue.addListener(this);
    chordChannelValue.addListener(this);
    strumChannelValue.addListener(this);
    strumPlateCcValue.addListener(this);
    chordVoicingStyleValue.addListener(this);
    strumVoicingStyleValue.addListener(this);
    chordVoicingFilePathValue.addListener(this);
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
    } else if (value.refersToSameSourceAs(strumPlateCcValue)) {
        settings.strum_plate_cc = static_cast<int>(strumPlateCcValue.getValue());
        settingsChanged = true;
    } else if (value.refersToSameSourceAs(chordVoicingStyleValue)) {
        int index = static_cast<int>(chordVoicingStyleValue.getValue());
        switch (index) {
            case 0:
                settings.chord_voicing_style = GeneratedSettings::RootPositionStyle{};
                break;
            case 1: {
                GeneratedSettings::FileStyle fs;
                fs.path = magicState.getPropertyAsValue("chords_json_file").toString().toStdString();
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
    }

    if (settingsChanged) {
        saveSettingsToValueTree();
        sendSettingsToDaemon();
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
    magicState.getValueTree().setProperty("variant_chord_voicing_style",
                                          static_cast<int>(settings.chord_voicing_style.index()), nullptr);
    magicState.getValueTree().setProperty("variant_strum_voicing_style",
                                          static_cast<int>(settings.strum_voicing_style.index()), nullptr);

    // Write file path from FileStyle if that's the active variant
    if (auto* fileStyle = std::get_if<GeneratedSettings::FileStyle>(&settings.chord_voicing_style)) {
        magicState.getPropertyAsValue("chords_json_file").setValue(juce::String(fileStyle->path));
    }

    // Write scalar settings to ValueTree properties
    magicState.getValueTree().setProperty("chord_channel", settings.chord_channel, nullptr);
    magicState.getValueTree().setProperty("strum_channel", settings.strum_channel, nullptr);
    magicState.getValueTree().setProperty("strum_plate_cc", settings.strum_plate_cc, nullptr);
    magicState.getPropertyAsValue("midi_device_name").setValue(juce::String(settings.midi_device_name));
}

void OmnifyAudioProcessor::loadSettingsFromValueTree() {
    auto jsonString = magicState.getValueTree().getProperty(SETTINGS_JSON_KEY, "").toString();
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
    magicState.getValueTree().setProperty(SETTINGS_JSON_KEY, juce::String(jsonString), nullptr);
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
    DBG("OmnifyAudioProcessor: Daemon ready, sending settings");
    sendSettingsToDaemon();
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
