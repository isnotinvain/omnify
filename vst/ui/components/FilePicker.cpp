#include "FilePicker.h"

FilePicker::FilePicker() {
    addAndMakeVisible(pathLabel);
    addAndMakeVisible(browseButton);

    // Style the label
    pathLabel.setColour(juce::Label::outlineColourId, LcarsColors::orange);
    pathLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    pathLabel.setJustificationType(juce::Justification::centredLeft);

    // Style the button
    browseButton.setColour(juce::TextButton::buttonColourId, LcarsColors::orange);
    browseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

    browseButton.onClick = [this]() { openFileChooser(); };
}

FilePicker::~FilePicker() = default;

void FilePicker::setFileFilter(const juce::String& filter) { fileFilter = filter.isEmpty() ? "*" : filter; }

void FilePicker::bindToValue(juce::Value& value) {
    boundValue.referTo(value);
    boundValue.addListener(this);

    // Set initial text from value
    pathLabel.setText(value.toString(), juce::dontSendNotification);
}

void FilePicker::valueChanged(juce::Value& value) {
    if (&value == &boundValue) {
        pathLabel.setText(value.toString(), juce::dontSendNotification);
    }
}

void FilePicker::resized() {
    auto bounds = getLocalBounds();
    browseButton.setBounds(bounds.removeFromRight(80));
    pathLabel.setBounds(bounds.reduced(2));
}

void FilePicker::openFileChooser() {
    auto currentPath = boundValue.toString();
    auto startDir =
        currentPath.isNotEmpty() ? juce::File(currentPath).getParentDirectory() : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    fileChooser = std::make_unique<juce::FileChooser>("Select a file", startDir, fileFilter);

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        auto result = fc.getResult();
        if (result.existsAsFile()) {
            const auto& newPath = result.getFullPathName();
            boundValue.setValue(newPath);
            pathLabel.setText(newPath, juce::dontSendNotification);
        }
    });
}
