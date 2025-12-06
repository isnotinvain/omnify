#include "GuiItems.h"

#include "../PluginProcessor.h"

const juce::Identifier MidiLearnItem::pCaption{"caption"};

MidiLearnItem::MidiLearnItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    addAndMakeVisible(midiLearnComponent);
}

void MidiLearnItem::update() {
    auto captionText = magicBuilder.getStyleProperty(pCaption, configNode).toString();
    if (captionText.isNotEmpty()) {
        midiLearnComponent.setCaption(captionText);
    }
}

juce::Component* MidiLearnItem::getWrappedComponent() { return &midiLearnComponent; }

std::vector<foleys::SettableProperty> MidiLearnItem::getSettableProperties() const {
    std::vector<foleys::SettableProperty> props;
    props.push_back({configNode, pCaption, foleys::SettableProperty::Text, {}, {}});
    return props;
}

ChordQualitySelectorItem::ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder,
                                                   const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    if (auto* processor =
            dynamic_cast<OmnifyAudioProcessor*>(builder.getMagicState().getProcessor())) {
        chordQualitySelector = processor->getChordQualitySelector();
        addAndMakeVisible(chordQualitySelector);
    }
}

void ChordQualitySelectorItem::update() {}

juce::Component* ChordQualitySelectorItem::getWrappedComponent() { return chordQualitySelector; }