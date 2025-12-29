#include "PluginEditor.h"

#include "datamodel/DawOrDevice.h"
#include "datamodel/OmnifySettings.h"
#include "ui/LcarsLookAndFeel.h"

OmnifyAudioProcessorEditor::OmnifyAudioProcessorEditor(OmnifyAudioProcessor& p)
    : AudioProcessorEditor(&p), omnifyProcessor(p), chordSettings(p), strumSettings(p), chordQualityPanel(p) {
    // Disable resizing
    setResizable(false, false);
    setSize(900, 610);

    // Title
    titleLabel.setText("OMNIFY", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // MIDI I/O Panel
    midiIOPanel.onInputChanged = [this](bool useDaw, const juce::String& deviceName) {
        omnifyProcessor.modifySettings([useDaw, deviceName](OmnifySettings& s) {
            s.input = useDaw ? DawOrDevice{Daw{}} : DawOrDevice{Device{deviceName.toStdString()}};
        });
    };
    midiIOPanel.onOutputChanged = [this](bool useDaw, const juce::String& portName) {
        omnifyProcessor.modifySettings([useDaw, portName](OmnifySettings& s) {
            s.output = useDaw ? DawOrDevice{Daw{}} : DawOrDevice{Device{portName.toStdString()}};
        });
    };
    addAndMakeVisible(midiIOPanel);

    // Panels
    addAndMakeVisible(chordSettings);
    addAndMakeVisible(strumSettings);
    addAndMakeVisible(chordQualityPanel);

    refreshFromSettings();
}

OmnifyAudioProcessorEditor::~OmnifyAudioProcessorEditor() = default;

void OmnifyAudioProcessorEditor::refreshFromSettings() {
    auto settings = omnifyProcessor.getSettings();

    // MIDI I/O
    midiIOPanel.setInputDaw(isDaw(settings->input));
    if (isDevice(settings->input)) {
        midiIOPanel.setInputDevice(juce::String(getDeviceName(settings->input)));
    }
    midiIOPanel.setOutputDaw(isDaw(settings->output));
    if (isDevice(settings->output)) {
        midiIOPanel.setOutputPortName(juce::String(getDeviceName(settings->output)));
    } else {
        midiIOPanel.setOutputPortName("Omnify");
    }

    // Panels
    chordSettings.refreshFromSettings();
    strumSettings.refreshFromSettings();
    chordQualityPanel.refreshFromSettings();
}

void OmnifyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    // Draw capsule behind title
    auto titleBounds = titleLabel.getBounds().toFloat();
    float radius = titleBounds.getHeight() * 0.5F;
    g.setColour(LcarsColors::red);
    g.fillRoundedRectangle(titleBounds, radius);
}

void OmnifyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds().reduced(6);

    // Top row: Title (1/3) + MIDI I/O panel (2/3)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTitle));
    }

    auto topArea = bounds.removeFromTop(70);
    juce::FlexBox topRow;
    topRow.flexDirection = juce::FlexBox::Direction::row;
    topRow.items.add(juce::FlexItem(titleLabel).withFlex(1.0F).withMargin(3));
    topRow.items.add(juce::FlexItem(midiIOPanel).withFlex(2.0F).withMargin(3));
    topRow.performLayout(topArea);

    bounds.removeFromTop(6);

    // Main area: 3 equal columns using FlexBox
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.items.add(juce::FlexItem(chordSettings).withFlex(1.0F).withMargin(3));
    fb.items.add(juce::FlexItem(chordQualityPanel).withFlex(1.0F).withMargin(3));
    fb.items.add(juce::FlexItem(strumSettings).withFlex(1.0F).withMargin(3));
    fb.performLayout(bounds);
}
