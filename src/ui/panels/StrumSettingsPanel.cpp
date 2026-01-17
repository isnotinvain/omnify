#include "StrumSettingsPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/OmnifySettings.h"
#include "../LcarsLookAndFeel.h"

StrumSettingsPanel::StrumSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
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
    for (const auto& [type, style] : strumVoicings()) {
        voicingStyleComboBox.addItem(style->displayName(), itemId++);
        voicingStyleTypes.push_back(type);
    }
    addAndMakeVisible(voicingStyleComboBox);

    // Description label for selected voicing (matches VariantSelector styling)
    voicingDescriptionLabel.setColour(juce::Label::textColourId, LcarsColors::red);
    voicingDescriptionLabel.setJustificationType(juce::Justification::topLeft);
    voicingDescriptionLabel.setMinimumHorizontalScale(1.0f);  // Don't shrink text
    addAndMakeVisible(voicingDescriptionLabel);

    // Strum Plate CC
    strumPlateLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    strumPlateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(strumPlateLabel);

    strumPlateCcLearn.setAcceptMode(MidiAcceptMode::CCsOnly);
    addAndMakeVisible(strumPlateCcLearn);

    // Sliders
    gateLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    gateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gateLabel);
    addAndMakeVisible(gateSlider);

    cooldownLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    cooldownLabel.setJustificationType(juce::Justification::centredLeft);
    cooldownLabel.setMinimumHorizontalScale(1.0F);
    addAndMakeVisible(cooldownLabel);
    addAndMakeVisible(cooldownSlider);

    setupCallbacks();
    refreshFromSettings();
}

StrumSettingsPanel::~StrumSettingsPanel() = default;

void StrumSettingsPanel::setupCallbacks() {
    auto& apvts = processor.getAPVTS();

    // MIDI Channel
    channelComboBox.onChange = [this]() {
        processor.modifySettings([channel = channelComboBox.getSelectedId()](OmnifySettings& s) { s.strumChannel = channel; });
    };

    // Voicing Style selector
    voicingStyleComboBox.onChange = [this]() {
        int index = voicingStyleComboBox.getSelectedItemIndex();
        if (index >= 0 && index < static_cast<int>(voicingStyleTypes.size())) {
            auto type = voicingStyleTypes[static_cast<size_t>(index)];
            const auto* style = strumVoicings().at(type);
            processor.modifySettings([style](OmnifySettings& s) { s.strumVoicingStyle = style; });
            updateVoicingDescription();
        }
    };

    // Strum Plate CC MIDI learn
    strumPlateCcLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val](OmnifySettings& s) {
            if (val.type == MidiLearnedType::CC) {
                s.strumPlateCC = val.value;
            } else {
                s.strumPlateCC = -1;
            }
        });
    };

    // APVTS slider attachments (these remain as APVTS for real-time automation)
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "strum_gate_time_ms", gateSlider);
    cooldownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "strum_cooldown_ms", cooldownSlider);
}

void StrumSettingsPanel::refreshFromSettings() {
    auto settings = processor.getSettings();

    // MIDI Channel
    channelComboBox.setSelectedId(settings->strumChannel, juce::dontSendNotification);

    // Strum Plate CC
    MidiLearnedValue strumVal;
    if (settings->strumPlateCC >= 0) {
        strumVal.type = MidiLearnedType::CC;
        strumVal.value = settings->strumPlateCC;
    }
    strumPlateCcLearn.setLearnedValue(strumVal);

    // Voicing style selector - find matching index
    if (settings->strumVoicingStyle) {
        auto currentType = strumVoicingTypeFor(settings->strumVoicingStyle);
        for (size_t i = 0; i < voicingStyleTypes.size(); ++i) {
            if (voicingStyleTypes[i] == currentType) {
                voicingStyleComboBox.setSelectedItemIndex(static_cast<int>(i), juce::dontSendNotification);
                break;
            }
        }
    }
    updateVoicingDescription();
}

void StrumSettingsPanel::updateVoicingDescription() {
    int index = voicingStyleComboBox.getSelectedItemIndex();
    if (index >= 0 && index < static_cast<int>(voicingStyleTypes.size())) {
        auto type = voicingStyleTypes[static_cast<size_t>(index)];
        const auto* style = strumVoicings().at(type);
        voicingDescriptionLabel.setText(style->description(), juce::dontSendNotification);
    }
}

void StrumSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), LcarsLookAndFeel::borderRadius, 1.0F);

    // Separator line between Midi Channel and Voicing
    g.setColour(LcarsColors::africanViolet);
    g.drawHorizontalLine(separatorY, 10.0F, static_cast<float>(getWidth() - 10));
}

void StrumSettingsPanel::resized() {
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
        channelLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingDescriptionLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny));
        strumPlateLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        gateLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        cooldownLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
    }

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
    // Cooldown row
    auto cooldownRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    cooldownSlider.setBounds(cooldownRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    cooldownLabel.setBounds(cooldownRowBounds);
    bounds.removeFromBottom(4);

    // Gate row
    auto gateRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    gateSlider.setBounds(gateRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    gateLabel.setBounds(gateRowBounds);
    bounds.removeFromBottom(4);

    // Strum CC row
    auto strumCcRowBounds = bounds.removeFromBottom(LcarsLookAndFeel::rowHeight);
    strumPlateCcLearn.setBounds(strumCcRowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
    strumPlateLabel.setBounds(strumCcRowBounds);
    bounds.removeFromBottom(4);

    // Voicing combo box at top of remaining space (matches VariantSelector layout)
    LcarsLookAndFeel::setComboBoxFontSize(voicingStyleComboBox, LcarsLookAndFeel::fontSizeSmall);
    voicingStyleComboBox.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(8);  // Padding between combo box and description

    // Description gets remaining middle space
    voicingDescriptionLabel.setBounds(bounds);
}
