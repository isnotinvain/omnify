#include "ChordSettingsPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/MidiButton.h"
#include "../../datamodel/OmnifySettings.h"
#include "../../datamodel/VoicingModifier.h"
#include "../LcarsLookAndFeel.h"

ChordSettingsPanel::ChordSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title - font will be set in resized() after LookAndFeel is available
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::red);
    addAndMakeVisible(titleLabel);

    // MIDI Channel
    channelLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    channelLabel.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

    // Voicing Style
    voicingLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(voicingLabel);

    // Voicing Style ComboBox - iterate voicings map to build options
    int itemId = 1;
    for (const auto& [type, style] : chordVoicings()) {
        voicingStyleComboBox.addItem(style->displayName(), itemId++);
        voicingStyleTypes.push_back(type);
    }
    addAndMakeVisible(voicingStyleComboBox);

    // Description label for selected voicing (matches VariantSelector styling)
    voicingDescriptionLabel.setColour(juce::Label::textColourId, LcarsColors::red);
    voicingDescriptionLabel.setJustificationType(juce::Justification::topLeft);
    voicingDescriptionLabel.setMinimumHorizontalScale(1.0f);  // Don't shrink text
    addAndMakeVisible(voicingDescriptionLabel);

    // Voicing Modifier
    voicingModifierLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    voicingModifierLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(voicingModifierLabel);

    voicingModifierButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    voicingModifierButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::black);
    voicingModifierButton.setColour(juce::TextButton::textColourOffId, LcarsColors::orange);
    voicingModifierButton.setColour(juce::TextButton::textColourOnId, LcarsColors::orange);
    voicingModifierButton.setColour(juce::ComboBox::outlineColourId, LcarsColors::orange);
    addAndMakeVisible(voicingModifierButton);

    // Latch controls
    latchLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    latchLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(latchLabel);
    addAndMakeVisible(latchToggleLearn);

    toggleLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    toggleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(toggleLabel);
    latchIsToggle.setColour(juce::ToggleButton::tickColourId, LcarsColors::orange);
    latchIsToggle.getProperties().set("onText", "Toggle");
    latchIsToggle.getProperties().set("offText", "Momentary");
    addAndMakeVisible(latchIsToggle);

    // Stop button
    stopLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    stopLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(stopLabel);
    addAndMakeVisible(stopButtonLearn);

    setupCallbacks();
    refreshFromSettings();
}

ChordSettingsPanel::~ChordSettingsPanel() = default;

void ChordSettingsPanel::setupCallbacks() {
    // MIDI Channel
    channelComboBox.onChange = [this]() {
        processor.modifySettings([channel = channelComboBox.getSelectedId()](OmnifySettings& s) { s.chordChannel = channel; });
    };

    // Voicing Style selector
    voicingStyleComboBox.onChange = [this]() {
        int index = voicingStyleComboBox.getSelectedItemIndex();
        if (index >= 0 && index < static_cast<int>(voicingStyleTypes.size())) {
            auto type = voicingStyleTypes[static_cast<size_t>(index)];
            const auto* style = chordVoicings().at(type);
            processor.modifySettings([style](OmnifySettings& s) { s.chordVoicingStyle = style; });
            updateVoicingDescription();
        }
    };

    // Voicing Modifier - cycles through None -> Fixed -> Smooth -> None
    voicingModifierButton.onClick = [this]() {
        auto settings = processor.getSettings();
        VoicingModifier next;
        switch (settings->voicingModifier) {
            case VoicingModifier::NONE:
                next = VoicingModifier::FIXED;
                break;
            case VoicingModifier::FIXED:
                next = VoicingModifier::SMOOTH;
                break;
            case VoicingModifier::SMOOTH:
                next = VoicingModifier::NONE;
                break;
        }
        processor.modifySettings([next](OmnifySettings& s) { s.voicingModifier = next; });
        refreshFromSettings();
    };

    // Latch button MIDI learn
    latchToggleLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val, isToggle = latchIsToggle.getToggleState()](OmnifySettings& s) {
            if (val.type == MidiLearnedType::Note) {
                s.latchButton = MidiButton::fromNote(val.value);
            } else if (val.type == MidiLearnedType::CC) {
                s.latchButton = MidiButton::fromCC(val.value, isToggle);
            } else {
                s.latchButton = MidiButton{};
            }
        });
    };

    // Latch toggle mode
    latchIsToggle.onClick = [this]() {
        processor.modifySettings([isToggle = latchIsToggle.getToggleState()](OmnifySettings& s) { s.latchButton.ccIsToggle = isToggle; });
    };

    // Stop button MIDI learn
    stopButtonLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val](OmnifySettings& s) {
            if (val.type == MidiLearnedType::Note) {
                s.stopButton = MidiButton::fromNote(val.value);
            } else if (val.type == MidiLearnedType::CC) {
                s.stopButton = MidiButton::fromCC(val.value, false);
            } else {
                s.stopButton = MidiButton{};
            }
        });
    };
}

void ChordSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), LcarsLookAndFeel::borderRadius, 1.0F);

    // Separator line between Midi Channel and Voicing
    g.setColour(LcarsColors::africanViolet);
    g.drawHorizontalLine(separatorY, 10.0F, static_cast<float>(getWidth() - 10));
}

