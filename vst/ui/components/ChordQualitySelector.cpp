#include "ChordQualitySelector.h"

#include <algorithm>

#include "../LcarsLookAndFeel.h"

ChordQualitySelector::ChordQualitySelector() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];
        ChordQuality quality = ALL_CHORD_QUALITIES[i];

        row.label.setText(getChordQualityData(quality).niceName, juce::dontSendNotification);
        row.label.setColour(juce::Label::textColourId, labelColor);
        addAndMakeVisible(row.label);
        addAndMakeVisible(row.midiLearn);

        row.midiLearn.setAspectRatio(midiLearnAspectRatio);

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

void ChordQualitySelector::setMidiLearnAspectRatio(float ratio) {
    midiLearnAspectRatio = ratio;
    for (auto& row : rows) {
        row.midiLearn.setAspectRatio(ratio);
    }
}

void ChordQualitySelector::resized() {
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        auto font = laf->getOrbitronFont(LcarsLookAndFeel::fontSizeMedium);
        for (auto& row : rows) {
            row.label.setFont(font);
        }
    }

    auto bounds = getLocalBounds();
    int totalSpacing = rowSpacing * (static_cast<int>(NUM_QUALITIES) - 1);
    int rowHeight = (bounds.getHeight() - totalSpacing) / static_cast<int>(NUM_QUALITIES);

    for (auto& row : rows) {
        auto rowBounds = bounds.removeFromTop(rowHeight);

        // Calculate MIDI learn width based on aspect ratio
        int midiLearnWidth = static_cast<int>(static_cast<float>(rowHeight) * midiLearnAspectRatio);
        midiLearnWidth = std::min(midiLearnWidth, rowBounds.getWidth() / 2);

        row.midiLearn.setBounds(rowBounds.removeFromRight(midiLearnWidth));
        row.label.setBounds(rowBounds);
        bounds.removeFromTop(rowSpacing);
    }
}
