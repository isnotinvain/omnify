#include "ChordQualitySelector.h"

#include <algorithm>

#include "../LcarsLookAndFeel.h"

ChordQualitySelector::ChordQualitySelector() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];
        ChordQuality quality = ALL_CHORD_QUALITIES[i];

        row.label.setText(getChordQualityData(quality).niceName, juce::dontSendNotification);
        row.label.setColour(juce::Label::textColourId, labelColor);
        row.label.setMinimumHorizontalScale(1.0F);
        addAndMakeVisible(row.label);
        addAndMakeVisible(row.midiLearn);

        // Set up callback for when user learns a new value
        row.midiLearn.onValueChanged = [this, quality](MidiLearnedValue val) {
            if (onQualityMidiChanged) {
                onQualityMidiChanged(quality, val);
            }
        };
    }
}

void ChordQualitySelector::setMidiMapping(ChordQuality quality, MidiLearnedValue val) {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        if (ALL_CHORD_QUALITIES[i] == quality) {
            rows[i].midiLearn.setLearnedValue(val);
            return;
        }
    }
}

void ChordQualitySelector::setLabelColor(juce::Colour color) {
    labelColor = color;
    for (auto& row : rows) {
        row.label.setColour(juce::Label::textColourId, labelColor);
    }
}

void ChordQualitySelector::resized() {
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        auto font = laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall);
        for (auto& row : rows) {
            row.label.setFont(font);
        }
    }

    auto bounds = getLocalBounds();

    // Bottom-align: calculate total height needed and skip the top portion
    int totalHeight = static_cast<int>(NUM_QUALITIES) * LcarsLookAndFeel::rowHeight +
                      (static_cast<int>(NUM_QUALITIES) - 1) * rowSpacing;
    bounds.removeFromTop(bounds.getHeight() - totalHeight);

    for (auto& row : rows) {
        auto rowBounds = bounds.removeFromTop(LcarsLookAndFeel::rowHeight);
        row.midiLearn.setBounds(rowBounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
        row.label.setBounds(rowBounds);
        bounds.removeFromTop(rowSpacing);
    }
}
