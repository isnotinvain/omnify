#include "ChordSettingsPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/MidiButton.h"
#include "../../datamodel/OmnifySettings.h"
#include "../../voicing_styles/FromFile.h"
#include "../LcarsLookAndFeel.h"
#include "../components/FromFileView.h"

ChordSettingsPanel::ChordSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title - font will be set in resized() after LookAndFeel is available
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::red);
    addAndMakeVisible(titleLabel);

    // MIDI Channel
    channelLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

    // Voicing Style
    voicingLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    addAndMakeVisible(voicingLabel);

    // Voicing Style Selector - iterate registry to build options
    for (const auto& [typeName, entry] : processor.getChordVoicingRegistry().getRegistry()) {
        std::unique_ptr<juce::Component> view;
        if (typeName == "FromFile") {
            auto fromFileView = std::make_unique<FromFileView>();
            fromFileView->onPathChanged = [this](const std::string& newPath) {
                processor.modifySettings([newPath](OmnifySettings& s) {
                    if (auto* fromFile = dynamic_cast<FromFile<VoicingFor::Chord>*>(s.chordVoicingStyle.get())) {
                        fromFile->setPath(newPath);
                    }
                });
                fromFileChordView->setPath(newPath);
            };
            fromFileChordView = fromFileView.get();
            view = std::move(fromFileView);
        } else {
            view = std::make_unique<juce::Component>();
        }
        voicingStyleSelector.addVariantNotOwned(entry.style->displayName(), view.get(), entry.style->description());
        voicingStyleViews.push_back(std::move(view));
        voicingStyleTypeNames.push_back(typeName);
    }
    addAndMakeVisible(voicingStyleSelector);

    // Latch controls
    latchLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    latchLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(latchLabel);
    addAndMakeVisible(latchToggleLearn);

    toggleLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    toggleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(toggleLabel);
    latchIsToggle.setColour(juce::ToggleButton::tickColourId, LcarsColors::orange);
    latchIsToggle.getProperties().set("onText", "Toggle");
    latchIsToggle.getProperties().set("offText", "Momentary");
    addAndMakeVisible(latchIsToggle);

    // Stop button
    stopLabel.setColour(juce::Label::textColourId, LcarsColors::africanViolet);
    stopLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(stopLabel);
    addAndMakeVisible(stopButtonLearn);

    setupCallbacks();
    refreshFromSettings();
}

ChordSettingsPanel::~ChordSettingsPanel() = default;

void ChordSettingsPanel::setupCallbacks() {
    // MIDI Channel
    channelComboBox.onChange = [this]() {
        processor.modifySettings([channel = channelComboBox.getSelectedId()](OmnifySettings& s) { s.chordChannel = channel; });
    };

    // Voicing Style selector
    voicingStyleSelector.onSelectionChanged = [this](int index) {
        if (index >= 0 && index < static_cast<int>(voicingStyleTypeNames.size())) {
            const auto& typeName = voicingStyleTypeNames[static_cast<size_t>(index)];
            const auto& registry = processor.getChordVoicingRegistry().getRegistry();
            auto it = registry.find(typeName);
            if (it != registry.end()) {
                processor.modifySettings([style = it->second.style](OmnifySettings& s) { s.chordVoicingStyle = style; });
            }
        }
    };

    // Latch button MIDI learn
    latchToggleLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val, isToggle = latchIsToggle.getToggleState()](OmnifySettings& s) {
            if (val.type == MidiLearnedType::Note) {
                s.latchButton = MidiButton::fromNote(val.value);
            } else if (val.type == MidiLearnedType::CC) {
                s.latchButton = MidiButton::fromCC(val.value, isToggle);
            } else {
                s.latchButton = MidiButton{};
            }
        });
    };

    // Latch toggle mode
    latchIsToggle.onClick = [this]() {
        processor.modifySettings([isToggle = latchIsToggle.getToggleState()](OmnifySettings& s) { s.latchButton.ccIsToggle = isToggle; });
    };

    // Stop button MIDI learn
    stopButtonLearn.onValueChanged = [this](MidiLearnedValue val) {
        processor.modifySettings([val](OmnifySettings& s) {
            if (val.type == MidiLearnedType::Note) {
                s.stopButton = MidiButton::fromNote(val.value);
            } else if (val.type == MidiLearnedType::CC) {
                s.stopButton = MidiButton::fromCC(val.value, false);
            } else {
                s.stopButton = MidiButton{};
            }
        });
    };
}

void ChordSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), LcarsLookAndFeel::borderRadius, 1.0F);
}

