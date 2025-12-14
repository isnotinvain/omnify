#include "ChordQualityPanel.h"

#include "../../PluginProcessor.h"

ChordQualityPanel::ChordQualityPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title
    titleLabel.setFont(juce::Font(juce::FontOptions(24.0f)));
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(titleLabel);

    // Style selector (One Button Each vs One CC for All)
    styleSelector.addVariantNotOwned("One Button Each", &qualityGrid);
    styleSelector.addVariantNotOwned("One CC for All", &singleCcContainer);
    addAndMakeVisible(styleSelector);

    // Quality grid
    qualityGrid.setFontSize(20.0f);
    qualityGrid.setLabelColor(LcarsColors::orange);
    qualityGrid.setMidiLearnAspectRatio(2.0f);
    addAndMakeVisible(qualityGrid);

    // Single CC container
    singleCcLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    singleCcLabel.setJustificationType(juce::Justification::centred);
    singleCcContainer.addAndMakeVisible(singleCcLabel);

    singleCcLearn.setAcceptMode(MidiAcceptMode::CCsOnly);
    singleCcContainer.addAndMakeVisible(singleCcLearn);
    addAndMakeVisible(singleCcContainer);

    // Set up value bindings
    setupValueBindings();
}

ChordQualityPanel::~ChordQualityPanel() = default;

void ChordQualityPanel::setupValueBindings() {
    auto& stateTree = processor.getStateTree();

    // Selection style variant
    chordQualitySelectionStyleValue.referTo(
        stateTree.getPropertyAsValue("variant_chord_quality_selection_style", nullptr));
    styleSelector.bindToValue(chordQualitySelectionStyleValue);

    // Quality grid bindings
    qualityGrid.bindToValueTree(stateTree);

    // Single CC MIDI learn bindings
    singleCcTypeValue.referTo(stateTree.getPropertyAsValue("chord_quality_cc_type", nullptr));
    singleCcNumberValue.referTo(stateTree.getPropertyAsValue("chord_quality_cc_number", nullptr));

    auto updateSingleCcFromValues = [this]() {
        auto typeStr = singleCcTypeValue.getValue().toString();
        int number = static_cast<int>(singleCcNumberValue.getValue());
        MidiLearnedValue val;
        if (typeStr == "cc") {
            val.type = MidiLearnedType::CC;
            val.value = number;
        }
        singleCcLearn.setLearnedValue(val);
    };
    updateSingleCcFromValues();

    singleCcLearn.onValueChanged = [this](MidiLearnedValue val) {
        if (val.type == MidiLearnedType::CC) {
            singleCcTypeValue.setValue("cc");
        } else {
            singleCcTypeValue.setValue("");
        }
        singleCcNumberValue.setValue(val.value);
    };
}

void ChordQualityPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::orange);
    g.drawRect(getLocalBounds(), 2);
}

void ChordQualityPanel::resized() {
    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(4);

    styleSelector.setBounds(bounds);

    // Layout for single CC container
    auto containerBounds = singleCcContainer.getLocalBounds();
    if (containerBounds.getHeight() > 0) {
        singleCcLabel.setBounds(containerBounds.removeFromTop(30));
        singleCcLearn.setBounds(containerBounds.reduced(20));
    }
}
