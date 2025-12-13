#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "DaemonManager.h"
#include "GeneratedAdditionalSettings.h"
#include "GeneratedParams.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public foleys::MagicProcessor,
                             private juce::Value::Listener,
                             private DaemonManager::Listener {
   public:
    OmnifyAudioProcessor();
    ~OmnifyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void initialiseBuilder(foleys::MagicGUIBuilder& builder) override;

    void postSetStateInformation() override;

    // AdditionalSettings access
    const GeneratedAdditionalSettings::Settings& getAdditionalSettings() const { return additionalSettings; }

   private:
    GeneratedParams::Params params;
    juce::AudioProcessorValueTreeState parameters;

    // Complex settings stored as JSON in ValueTree
    GeneratedAdditionalSettings::Settings additionalSettings;

    // ValueTree property key for JSON storage
    static constexpr const char* ADDITIONAL_SETTINGS_KEY = "additional_settings_json";

    // Value listeners for variant selectors and file paths
    juce::Value chordVoicingStyleValue;
    juce::Value strumVoicingStyleValue;
    juce::Value chordVoicingFilePathValue;
    void valueChanged(juce::Value& value) override;
    void setupValueListeners();

    // Load/save additional settings from ValueTree
    void loadAdditionalSettingsFromValueTree();
    void saveAdditionalSettingsToValueTree();

    // Load default settings from bundled JSON (called on first run)
    void loadDefaultSettings();

    // Write variant indexes to ValueTree (for UI to read on rebuild)
    void pushVariantIndexesToValueTree();

    // DaemonManager::Listener
    void daemonReady() override;

    // Send all current settings to daemon via OSC
    void sendSettingsToDaemon();

    // Python daemon process manager
    DaemonManager daemonManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
