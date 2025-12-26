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
    juce::Label titleLabel{"", "Chords"};

    // MIDI Channel
    juce::Label channelLabel{"", "Midi Channel"};
    juce::ComboBox channelComboBox;

    // Voicing Style
    juce::Label voicingLabel{"", "Voicing"};
    VariantSelector voicingStyleSelector;
    std::vector<std::unique_ptr<juce::Component>> voicingStyleViews;
    std::vector<std::string> voicingStyleTypeNames;  // Maps UI index to registry type name
    FromFileView* fromFileChordView = nullptr;       // Non-owning pointer for updating path

    // Latch controls
    juce::Label latchLabel{"", "Latch"};
    MidiLearnComponent latchToggleLearn;
    juce::Label toggleLabel{"", "Latch Mode"};
    juce::ToggleButton latchIsToggle;

    // Stop button
    juce::Label stopLabel{"", "Stop All"};
    MidiLearnComponent stopButtonLearn;

    // Separator line position
    int separatorY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSettingsPanel)
};
