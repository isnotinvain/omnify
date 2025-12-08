#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "components/ChordQualitySelectorComponent.h"
#include "components/MidiLearnComponent.h"

class OmnifyAudioProcessor;

class MidiLearnItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(MidiLearnItem)

    static const juce::Identifier pAcceptMode;
    static inline const juce::StringArray pAcceptModes{"Notes Only", "CCs Only", "Both"};

    MidiLearnItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

   private:
    MidiLearnComponent midiLearnComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnItem)
};

class ChordQualitySelectorItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(ChordQualitySelectorItem)

    static const juce::Identifier pFontSize;

    ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

   private:
    ChordQualitySelectorComponent chordQualitySelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualitySelectorItem)
};