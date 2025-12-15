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

   private:
    void setupValueBindings();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Strum Settings"};

    // MIDI Channel
    juce::Label channelLabel{"", "MIDI Channel"};
    juce::ComboBox channelComboBox;
    juce::Value strumChannelValue;

    // Voicing Style
    VariantSelector voicingStyleSelector;
    juce::Component plainAscendingView;  // Empty placeholder
    juce::Component omnichordView;       // Empty placeholder
    juce::Value strumVoicingStyleValue;

    // Strum Plate CC
    juce::Label strumPlateLabel{"", "Strum CC"};
    MidiLearnComponent strumPlateCcLearn;
    juce::Value strumPlateCcTypeValue;
    juce::Value strumPlateCcNumberValue;

    // Gate and Cooldown sliders
    juce::Slider gateSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::Slider cooldownSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::Label gateLabel{"", "Gate"};
    juce::Label cooldownLabel{"", "Cooldown"};

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cooldownAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrumSettingsPanel)
};
