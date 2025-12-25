#include "FromFileView.h"

#include "../LcarsLookAndFeel.h"

FromFileView::FromFileView() {
    pathLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    pathLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pathLabel);
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
    browseButtonBounds = bounds.removeFromRight(100);
    bounds.removeFromRight(8);
    pathLabel.setBounds(bounds);
}

void FromFileView::paint(juce::Graphics& g) {
    auto bounds = browseButtonBounds.toFloat();
    const float borderThickness = 1.0F;
    const float radius = bounds.getHeight() * 0.5F;

    // Background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius);

    // Border
    g.setColour(LcarsColors::orange);
    g.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius, borderThickness);

    // Text
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        g.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
    }
    g.setColour(LcarsColors::orange);
    g.drawText("Browse", browseButtonBounds, juce::Justification::centred);
}

void FromFileView::mouseDown(const juce::MouseEvent& event) {
    if (browseButtonBounds.contains(event.getPosition())) {
        launchFileBrowser();
    }
}

void FromFileView::launchFileBrowser() {
    fileChooser = std::make_unique<juce::FileChooser>("Select voicing file", juce::File{}, "*.json");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
        auto result = fc.getResult();
        if (result.existsAsFile() && onPathChanged) {
            onPathChanged(result.getFullPathName().toStdString());
        }
    });
}
