#include "MidiIOPanel.h"

#include "../LcarsColors.h"
#include "../LcarsLookAndFeel.h"

MidiIOPanel::MidiIOPanel() {
    // Input label
    inputLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    inputLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(inputLabel);

    // Input DAW toggle
    inputDawToggle.setColour(juce::ToggleButton::tickColourId, LcarsColors::orange);
    inputDawToggle.getProperties().set("onText", "From DAW");
    inputDawToggle.getProperties().set("offText", "From Device");
    LcarsLookAndFeel::setToggleButtonFontSize(inputDawToggle, LcarsLookAndFeel::fontSizeMiniscule);
    inputDawToggle.setToggleState(true, juce::dontSendNotification);
    inputDawToggle.onClick = [this]() {
        bool useDaw = inputDawToggle.getToggleState();
        inputDeviceCombo.setEnabled(!useDaw);
        inputDeviceCombo.setVisible(!useDaw);
        resized();
        notifyInputChanged();
    };
    addAndMakeVisible(inputDawToggle);

    // Input device combo (hidden by default since DAW toggle is on)
    inputDeviceCombo.setEnabled(false);
    inputDeviceCombo.setVisible(false);
    LcarsLookAndFeel::setComboBoxFontSize(inputDeviceCombo, LcarsLookAndFeel::fontSizeTiny);
    inputDeviceCombo.onChange = [this]() {
        int idx = inputDeviceCombo.getSelectedItemIndex();
        if (idx >= 0 && idx < deviceNames.size()) {
            currentDeviceName = deviceNames[idx];
            notifyInputChanged();
        }
    };
    addAndMakeVisible(inputDeviceCombo);

    // Output label
    outputLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    outputLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(outputLabel);

    // Output DAW toggle
    outputDawToggle.setColour(juce::ToggleButton::tickColourId, LcarsColors::orange);
    outputDawToggle.getProperties().set("onText", "To DAW");
    outputDawToggle.getProperties().set("offText", "To Port");
    LcarsLookAndFeel::setToggleButtonFontSize(outputDawToggle, LcarsLookAndFeel::fontSizeMiniscule);
    outputDawToggle.setToggleState(true, juce::dontSendNotification);
    outputDawToggle.onClick = [this]() {
        bool useDaw = outputDawToggle.getToggleState();
        outputPortCombo.setEnabled(!useDaw);
        outputPortCombo.setVisible(!useDaw);
        resized();
        notifyOutputChanged();
    };
    addAndMakeVisible(outputDawToggle);

    // Output port combo (hidden by default since DAW toggle is on)
    outputPortCombo.setEnabled(false);
    outputPortCombo.setVisible(false);
    outputPortCombo.addItem("Omnify", 1);
    outputPortCombo.addItem("Omnify: BMO", 2);
    outputPortCombo.addItem("Omnify: Marcy", 3);
    outputPortCombo.addItem("Omnify: PB", 4);
    outputPortCombo.setSelectedId(1, juce::dontSendNotification);
    LcarsLookAndFeel::setComboBoxFontSize(outputPortCombo, LcarsLookAndFeel::fontSizeTiny);
    outputPortCombo.onChange = [this]() { notifyOutputChanged(); };
    addAndMakeVisible(outputPortCombo);

    refreshDeviceList();
    startTimer(2000);
}

MidiIOPanel::~MidiIOPanel() { stopTimer(); }

void MidiIOPanel::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    const float halfWidth = bounds.getWidth() / 2.0F;
    const float gap = 3.0F;

    g.setColour(LcarsColors::africanViolet);

    // Input section border (left half)
    auto inputBounds = bounds.removeFromLeft(halfWidth - gap);
    g.drawRoundedRectangle(inputBounds, LcarsLookAndFeel::borderRadius, 1.0F);

    // Output section border (right half)
    bounds.removeFromLeft(gap * 2.0F);
    g.drawRoundedRectangle(bounds, LcarsLookAndFeel::borderRadius, 1.0F);
}

