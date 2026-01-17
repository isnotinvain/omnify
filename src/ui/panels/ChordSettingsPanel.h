#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../datamodel/VoicingType.h"
#include "../components/MidiLearnComponent.h"

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
    void updateVoicingDescription();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Chords"};

    // MIDI Channel
    juce::Label channelLabel{"", "Midi Channel"};
    juce::ComboBox channelComboBox;

    // Voicing Style
    juce::Label voicingLabel{"", "Voicing"};
    juce::ComboBox voicingStyleComboBox;
    juce::Label voicingDescriptionLabel;
    std::vector<ChordVoicingType> voicingStyleTypes;

    // Voicing Modifier
    juce::Label voicingModifierLabel{"", "Modifier"};
    juce::TextButton voicingModifierButton;

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