void ChordSettingsPanel::refreshFromSettings() {
    auto settings = processor.getSettings();

    // MIDI Channel
    channelComboBox.setSelectedId(settings->chordChannel, juce::dontSendNotification);

    // Latch button
    const auto& latch = settings->latchButton;
    MidiLearnedValue latchVal;
    if (latch.note >= 0) {
        latchVal.type = MidiLearnedType::Note;
        latchVal.value = latch.note;
    } else if (latch.cc >= 0) {
        latchVal.type = MidiLearnedType::CC;
        latchVal.value = latch.cc;
    }
    latchToggleLearn.setLearnedValue(latchVal);
    latchIsToggle.setToggleState(latch.ccIsToggle, juce::dontSendNotification);

    // Stop button
    const auto& stop = settings->stopButton;
    MidiLearnedValue stopVal;
    if (stop.note >= 0) {
        stopVal.type = MidiLearnedType::Note;
        stopVal.value = stop.note;
    } else if (stop.cc >= 0) {
        stopVal.type = MidiLearnedType::CC;
        stopVal.value = stop.cc;
    }
    stopButtonLearn.setLearnedValue(stopVal);

    // Voicing style selector - find matching index via registry lookup
    if (settings->chordVoicingStyle) {
        auto currentType = processor.getChordVoicingRegistry().getTypeName(settings->chordVoicingStyle.get());
        if (currentType) {
            for (size_t i = 0; i < voicingStyleTypeNames.size(); ++i) {
                if (voicingStyleTypeNames[i] == *currentType) {
                    voicingStyleSelector.setSelectedIndex(static_cast<int>(i), juce::dontSendNotification);
                    break;
                }
            }

            // Update FromFile view with current path
            if (*currentType == "FromFile" && fromFileChordView) {
                if (auto* fromFile = dynamic_cast<FromFile<VoicingFor::Chord>*>(settings->chordVoicingStyle.get())) {
                    fromFileChordView->setPath(fromFile->getPath());
                }
            }
        }
    }
}

void ChordSettingsPanel::resized() {
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    // Title
    float titleHeightF = LcarsLookAndFeel::fontSizeLarge + 10.0F;
    fb.items.add(juce::FlexItem(titleLabel).withHeight(titleHeightF).withMargin(4));

    // Channel row
    juce::FlexBox channelRow;
    channelRow.flexDirection = juce::FlexBox::Direction::row;
    channelRow.items.add(juce::FlexItem(channelLabel).withFlex(1.0F));
    channelRow.items.add(juce::FlexItem(channelComboBox).withWidth(80.0F));
    fb.items.add(juce::FlexItem().withHeight(35.0F).withMargin(4));

    // Voicing style selector
    fb.items.add(juce::FlexItem(voicingStyleSelector).withHeight(80.0F).withMargin(4));

    // Latch controls
    fb.items.add(juce::FlexItem(latchLabel).withHeight(20.0F).withMargin(juce::FlexItem::Margin(8, 4, 0, 4)));

    juce::FlexBox latchRow;
    latchRow.flexDirection = juce::FlexBox::Direction::row;
    latchRow.items.add(juce::FlexItem(latchToggleLearn).withFlex(1.0F));
    latchRow.items.add(juce::FlexItem(latchIsToggle).withFlex(1.0F));
    fb.items.add(juce::FlexItem().withHeight(40.0F).withMargin(4));

    // Stop button
    fb.items.add(juce::FlexItem(stopLabel).withHeight(20.0F).withMargin(juce::FlexItem::Margin(8, 4, 0, 4)));
    fb.items.add(juce::FlexItem(stopButtonLearn).withHeight(40.0F).withMargin(4));

    fb.performLayout(getLocalBounds().reduced(6));

    // Set fonts from LookAndFeel (must be done after component is added to hierarchy)
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        titleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeLarge));
        channelLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        voicingLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        latchLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        toggleLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
        stopLabel.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
    }

    // Manual layout for nested rows since FlexBox doesn't nest well in performLayout
    auto bounds = getLocalBounds().reduced(10, 2);

    int titleHeight = static_cast<int>(LcarsLookAndFeel::fontSizeLarge) + 10;
    titleLabel.setBounds(bounds.removeFromTop(titleHeight));
    bounds.removeFromTop(4);

    LcarsLookAndFeel::setComboBoxFontSize(channelComboBox, LcarsLookAndFeel::fontSizeSmall);
    auto channelRowBounds = bounds.removeFromTop(30);
    channelLabel.setBounds(channelRowBounds.removeFromLeft(channelRowBounds.getWidth() - 80));
    channelComboBox.setBounds(channelRowBounds);
    bounds.removeFromTop(4);

    voicingLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(4);

    // Bottom section: 3 rows aligned to bottom (remove these first so selector gets remaining space)
    // Stop row: label on left, midi learn on right
    auto stopRowBounds = bounds.removeFromBottom(40);
    stopButtonLearn.setBounds(stopRowBounds.removeFromRight(130));
    stopLabel.setBounds(stopRowBounds);
    bounds.removeFromBottom(4);

    // Toggle row: label on left, checkbox on right
    auto toggleRowBounds = bounds.removeFromBottom(40);
    latchIsToggle.setBounds(toggleRowBounds.removeFromRight(130));
    toggleLabel.setBounds(toggleRowBounds);
    bounds.removeFromBottom(4);

    // Latch row: label on left, midi learn on right
    auto latchRowBounds = bounds.removeFromBottom(40);
    latchToggleLearn.setBounds(latchRowBounds.removeFromRight(130));
    latchLabel.setBounds(latchRowBounds);
    bounds.removeFromBottom(4);

    // Voicing selector gets remaining middle space
    voicingStyleSelector.setBounds(bounds);
}
