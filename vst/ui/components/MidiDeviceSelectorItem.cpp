#include "MidiDeviceSelectorItem.h"

MidiDeviceSelectorComponent::MidiDeviceSelectorComponent() {
    addAndMakeVisible(comboBox);
    addAndMakeVisible(captionLabel);

    captionLabel.setText("MIDI Device", juce::dontSendNotification);
    captionLabel.setJustificationType(juce::Justification::centredRight);

    comboBox.onChange = [this]() {
        int idx = comboBox.getSelectedItemIndex();
        if (idx >= 0 && idx < deviceNames.size()) {
            currentDeviceName = deviceNames[idx];
            enableMidiDeviceInStandalone(currentDeviceName);
            if (onDeviceSelected) {
                onDeviceSelected(currentDeviceName);
            }
        }
    };

    refreshDeviceList();

    // Poll for device changes every 2 seconds
    startTimer(2000);
}

MidiDeviceSelectorComponent::~MidiDeviceSelectorComponent() { stopTimer(); }

void MidiDeviceSelectorComponent::resized() {
    auto bounds = getLocalBounds();

    // Caption on left, combo box on right
    int captionWidth = bounds.getWidth() / 3;
    captionLabel.setBounds(bounds.removeFromLeft(captionWidth));
    comboBox.setBounds(bounds);
}

void MidiDeviceSelectorComponent::setSelectedDevice(const juce::String& deviceName) {
    currentDeviceName = deviceName;

    int idx = deviceNames.indexOf(deviceName);
    if (idx >= 0) {
        comboBox.setSelectedItemIndex(idx, juce::dontSendNotification);
    } else if (deviceNames.size() > 0) {
        if (deviceName.isEmpty()) {
            // No device selected yet - default to first device
            comboBox.setSelectedItemIndex(0, juce::dontSendNotification);
            currentDeviceName = deviceNames[0];
        } else {
            // Saved device not found - show placeholder
            comboBox.setSelectedId(0, juce::dontSendNotification);
            comboBox.setText(deviceName + " (not found)", juce::dontSendNotification);
        }
    }

    if (currentDeviceName.isNotEmpty()) {
        enableMidiDeviceInStandalone(currentDeviceName);
    }
}

void MidiDeviceSelectorComponent::refreshDeviceList() {
    auto devices = juce::MidiInput::getAvailableDevices();

    juce::StringArray newNames;
    for (const auto& device : devices) {
        newNames.add(device.name);
    }

    // Only update if the list changed
    if (newNames != deviceNames) {
        deviceNames = newNames;

        // Remember current selection
        juce::String savedSelection = currentDeviceName;

        comboBox.clear(juce::dontSendNotification);
        int id = 1;
        for (const auto& name : deviceNames) {
            comboBox.addItem(name, id++);
        }

        // Restore selection
        setSelectedDevice(savedSelection);
    }
}

void MidiDeviceSelectorComponent::setCaption(const juce::String& text) { captionLabel.setText(text, juce::dontSendNotification); }

void MidiDeviceSelectorComponent::timerCallback() { refreshDeviceList(); }

void MidiDeviceSelectorComponent::enableMidiDeviceInStandalone(const juce::String& deviceName) {
    // In standalone mode, the StandaloneFilterWindow handles MIDI device management.
    // This component is display-only for plugin mode.
    juce::ignoreUnused(deviceName);
}
