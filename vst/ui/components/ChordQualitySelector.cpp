#include "ChordQualitySelector.h"

#include <algorithm>

#include "../LcarsLookAndFeel.h"

ChordQualitySelector::ChordQualitySelector() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];
        row.label.setText(GeneratedSettings::ChordQualities::NAMES[i], juce::dontSendNotification);
        row.label.setColour(juce::Label::textColourId, labelColor);
        addAndMakeVisible(row.label);
        addAndMakeVisible(row.midiLearn);

        row.midiLearn.setAspectRatio(midiLearnAspectRatio);

        // Set up callback for when user learns a new value
        row.midiLearn.onValueChanged = [this, i](MidiLearnedValue val) { onMidiLearnChanged(i, val); };
    }
}

ChordQualitySelector::~ChordQualitySelector() {
    for (auto& row : rows) {
        row.typeValue.removeListener(this);
        row.numberValue.removeListener(this);
    }
}

void ChordQualitySelector::bindToValueTree(juce::ValueTree& tree) {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];

        // Remove old listeners before rebinding
        row.typeValue.removeListener(this);
        row.numberValue.removeListener(this);

        // Property names use enum name: chord_quality_MAJOR_type, chord_quality_MINOR_number, etc.
        auto prefix = juce::String("chord_quality_") + GeneratedSettings::ChordQualities::ENUM_NAMES[i];
        row.typeValue.referTo(tree.getPropertyAsValue(prefix + "_type", nullptr));
        row.numberValue.referTo(tree.getPropertyAsValue(prefix + "_number", nullptr));

        row.typeValue.addListener(this);
        row.numberValue.addListener(this);
    }

    // Load initial values from ValueTree
    updateComponentsFromValues();
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
    int totalSpacing = rowSpacing * (NUM_QUALITIES - 1);
    int rowHeight = (bounds.getHeight() - totalSpacing) / NUM_QUALITIES;

    for (auto& row : rows) {
        auto rowBounds = bounds.removeFromTop(rowHeight);

        // Calculate MIDI learn width based on aspect ratio
        int midiLearnWidth = static_cast<int>(rowHeight * midiLearnAspectRatio);
        midiLearnWidth = std::min(midiLearnWidth, rowBounds.getWidth() / 2);

        row.midiLearn.setBounds(rowBounds.removeFromRight(midiLearnWidth));
        row.label.setBounds(rowBounds);
        bounds.removeFromTop(rowSpacing);
    }
}

void ChordQualitySelector::valueChanged(juce::Value& /*value*/) {
    // ValueTree changed - update components
    updateComponentsFromValues();
}

void ChordQualitySelector::updateComponentsFromValues() {
    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];
        auto typeStr = row.typeValue.getValue().toString();
        int number = static_cast<int>(row.numberValue.getValue());

        MidiLearnedValue val;
        if (typeStr == "note") {
            val.type = MidiLearnedType::Note;
            val.value = number;
        } else if (typeStr == "cc") {
            val.type = MidiLearnedType::CC;
            val.value = number;
        } else {
            val.type = MidiLearnedType::None;
            val.value = -1;
        }

        row.midiLearn.setLearnedValue(val);
    }
}

void ChordQualitySelector::onMidiLearnChanged(size_t qualityIndex, MidiLearnedValue val) {
    if (qualityIndex >= NUM_QUALITIES) {
        return;
    }

    auto& row = rows[qualityIndex];

    // Update ValueTree properties
    if (val.type == MidiLearnedType::Note) {
        row.typeValue.setValue("note");
    } else if (val.type == MidiLearnedType::CC) {
        row.typeValue.setValue("cc");
    } else {
        row.typeValue.setValue("");
    }
    row.numberValue.setValue(val.value);
}
