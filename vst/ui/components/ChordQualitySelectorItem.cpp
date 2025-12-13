#include "../GuiItems.h"

const juce::Identifier ChordQualitySelectorItem::pFontSize{"font-size"};
const juce::Identifier ChordQualitySelectorItem::pLabelColor{"label-color"};
const juce::Identifier ChordQualitySelectorItem::pMidiLearnWidth{"midi-learn-width"};
const juce::Identifier ChordQualitySelectorItem::pRowSpacing{"row-spacing"};
const juce::Identifier ChordQualitySelectorItem::pMidiLearnAspectRatio{"midi-learn-aspect-ratio"};

ChordQualitySelectorItem::ChordQualitySelectorItem(foleys::MagicGUIBuilder& builder,
                                                   const juce::ValueTree& node)
    : foleys::GuiItem(builder, node) {
    addAndMakeVisible(container);

    setColourTranslation({{"label-color", juce::Label::textColourId}});

    for (size_t i = 0; i < NUM_QUALITIES; ++i) {
        auto& row = rows[i];
        row.label.setText(GeneratedAdditionalSettings::ChordQualities::NAMES[i],
                          juce::dontSendNotification);
        container.addAndMakeVisible(row.label);
        container.addAndMakeVisible(row.midiLearn);
    }
}

void ChordQualitySelectorItem::update() {
    auto fontSize = magicBuilder.getStyleProperty(pFontSize, configNode);
    auto labelColor = magicBuilder.getStyleProperty(pLabelColor, configNode);
    auto aspectRatioVar = magicBuilder.getStyleProperty(pMidiLearnAspectRatio, configNode);

    float fontSizeVal = fontSize.isVoid() ? 14.0F : static_cast<float>(fontSize);
    float aspectRatio = aspectRatioVar.isVoid() ? 0.0F : static_cast<float>(aspectRatioVar);
    juce::Colour labelColorVal = juce::Colours::white;
    if (!labelColor.isVoid()) {
        auto colorStr = labelColor.toString();
        if (colorStr.startsWith("$")) {
            labelColorVal = magicBuilder.getStylesheet().getColour(colorStr);
        } else {
            labelColorVal = juce::Colour::fromString(colorStr);
        }
    }

    for (auto& row : rows) {
        row.label.setFont(juce::Font(juce::FontOptions(fontSizeVal)));
        row.label.setColour(juce::Label::textColourId, labelColorVal);
        row.midiLearn.setAspectRatio(aspectRatio);
    }
}

void ChordQualitySelectorItem::resized() {
    container.setBounds(getLocalBounds());

    auto midiLearnWidthVar = magicBuilder.getStyleProperty(pMidiLearnWidth, configNode);
    auto rowSpacingVar = magicBuilder.getStyleProperty(pRowSpacing, configNode);

    int midiLearnWidth = midiLearnWidthVar.isVoid() ? 80 : static_cast<int>(midiLearnWidthVar);
    int rowSpacing = rowSpacingVar.isVoid() ? 2 : static_cast<int>(rowSpacingVar);

    auto bounds = container.getLocalBounds();
    int totalSpacing = rowSpacing * (NUM_QUALITIES - 1);
    int rowHeight = (bounds.getHeight() - totalSpacing) / NUM_QUALITIES;

    for (auto& row : rows) {
        auto rowBounds = bounds.removeFromTop(rowHeight);
        row.midiLearn.setBounds(rowBounds.removeFromRight(midiLearnWidth));
        row.label.setBounds(rowBounds);
        bounds.removeFromTop(rowSpacing);
    }
}

juce::Component* ChordQualitySelectorItem::getWrappedComponent() {
    return &container;
}

std::vector<foleys::SettableProperty> ChordQualitySelectorItem::getSettableProperties() const {
    std::vector<foleys::SettableProperty> props;
    props.push_back({configNode, pFontSize, foleys::SettableProperty::Number, 14.0F, {}});
    props.push_back({configNode, pLabelColor, foleys::SettableProperty::Colour, "FFFFFFFF", {}});
    props.push_back({configNode, pMidiLearnWidth, foleys::SettableProperty::Number, 80.0F, {}});
    props.push_back({configNode, pRowSpacing, foleys::SettableProperty::Number, 2.0F, {}});
    props.push_back({configNode, pMidiLearnAspectRatio, foleys::SettableProperty::Number, 0.0F, {}});
    return props;
}

MidiLearnedValue ChordQualitySelectorItem::getLearnerValue(size_t qualityIndex) const {
    if (qualityIndex < NUM_QUALITIES) {
        return rows[qualityIndex].midiLearn.getLearnedValue();
    }
    return {};
}

void ChordQualitySelectorItem::setLearnerValue(size_t qualityIndex, MidiLearnedValue value) {
    if (qualityIndex < NUM_QUALITIES) {
        rows[qualityIndex].midiLearn.setLearnedValue(value);
    }
}

void ChordQualitySelectorItem::setOnValueChanged(
    size_t qualityIndex, std::function<void(MidiLearnedValue)> callback) {
    if (qualityIndex < NUM_QUALITIES) {
        rows[qualityIndex].midiLearn.onValueChanged = std::move(callback);
    }
}