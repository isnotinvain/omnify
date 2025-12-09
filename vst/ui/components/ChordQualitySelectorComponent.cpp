#include "ChordQualitySelectorComponent.h"

#include "../LcarsColors.h"

ChordQualitySelectorComponent::ChordQualitySelectorComponent() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        learners[i] = std::make_unique<MidiLearnComponent>();
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
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();
    int rowHeight = bounds.getHeight() / NUM_QUALITIES;
    int learnerWidth = static_cast<int>(rowHeight * 2.5f) + 4;

    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto rowBounds = bounds.removeFromTop(rowHeight);
        rowBounds.removeFromRight(learnerWidth);  // Skip learner area
        auto labelBounds = rowBounds;

        g.setColour(LcarsColors::orange);
        g.setFont(getLookAndFeel().getPopupMenuFont().withHeight(fontSize));
        g.drawText(GeneratedParams::ChordQualities::NAMES[i], labelBounds.reduced(5),
                   juce::Justification::centredLeft);
    }
}

void ChordQualitySelectorComponent::resized() {
    auto bounds = getLocalBounds();
    int rowHeight = bounds.getHeight() / NUM_QUALITIES;
    int learnerWidth = static_cast<int>(rowHeight * 2.5f) + 4;

    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto rowBounds = bounds.removeFromTop(rowHeight);
        auto learnerBounds = rowBounds.removeFromRight(learnerWidth);
        learners[i]->setBounds(learnerBounds.reduced(2));
    }
}