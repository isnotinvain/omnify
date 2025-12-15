#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../LcarsColors.h"
#include "../components/FilePicker.h"
#include "../components/MidiLearnComponent.h"
#include "../components/VariantSelector.h"

// Forward declaration
class OmnifyAudioProcessor;

class ChordSettingsPanel : public juce::Component {
   public:
    explicit ChordSettingsPanel(OmnifyAudioProcessor& processor);
    ~ChordSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

   private:
    void setupValueBindings();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Chord Settings"};

    // MIDI Channel
    juce::Label channelLabel{"", "MIDI Channel"};
    juce::ComboBox channelComboBox;
    juce::Value chordChannelValue;

    // Voicing Style
    VariantSelector voicingStyleSelector;
    juce::Component rootPositionView;                                   // Empty placeholder for RootPositionStyle
    std::vector<std::unique_ptr<juce::Component>> bundledVoicingViews;  // Placeholders for BundledFileStyle
    FilePicker filePicker;                                              // For "From File" (FileStyle) variant
    juce::Component omni84View;                                         // Empty placeholder for Omni84Style
    juce::Value chordVoicingStyleValue;
    juce::Value chordVoicingFilePathValue;
    juce::Value bundledVoicingFilenameValue;

    // Latch controls
    juce::Label latchLabel{"", "Latch On / Off"};
    MidiLearnComponent latchToggleLearn;
    juce::ToggleButton latchIsToggle{"Toggle Mode"};
    juce::Value latchToggleTypeValue;
    juce::Value latchToggleNumberValue;
    juce::Value latchIsToggleValue;

    // Stop button
    juce::Label stopLabel{"", "Stop Chords"};
    MidiLearnComponent stopButtonLearn;
    juce::Value stopButtonTypeValue;
    juce::Value stopButtonNumberValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSettingsPanel)
};
