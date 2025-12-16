#include "ChordSettingsPanel.h"

#include "../../PluginProcessor.h"
#include "../../datamodel/MidiButton.h"
#include "../../datamodel/OmnifySettings.h"
#include "../../voicing_styles/FromFile.h"
#include "../LcarsLookAndFeel.h"
#include "../components/FromFileView.h"

ChordSettingsPanel::ChordSettingsPanel(OmnifyAudioProcessor& p) : processor(p) {
    // Title - font will be set in resized() after LookAndFeel is available
    titleLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(titleLabel);

    // MIDI Channel
    channelLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 16; ++i) {
        channelComboBox.addItem(juce::String(i), i);
    }
    addAndMakeVisible(channelComboBox);

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
        voicingStyleSelector.addVariantNotOwned(entry.style->displayName(), view.get());
        voicingStyleViews.push_back(std::move(view));
        voicingStyleTypeNames.push_back(typeName);
    }
    addAndMakeVisible(voicingStyleSelector);

    // Latch controls
    latchLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
    addAndMakeVisible(latchLabel);
    addAndMakeVisible(latchToggleLearn);

    latchIsToggle.setColour(juce::ToggleButton::textColourId, LcarsColors::orange);
    latchIsToggle.setColour(juce::ToggleButton::tickColourId, LcarsColors::orange);
    addAndMakeVisible(latchIsToggle);

    // Stop button
    stopLabel.setColour(juce::Label::textColourId, LcarsColors::orange);
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
    g.drawRect(getLocalBounds(), 2);
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
    fb.items.add(juce::FlexItem(titleLabel).withHeight(30.0F).withMargin(4));

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
    }

    // Manual layout for nested rows since FlexBox doesn't nest well in performLayout
    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(4);

    auto channelRowBounds = bounds.removeFromTop(35);
    channelLabel.setBounds(channelRowBounds.removeFromLeft(channelRowBounds.getWidth() - 80));
    channelComboBox.setBounds(channelRowBounds);
    bounds.removeFromTop(4);

    voicingStyleSelector.setBounds(bounds.removeFromTop(80));
    bounds.removeFromTop(8);

    latchLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);

    auto latchRowBounds = bounds.removeFromTop(40);
    latchToggleLearn.setBounds(latchRowBounds.removeFromLeft(latchRowBounds.getWidth() / 2));
    latchIsToggle.setBounds(latchRowBounds);
    bounds.removeFromTop(8);

    stopLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);
    stopButtonLearn.setBounds(bounds.removeFromTop(40));
}
