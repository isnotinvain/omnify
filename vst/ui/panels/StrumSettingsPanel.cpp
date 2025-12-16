#include "StrumSettingsPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/OmnifySettings.h"
#include "../LcarsLookAndFeel.h"

StrumSettingsPanel::StrumSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title - font will be set in resized() after LookAndFeel is available
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(titleLabel);

    // MIDI Channel
    channelLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

    // Voicing Style Selector - iterate registry to build options
    for (const auto& [typeName, entry] : processor.getStrumVoicingRegistry().getRegistry()) {
        auto view = std::make_unique<juce::Component>();
        voicingStyleSelector.addVariantNotOwned(entry.style->displayName(), view.get());
        voicingStyleViews.push_back(std::move(view));
        voicingStyleTypeNames.push_back(typeName);
    }
    addAndMakeVisible(voicingStyleSelector);

    // Strum Plate CC
    strumPlateLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(strumPlateLabel);

    strumPlateCcLearn.setAcceptMode(MidiAcceptMode::CCsOnly);
    addAndMakeVisible(strumPlateCcLearn);

    // Sliders
    gateLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    gateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gateLabel);

    cooldownLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    cooldownLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(cooldownLabel);

    gateSlider.setColour(juce::Slider::thumbColourId, LcarsColors::orange);
    gateSlider.setColour(juce::Slider::rotarySliderFillColourId, LcarsColors::moonlitViolet);
    gateSlider.setColour(juce::Slider::rotarySliderOutlineColourId, LcarsColors::africanViolet);
    gateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gateSlider);

    cooldownSlider.setColour(juce::Slider::thumbColourId, LcarsColors::orange);
    cooldownSlider.setColour(juce::Slider::rotarySliderFillColourId, LcarsColors::moonlitViolet);
    cooldownSlider.setColour(juce::Slider::rotarySliderOutlineColourId, LcarsColors::africanViolet);
    cooldownSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
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
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "strum_gate_time_secs", gateSlider);
    cooldownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "strum_cooldown_secs", cooldownSlider);
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
    g.drawRect(getLocalBounds(), 2);
}

void StrumSettingsPanel::resized() {
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
    }

    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(4);

    auto channelRowBounds = bounds.removeFromTop(35);
    channelLabel.setBounds(channelRowBounds.removeFromLeft(channelRowBounds.getWidth() - 80));
    channelComboBox.setBounds(channelRowBounds);
    bounds.removeFromTop(4);

    voicingStyleSelector.setBounds(bounds.removeFromTop(60));
    bounds.removeFromTop(8);

    strumPlateLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);
    strumPlateCcLearn.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(8);

    // Sliders side by side
    auto sliderArea = bounds;
    int sliderWidth = sliderArea.getWidth() / 2;

    auto gateArea = sliderArea.removeFromLeft(sliderWidth);
    gateLabel.setBounds(gateArea.removeFromTop(20));
    gateSlider.setBounds(gateArea);

    auto cooldownArea = sliderArea;
    cooldownLabel.setBounds(cooldownArea.removeFromTop(20));
    cooldownSlider.setBounds(cooldownArea);
}
