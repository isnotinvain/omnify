#include "PluginEditor.h"

//==============================================================================
OmnifyAudioProcessorEditor::OmnifyAudioProcessorEditor(OmnifyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
    // Title label
    titleLabel.setText("Omnify", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0F, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Chord Voicing Style combo box
    chordVoicingLabel.setText("Chord Voicing Style", juce::dontSendNotification);
    chordVoicingLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(chordVoicingLabel);

    for (int i = 0; i < ChordVoicingStyles::choices.size(); ++i)
        chordVoicingCombo.addItem(ChordVoicingStyles::choices[i], i + 1);
    addAndMakeVisible(chordVoicingCombo);

    // Strum Voicing Style combo box
    strumVoicingLabel.setText("Strum Voicing Style", juce::dontSendNotification);
    strumVoicingLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(strumVoicingLabel);

    for (int i = 0; i < StrumVoicingStyles::choices.size(); ++i)
        strumVoicingCombo.addItem(StrumVoicingStyles::choices[i], i + 1);
    addAndMakeVisible(strumVoicingCombo);

    // Attach parameters
    auto& params = processorRef.getParameters();
    chordVoicingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, ParamIDs::CHORD_VOICING_STYLE, chordVoicingCombo);
    strumVoicingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, ParamIDs::STRUM_VOICING_STYLE, strumVoicingCombo);

    setSize(300, 120);
}

OmnifyAudioProcessorEditor::~OmnifyAudioProcessorEditor() = default;

//==============================================================================
void OmnifyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void OmnifyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(40));

    // Combo boxes
    auto comboArea = bounds.reduced(10, 5);

    auto chordComboArea = comboArea.removeFromTop(25);
    chordVoicingLabel.setBounds(chordComboArea.removeFromLeft(120));
    chordVoicingCombo.setBounds(chordComboArea);

    comboArea.removeFromTop(5);

    auto strumComboArea = comboArea.removeFromTop(25);
    strumVoicingLabel.setBounds(strumComboArea.removeFromLeft(120));
    strumVoicingCombo.setBounds(strumComboArea);
}
