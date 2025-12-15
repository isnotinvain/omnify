#include "ChordQualityPanel.h"

#include "../../PluginProcessor.h"
#include "../LcarsLookAndFeel.h"

ChordQualityPanel::ChordQualityPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title - font will be set in resized() after LookAndFeel is available
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(titleLabel);

    // Quality grid (configure before adding to selector)
    // Font size will be set in resized() after LookAndFeel is available
    qualityGrid.setLabelColor(LcarsColors::orange);
    qualityGrid.setMidiLearnAspectRatio(2.0F);

    // Single CC container (configure before adding to selector)
    singleCcLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    singleCcLabel.setJustificationType(juce::Justification::centred);
    singleCcContainer.addAndMakeVisible(singleCcLabel);
    singleCcLearn.setAcceptMode(MidiAcceptMode::CCsOnly);
    singleCcContainer.addAndMakeVisible(singleCcLearn);

    // Style selector (One Button Each vs One CC for All)
    // Note: addVariantNotOwned adds components as children of the VariantSelector,
    // so we don't addAndMakeVisible them separately
    styleSelector.addVariantNotOwned("One Button Each", &qualityGrid);
    styleSelector.addVariantNotOwned("One CC for All", &singleCcContainer);
    addAndMakeVisible(styleSelector);

    // Set up value bindings
    setupValueBindings();
}

ChordQualityPanel::~ChordQualityPanel() = default;

void ChordQualityPanel::setupValueBindings() {
    auto& stateTree = processor.getStateTree();

    // Selection style variant
    chordQualitySelectionStyleValue.referTo(stateTree.getPropertyAsValue("variant_chord_quality_selection_style", nullptr));
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
    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
    }

    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(4);

    styleSelector.setBounds(bounds);

    // Layout for single CC container - must be done after styleSelector.setBounds
    // because VariantSelector::resized() sets the container's bounds
    auto containerBounds = singleCcContainer.getLocalBounds();
    singleCcLabel.setBounds(containerBounds.removeFromTop(30));
    singleCcLearn.setBounds(containerBounds.reduced(20, 10));
}
