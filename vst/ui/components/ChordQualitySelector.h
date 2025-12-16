#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "../../datamodel/ChordQuality.h"
#include "../LcarsColors.h"
#include "MidiLearnComponent.h"

// Grid of chord quality labels + MIDI learn buttons.
// Each row has a label (e.g., "Major") and a MidiLearnComponent.
class ChordQualitySelector : public juce::Component {
   public:
    static constexpr size_t NUM_QUALITIES = ALL_CHORD_QUALITIES.size();

    ChordQualitySelector();
    ~ChordQualitySelector() override = default;

    // Callback when a MIDI learn value changes
    std::function<void(ChordQuality quality, MidiLearnedValue val)> onQualityMidiChanged;

    // Refresh the displayed values from external data
    void setMidiMapping(ChordQuality quality, MidiLearnedValue val);

    // Styling
    void setLabelColor(juce::Colour color);
    void setMidiLearnAspectRatio(float ratio);

    void resized() override;

   private:
    struct Row {
        juce::Label label;
        MidiLearnComponent midiLearn;
    };
    std::array<Row, NUM_QUALITIES> rows;

    juce::Colour labelColor = LcarsColors::orange;
    float midiLearnAspectRatio = 2.0f;
    int rowSpacing = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelector)
};
