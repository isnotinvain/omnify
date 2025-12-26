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
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

    // Voicing Style
    voicingLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(voicingLabel);

    // Voicing Style Selector - iterate registry to build options
    for (const auto& [typeName, entry] : processor.getStrumVoicingRegistry().getRegistry()) {
        auto view = std::make_unique<juce::Component>();
        view->getProperties().set("preferredHeight", 0);
        voicingStyleSelector.addVariantNotOwned(entry.style->displayName(), view.get(), entry.style->description());
        voicingStyleViews.push_back(std::move(view));
        voicingStyleTypeNames.push_back(typeName);
    }
    addAndMakeVisible(voicingStyleSelector);

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
    voicingStyleSelector.onSelectionChanged = [this](int index) {
        if (index >= 0 && index < static_cast<int>(voicingStyleTypeNames.size())) {
            const auto& typeName = voicingStyleTypeNames[static_cast<size_t>(index)];
            const auto& registry = processor.getStrumVoicingRegistry().getRegistry();
            auto it = registry.find(typeName);
            if (it != registry.end()) {
                processor.modifySettings([style = it->second.style](OmnifySettings& s) { s.strumVoicingStyle = style; });
            }
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

    // Voicing style selector - find matching index via registry lookup
    if (settings->strumVoicingStyle) {
        auto currentType = processor.getStrumVoicingRegistry().getTypeName(settings->strumVoicingStyle.get());
        if (currentType) {
            for (size_t i = 0; i < voicingStyleTypeNames.size(); ++i) {
                if (voicingStyleTypeNames[i] == *currentType) {
                    voicingStyleSelector.setSelectedIndex(static_cast<int>(i), juce::dontSendNotification);
                    break;
                }
            }
        }
    }
}

void StrumSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), LcarsLookAndFeel::borderRadius, 1.0F);
}

void StrumSettingsPanel::resized() {
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
        channelLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
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

    voicingLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);

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

    // Voicing selector gets remaining middle space
    voicingStyleSelector.setBounds(bounds);
}
