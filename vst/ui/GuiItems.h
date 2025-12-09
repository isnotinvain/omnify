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

class LcarsSettingsItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(LcarsSettingsItem)

    // Tab settings
    static const juce::Identifier pTabFontSize;
    static const juce::Identifier pTabActiveColor;
    static const juce::Identifier pTabInactiveColor;
    static const juce::Identifier pTabTextColor;

    // Font size multipliers
    static const juce::Identifier pComboboxLabelFontMultiplier;
    static const juce::Identifier pComboboxFontMultiplier;
    static const juce::Identifier pTextButtonFontMultiplier;
    static const juce::Identifier pTabButtonFontMultiplier;
    static const juce::Identifier pPopupMenuFontSize;
    static const juce::Identifier pPopupMenuItemFontMultiplier;
    static const juce::Identifier pPopupMenuItemHeightMultiplier;

    // ComboBox drawing
    static const juce::Identifier pComboboxBorderRadius;
    static const juce::Identifier pComboboxArrowSize;
    static const juce::Identifier pComboboxArrowPadding;

    // Other
    static const juce::Identifier pPopupMenuBorderSize;

    LcarsSettingsItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

   private:
    juce::Component placeholder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LcarsSettingsItem)
};