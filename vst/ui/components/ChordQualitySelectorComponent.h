#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

#include "../../GeneratedParams.h"
#include "MidiLearnComponent.h"

class ChordQualitySelectorComponent : public juce::Component {
   public:
    static constexpr int NUM_QUALITIES = GeneratedParams::ChordQualities::NUM_QUALITIES;

    ChordQualitySelectorComponent();

    MidiLearnedValue getLearnerValue(size_t qualityIndex) const;

    // repaint() is sufficient here - font size only affects paint(), not layout
    // if that changes in the future need to call resized() here too
    void setFontSize(float size) {
        fontSize = size;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

   private:
    float fontSize = 24.0f;
    std::array<std::unique_ptr<MidiLearnComponent>, NUM_QUALITIES> learners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelectorComponent)
};