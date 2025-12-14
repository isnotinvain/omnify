#pragma once

#include <array>
#include <mutex>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DaemonManager.h"
#include "GeneratedSettings.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public juce::AudioProcessor,
                             private juce::Value::Listener,
                             private juce::AudioProcessorValueTreeState::Listener,
                             private DaemonManager::Listener {
   public:
    OmnifyAudioProcessor();
    ~OmnifyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Settings access
    const GeneratedSettings::DaemomnifySettings& getSettings() const { return settings; }

    // State tree access for UI
    juce::ValueTree& getStateTree() { return stateTree; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

   private:
    // State tree for UI properties (replaces magicState.getValueTree())
    juce::ValueTree stateTree{"OmnifyState"};

    // Only 2 APVTS params - for DAW automation of realtime controls
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterFloat* strumGateTimeParam = nullptr;
    juce::AudioParameterFloat* strumCooldownParam = nullptr;

    // All settings stored in one flat struct
    GeneratedSettings::DaemomnifySettings settings;

    // ValueTree property key for JSON storage of full settings
    static constexpr const char* SETTINGS_JSON_KEY = "settings_json";

    // Value listeners for property-based settings
    juce::Value midiDeviceNameValue;
    juce::Value chordChannelValue;
    juce::Value strumChannelValue;
    juce::Value chordVoicingStyleValue;
    juce::Value strumVoicingStyleValue;
    juce::Value chordVoicingFilePathValue;

    // MidiLearn values (type is "note" or "cc", number is the midi note/cc number)
    juce::Value latchToggleButtonTypeValue;
    juce::Value latchToggleButtonNumberValue;
    juce::Value latchIsToggleValue;
    juce::Value stopButtonTypeValue;
    juce::Value stopButtonNumberValue;
    juce::Value strumPlateCcTypeValue;
    juce::Value strumPlateCcNumberValue;

    // Chord quality selector values (9 qualities, each with type and number)
    static constexpr size_t NUM_CHORD_QUALITIES = 9;
    std::array<juce::Value, NUM_CHORD_QUALITIES> chordQualityTypeValues;
    std::array<juce::Value, NUM_CHORD_QUALITIES> chordQualityNumberValues;

    // "One CC for All" chord quality selection (CCRangePerChordQuality variant)
    juce::Value chordQualityCcTypeValue;
    juce::Value chordQualityCcNumberValue;

    // Chord quality selection style variant index (0 = ButtonPerChordQuality, 1 = CCRangePerChordQuality)
    juce::Value chordQualitySelectionStyleValue;

    void valueChanged(juce::Value& value) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void setupValueListeners();

    // Load/save settings from ValueTree
    void loadSettingsFromValueTree();
    void saveSettingsToValueTree();

    // Load default settings from bundled JSON (called on first run)
    void loadDefaultSettings();

    // Write variant indexes to ValueTree (for UI to read on rebuild)
    void pushVariantIndexesToValueTree();

    // DaemonManager::Listener
    void daemonReady() override;

    // Send all current settings to daemon via OSC
    void sendSettingsToDaemon();

    // Try to send initial settings (called when either condition becomes true)
    void trySendInitialSettings();

    // Send realtime parameter updates via OSC
    void sendRealtimeParam(const juce::String& address, float value);

    // Python daemon process manager
    DaemonManager daemonManager;

    // Mutex-protected flags for thread-safe initialization sequencing
    std::mutex initMutex;
    bool daemonIsReady = false;
    bool settingsAreLoaded = false;
    bool initialSettingsSent = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
