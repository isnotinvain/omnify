#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "GeneratedAdditionalSettings.h"
#include "GeneratedParams.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public foleys::MagicProcessor {
   public:
    OmnifyAudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void initialiseBuilder(foleys::MagicGUIBuilder& builder) override;
    juce::AudioProcessorEditor* createEditor() override;

    // AdditionalSettings access
    const GeneratedAdditionalSettings::Settings& getAdditionalSettings() const { return additionalSettings; }
    void setAdditionalSettings(const GeneratedAdditionalSettings::Settings& settings);

   private:
    void wireUpSettingsBindings(foleys::MagicGUIBuilder& builder);
    GeneratedParams::Params params;
    juce::AudioProcessorValueTreeState parameters;

    // Complex settings stored as JSON in ValueTree
    GeneratedAdditionalSettings::Settings additionalSettings;

    // ValueTree property key for JSON storage
    static constexpr const char* ADDITIONAL_SETTINGS_KEY = "additional_settings_json";

    // Load/save additional settings from ValueTree
    void loadAdditionalSettingsFromValueTree();
    void saveAdditionalSettingsToValueTree();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
