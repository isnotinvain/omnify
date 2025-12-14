#include "StrumSettingsPanel.h"

#include "../../PluginProcessor.h"

StrumSettingsPanel::StrumSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title
    titleLabel.setFont(juce::Font(juce::FontOptions(24.0f)));
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(titleLabel);

    // MIDI Channel
    channelLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

    // Voicing Style Selector
    voicingStyleSelector.addVariantNotOwned("Plain Ascending", &plainAscendingView);
    voicingStyleSelector.addVariantNotOwned("Omnichord", &omnichordView);
    addAndMakeVisible(voicingStyleSelector);
    addAndMakeVisible(plainAscendingView);
    addAndMakeVisible(omnichordView);

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

    // Set up value bindings
    setupValueBindings();
}

StrumSettingsPanel::~StrumSettingsPanel() = default;

void StrumSettingsPanel::setupValueBindings() {
    auto& stateTree = processor.getStateTree();
    auto& apvts = processor.getAPVTS();

    // MIDI Channel
    strumChannelValue.referTo(stateTree.getPropertyAsValue("strum_channel", nullptr));
    channelComboBox.setSelectedId(static_cast<int>(strumChannelValue.getValue()), juce::dontSendNotification);
    channelComboBox.onChange = [this]() {
        strumChannelValue.setValue(channelComboBox.getSelectedId());
    };

    // Voicing Style
    strumVoicingStyleValue.referTo(stateTree.getPropertyAsValue("variant_strum_voicing_style", nullptr));
    voicingStyleSelector.bindToValue(strumVoicingStyleValue);

    // Strum Plate CC MIDI learn
    strumPlateCcTypeValue.referTo(stateTree.getPropertyAsValue("strum_plate_cc_type", nullptr));
    strumPlateCcNumberValue.referTo(stateTree.getPropertyAsValue("strum_plate_cc_number", nullptr));

    auto updateStrumPlateFromValues = [this]() {
        auto typeStr = strumPlateCcTypeValue.getValue().toString();
        int number = static_cast<int>(strumPlateCcNumberValue.getValue());
        MidiLearnedValue val;
        if (typeStr == "cc") {
            val.type = MidiLearnedType::CC;
            val.value = number;
        }
        strumPlateCcLearn.setLearnedValue(val);
    };
    updateStrumPlateFromValues();

    strumPlateCcLearn.onValueChanged = [this](MidiLearnedValue val) {
        if (val.type == MidiLearnedType::CC) {
            strumPlateCcTypeValue.setValue("cc");
        } else {
            strumPlateCcTypeValue.setValue("");
        }
        strumPlateCcNumberValue.setValue(val.value);
    };

    // APVTS slider attachments
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "strum_gate_time_secs", gateSlider);
    cooldownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "strum_cooldown_secs", cooldownSlider);
}

void StrumSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRect(getLocalBounds(), 2);
}

void StrumSettingsPanel::resized() {
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
