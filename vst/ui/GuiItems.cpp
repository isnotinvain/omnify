#include "GuiItems.h"

#include "../PluginProcessor.h"

const juce::Identifier MidiLearnItem::pAcceptMode{"accept-mode"};

MidiLearnItem::MidiLearnItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    addAndMakeVisible(midiLearnComponent);
}

void MidiLearnItem::update() {
    auto acceptModeText = magicBuilder.getStyleProperty(pAcceptMode, configNode).toString();
    if (acceptModeText == "Notes Only") {
        midiLearnComponent.setAcceptMode(MidiAcceptMode::NotesOnly);
    } else if (acceptModeText == "CCs Only") {
        midiLearnComponent.setAcceptMode(MidiAcceptMode::CCsOnly);
    } else {
        midiLearnComponent.setAcceptMode(MidiAcceptMode::Both);
    }
}

juce::Component* MidiLearnItem::getWrappedComponent() { return &midiLearnComponent; }

std::vector<foleys::SettableProperty> MidiLearnItem::getSettableProperties() const {
    std::vector<foleys::SettableProperty> props;
    props.push_back({configNode, pAcceptMode, foleys::SettableProperty::Choice, pAcceptModes[2],
                     magicBuilder.createChoicesMenuLambda(pAcceptModes)});
    return props;
}

const juce::Identifier ChordQualitySelectorItem::pFontSize{"font-size"};

ChordQualitySelectorItem::ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder,
                                                   const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    addAndMakeVisible(chordQualitySelector);
}

void ChordQualitySelectorItem::update() {
    auto fontSizeVar = magicBuilder.getStyleProperty(pFontSize, configNode);
    if (!fontSizeVar.isVoid()) {
        chordQualitySelector.setFontSize(static_cast<float>(fontSizeVar));
    }
}

juce::Component* ChordQualitySelectorItem::getWrappedComponent() { return &chordQualitySelector; }

std::vector<foleys::SettableProperty> ChordQualitySelectorItem::getSettableProperties() const {
    std::vector<foleys::SettableProperty> props;
    props.push_back({configNode, pFontSize, foleys::SettableProperty::Number, 24.0f, {}});
    return props;
}

// LcarsSettingsItem implementation
// Tab settings
const juce::Identifier LcarsSettingsItem::pTabFontSize{"tab-font-size"};
const juce::Identifier LcarsSettingsItem::pTabActiveColor{"tab-active-color"};
const juce::Identifier LcarsSettingsItem::pTabInactiveColor{"tab-inactive-color"};
const juce::Identifier LcarsSettingsItem::pTabTextColor{"tab-text-color"};
const juce::Identifier LcarsSettingsItem::pTabUnderlineColor{"tab-underline-color"};

// Font size multipliers
const juce::Identifier LcarsSettingsItem::pComboboxLabelFontMultiplier{
    "combobox-label-font-multiplier"};
const juce::Identifier LcarsSettingsItem::pComboboxFontMultiplier{"combobox-font-multiplier"};
const juce::Identifier LcarsSettingsItem::pTextButtonFontMultiplier{"text-button-font-multiplier"};
const juce::Identifier LcarsSettingsItem::pTabButtonFontMultiplier{"tab-button-font-multiplier"};
const juce::Identifier LcarsSettingsItem::pPopupMenuFontSize{"popup-menu-font-size"};
const juce::Identifier LcarsSettingsItem::pPopupMenuItemFontMultiplier{
    "popup-menu-item-font-multiplier"};
const juce::Identifier LcarsSettingsItem::pPopupMenuItemHeightMultiplier{
    "popup-menu-item-height-multiplier"};

// ComboBox drawing
const juce::Identifier LcarsSettingsItem::pComboboxBorderRadius{"combobox-border-radius"};
const juce::Identifier LcarsSettingsItem::pComboboxArrowSize{"combobox-arrow-size"};
const juce::Identifier LcarsSettingsItem::pComboboxArrowPadding{"combobox-arrow-padding"};

// Other
const juce::Identifier LcarsSettingsItem::pPopupMenuBorderSize{"popup-menu-border-size"};

LcarsSettingsItem::LcarsSettingsItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    placeholder.setVisible(false);
    addAndMakeVisible(placeholder);
    configNode.setProperty(foleys::IDs::minWidth, 0, nullptr);
    configNode.setProperty(foleys::IDs::maxWidth, 0, nullptr);

    setColourTranslation({{"tab-active-color", 0},
                          {"tab-inactive-color", 1},
                          {"tab-text-color", 2},
                          {"tab-underline-color", 3}});
}

void LcarsSettingsItem::update() {
    // When settings change, repaint and relayout the entire GUI
    if (auto* topLevel = getTopLevelComponent()) {
        topLevel->resized();
        topLevel->repaint();
    }
}

juce::Component* LcarsSettingsItem::getWrappedComponent() { return &placeholder; }

std::vector<foleys::SettableProperty> LcarsSettingsItem::getSettableProperties() const {
    std::vector<foleys::SettableProperty> props;

    // Tab settings
    props.push_back({configNode, pTabFontSize, foleys::SettableProperty::Number, 14.0f, {}});

    // Font size multipliers
    props.push_back(
        {configNode, pComboboxLabelFontMultiplier, foleys::SettableProperty::Number, 0.8f, {}});
    props.push_back(
        {configNode, pComboboxFontMultiplier, foleys::SettableProperty::Number, 0.85f, {}});
    props.push_back(
        {configNode, pTextButtonFontMultiplier, foleys::SettableProperty::Number, 0.6f, {}});
    props.push_back(
        {configNode, pTabButtonFontMultiplier, foleys::SettableProperty::Number, 0.6f, {}});
    props.push_back({configNode, pPopupMenuFontSize, foleys::SettableProperty::Number, 15.0f, {}});
    props.push_back(
        {configNode, pPopupMenuItemFontMultiplier, foleys::SettableProperty::Number, 0.8f, {}});
    props.push_back(
        {configNode, pPopupMenuItemHeightMultiplier, foleys::SettableProperty::Number, 1.3f, {}});

    // ComboBox drawing
    props.push_back(
        {configNode, pComboboxBorderRadius, foleys::SettableProperty::Number, 4.0f, {}});
    props.push_back({configNode, pComboboxArrowSize, foleys::SettableProperty::Number, 6.0f, {}});
    props.push_back(
        {configNode, pComboboxArrowPadding, foleys::SettableProperty::Number, 8.0f, {}});

    // Other
    props.push_back({configNode, pPopupMenuBorderSize, foleys::SettableProperty::Number, 2.0f, {}});

    return props;
}