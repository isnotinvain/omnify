#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

//==============================================================================
class OmnifyAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    explicit OmnifyAudioProcessorEditor(OmnifyAudioProcessor&);
    ~OmnifyAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

   private:
    OmnifyAudioProcessor& processorRef;

    // UI Components
    juce::ComboBox chordVoicingCombo;
    juce::ComboBox strumVoicingCombo;

    juce::Label titleLabel;
    juce::Label chordVoicingLabel;
    juce::Label strumVoicingLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordVoicingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> strumVoicingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessorEditor)
};