#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../GeneratedSettings.h"
#include "../LcarsColors.h"
#include "MidiLearnComponent.h"

// Grid of chord quality labels + MIDI learn buttons.
// Each row has a label (e.g., "Major") and a MidiLearnComponent.
class ChordQualitySelector : public juce::Component, private juce::Value::Listener {
   public:
    static constexpr int NUM_QUALITIES = GeneratedSettings::ChordQualities::NUM_QUALITIES;

    ChordQualitySelector();
    ~ChordQualitySelector() override;

    // Bind to a ValueTree for persistence
    // Uses properties: chord_quality_MAJOR_type, chord_quality_MAJOR_number, etc.
    void bindToValueTree(juce::ValueTree& tree);

    // Styling
    void setFontSize(float size);
    void setLabelColor(juce::Colour color);
    void setMidiLearnAspectRatio(float ratio);

    void resized() override;

   private:
    void valueChanged(juce::Value& value) override;
    void updateComponentsFromValues();
    void onMidiLearnChanged(size_t qualityIndex, MidiLearnedValue val);

    struct Row {
        juce::Label label;
        MidiLearnComponent midiLearn;
        juce::Value typeValue;
        juce::Value numberValue;
    };
    std::array<Row, NUM_QUALITIES> rows;

    float fontSize = 14.0f;
    juce::Colour labelColor = LcarsColors::orange;
    float midiLearnAspectRatio = 2.0f;
    int rowSpacing = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelector)
};
