#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "../../GeneratedAdditionalSettings.h"
#include "MidiLearnComponent.h"

class ChordQualitySelectorItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(ChordQualitySelectorItem)

    static const juce::Identifier pFontSize;
    static const juce::Identifier pLabelColor;
    static const juce::Identifier pMidiLearnWidth;
    static const juce::Identifier pRowSpacing;

    static constexpr int NUM_QUALITIES = GeneratedAdditionalSettings::ChordQualities::NUM_QUALITIES;

    ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    void resized() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

    MidiLearnedValue getLearnerValue(size_t qualityIndex) const;

   private:
    juce::Component container;

    struct Row {
        juce::Label label;
        MidiLearnComponent midiLearn;
    };
    std::array<Row, NUM_QUALITIES> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelectorItem)
};