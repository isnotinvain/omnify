#include "FromFileView.h"

FromFileView::FromFileView() {
    pathLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    pathLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pathLabel);

    browseButton.setColour(juce::TextButton::buttonColourId, LcarsColors::africanViolet);
    browseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    browseButton.onClick = [this]() {
        fileChooser = std::make_unique<juce::FileChooser>("Select voicing file", juce::File{}, "*.json");

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.existsAsFile() && onPathChanged) {
                onPathChanged(result.getFullPathName().toStdString());
            }
        });
    };
    addAndMakeVisible(browseButton);
}

void FromFileView::setPath(const std::string& path) {
    if (path.empty()) {
        pathLabel.setText("No file selected", juce::dontSendNotification);
    } else {
        juce::File f(path);
        pathLabel.setText(f.getFileName(), juce::dontSendNotification);
        pathLabel.setTooltip(path);
    }
}

void FromFileView::resized() {
    auto bounds = getLocalBounds();
    browseButton.setBounds(bounds.removeFromRight(80));
    bounds.removeFromRight(8);
    pathLabel.setBounds(bounds);
}
