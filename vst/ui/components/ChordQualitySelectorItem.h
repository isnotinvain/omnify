#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "../../GeneratedSettings.h"
#include "MidiLearnComponent.h"

class ChordQualitySelectorItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(ChordQualitySelectorItem)

    static const juce::Identifier pFontSize;
    static const juce::Identifier pLabelColor;
    static const juce::Identifier pMidiLearnWidth;
    static const juce::Identifier pRowSpacing;
    static const juce::Identifier pMidiLearnAspectRatio;

    static constexpr int NUM_QUALITIES = GeneratedSettings::ChordQualities::NUM_QUALITIES;

    ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    void resized() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

    MidiLearnedValue getLearnerValue(size_t qualityIndex) const;
    void setLearnerValue(size_t qualityIndex, MidiLearnedValue value);
    void setOnValueChanged(size_t qualityIndex, std::function<void(MidiLearnedValue)> callback);

   private:
    juce::Component container;

    struct Row {
        juce::Label label;
        MidiLearnComponent midiLearn;
    };
    std::array<Row, NUM_QUALITIES> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelectorItem)
};