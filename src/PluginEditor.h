#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/components/MidiIOPanel.h"
#include "ui/panels/ChordQualityPanel.h"
#include "ui/panels/ChordSettingsPanel.h"
#include "ui/panels/StrumSettingsPanel.h"

class OmnifyAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    explicit OmnifyAudioProcessorEditor(OmnifyAudioProcessor&);
    ~OmnifyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshFromSettings();

   private:
    OmnifyAudioProcessor& omnifyProcessor;

    // Top-level components
    juce::Label titleLabel;
    MidiIOPanel midiIOPanel;
    ChordSettingsPanel chordSettings;
    StrumSettingsPanel strumSettings;
    ChordQualityPanel chordQualityPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessorEditor)
};
