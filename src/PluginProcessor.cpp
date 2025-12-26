#include "PluginProcessor.h"

#include <nlohmann/json.hpp>

#include "BinaryData.h"
#include "PluginEditor.h"
#include "voicing_styles/FromFile.h"
#include "voicing_styles/Omni84.h"
#include "voicing_styles/OmnichordChords.h"
#include "voicing_styles/OmnichordStrum.h"
#include "voicing_styles/PlainAscending.h"
#include "voicing_styles/RootPosition.h"

namespace {
// Create the minimal APVTS layout with just 2 realtime params
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(juce::AudioParameterFloat*& strumGateTimeParam,
                                                                          juce::AudioParameterFloat*& strumCooldownParam) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto gateParam =
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("strum_gate_time_ms", 1), "Strum Gate Time", 0.0F, 2000.0F, 500.0F);
    strumGateTimeParam = gateParam.get();
    layout.add(std::move(gateParam));

    auto cooldownParam =
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("strum_cooldown_ms", 1), "Strum Cooldown", 0.0F, 2000.0F, 300.0F);
    strumCooldownParam = cooldownParam.get();
    layout.add(std::move(cooldownParam));

    return layout;
}
}  // namespace

void OmnifyAudioProcessor::initVoicingRegistries() {
    chordVoicingRegistry.registerStyle("RootPosition", std::make_shared<RootPosition>(), RootPosition::from_json);
    chordVoicingRegistry.registerStyle("FromFile", std::make_shared<FromFile<VoicingFor::Chord>>(""), FromFile<VoicingFor::Chord>::from_json);
    chordVoicingRegistry.registerStyle("Omnichord", std::make_shared<OmnichordChords>(), OmnichordChords::from_json);

    chordVoicingRegistry.registerStyle("Omni84", std::make_shared<Omni84>(), Omni84::from_json);

    strumVoicingRegistry.registerStyle("PlainAscending", std::make_shared<PlainAscending>(), PlainAscending::from_json);
    strumVoicingRegistry.registerStyle("Omnichord", std::make_shared<OmnichordStrum>(), OmnichordStrum::from_json);
}

OmnifyAudioProcessor::OmnifyAudioProcessor()
    : AudioProcessor(
          BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout(strumGateTimeParam, strumCooldownParam)) {
    juce::LookAndFeel::setDefaultLookAndFeel(&lcarsLookAndFeel);
    initVoicingRegistries();

    parameters.addParameterListener("strum_gate_time_ms", this);
    parameters.addParameterListener("strum_cooldown_ms", this);

    midiScheduler = std::make_unique<MidiMessageScheduler>();
    realtimeParams = std::make_shared<RealtimeParams>();
    omnifySettings = std::make_shared<OmnifySettings>();

    omnify = std::make_unique<Omnify>(*midiScheduler, omnifySettings, realtimeParams);
    daemomnify = std::make_unique<Daemomnify>(*omnify, *midiScheduler, "Omnify");
    daemomnify->start();
    startTimer(100);  // Check MIDI devices every 100ms

    loadDefaultSettings();
}

OmnifyAudioProcessor::~OmnifyAudioProcessor() {
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    stopTimer();

    if (daemomnify) {
        daemomnify->stop();
    }
    closeMidiLearnInput();

    parameters.removeParameterListener("strum_gate_time_ms", this);
    parameters.removeParameterListener("strum_cooldown_ms", this);
}

void OmnifyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) { juce::ignoreUnused(sampleRate, samplesPerBlock); }

void OmnifyAudioProcessor::releaseResources() {}

void OmnifyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(buffer, midiMessages);
}

juce::AudioProcessorEditor* OmnifyAudioProcessor::createEditor() { return new OmnifyAudioProcessorEditor(*this); }

void OmnifyAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // Sync realtime params back to settings before saving
    auto settings = std::atomic_load(&omnifySettings);
    settings->strumGateTimeMs = realtimeParams->strumGateTimeMs.load();
    settings->strumCooldownMs = realtimeParams->strumCooldownMs.load();

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
        auto apvtsState = combined.getChildWithName(parameters.state.getType());
        if (apvtsState.isValid()) {
            parameters.replaceState(apvtsState);
        }

        auto savedStateTree = combined.getChildWithName(stateTree.getType());
        if (savedStateTree.isValid()) {
            stateTree = savedStateTree.createCopy();
        }

        loadSettingsFromValueTree();

        // Tell editor to refresh if it exists
        if (auto* editor = dynamic_cast<OmnifyAudioProcessorEditor*>(getActiveEditor())) {
            editor->refreshFromSettings();
        }
    }
}

