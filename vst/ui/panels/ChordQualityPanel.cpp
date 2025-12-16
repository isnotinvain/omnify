#include "ChordQualityPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/ChordQualitySelectionStyle.h"
#include "../../datamodel/OmnifySettings.h"
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
    styleSelector.addVariantNotOwned("One Button Each", &qualityGrid);
    styleSelector.addVariantNotOwned("One CC for All", &singleCcContainer);
    addAndMakeVisible(styleSelector);

    setupCallbacks();
    refreshFromSettings();
}

ChordQualityPanel::~ChordQualityPanel() = default;

void ChordQualityPanel::setupCallbacks() {
    // Single CC MIDI learn callback
    singleCcLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val](OmnifySettings& s) {
            if (val.type == MidiLearnedType::CC) {
                s.chordQualitySelectionStyle = CCRangePerChordQuality{val.value};
            }
        });
    };

    // Quality grid callback for ButtonPerChordQuality
    qualityGrid.onQualityMidiChanged = [this](ChordQuality quality, MidiLearnedValue val) {
        processor.modifySettings([quality, val](OmnifySettings& s) {
            // Ensure we're using ButtonPerChordQuality variant
            if (!std::holds_alternative<ButtonPerChordQuality>(s.chordQualitySelectionStyle.value)) {
                s.chordQualitySelectionStyle = ButtonPerChordQuality{};
            }
            auto& mapping = std::get<ButtonPerChordQuality>(s.chordQualitySelectionStyle.value);

            // Clear any existing mapping for this quality
            for (auto it = mapping.notes.begin(); it != mapping.notes.end();) {
                if (it->second == quality) {
                    it = mapping.notes.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = mapping.ccs.begin(); it != mapping.ccs.end();) {
                if (it->second == quality) {
                    it = mapping.ccs.erase(it);
                } else {
                    ++it;
                }
            }

            // Add new mapping
            if (val.type == MidiLearnedType::Note) {
                mapping.notes[val.value] = quality;
            } else if (val.type == MidiLearnedType::CC) {
                mapping.ccs[val.value] = quality;
            }
        });
    };
}

void ChordQualityPanel::refreshFromSettings() {
    auto settings = processor.getSettings();
    const auto& style = settings->chordQualitySelectionStyle;

    // Set the variant selector based on which type is active
    int variantIndex = std::holds_alternative<ButtonPerChordQuality>(style.value) ? 0 : 1;
    styleSelector.setSelectedIndex(variantIndex);

    if (std::holds_alternative<ButtonPerChordQuality>(style.value)) {
        const auto& mapping = std::get<ButtonPerChordQuality>(style.value);

        // Refresh each quality's MIDI mapping in the grid
        for (ChordQuality quality : ALL_CHORD_QUALITIES) {
            MidiLearnedValue val;

            // Check notes map for this quality
            for (const auto& [noteNum, q] : mapping.notes) {
                if (q == quality) {
                    val.type = MidiLearnedType::Note;
                    val.value = noteNum;
                    break;
                }
            }

            // Check ccs map for this quality (if not found in notes)
            if (val.type == MidiLearnedType::None) {
                for (const auto& [ccNum, q] : mapping.ccs) {
                    if (q == quality) {
                        val.type = MidiLearnedType::CC;
                        val.value = ccNum;
                        break;
                    }
                }
            }

            qualityGrid.setMidiMapping(quality, val);
        }
    } else if (std::holds_alternative<CCRangePerChordQuality>(style.value)) {
        const auto& ccRange = std::get<CCRangePerChordQuality>(style.value);
        MidiLearnedValue ccVal;
        if (ccRange.cc >= 0) {
            ccVal.type = MidiLearnedType::CC;
            ccVal.value = ccRange.cc;
        }
        singleCcLearn.setLearnedValue(ccVal);
    }
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
