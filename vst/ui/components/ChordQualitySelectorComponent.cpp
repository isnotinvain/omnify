#include "ChordQualitySelectorComponent.h"

ChordQualitySelectorComponent::ChordQualitySelectorComponent() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        learners[i] = std::make_unique<MidiLearnComponent>();
        learners[i]->setCaption("");
        addAndMakeVisible(learners[i].get());
    }
}

MidiLearnedValue ChordQualitySelectorComponent::getLearnerValue(size_t qualityIndex) const {
    if (qualityIndex < NUM_QUALITIES) {
        return learners[qualityIndex]->getLearnedValue();
    }
    return {};
}

void ChordQualitySelectorComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey.darker());

    auto bounds = getLocalBounds();
    int rowHeight = bounds.getHeight() / NUM_QUALITIES;
    int labelWidth = 120;

    g.setColour(juce::Colours::white);

    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto rowBounds = bounds.removeFromTop(rowHeight);
        auto labelBounds = rowBounds.removeFromLeft(labelWidth);
        g.drawText(GeneratedParams::ChordQualities::NAMES[i], labelBounds.reduced(5),
                   juce::Justification::centredRight);
    }
}

void ChordQualitySelectorComponent::resized() {
    auto bounds = getLocalBounds();
    int rowHeight = bounds.getHeight() / NUM_QUALITIES;
    int labelWidth = 120;

    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto rowBounds = bounds.removeFromTop(rowHeight);
        rowBounds.removeFromLeft(labelWidth);  // Skip label area
        learners[i]->setBounds(rowBounds.reduced(2));
    }
}