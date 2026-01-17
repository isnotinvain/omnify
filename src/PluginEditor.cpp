#include "PluginEditor.h"

#include <array>

#include "datamodel/DawOrDevice.h"
#include "datamodel/OmnifySettings.h"
#include "ui/LcarsLookAndFeel.h"

OmnifyAudioProcessorEditor::OmnifyAudioProcessorEditor(OmnifyAudioProcessor& p)
    : AudioProcessorEditor(&p), omnifyProcessor(p), chordSettings(p), strumSettings(p), chordQualityPanel(p) {
    // Disable resizing
    setResizable(false, false);
    setSize(900, 700);

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

    // Bottom row - chord quality display
    chordQualityDisplay.setJustificationType(juce::Justification::centred);
    chordQualityDisplay.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(chordQualityDisplay);

    // Bottom row - keyboard display
    keyboardDisplay.setWhiteKeyColour(LcarsColors::africanViolet);
    keyboardDisplay.setBlackKeyColour(juce::Colours::black);
    keyboardDisplay.setHighlightColour(LcarsColors::orange);
    keyboardDisplay.setKeyRange(36, 96);  // C2 to C7
    addAndMakeVisible(keyboardDisplay);

    refreshFromSettings();

    setWantsKeyboardFocus(true);
    startTimerHz(30);
}

OmnifyAudioProcessorEditor::~OmnifyAudioProcessorEditor() { stopTimer(); }

bool OmnifyAudioProcessorEditor::keyPressed(const juce::KeyPress& key) {
    // Keys 1-9 select chord qualities
    int keyCode = key.getKeyCode();
    if (keyCode >= '1' && keyCode <= '9') {
        int index = keyCode - '1';
        omnifyProcessor.setChordQuality(ALL_CHORD_QUALITIES[static_cast<size_t>(index)]);
        return true;
    }
    return false;
}

void OmnifyAudioProcessorEditor::timerCallback() { updateDisplayState(); }

void OmnifyAudioProcessorEditor::updateDisplayState() {
    // Update chord quality display
    auto quality = omnifyProcessor.getDisplayChordQuality();
    const auto& qualityData = getChordQualityData(quality);
    int root = omnifyProcessor.getDisplayCurrentRoot();

    static constexpr std::array<const char*, 12> noteNames = {"C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"};
    if (root >= 0) {
        juce::String chordName = juce::String(noteNames[static_cast<size_t>(root % 12)]) + " " + qualityData.suffix;
        chordQualityDisplay.setText(chordName, juce::dontSendNotification);
    } else {
        chordQualityDisplay.setText(qualityData.niceName, juce::dontSendNotification);
    }

    // Update keyboard display
    auto chordNotes = omnifyProcessor.getDisplayChordNotes();
    std::array<bool, 128> activeNotes{};
    for (uint8_t i = 0; i < chordNotes.count; i++) {
        activeNotes[static_cast<size_t>(chordNotes.notes[i].note)] = true;
    }
    keyboardDisplay.setActiveNotes(activeNotes);
}

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
        chordQualityDisplay.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTitle));
    }

    auto topArea = bounds.removeFromTop(70);
    juce::FlexBox topRow;
    topRow.flexDirection = juce::FlexBox::Direction::row;
    topRow.items.add(juce::FlexItem(titleLabel).withFlex(1.0F).withMargin(3));
    topRow.items.add(juce::FlexItem(midiIOPanel).withFlex(2.0F).withMargin(3));
    topRow.performLayout(topArea);

    bounds.removeFromTop(6);

    // Bottom row: Chord quality display (1/3) + Keyboard (2/3)
    auto bottomArea = bounds.removeFromBottom(70);
    juce::FlexBox bottomRow;
    bottomRow.flexDirection = juce::FlexBox::Direction::row;
    bottomRow.items.add(juce::FlexItem(chordQualityDisplay).withFlex(1.0F).withMargin(3));
    bottomRow.items.add(juce::FlexItem(keyboardDisplay).withFlex(2.0F).withMargin(3));
    bottomRow.performLayout(bottomArea);

    bounds.removeFromBottom(6);

    // Main area: 3 equal columns using FlexBox
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.items.add(juce::FlexItem(chordSettings).withFlex(1.0F).withMargin(3));
    fb.items.add(juce::FlexItem(chordQualityPanel).withFlex(1.0F).withMargin(3));
    fb.items.add(juce::FlexItem(strumSettings).withFlex(1.0F).withMargin(3));
    fb.performLayout(bounds);
}
