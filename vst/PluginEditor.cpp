#include "PluginEditor.h"

//==============================================================================
OmnifyAudioProcessorEditor::OmnifyAudioProcessorEditor(OmnifyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
    // Title label
    titleLabel.setText("Omnify", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0F, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Gain slider
    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    // Mix slider
    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mixLabel);

    // Bypass button
    bypassButton.setButtonText("Bypass");
    addAndMakeVisible(bypassButton);

    // Attach parameters
    auto& params = processorRef.getParameters();
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, ParamIDs::GAIN, gainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, ParamIDs::MIX, mixSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, ParamIDs::BYPASS, bypassButton);

    setSize(300, 200);
}

OmnifyAudioProcessorEditor::~OmnifyAudioProcessorEditor() = default;

//==============================================================================
void OmnifyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void OmnifyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(40));

    // Knobs area
    auto knobArea = bounds.removeFromTop(120);
    auto knobWidth = knobArea.getWidth() / 2;

    // Gain knob (left)
    auto gainArea = knobArea.removeFromLeft(knobWidth);
    gainLabel.setBounds(gainArea.removeFromTop(20));
    gainSlider.setBounds(gainArea);

    // Mix knob (right)
    mixLabel.setBounds(knobArea.removeFromTop(20));
    mixSlider.setBounds(knobArea);

    // Bypass button at bottom
    bypassButton.setBounds(bounds.reduced(80, 5));
}