void ChordSettingsPanel::refreshFromSettings() {
    auto settings = processor.getSettings();

    // MIDI Channel
    channelComboBox.setSelectedId(settings->chordChannel, juce::dontSendNotification);

    // Latch button
    const auto& latch = settings->latchButton;
    MidiLearnedValue latchVal;
    if (latch.note >= 0) {
        latchVal.type = MidiLearnedType::Note;
        latchVal.value = latch.note;
    } else if (latch.cc >= 0) {
        latchVal.type = MidiLearnedType::CC;
        latchVal.value = latch.cc;
    }
    latchToggleLearn.setLearnedValue(latchVal);
    latchIsToggle.setToggleState(latch.ccIsToggle, juce::dontSendNotification);

    // Stop button
    const auto& stop = settings->stopButton;
    MidiLearnedValue stopVal;
    if (stop.note >= 0) {
        stopVal.type = MidiLearnedType::Note;
        stopVal.value = stop.note;
    } else if (stop.cc >= 0) {
        stopVal.type = MidiLearnedType::CC;
        stopVal.value = stop.cc;
    }
    stopButtonLearn.setLearnedValue(stopVal);

    // Voicing Modifier
    switch (settings->voicingModifier) {
        case VoicingModifier::NONE:
            voicingModifierButton.setButtonText("None");
            break;
        case VoicingModifier::FIXED:
            voicingModifierButton.setButtonText("Fixed");
            break;
        case VoicingModifier::SMOOTH:
            voicingModifierButton.setButtonText("Smooth");
            break;
    }

    // Voicing style selector - find matching index
    if (settings->chordVoicingStyle) {
        auto currentType = chordVoicingTypeFor(settings->chordVoicingStyle);
        for (size_t i = 0; i < voicingStyleTypes.size(); ++i) {
            if (voicingStyleTypes[i] == currentType) {
                voicingStyleComboBox.setSelectedItemIndex(static_cast<int>(i), juce::dontSendNotification);
                break;
            }
        }
    }
    updateVoicingDescription();
}

void ChordSettingsPanel::updateVoicingDescription() {
    int index = voicingStyleComboBox.getSelectedItemIndex();
    if (index >= 0 && index < static_cast<int>(voicingStyleTypes.size())) {
        auto type = voicingStyleTypes[static_cast<size_t>(index)];
        const auto* style = chordVoicings().at(type);
        voicingDescriptionLabel.setText(style->description(), juce::dontSendNotification);
    }
}

void ChordSettingsPanel::resized() {
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    // Title
    float titleHeightF = LcarsLookAndFeel::fontSizeLarge + 10.0F;
    fb.items.add(juce::FlexItem(titleLabel).withHeight(titleHeightF).withMargin(4));

    fb.items.add(juce::FlexItem().withHeight(35.0F).withMargin(4));

    // Latch controls
    fb.items.add(juce::FlexItem(latchLabel).withHeight(20.0F).withMargin(juce::FlexItem::Margin(8, 4, 0, 4)));

    fb.items.add(juce::FlexItem().withHeight(40.0F).withMargin(4));

    // Stop button
    fb.items.add(juce::FlexItem(stopLabel).withHeight(20.0F).withMargin(juce::FlexItem::Margin(8, 4, 0, 4)));
    fb.items.add(juce::FlexItem(stopButtonLearn).withHeight(40.0F).withMargin(4));

    fb.performLayout(getLocalBounds().reduced(6));

    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
        channelLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingDescriptionLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny));
        latchLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        toggleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        stopLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingModifierLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
    }

    // Manual layout for nested rows since FlexBox doesn't nest well in performLayout
    auto bounds = getLocalBounds().reduced(10, 2);

    int titleHeight = static_cast<int>(LcarsLookAndFeel::fontSizeLarge) + 10;
    titleLabel.setBounds(bounds.removeFromTop(titleHeight));
    bounds.removeFromTop(4);

    LcarsLookAndFeel::setComboBoxFontSize(channelComboBox, LcarsLookAndFeel::fontSizeSmall);
    auto channelRowBounds = bounds.removeFromTop(30);
    channelLabel.setBounds(channelRowBounds.removeFromLeft(channelRowBounds.getWidth() - 80));
    channelComboBox.setBounds(channelRowBounds);
    bounds.removeFromTop(4);

    separatorY = bounds.getY();
    bounds.removeFromTop(5);

    voicingLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(4);

    // Bottom section: 3 rows aligned to bottom (remove these first so selector gets remaining space)
    // Stop row: label on left, midi learn on right
    auto stopRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    stopButtonLearn.setBounds(stopRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    stopLabel.setBounds(stopRowBounds);
    bounds.removeFromBottom(4);

    // Toggle row: label on left, checkbox on right
    auto toggleRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    latchIsToggle.setBounds(toggleRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    toggleLabel.setBounds(toggleRowBounds);
    bounds.removeFromBottom(4);

    // Latch row: label on left, midi learn on right
    auto latchRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    latchToggleLearn.setBounds(latchRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    latchLabel.setBounds(latchRowBounds);
    bounds.removeFromBottom(4);

    // Voicing Modifier row: label on left, button on right
    auto modifierRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    voicingModifierButton.setBounds(modifierRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    voicingModifierLabel.setBounds(modifierRowBounds);
    bounds.removeFromBottom(4);

    // Voicing combo box at top of remaining space (matches VariantSelector layout)
    LcarsLookAndFeel::setComboBoxFontSize(voicingStyleComboBox, LcarsLookAndFeel::fontSizeSmall);
    voicingStyleComboBox.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(8);  // Padding between combo box and description

    // Description gets remaining middle space
    voicingDescriptionLabel.setBounds(bounds);
}