void OmnifyAudioProcessor::modifySettings(std::function<void(OmnifySettings&)> mutator) {
    auto newSettings = std::make_shared<OmnifySettings>(*omnifySettings);
    mutator(*newSettings);
    omnify->updateSettings(newSettings);
    std::atomic_store(&omnifySettings, newSettings);
    saveSettingsToValueTree();
}

void OmnifyAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == "strum_gate_time_ms") {
        realtimeParams->strumGateTimeMs.store(static_cast<int>(newValue));
    } else if (parameterID == "strum_cooldown_ms") {
        realtimeParams->strumCooldownMs.store(static_cast<int>(newValue));
    }
}

void OmnifyAudioProcessor::timerCallback() {
    if (daemomnify) {
        daemomnify->checkDevices();
    }
}

void OmnifyAudioProcessor::applySettingsFromJson(const juce::String& jsonString) {
    auto j = nlohmann::json::parse(jsonString.toStdString());
    auto newSettings = std::make_shared<OmnifySettings>(OmnifySettings::from_json(j, chordVoicingRegistry, strumVoicingRegistry));

    omnify->updateSettings(newSettings, true);
    std::atomic_store(&omnifySettings, newSettings);

    if (strumGateTimeParam) {
        strumGateTimeParam->setValueNotifyingHost(strumGateTimeParam->convertTo0to1(static_cast<float>(newSettings->strumGateTimeMs)));
    }
    if (strumCooldownParam) {
        strumCooldownParam->setValueNotifyingHost(strumCooldownParam->convertTo0to1(static_cast<float>(newSettings->strumCooldownMs)));
    }

    setMidiInputDevice(juce::String(newSettings->midiDeviceName));
}

void OmnifyAudioProcessor::loadSettingsFromValueTree() {
    auto jsonString = stateTree.getProperty(SETTINGS_JSON_KEY, "").toString();
    if (jsonString.isEmpty()) {
        return;
    }

    try {
        applySettingsFromJson(jsonString);
    } catch (const std::exception& e) {
        DBG("Failed to load settings from ValueTree: " << e.what());
    }
}

void OmnifyAudioProcessor::saveSettingsToValueTree() {
    auto settings = std::atomic_load(&omnifySettings);
    auto jsonString = settings->to_json().dump();
    stateTree.setProperty(SETTINGS_JSON_KEY, juce::String(jsonString), nullptr);
}

void OmnifyAudioProcessor::loadDefaultSettings() {
    try {
        juce::String jsonStr(BinaryData::default_settings_json, BinaryData::default_settings_jsonSize);
        applySettingsFromJson(jsonStr);
        saveSettingsToValueTree();
        DBG("Loaded default settings from bundled JSON");
    } catch (const std::exception& e) {
        DBG("Failed to load default settings: " << e.what());
    }
}

//==============================================================================
void OmnifyAudioProcessor::setMidiInputDevice(const juce::String& deviceName) {
    if (deviceName.isEmpty()) {
        daemomnify->setInputDevice(std::nullopt);
        closeMidiLearnInput();
        return;
    }

    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices) {
        if (device.name == deviceName) {
            daemomnify->setInputDevice(device.identifier);
            openMidiLearnInput(deviceName);
            return;
        }
    }

    daemomnify->setInputDevice(std::nullopt);
    closeMidiLearnInput();
}

void OmnifyAudioProcessor::openMidiLearnInput(const juce::String& deviceName) {
    // Close existing input first
    closeMidiLearnInput();

    if (deviceName.isEmpty()) {
        return;
    }

    // Find the device by name
    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices) {
        if (device.name == deviceName) {
            midiLearnInput = juce::MidiInput::openDevice(device.identifier, this);
            if (midiLearnInput) {
                midiLearnInput->start();
                logger->log("Opened MIDI input for MIDI Learn: " + deviceName);
            }
            return;
        }
    }

    logger->log("MIDI device not found for MIDI Learn: " + deviceName);
}

void OmnifyAudioProcessor::closeMidiLearnInput() {
    if (midiLearnInput) {
        midiLearnInput->stop();
        midiLearnInput.reset();
        logger->log("Closed MIDI Learn input");
    }
}

void OmnifyAudioProcessor::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
    MidiLearnComponent::broadcastMidi(message);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmnifyAudioProcessor(); }
