#include "ChordSettingsPanel.h"

#include "../../GeneratedSettings.h"
#include "../../PluginProcessor.h"
#include "../LcarsLookAndFeel.h"

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

    // Voicing Style Selector
    // Note: addVariantNotOwned adds components as children of the VariantSelector,
    // so we don't addAndMakeVisible them separately
    voicingStyleSelector.addVariantNotOwned("Root Position", &rootPositionView);

    // Add bundled voicing options (from generated code)
    for (auto& i : GeneratedSettings::BundledChordVoicings::DISPLAY_NAMES) {
        auto view = std::make_unique<juce::Component>();
        voicingStyleSelector.addVariantNotOwned(i, view.get());
        bundledVoicingViews.push_back(std::move(view));
    }

    voicingStyleSelector.addVariantNotOwned("From File", &filePicker);
    voicingStyleSelector.addVariantNotOwned("Omni-84", &omni84View);
    addAndMakeVisible(voicingStyleSelector);

    filePicker.setFileFilter("*.json");

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

    // Set up value bindings
    setupValueBindings();
}

ChordSettingsPanel::~ChordSettingsPanel() = default;

void ChordSettingsPanel::setupValueBindings() {
    auto& stateTree = processor.getStateTree();

    // MIDI Channel
    chordChannelValue.referTo(stateTree.getPropertyAsValue("chord_channel", nullptr));
    channelComboBox.setSelectedId(static_cast<int>(chordChannelValue.getValue()), juce::dontSendNotification);
    channelComboBox.onChange = [this]() { chordChannelValue.setValue(channelComboBox.getSelectedId()); };

    // Voicing Style
    chordVoicingStyleValue.referTo(stateTree.getPropertyAsValue("variant_chord_voicing_style", nullptr));
    voicingStyleSelector.bindToValue(chordVoicingStyleValue);

    // File path for "From File" variant
    chordVoicingFilePathValue.referTo(stateTree.getPropertyAsValue("chord_voicing_file", nullptr));
    filePicker.bindToValue(chordVoicingFilePathValue);

    // Latch toggle MIDI learn
    latchToggleTypeValue.referTo(stateTree.getPropertyAsValue("latch_toggle_button_type", nullptr));
    latchToggleNumberValue.referTo(stateTree.getPropertyAsValue("latch_toggle_button_number", nullptr));

    auto updateLatchFromValues = [this]() {
        auto typeStr = latchToggleTypeValue.getValue().toString();
        int number = static_cast<int>(latchToggleNumberValue.getValue());
        MidiLearnedValue val;
        if (typeStr == "note") {
            val.type = MidiLearnedType::Note;
            val.value = number;
        } else if (typeStr == "cc") {
            val.type = MidiLearnedType::CC;
            val.value = number;
        }
        latchToggleLearn.setLearnedValue(val);
    };
    updateLatchFromValues();

    latchToggleLearn.onValueChanged = [this](MidiLearnedValue val) {
        if (val.type == MidiLearnedType::Note) {
            latchToggleTypeValue.setValue("note");
        } else if (val.type == MidiLearnedType::CC) {
            latchToggleTypeValue.setValue("cc");
        } else {
            latchToggleTypeValue.setValue("");
        }
        latchToggleNumberValue.setValue(val.value);
    };

    // Latch is toggle
    latchIsToggleValue.referTo(stateTree.getPropertyAsValue("latch_is_toggle", nullptr));
    latchIsToggle.setToggleState(static_cast<bool>(latchIsToggleValue.getValue()), juce::dontSendNotification);
    latchIsToggle.onClick = [this]() { latchIsToggleValue.setValue(latchIsToggle.getToggleState()); };

    // Stop button MIDI learn
    stopButtonTypeValue.referTo(stateTree.getPropertyAsValue("stop_button_type", nullptr));
    stopButtonNumberValue.referTo(stateTree.getPropertyAsValue("stop_button_number", nullptr));

    auto updateStopFromValues = [this]() {
        auto typeStr = stopButtonTypeValue.getValue().toString();
        int number = static_cast<int>(stopButtonNumberValue.getValue());
        MidiLearnedValue val;
        if (typeStr == "note") {
            val.type = MidiLearnedType::Note;
            val.value = number;
        } else if (typeStr == "cc") {
            val.type = MidiLearnedType::CC;
            val.value = number;
        }
        stopButtonLearn.setLearnedValue(val);
    };
    updateStopFromValues();

    stopButtonLearn.onValueChanged = [this](MidiLearnedValue val) {
        if (val.type == MidiLearnedType::Note) {
            stopButtonTypeValue.setValue("note");
        } else if (val.type == MidiLearnedType::CC) {
            stopButtonTypeValue.setValue("cc");
        } else {
            stopButtonTypeValue.setValue("");
        }
        stopButtonNumberValue.setValue(val.value);
    };
}

void ChordSettingsPanel::paint(juce::Graphics& g) {
    g.setColour(LcarsColors::africanViolet);
    g.drawRect(getLocalBounds(), 2);
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
