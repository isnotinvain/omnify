#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

// Factory macro for Container subclasses that need createSubComponents() called.
// Foleys only calls createSubComponents() automatically for View elements, not factory items.
#define DECLARE_CONTAINER_FACTORY(itemName)                                        \
    static inline std::unique_ptr<foleys::GuiItem> factory(                        \
        foleys::MagicGUIBuilder& builder, const juce::ValueTree& node) {           \
        auto item = std::make_unique<itemName>(builder, node);                     \
        item->createSubComponents();                                               \
        return item;                                                               \
    }

#include "components/ChordQualitySelectorItem.h"
#include "components/FilePickerItem.h"
#include "components/MidiDeviceSelectorItem.h"
#include "components/MidiLearnComponent.h"
#include "components/VariantSelectorItem.h"

class OmnifyAudioProcessor;

class MidiLearnItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(MidiLearnItem)

    static const juce::Identifier pAcceptMode;
    static const juce::Identifier pAspectRatio;
    static inline const juce::StringArray pAcceptModes{"Notes Only", "CCs Only", "Both"};

    MidiLearnItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

    MidiLearnComponent& getMidiLearnComponent() { return midiLearnComponent; }

   private:
    MidiLearnComponent midiLearnComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnItem)
};

class LcarsSettingsItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(LcarsSettingsItem)

    // Tab settings
    static const juce::Identifier pTabFontSize;
    static const juce::Identifier pTabActiveColor;
    static const juce::Identifier pTabInactiveColor;
    static const juce::Identifier pTabTextColor;
    static const juce::Identifier pTabUnderlineColor;

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