#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../datamodel/VoicingType.h"
#include "../components/MidiLearnComponent.h"

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
    void updateVoicingDescription();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Strum"};

    // MIDI Channel
    juce::Label channelLabel{"", "Midi Channel"};
    juce::ComboBox channelComboBox;

    // Voicing Style
    juce::Label voicingLabel{"", "Voicing"};
    juce::ComboBox voicingStyleComboBox;
    juce::Label voicingDescriptionLabel;
    std::vector<StrumVoicingType> voicingStyleTypes;

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

    // Separator line position
    int separatorY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrumSettingsPanel)
};