void MidiIOPanel::resized() {
    auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel());

    auto bounds = getLocalBounds();
    const int halfWidth = bounds.getWidth() / 2;
    const int gap = 3;
    const int padding = 6;

    const bool inputDawMode = inputDawToggle.getToggleState();
    const bool outputDawMode = outputDawToggle.getToggleState();

    // Input section (left half)
    auto inputSection = bounds.removeFromLeft(halfWidth - gap).reduced(padding, 4);

    if (inputDawMode) {
        // DAW mode: larger text, vertically centered single row
        const int dawRowHeight = 36;
        const int dawToggleWidth = 140;

        if (laf != nullptr) {
            inputLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
            LcarsLookAndFeel::setToggleButtonFontSize(inputDawToggle, LcarsLookAndFeel::fontSizeSmall);
        }

        auto centeredRow = inputSection.withSizeKeepingCentre(inputSection.getWidth(), dawRowHeight);
        inputLabel.setBounds(centeredRow.removeFromLeft(centeredRow.getWidth() - dawToggleWidth));
        inputDawToggle.setBounds(centeredRow);
    } else {
        // Device mode: two rows with smaller text
        const int rowHeight = 26;
        const int toggleWidth = 120;
        const int rowSpacing = 2;

        if (laf != nullptr) {
            inputLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny));
            LcarsLookAndFeel::setToggleButtonFontSize(inputDawToggle, LcarsLookAndFeel::fontSizeMiniscule);
        }

        auto inputTopRow = inputSection.removeFromTop(rowHeight);
        inputLabel.setBounds(inputTopRow.removeFromLeft(inputTopRow.getWidth() - toggleWidth));
        inputDawToggle.setBounds(inputTopRow);

        inputSection.removeFromTop(rowSpacing);
        inputDeviceCombo.setBounds(inputSection.removeFromTop(rowHeight));
    }

    // Output section (right half)
    bounds.removeFromLeft(gap * 2);
    auto outputSection = bounds.reduced(padding, 4);

    if (outputDawMode) {
        // DAW mode: larger text, vertically centered single row
        const int dawRowHeight = 36;
        const int dawToggleWidth = 140;

        if (laf != nullptr) {
            outputLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
            LcarsLookAndFeel::setToggleButtonFontSize(outputDawToggle, LcarsLookAndFeel::fontSizeSmall);
        }

        auto centeredRow = outputSection.withSizeKeepingCentre(outputSection.getWidth(), dawRowHeight);
        outputLabel.setBounds(centeredRow.removeFromLeft(centeredRow.getWidth() - dawToggleWidth));
        outputDawToggle.setBounds(centeredRow);
    } else {
        // Port mode: two rows with smaller text
        const int rowHeight = 26;
        const int toggleWidth = 120;
        const int rowSpacing = 2;

        if (laf != nullptr) {
            outputLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny));
            LcarsLookAndFeel::setToggleButtonFontSize(outputDawToggle, LcarsLookAndFeel::fontSizeMiniscule);
        }

        auto outputTopRow = outputSection.removeFromTop(rowHeight);
        outputLabel.setBounds(outputTopRow.removeFromLeft(outputTopRow.getWidth() - toggleWidth));
        outputDawToggle.setBounds(outputTopRow);

        outputSection.removeFromTop(rowSpacing);
        outputPortCombo.setBounds(outputSection.removeFromTop(rowHeight));
    }
}

void MidiIOPanel::timerCallback() { refreshDeviceList(); }

void MidiIOPanel::refreshDeviceList() {
    auto devices = juce::MidiInput::getAvailableDevices();

    juce::StringArray newNames;
    for (const auto& device : devices) {
        if (!device.name.startsWith("Omnify")) {
            newNames.add(device.name);
        }
    }

    if (newNames != deviceNames) {
        deviceNames = newNames;
        juce::String savedSelection = currentDeviceName;

        inputDeviceCombo.clear(juce::dontSendNotification);
        int id = 1;
        for (const auto& name : deviceNames) {
            inputDeviceCombo.addItem(name, id++);
        }

        setInputDevice(savedSelection);
    }
}

void MidiIOPanel::notifyInputChanged() {
    if (onInputChanged) {
        onInputChanged(inputDawToggle.getToggleState(), currentDeviceName);
    }
}

void MidiIOPanel::notifyOutputChanged() {
    if (onOutputChanged) {
        onOutputChanged(outputDawToggle.getToggleState(), outputPortCombo.getText());
    }
}

void MidiIOPanel::setInputDaw(bool useDaw) {
    inputDawToggle.setToggleState(useDaw, juce::dontSendNotification);
    inputDeviceCombo.setEnabled(!useDaw);
    inputDeviceCombo.setVisible(!useDaw);
    resized();
}

void MidiIOPanel::setInputDevice(const juce::String& deviceName) {
    currentDeviceName = deviceName;

    int idx = deviceNames.indexOf(deviceName);
    if (idx >= 0) {
        inputDeviceCombo.setSelectedItemIndex(idx, juce::dontSendNotification);
    } else if (deviceNames.size() > 0) {
        if (deviceName.isEmpty()) {
            inputDeviceCombo.setSelectedItemIndex(0, juce::dontSendNotification);
            currentDeviceName = deviceNames[0];
        } else {
            inputDeviceCombo.setSelectedId(0, juce::dontSendNotification);
            inputDeviceCombo.setText(deviceName + " (not found)", juce::dontSendNotification);
        }
    }
}

void MidiIOPanel::setOutputDaw(bool useDaw) {
    outputDawToggle.setToggleState(useDaw, juce::dontSendNotification);
    outputPortCombo.setEnabled(!useDaw);
    outputPortCombo.setVisible(!useDaw);
    resized();
}

void MidiIOPanel::setOutputPortName(const juce::String& portName) {
    for (int i = 0; i < outputPortCombo.getNumItems(); ++i) {
        if (outputPortCombo.getItemText(i) == portName) {
            outputPortCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }
    outputPortCombo.setSelectedId(1, juce::dontSendNotification);
}
