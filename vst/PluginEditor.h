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
    juce::Slider gainSlider;
    juce::Slider mixSlider;
    juce::ToggleButton bypassButton;

    juce::Label gainLabel;
    juce::Label mixLabel;
    juce::Label titleLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessorEditor)
};