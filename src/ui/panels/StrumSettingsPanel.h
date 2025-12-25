#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../LcarsColors.h"
#include "../components/MidiLearnComponent.h"
#include "../components/VariantSelector.h"

// Forward declaration
class OmnifyAudioProcessor;

class StrumSettingsPanel : public juce::Component {
   public:
    explicit StrumSettingsPanel(OmnifyAudioProcessor& processor);
    ~StrumSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshFromSettings();

   private:
    void setupCallbacks();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Strum"};

    // MIDI Channel
    juce::Label channelLabel{"", "Midi Channel"};
    juce::ComboBox channelComboBox;

    // Voicing Style
    juce::Label voicingLabel{"", "Voicing"};
    VariantSelector voicingStyleSelector;
    std::vector<std::unique_ptr<juce::Component>> voicingStyleViews;
    std::vector<std::string> voicingStyleTypeNames;  // Maps UI index to registry type name

    // Strum Plate CC
    juce::Label strumPlateLabel{"", "Strum CC"};
    MidiLearnComponent strumPlateCcLearn;

    // Gate and Cooldown sliders
    juce::Slider gateSlider{juce::Slider::LinearBar, juce::Slider::NoTextBox};
    juce::Slider cooldownSlider{juce::Slider::LinearBar, juce::Slider::NoTextBox};
    juce::Label gateLabel{"", "Gate"};
    juce::Label cooldownLabel{"", "Cooldown"};

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cooldownAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrumSettingsPanel)
};
