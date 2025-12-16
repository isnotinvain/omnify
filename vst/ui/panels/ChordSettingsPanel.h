#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../LcarsColors.h"
#include "../components/FromFileView.h"
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
    void refreshFromSettings();

   private:
    void setupCallbacks();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Chord Settings"};

    // MIDI Channel
    juce::Label channelLabel{"", "MIDI Channel"};
    juce::ComboBox channelComboBox;

    // Voicing Style
    VariantSelector voicingStyleSelector;
    std::vector<std::unique_ptr<juce::Component>> voicingStyleViews;
    std::vector<std::string> voicingStyleTypeNames;  // Maps UI index to registry type name
    FromFileView* fromFileChordView = nullptr;       // Non-owning pointer for updating path

    // Latch controls
    juce::Label latchLabel{"", "Latch On / Off"};
    MidiLearnComponent latchToggleLearn;
    juce::ToggleButton latchIsToggle{"Toggle Mode"};

    // Stop button
    juce::Label stopLabel{"", "Stop Chords"};
    MidiLearnComponent stopButtonLearn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSettingsPanel)
};
