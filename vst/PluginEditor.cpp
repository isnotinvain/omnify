#include "PluginEditor.h"

OmnifyAudioProcessorEditor::OmnifyAudioProcessorEditor(OmnifyAudioProcessor& p)
    : AudioProcessorEditor(&p),
      omnifyProcessor(p),
      chordSettings(p),
      strumSettings(p),
      chordQualityPanel(p) {
    // Apply look and feel to this editor and all children
    setLookAndFeel(&lcarsLookAndFeel);

    // Disable resizing
    setResizable(false, false);
    setSize(900, 600);

    // MIDI Device Selector
    midiDeviceSelector.setCaption("MIDI Device");
    midiDeviceValue = omnifyProcessor.getStateTree().getPropertyAsValue("midi_device_name", nullptr);
    midiDeviceSelector.bindToValue(midiDeviceValue);
    addAndMakeVisible(midiDeviceSelector);

    // Panels
    addAndMakeVisible(chordSettings);
    addAndMakeVisible(strumSettings);
    addAndMakeVisible(chordQualityPanel);
}

OmnifyAudioProcessorEditor::~OmnifyAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void OmnifyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
}

void OmnifyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds().reduced(6);

    // Top row: MIDI device selector (spans full width)
    auto topArea = bounds.removeFromTop(50);
    midiDeviceSelector.setBounds(topArea);

    bounds.removeFromTop(6);

    // Main area: 3 equal columns using FlexBox
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.items.add(juce::FlexItem(chordSettings).withFlex(1.0f).withMargin(3));
    fb.items.add(juce::FlexItem(strumSettings).withFlex(1.0f).withMargin(3));
    fb.items.add(juce::FlexItem(chordQualityPanel).withFlex(1.0f).withMargin(3));
    fb.performLayout(bounds);
}
